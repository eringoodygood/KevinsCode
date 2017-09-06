#include <iomanip>
#include <iostream>
#include <fstream>
#include "TCanvas.h"
#include "TGraph.h"
#include "TH1F.h"
#include "TLine.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TRandom3.h"

using namespace std;

const int NGRAPHS   = 1;
const int NSAMPLES  = 4100; //16384 (set in the wavedump config file)
const int ChanToAnalyze = 0; //front channels: 0,1,2,3
const int Tclk = 2; // Sampling period in nanoseconds for the 1730 digitizer

// Trapezoidal Filter Parameters
const int Tk = 50; //    This code is assuming the convention k<l !!!!!! 
const int Tl = 550; //   If l>k, The flat top time m = l-k.
//const int Ttau = 59.7e3; // 1/e Decay Time in nanoseconds. (high gain 60 mV/MeV)
const double Ttau = 19.86e3;// (pulser data) 11e3; 1/e (27 mV/MeV chip)
const int Tdelay = 50; // flat top delay
const int Twidth = 200; // width of flat top to average for the energy 

// Derivative Filter Parameters 
const int Dk = 155; // 
const int Dl = 157; // MUST HAVE: Dl >= Dk + 1 !!!!!!

// Second Derivative Filter Parameter (needs revision to add in integral smoothing
const int DDk = 155;//
const int DDl = 157;// MUST HAVE; DDl >= DDk + 1 !!!!!! (not a mathematical resriction in general
//                                   but I assumed this in my implementation )
const int delta = 1; // spacing for second derivative 

// Double Derviative Trigger/Timing Parameters
const int polarity = -1; // 1 = positive, -1 = negative
const double thresh = 0.5; // Threshold for first derivative trigger
const int trigDelay = 200; //   Look for the first zero crossing in the second derivative within
//   a fixed time window
const int trigHO = 500;
bool armed = false; // arm the trigger if second derivative amplitude > thresh;
bool holdOff = false; // hold off to prevent immediate retriggers


int readBin();

static inline void loadbar(unsigned int iter, unsigned int NumIterations, unsigned int Resolution = 50)
{
  if ( (iter != NumIterations) && (iter % (NumIterations/100+1) != 0) ) return;
  
  float	ratio  =  iter/(float)NumIterations;
  unsigned int c =  ratio * Resolution;
  
  cout << setw(3) << (int)(ratio*100) << "% [";
  for (unsigned int j=0; j<c; j++) cout << "=";
  for (unsigned int k=c; k<Resolution; k++) cout << " ";
  cout << "]\r" << flush;
  if(iter==NumIterations) cout << endl;
}
int readBin()
{	
  streampos size;
  unsigned short* buffer;
  int headerlines = 12;
  TCanvas *c0 = new TCanvas("c0","c0",0,0,600,400);
  c0->SetGridy();
  TCanvas *c1 = new TCanvas("c1","Trapezoidal Filter",650,0,600,400);
  c1->SetGridy();
  TCanvas *c2 = new TCanvas("c2","Derivative Filter",0,450,600,400);
  TCanvas *c3 = new TCanvas("c3","Second Derivative Filter",650,450,600,400);
  //	c0->cd(1);
  TGraph *graphW;
  TGraph *graphT;
  TGraph *graphD;
  TGraph *graphD2;
  
  graphW = new TGraph(NSAMPLES);
  graphW->SetName("graphW");
  graphT = new TGraph(NSAMPLES);
  graphT->SetName("graphT");
  graphD = new TGraph(NSAMPLES);
  graphD->SetName("graphD");
  graphD2 = new TGraph(NSAMPLES);
  graphD2->SetName("graphD2");
  
  TLine *tTrig = new TLine();  // trigger time
  TLine *tStart = new TLine(); // energy averaging start = trigger time + delay
  TLine *tStop = new TLine();  // energy averaging stop  = energy start + width
  TLine *tArmed = new TLine(); // trigger armed when first derivative goes above "thresh"
  TLine *tDelay = new TLine(); // until zero crossing is found within a delay of "tDelay"
  TLine *tHO = new TLine();    // or until "trigHO" resets the trigger.
  
  ifstream dataFile;
  dataFile.open(Form("data/wave%i.dat",ChanToAnalyze),ios::in|ios::binary|ios::ate);
  
  unsigned short *point;
  TFile *ofile = new TFile(Form("wave%i.root",ChanToAnalyze),"RECREATE");
  TTree *otree = new TTree("T","processed waveform tree");
  
  if(dataFile.is_open())
    {
      cout << Form("Analyzing: data/wave%i.dat",ChanToAnalyze) << endl;
      
      //variables for derivative filter (1st and 2nd derivative)
      double d1 = 0.0; //
      double d1_1 = 0.0;
      double d2 = 0.0;
      double d2_1 = 0.0;
      
      //variables for recursive trapezoidal filter
      double dkl = 0.0;
      double pn_1 = 0.0;
      double pn = 0.0;
      double rn = 0.0;
      double sn = 0.0;
      double sn_1 = 0.0;
      
      double t1 = -1; // trigger time (linear interpolated)
      int trigIndex = -1;
      int armedIndex = -1; // index where trigger was armed
      int trigPerEvent = 0;
      double energy = 0;
      int event = -1;
      
      otree->Branch("eventID",&event,"eventID/I");
      otree->Branch("trigID",&trigPerEvent,"trigID/I");
      otree->Branch("energy",&energy,"energy/D");
      otree->Branch("t1",&t1,"t1/D");
      
      
      double TM = Ttau/Tclk - 0.5; // This approximation is totally valid.
      
      
      size = dataFile.tellg();
      buffer = new unsigned short[size];
      point = buffer;
      dataFile.seekg(0, ios::beg);
      dataFile.read((char*)buffer , size);
      //single event in Nbytes
      
      int OneEventSize = (*(buffer+1)<<16)+(*buffer);  //First character in the buffer is the size of a single event
      int RecordLength = (OneEventSize - 24)/2;
      int Nevents = size/OneEventSize;

      cout << *buffer << endl;
      cout << *(buffer+1) << endl;
      cout << "Size of file: "   << size         << " bytes" << endl;
      cout << "OneEventSize "    << OneEventSize <<   endl;
      cout << "Nevents "         << Nevents      << endl;
      cout << "Record Length = " << RecordLength << endl;

      // In case you want to know how big stuff is.
      //      cout << "char is "     << sizeof(char)  << " bytes"  << endl;
      //      cout << "short is "    << sizeof(short) << " bytes"  << endl;
      //      cout << "int is "      << sizeof(int)   << " bytes"  << endl;
      //      cout << "long is "     << sizeof(long)  << " bytes"  << endl;
      //      cout << "one byte is " << CHAR_BIT      << " bits"  << endl;
      
      //Check that arrays are the right size. (Instead of putting a bunch of ``NEWs'' everywhere)
      if(NSAMPLES!= RecordLength)
	{
	  cerr << "NSAMPLES in readBin.C should be set to " << RecordLength << endl;
	  delete[] buffer;
	  return -1;
	}
      
      //here we go over the events
      for(event=0; event < Nevents; event++)
	{
	  // Initialize variables at the beginning of each event
	  d1  = 0.0;
	  d2  = 0.0;
	  
	  dkl = 0.0;
	  pn = 0.0;
	  rn = 0.0;
	  sn = 0.0;
	  
	  // To play games recursively we need to store previous values.
	  sn_1 = 0.0; // sn[n-1]
	  pn_1 = 0.0; // p[n-1]
	  d1_1 = 0.0; // d1[n-1]
	  d2_1 = 0.0; // d2[n-1]
	  
	  armed = false; // reset any previous triggers
	  holdOff = false; // no trigger hold off
	  t1 = -1; // set default time to unphysical value
	  armedIndex = -1;
	  trigIndex = -1;
	  trigPerEvent = 0;
	  
	  
	  //Skip over headerlines to get to waveform data
	  point+=headerlines;
	  //point++;
	  
	  
	  //here we go over the waveform
	  for(int i=0; i<NSAMPLES; i++)
	    {		
	      //store values for next step before modifying them
	      pn_1 = pn;
	      sn_1 = sn;
	      d1_1 = d1;
	      d2_1 = d2;
	      
	      if(i>Dk+Dl)
		{
		  //in air quotes: d1[n] = v[n] - v[n-Dk] - v[n-Dl] + v[n-Dk-Dl]
		  d1 = (double)(*point) - (double)(*(point - Dk)) - (double)(*(point-Dl)) + (double)(*(point-Dk-Dl));
		  
		  if(i>3*Dl+2*Dl+delta)
		    {
		      d2 = d1 - (double)(*(point-Dk-Dl-delta))/Dl/Dk + (double)(*(point-2*Dk-Dl-delta))/Dl/Dk 
			+ (double)(*(point-Dk-2*Dl-delta))/Dl/Dk - (double)(*(point-2*Dk-2*Dl-delta))/Dl/Dk;
		    }
		  d1 /= (Dk*Dl);
		  d1 += d1_1;
		  d2 += d2_1;
		  d2/=(delta+Dk+Dl);
		}
	      else
		{
		  d1 = 0.0;
		  d2 = 0.0;
		}
	      // reads: dkl = dk[n] - ( v[n-l] - v[n-l-k] )
	      //
	      // you could have dkl non-zero before this if you first subtract baseline from vn
	      // it's best to just let the filter eat some baseline before the pulse rises.
	      // [Rule of thumb] waveform = 1/3 baseline + 2/3 signal
	      dkl = (i<Tk+Tl) ? 0 : (double)(*point) - (double)(*(point-Tk)) - (double)(*(point-Tl)) + (double)(*(point-Tk-Tl)); // (1)
	      dkl/=(Tk*Tl); // Normalization that is not in Jordanov et al.
	      pn  = pn_1 + dkl;                                                 // (2)				
	      rn  = pn   + TM*dkl;                                              // (3)  
	      sn  = sn_1 + rn;                                                  // (4) 				
	      
	      // Trigger on second derivative
	      if(d1*polarity>thresh && !holdOff)
		{
		  armed = true;
		  holdOff = true;
		  armedIndex = i;
		}
	      //Mark zero crossing in second derivative
	      if(armed && d2*polarity<0 && d2_1*polarity>0 && i<armedIndex+trigDelay)
		{
		  trigIndex = i;
		  t1 = (double)trigIndex + d2_1/(d2_1-d2); 
		  t1*=4.0; // convert to nanoseconds
		  trigPerEvent++;
		  armed = false; // reset armed, but not hold off.
		}
	      if(armed && i>armedIndex + trigHO)
		{
		  holdOff = false;
		  armed = false;
		}
	      
	      tTrig->SetX1(trigIndex*Tclk);
	      tTrig->SetY1(10000.0);
	      tTrig->SetY2(0);
	      tTrig->SetVertical();
	      tTrig->SetLineColor(2);
	      tStart->SetX1(t1+Tk*Tclk+Tdelay*Tclk);
	      tStart->SetY2(0);
	      tStart->SetY1(10000);
	      tStart->SetVertical();
	      tStop->SetX1(t1+Tk*Tclk+Tdelay*Tclk+Twidth*Tclk);
	      tStop->SetY2(0);
	      tStop->SetY1(10000);
	      tStop->SetVertical();
	      tArmed->SetLineColor(3);
	      tArmed->SetX1(armedIndex*Tclk);
	      tArmed->SetY2(0);
	      tArmed->SetY1(10000);
	      tArmed->SetVertical();
	      tHO->SetX1(armedIndex*Tclk+trigHO*Tclk);
	      tHO->SetY2(0);
	      tHO->SetY1(10000);
	      tHO->SetVertical();
	      tDelay->SetX1(armedIndex*Tclk+trigDelay*Tclk);
	      tDelay->SetY2(0);
	      tDelay->SetY1(10000);
	      tDelay->SetVertical();
	      // end trigger on second derivative
	      
	      if(trigPerEvent>0)
		{
		  if(i>trigIndex+Tk+Tdelay && i<trigIndex+Tk+Tdelay+Twidth)
		    {
		      energy+=sn/Twidth;
		    }
		  else if(i>trigIndex+Tk+Tdelay+Twidth && energy*polarity>0)
		    {
		      energy*=polarity;
		      //cout <<  "Event " << event << " Trig " << trigPerEvent << " energy " << energy << " time " << t1 << endl;
		      otree->Fill();
		      energy = 0;
		    }
		}
	      
	      // Plot Stuff Here (time axis is in nanoseconds)
	      
	      graphW->SetPoint(i,i*Tclk,(polarity>0) ? *point : 16384 - (*point));  //quick version of if-else statement
		      graphT->SetPoint(i,i*Tclk,sn*polarity);
		      graphD->SetPoint(i,i*Tclk,d1*polarity);
		      graphD2->SetPoint(i,i*Tclk,d2*polarity);	
	      //Iterate our data pointer
	      point++;
	    }// end loop over waveform
	  c0->cd();								
	  
	  // graphD->SetMaximum(0.5);
	  // graphD2->SetMaximum(0.5);
	  // graphD->SetMinimum(-0.5);
	  //  graphD2->SetMinimum(-0.5);
	  graphW->Draw("ALP");
	  c0->SetTitle(Form("Event %i",event));
	  c1->cd();
	  graphT->Draw("ALP");
	  tTrig->Draw("SAME");
	  tStart->Draw("SAME");
	  tStop->Draw("SAME");
	  c2->cd();
	  graphD->Draw("ALP");
	  tArmed->Draw("SAME");
	  tHO->Draw("SAME");
	  tDelay->Draw("SAME");
	  c3->cd();
	  graphD2->Draw("ALP");
	  tArmed->Draw("SAME");
	  tHO->Draw("SAME");
	  tTrig->Draw("SAME+");
	  tDelay->Draw("SAME");
	  c0->Update();			
	  c1->Update();
	  c2->Update();
	  c3->Update();
	  c0->cd();
	  if(trigPerEvent>0)
	    {    
	      c0->WaitPrimitive();
	    }
	  


	  //c0->WaitPrimitive();
	  loadbar(event,Nevents);
	  //cout << n << endl;
	}
      loadbar(Nevents,Nevents);
      //cout << endl;
    }
  else
    {
      cout << Form("!!!File not found: data/wave%i.dat ",ChanToAnalyze) << endl;
      dataFile.clear();
      return 1;
    }
  c0->Update();
  c0->WaitPrimitive();
  otree->Write();
  ofile->Close();
  dataFile.close();
  dataFile.clear();
  
  delete[] buffer;
  
  delete graphW;
  delete graphT;
  delete graphD;
  delete graphD2;
  
  return 0;
}

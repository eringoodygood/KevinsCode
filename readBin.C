#include <iomanip>
#include <iostream>
#include <fstream>
#include "TCanvas.h"
#include "TGraph.h"
#include "TH1F.h"
#include "TLine.h"
#include "TFile.h"
#include "TTree.h"
#include "TRandom3.h"

using namespace std;

// Sorry Charlie :-)
#include "globals.h"

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
  if(iter == NumIterations) cout << endl;
}

int readBin()
{	
  bool armed = false;
  bool holdOff = false;

  int     w_v[NSAMPLES];    // keept these guys around
  int    d1_v[NSAMPLES];    // keep these guys around
  int    d2_v[NSAMPLES];    // keep these guys around
  double sn_v[NSAMPLES];    // keep these guys around


  for(int ns=0; ns<NSAMPLES; ns++)
    {
      w_v[ns]  = 0;
      d1_v[ns] = 0;
      d2_v[ns] = 0;
      sn_v[ns] = 0.0;
    }

  streampos size;
  unsigned short* buffer;
  int headerlines = 12;
  TCanvas *c0 = new TCanvas("c0","c0",0,0,600,400);
  TCanvas *c1 = new TCanvas("c1","Trapezoidal Filter",610,0,600,400);
  TCanvas *c2 = new TCanvas("c2","Derivative Filter",0,420,600,400);
  TCanvas *c3 = new TCanvas("c3","Second Derivative Filter",610,420,600,400);

  TGraph *graphW = new TGraph(NSAMPLES);
  TGraph *graphT = new TGraph(NSAMPLES);
  TGraph *graphD = new TGraph(NSAMPLES);
  TGraph *graphD2 = new TGraph(NSAMPLES);
 
  TLine *tTrig = new TLine();  // trigger time
  TLine *tStart = new TLine(); // energy averaging start = trigger time + delay
  TLine *tStop = new TLine();  // energy averaging stop  = energy start + width
  TLine *tArmed = new TLine(); // trigger armed when first derivative goes above "thresh"
  TLine *tDelay = new TLine(); // until zero crossing is found within a delay of "tDelay"
  TLine *tHO = new TLine();    // or until "trigHO" resets the trigger.
  
  ifstream dataFile;
  dataFile.open(Form("data/wave%i.dat",ChanToAnalyze),ios::in|ios::binary|ios::ate);
  
  unsigned short *point;
  TFile *ofile = new TFile(Form("output/wave%i.root",ChanToAnalyze),"RECREATE");
  TTree *otree = new TTree("T","processed waveform tree");
  
  if(dataFile.is_open())
    {
      cout << Form("Analyzing: data/wave%i.dat",ChanToAnalyze) << endl;
      
      //variables for derivative filter (1st and 2nd derivative)
      int MaxD1 = 0;
      int MinD1 = 0;
      int MaxD2 = 0;
      int MinD2 = 0;
      double MaxT = 0;
      double MinT = 0;
      int d1 = 0.0;
      int d1_1 = 0.0;
      int d2 = 0.0;
      int d2_1 = 0.0;
           
      //variables for recursive trapezoidal filter
      double dkl = 0.0;
      double pn_1 = 0.0;
      double pn = 0.0;
      double rn = 0.0;
      double sn = 0.0;
      double sn_1 = 0.0;
      
      double t1 = -1; // trigger time (linear interpolated)
      int trigID    = -1;
      int t1_v[5];    // Allow for up to 5 triggers per waveform. 
                      // More than that you are doing something wrong!
      int trigIndex_v[5];
      int trigIndex = -1;
      int armedIndex = -1; // index where trigger was armed
      int trigPerEvent = 0;
      double energy = 0;
      int event = -1;
      
      otree->Branch("eventID",&event,"eventID/I");
      otree->Branch("ntriggers",&trigPerEvent,"ntriggers/I");
      otree->Branch("trigID",&trigID,"trigID/I");
      otree->Branch("energy",&energy,"energy/D");
      otree->Branch("t1",&t1,"t1/D");
      
      double TM = Ttau/Tclk - 0.5; // This approximation is totally valid.
      
      size = dataFile.tellg();
      buffer = new unsigned short[size];
      point = buffer;
      dataFile.seekg(0, ios::beg);
      dataFile.read((char*)buffer , size);
      //single event in Nbytes
      
      int OneEventSize = (*(buffer+1)<<16)+(*buffer);  // First character in the buffer is 
                                                       // the size of a single event
      int RecordLength = (OneEventSize - 24)/2;
      int Nevents = size/OneEventSize;

      cout << "Size of file: "   << size         << " bytes" << endl;
      cout << "OneEventSize "    << OneEventSize <<   endl;
      cout << "Nevents "         << Nevents      << endl;
      cout << "Record Length = " << RecordLength << endl;
      
      //Check that arrays are the right size.
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
	  d1   = 0;   // d1[n]
	  d1_1 = 0;   //      d1[n-1]
	  d2   = 0;   // d2[n]
	  d2_1 = 0;   //      d2[n-1]
	  dkl  = 0.0; // 
	  pn   = 0.0; // p[n]
	  pn_1 = 0.0; //      p[n-1]
	  rn   = 0.0;
	  sn   = 0.0; // sn[n]
	  sn_1 = 0.0; //      sn[n-1]
	  MaxD1 = 0;
	  MinD1 = 0;
	  MaxD2 = 0;
	  MinD2 = 0;
	  MaxT = 0.0;
	  MinT = 0.0;
	  
	  armed = false; // reset any previous triggers
	  holdOff = false; // no trigger hold off
	  t1 = -1; // set default time to unphysical value
	  armedIndex = -1;
	  trigIndex = -1;
	  trigPerEvent = 0;
	  
	  // Check that the header has the same event size
	  if(OneEventSize != (*(point+1)<<16)+(*point) )
	    {
	      cerr << "Problem Reading data file. Unexpected Event Size" << endl;
	      delete[] buffer;
	      return -2;
	    }

	  // then skip the header
	  point+=headerlines; 
	  
	  //here we go over the waveform
	  for(int i=0; i<NSAMPLES; i++)
	    {		
	      //store values for next step before modifying them
	      pn_1 = pn;
	      sn_1 = sn;
	      d1_1 = d1;
	      d2_1 = d2;
	      
	      if(i>Dk+Dt)
		{
		  // First Derivative with smoothing
		  d1 = d1_1 + (*point) - (*(point - Dk)) - (*(point-Dt)) + (*(point-Dk-Dt));
		  MaxD1 = (d1>MaxD1) ? d1 : MaxD1;
		  MinD1 = (d1<MinD1) ? d1 : MinD1;		 
		}

	      if(i>DDk+DDt+DDr)
		{ 
		  //Second Derivative with first order smoothing
		  d2 = d2_1 + (*point) - (*(point-DDk)) - (*(point-DDt)) - (*(point-DDr))
		    + (*(point-DDk-DDr)) + (*(point-DDt-DDr)) + (*(point-DDk-DDt))
		    - (*(point-DDk-DDt-DDr));
		  
		  MaxD2 = (d2>MaxD2) ? d2 : MaxD2;
		  MinD2 = (d2<MinD2) ? d2 : MinD2;		  
		}

	      if(i>Tk+Tl)
		{
		  dkl = (double)( (*point) - (*(point-Tk)) - (*(point-Tl)) + (*(point-Tk-Tl)) ); // (1)
		  dkl /= (Tk*Tl);
		  pn  = pn_1 + dkl;                                                  // (2)
		  rn  = pn   + TM*dkl;                                               // (3)  
		  sn  = sn_1 + rn;                                                   // (4)
		  MinT = (sn<MinT) ? sn : MinT;
		  MaxT = (sn>MaxT) ? sn : MaxT;
		}

       	      w_v[i]  = (polarity>0) ? *point : 16384 - (*point);
	      sn_v[i] = sn*polarity;
	      d1_v[i] = d1*polarity;
	      d2_v[i] = d2*polarity;

	      // Trigger on second derivative
	      if(d2*polarity>thresh && !holdOff)
		{
		  armed = true;
		  holdOff = true;
		  armedIndex = i;
		}
	      //Mark zero crossing in second derivative
	      if(armed && d2*polarity<0 && d2_1*polarity>0 && i<armedIndex+trigDelay)
		{
		  trigIndex_v[trigPerEvent] = i; 
		  t1_v[trigPerEvent] = ( (double)trigIndex + d2_1/(d2_1-d2) )*Tclk; 
		  trigPerEvent++;
		  armed = false; // reset armed, but not hold off.
		}
	      else if(armed && d2==0 && i<armedIndex+trigDelay)
		{
		  trigIndex = i;
		  t1 = trigIndex*Tclk;
		  trigPerEvent++;
		  armed = false;
		}
	      if(armed && i>armedIndex + trigHO)
		{
		  holdOff = false;
		  armed = false;
		}
	      // end trigger on second derivative
	      
	      // Build Plots
	      graphW->SetPoint(i,i*Tclk,w_v[i]);
	      graphT->SetPoint(i,i*Tclk,sn_v[i]);
	      graphD->SetPoint(i,i*Tclk,d1_v[i]);
	      graphD2->SetPoint(i,i*Tclk,d2_v[i]);
	      //Iterate our data pointer
	      point++;
	    }// end loop over waveform

	  // Check that length of event buffer is big enough
	  if(trigPerEvent > 5)
	    {
	      cerr << "too many triggers per event" << endl;
	      delete[] buffer;
	      return -1;
	    }
	  
	  // Fill root tree
	  for(int tr = 0; tr < trigPerEvent; tr++)
	    { 
	      trigIndex = trigIndex_v[tr];
	      t1        = t1_v[tr];
	      trigID    = tr;
	      for(int s = trigIndex + Tk + Tdelay; s< trigIndex + Tk + Tdelay + Twidth; s++)
		{
		  energy += sn_v[s]/Twidth;
		}
	      if(energy > 0)
		{
		  otree->Fill(); // Fill 
		  energy = 0; // Reset
		}
	    

	      c0->cd();												  
	      
	      tTrig->SetX1(trigIndex*Tclk);
	      tTrig->SetY1(MinD2);
	      tTrig->SetY2(MaxD2);
	      tTrig->SetVertical();
	      tTrig->SetLineColor(2);
	      tStart->SetX1(trigIndex*Tclk+Tk*Tclk+Tdelay*Tclk);
	      tStart->SetY2(MaxT);
	      tStart->SetY1(MinT);
	      tStart->SetVertical();
	      tStop->SetX1(trigIndex*Tclk+Tk*Tclk+Tdelay*Tclk+Twidth*Tclk);
	      tStop->SetY2(MaxT);
	      tStop->SetY1(MinT);
	      tStop->SetVertical();
	      tArmed->SetLineColor(3);
	      tArmed->SetX1(armedIndex*Tclk);
	      tArmed->SetY2(MaxD2);
	      tArmed->SetY1(MinD2);
	      tArmed->SetVertical();
	      tHO->SetX1(armedIndex*Tclk+trigHO*Tclk);
	      tHO->SetY2(MaxD2);
	      tHO->SetY1(MinD2);
	      tHO->SetVertical();
	      tDelay->SetX1(armedIndex*Tclk+trigDelay*Tclk);
	      tDelay->SetY2(MaxD2);
	      tDelay->SetY1(MinD2);
	      tDelay->SetVertical();
	      
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
	      tTrig->Draw("SAME");
	      tDelay->Draw("SAME");
	      c0->Update();			
	      c1->Update();
	      c2->Update();
	      c3->Update();
	      c0->cd();
	      c0->WaitPrimitive();
	    }
	  loadbar(event,Nevents);	  
	}
      loadbar(Nevents,Nevents);
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

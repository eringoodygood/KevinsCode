{
  TFile *f1 = new TFile("chan0_back4.root");
  TFile *f2 = new TFile("chan0_back5.root");
  TFile *f3 = new TFile("chan0_back6.root");

  TH2F* h1 = (TH2F*)f1->Get("hLvE");
   h1->SetName("h1");
 
  TH2F* h2 = (TH2F*)f2->Get("hLvE");
  h2->SetName("h2");

  TH2F* h3 = (TH2F*)f3->Get("hLvE");
  h3->SetName("h3");

  h1->SetTitle("Silicon Position vs. Remaining Alpha Energy");
  h1->GetYaxis()->SetTitle("Energy Deposited in Silicon");
  h1->GetXaxis()->CenterTitle();
  h1->GetYaxis()->CenterTitle();
  h1->GetXaxis()->SetTitle("\\text{Position  } \\left\( Q_{L}/Q_{B} \\approx \\frac{x}{L}\\right\)");
  h1->SetMarkerStyle(6);
  //  h1->SetMarkerSize(5);
  h2->SetMarkerStyle(6);
  h2->SetMarkerColor(2);
  h3->SetMarkerStyle(6);
  h3->SetMarkerColor(3);
  //  h2->SetMarkerSize(5);
  h1->Draw();
  h2->Draw("SAME");
  h3->Draw("SAME");
}

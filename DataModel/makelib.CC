{
TString compilerflags = "-fdiagnostics-color=always -Wno-sign-compare";
TString pwd = gSystem->Getenv("PWD");

TString wcsimlibraryloc = pwd + "/DataModel";
TString wcsimincludeloc = wcsimlibraryloc;// + "/include/";
gInterpreter->AddIncludePath(wcsimincludeloc);

TString wcsimlibrarypath = wcsimlibraryloc + "/libWCSimRoot.so";
gSystem->Load(wcsimlibrarypath);
gSystem->Load("libWCSimRoot");

TString callcommand = ".L "+pwd+"/../../wcsim/root_work/analysiscaller.cxx+g";
gROOT->ProcessLine(callcommand.Data());

//TString analyzecommand = "WCSimAnalysis* theana = new WCSimAnalysis()";
//gROOT->ProcessLine(analyzecommand.Data());
}

void extract_timestamp(const char* rawfile){

	TFile *f = new TFile(rawfile);
	TTree *t = (TTree*) f->Get("TrigData");

	ULong64_t timestamp[256];
	t->SetBranchAddress("EventTimes",&timestamp);

	int nentries=t->GetEntries();

	for (int i=0; i<nentries;i++){
		t->GetEntry(i);
		std::cout <<"Entry "<<i<<", timestamp: "<<timestamp[0]<<std::endl;

	}
	f->Close();
	delete f;




}

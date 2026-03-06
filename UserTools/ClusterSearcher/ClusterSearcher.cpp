#include "ClusterSearcher.h"

static ClusterSearcher* fgClusterSearcher = 0;

ClusterSearcher* ClusterSearcher::Instance()
{
  if( !fgClusterSearcher ){
    fgClusterSearcher = new ClusterSearcher();
  }

  return fgClusterSearcher;
}

ClusterSearcher::ClusterSearcher():Tool(){}
	
ClusterSearcher::~ClusterSearcher() {
  
}

bool ClusterSearcher::Initialise(std::string configfile, DataModel &data){
    if(verbosity)cout<<"Initializing ClusterSearcher"<<endl;
  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  

  
  /// Get the Tool configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("IsMC",fisMC);
  m_variables.Get("ClusterMode",fClusterMode);
  m_variables.Get("PmtMinPulseHeight", fPmtMinPulseHeight);
  m_variables.Get("PmtNeighbourRadius", fPmtNeighbourRadius);
  m_variables.Get("PmtMinNeighbourDigits", fPmtMinNeighbourDigits);
  m_variables.Get("PmtClusterRadius", fPmtClusterRadius);
  m_variables.Get("PmtTimeWindowN", fPmtTimeWindowN);	
  m_variables.Get("PmtTimeWindowC", fPmtTimeWindowC);
  m_variables.Get("PmtMinHitsPerCluster", fPmtMinHitsPerCluster);	
  m_variables.Get("LappdMinPulseHeight", fLappdMinPulseHeight);
  m_variables.Get("LappdNeighbourRadius", fLappdNeighbourRadius);
  m_variables.Get("LappdMinNeighbourDigits", fLappdMinNeighbourDigits);
  m_variables.Get("LappdClusterRadius", fLappdClusterRadius );
  m_variables.Get("LappdTimeWindowN", fLappdTimeWindowN );
  m_variables.Get("LappdTimeWindowC", fLappdTimeWindowC );
  m_variables.Get("LappdMinHitsPerCluster", fLappdMinHitsPerCluster );
  m_variables.Get("MinClusterDigits", fMinClusterDigits );
  m_variables.Get("SinglePEGains",singlePEgains);
  m_variables.Get("FirstRun",fFirstRun);

  /// Fill map with settings of ClusterSearcher
  fClusteringParam = new std::map<std::string,double>;
  fClusteringParam->emplace("ClusterMode",fClusterMode);
  fClusteringParam->emplace("PmtMinPulseHeight",fPmtMinPulseHeight);
  fClusteringParam->emplace("PmtNeighbourRadius",fPmtNeighbourRadius);
  fClusteringParam->emplace("PmtMinNeighbourDigits",fPmtMinNeighbourDigits);
  fClusteringParam->emplace("PmtClusterRadius",fPmtClusterRadius);
  fClusteringParam->emplace("PmtTimeWindowN",fPmtTimeWindowN);
  fClusteringParam->emplace("PmtTimeWindowC",fPmtTimeWindowC);
  fClusteringParam->emplace("LappdMinPulseHeight",fLappdMinPulseHeight);
  fClusteringParam->emplace("LappdNeighbourRadius",fLappdNeighbourRadius);
  fClusteringParam->emplace("LappdMinNeighbourDigits",fLappdMinNeighbourDigits);
  fClusteringParam->emplace("LappdClusterRadius",fLappdClusterRadius);
  fClusteringParam->emplace("LappdTimeWindowN",fLappdTimeWindowN);
  fClusteringParam->emplace("LappdTimeWindowC",fLappdTimeWindowC);
  fClusteringParam->emplace("MinClusterDigits",fMinClusterDigits);

  Log("ClusterSearcher " + to_string(fClusterMode) + " configs Initialized", v_debug, verbosity);
  if (!fisMC){
    ifstream file_singlepe(singlePEgains.c_str());
    unsigned long temp_chankey;
    double temp_gain;
    while (!file_singlepe.eof()){
      file_singlepe >> temp_chankey >> temp_gain;
      pmt_gains.emplace(temp_chankey,temp_gain);
    }
    file_singlepe.close();
    m_data->CStore.Get("pmt_tubeid_to_channelkey",pmt_tubeid_to_channelkey);
    Log("ClusterSearcher " + to_string(fClusterMode) + " !MC Initialized", v_debug, verbosity);
  }

  // vector of selected digits
  fSelectAll = new std::vector<RecoDigit*>;
  fSelectByPulseHeight = new std::vector<RecoDigit*>;
  fSelectByNeighbours = new std::vector<RecoDigit*>;
  fSelectByClusters = new std::vector<RecoDigit*>;
  // only for test 
  fSelectByTruthInfo = new std::vector<RecoDigit*>; 
  // vector of clusters
  fClusterList = new std::vector<RecoCluster>;
  fRecoClusters = new std::vector<RecoCluster>;

  //Set clustering parameters in the RecoEvent store
  std::map<std::string, double>* pre_ClusteringParam = nullptr; // read existing container for parameters
  bool cluster_parameter_status = m_data->Stores.at("RecoEvent")->Get("ClusteringParameters", pre_ClusteringParam);
  if(!cluster_parameter_status) m_data->Stores.at("RecoEvent")->Set("ClusteringParameters", fClusteringParam);
  else {
    pre_ClusteringParam->insert(fClusteringParam->begin(), fClusteringParam->end());
    //m_data->Stores.at("RecoEvent")->Set("ClusteringParameters", pre_ClusteringParam);   
  }
  Log("ClusterSearcher "+to_string(fClusterMode)+" Initialized",v_debug,verbosity);
  return true;
}


bool ClusterSearcher::Execute(){
    if (fRecoClusters) Log("Prelim check1: fRecoClusters size: " + to_string(fRecoClusters->size()), v_debug, verbosity);
    if (fRecoClusters->size()!=0) fRecoClusters->clear();
    
    if(fFirstRun && pre_RecoClusters) pre_RecoClusters->clear();
  
  std::string name = "ClusterSearcher::Execute()";
  Log(name + ": Executing",v_error,verbosity);
	
  // print Selecting parameters
  if(verbosity>v_message) this->PrintParameters();
	
  // see if "ANNIEEvent" exists
  auto get_annieevent = m_data->Stores.count("ANNIEEvent");
  if(!get_annieevent){
    Log(name + ": No ANNIEEvent store!",v_error,verbosity); 
    return false;
  }
	
  /// see if "RecoEvent" exists
  auto get_recoevent = m_data->Stores.count("RecoEvent");
  if(!get_recoevent){
    Log(name + ": No RecoEvent store!",v_error,verbosity); 
    return false;
  }

  if (fisMC) {
    // get true vertex
    auto get_truevtx = m_data->Stores.at("RecoEvent")->Get("TrueVertex", fTrueVertex);
    if(!get_truevtx){ 
      Log(name + ": Error retrieving TrueVertex from RecoEvent!",v_error,verbosity); 
      return false; 
    }
  }
 
  // get digit list
  auto get_recodigit = m_data->Stores.at("RecoEvent")->Get("RecoDigit",fDigitList);  ///> Get digits from "RecoEvent" 
  if(!get_recodigit){ 
    Log("VtxSeedGenerator  Tool: Error retrieving RecoDigits,no digit from the RecoEvent store!",v_error,verbosity); 
    return false;
  }
  // copy to a new digit list
  std::vector<RecoDigit*>* digits = new std::vector<RecoDigit*>;
  for(int i=0;i<int(fDigitList->size());i++) {
  	RecoDigit* recodigitptr = &(fDigitList->at(i));
    digits->push_back((RecoDigit*)recodigitptr);
  }
  
  // Reset Digit Filter
  // ============
  for(int n=0; n<int(digits->size()); n++ ) {
    digits->at(n)->ResetFilter();
  }
  
  // Set Digit Filter
  // ==========
  SelectDigits(digits);
  
  fRecoClusters = this->RecoClusters(digits);
  // Digit clustering done!
  // =====
  
  pre_RecoClusters = nullptr;   // to read existing clusters
  bool cluster_status = m_data->Stores.at("RecoEvent")->Get("RecoClusters", pre_RecoClusters);
  
  Log("ClusterSearcher Tool: Final count of clusters: "+to_string(fRecoClusters->size()),v_debug,verbosity);
  if(!cluster_status) {
      pre_RecoClusters=new std::vector<RecoCluster>;
      for (int i = 0; i < fRecoClusters->size(); i++) {
          pre_RecoClusters->push_back(fRecoClusters->at(i));
      }
    fIsHitClusteringDone = true;
    Log("Hit Clustering done",v_debug,verbosity);
    m_data->Stores.at("RecoEvent")->Set("HitClusteringDone", fIsHitClusteringDone);
    m_data->Stores.at("RecoEvent")->Set("RecoClusters", pre_RecoClusters); 
  }
  else {
    pre_RecoClusters->insert(pre_RecoClusters->end(), fRecoClusters->cbegin(), fRecoClusters->cend()); // append new clusters to the existing cluster list
    //m_data->Stores.at("RecoEvent")->Set("RecoClusters", pre_RecoClusters); // save updated clusters
    //fRecoClusters->clear();
  }

  if (!fFirstRun && pre_RecoClusters->size() == 0) {
      Log("ClusterSearcher: NO CLUSTERS FOUND IN EVENT!",v_debug,verbosity);
      Log("Digit list size: "+to_string(fDigitList->size()),v_debug,verbosity);
  }

  delete digits; digits = 0;
  return true;
}

bool ClusterSearcher::Finalise(){
  //delete fClusteringParam; fClusteringParam = 0;      //Will be deleted by the store, don't manually delete
  delete fSelectAll; fSelectAll = 0;
  delete fSelectByPulseHeight; fSelectByPulseHeight = 0;
  delete fSelectByNeighbours; fSelectByNeighbours = 0;
  delete fSelectByClusters; fSelectByClusters = 0;
  //delete fRecoClusters; fRecoClusters = 0;    //Will be deleted by the store, don't manually delete
  delete fClusterList; fClusterList = 0;
  // for test
  delete fSelectByTruthInfo; fSelectByTruthInfo = 0;
  return true;
}

/*void ClusterSearcher::Config(int config)
{
  ClusterSearcher::Instance()->SetConfig(config);
}*/

void ClusterSearcher::PmtMinPulseHeight(double min)
{
  ClusterSearcher::Instance()->SetPmtMinPulseHeight(min);
}
  
void ClusterSearcher::PmtNeighbourRadius(double radius)
{
  ClusterSearcher::Instance()->SetPmtNeighbourRadius(radius);
}
  
void ClusterSearcher::PmtNeighbourDigits(int digits)
{
  ClusterSearcher::Instance()->SetPmtNeighbourDigits(digits);
}
  
void ClusterSearcher::PmtClusterRadius(double radius)
{
  ClusterSearcher::Instance()->SetPmtClusterRadius(radius);
}
  
void ClusterSearcher::MinClusterDigits(int digits)
{
  ClusterSearcher::Instance()->SetMinClusterDigits(digits);
}
  
void ClusterSearcher::PmtTimeWindowN(double windowN)
{
  ClusterSearcher::Instance()->SetPmtTimeWindowNeighbours(windowN);
}

void ClusterSearcher::PmtTimeWindowC(double windowC)
{
  ClusterSearcher::Instance()->SetPmtTimeWindowClusters(windowC);
}

void ClusterSearcher::PrintParameters()
{
  std::cout << " *** ClusterSearcher::PrintParameters() *** " << std::endl;
  
  std::cout << "  Clustering Parameters: " << std::endl
            << "   ClusterMode = " << fClusterMode<< std::endl
            << "   PmtMinPulseHeight = " << fPmtMinPulseHeight << std::endl
            << "   PmtNeighbourRadius = " << fPmtNeighbourRadius << std::endl
            << "   PmtMinNeighbourDigits = " << fPmtMinNeighbourDigits << std::endl
            << "   PmtClusterRadius = " << fPmtClusterRadius << std::endl
            << "   PmtTimeWindowN = " << fPmtTimeWindowN << std::endl
	          << "   PmtTimeWindowC = " << fPmtTimeWindowC << std::endl
            << "   LappdMinPulseHeight = " << fLappdMinPulseHeight << std::endl
            << "   LappdNeighbourRadius = " << fLappdNeighbourRadius << std::endl
            << "   LappdMinNeighbourDigits = " << fLappdMinNeighbourDigits << std::endl
            << "   LappdClusterRadius = " << fLappdClusterRadius << std::endl
            << "   LappdTimeWindowN = " << fLappdTimeWindowN << std::endl
	          << "   LappdTimeWindowC = " << fLappdTimeWindowC << std::endl
	          << "   MinClusterDigits = " << fMinClusterDigits << std::endl;
}

void ClusterSearcher::Reset()
{

  return;
}

// Set all filter status to 0 (default is 1)
std::vector<RecoDigit*>* ClusterSearcher::ResetDigits(std::vector<RecoDigit*>* DigitList)
{
  for(int idigit=0; idigit<int(DigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(DigitList->at(idigit));
    recoDigit->ResetFilter();
  }

  return DigitList;
}

std::vector<RecoDigit*>* ClusterSearcher::SelectDigits(std::vector<RecoDigit*>* DigitList)
{
  for(int idigit=0; idigit<int(DigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(DigitList->at(idigit));
    recoDigit->PassFilter();
  }

  return DigitList;
}

void ClusterSearcher::SelectDigits(std::vector<RecoDigit>* DigitList)
{
    for (int idigit = 0; idigit<int(DigitList->size()); idigit++) {
        RecoDigit recoDigit = (RecoDigit)(DigitList->at(idigit));
        recoDigit.PassFilter();
    }

    return;
}

std::vector<RecoDigit*>* ClusterSearcher::SelectAll(std::vector<RecoDigit*>* DigitList)
{
	std::string name = "ClusterSearcher::SelectAll() ";
  // clear vector of selected digits
  // ==============================
  fSelectAll->clear();
  // select all digits
  // =================
  for(int idigit=0; idigit<int(DigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(DigitList->at(idigit));
    fSelectAll->push_back(recoDigit);
  }

  // return vector of selected digits
  // ================================
  if(verbosity>v_message) std::cout << name << "  select all: " << fSelectAll->size() << std::endl;
  
  return fSelectAll;
}

std::vector<RecoDigit*>* ClusterSearcher::SelectByPulseHeight(std::vector<RecoDigit*>* DigitList)
{
  std::string name = "ClusterSearcher::SelectByPulseHeight() ";
  // clear vector of selected digits
  // ===============================
  fSelectByPulseHeight->clear();

  // Select by pulse height
  // ======================
  for(int idigit=0; idigit<int(DigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(DigitList->at(idigit));
    double qep = recoDigit->GetCalCharge();
    if (!fisMC){
      int pmtid = recoDigit->GetDetectorID();
      unsigned long chankey = pmt_tubeid_to_channelkey[pmtid];
      if (pmt_gains[chankey]>0) qep/=pmt_gains[chankey];
    }
    int detType = recoDigit->GetDigitType();
    if(detType == RecoDigit::lappd_v0) {
    	if( qep>fLappdMinPulseHeight ){
        fSelectByPulseHeight->push_back(recoDigit);
      }
    }
    else if(detType == RecoDigit::PMT8inch) {
    	if( qep>fPmtMinPulseHeight ){
        fSelectByPulseHeight->push_back(recoDigit);
      }
    }
    else Log(name + "<error:> detector type doesn't exist!",v_error,verbosity);
  }


  // return vector of selected digits
  // ================================
  if(verbosity>v_message) std::cout <<name << "  Select by pulse height: " << fSelectByPulseHeight->size() << std::endl;
  
  return fSelectByPulseHeight;
}

std::vector<RecoDigit*>* ClusterSearcher::SelectByNeighbours(std::vector<RecoDigit*>* DigitList)
{
	std::string name = "ClusterSearcher::SelectByNeighbours() ";
  // clear vector of Selected digits
  // ===============================
  fSelectByNeighbours->clear();

  // create array of neighbours
  // ==========================
  int Ndigits = DigitList->size();

  if( Ndigits<=0 ){
    return fSelectByNeighbours;
  }

  int* numNeighbours = new int[Ndigits];

  for( int idigit=0; idigit<Ndigits; idigit++ ){
    numNeighbours[idigit] = 0;
  }

  // count number of neighbours
  // ==========================
  for(int idigit1=0; idigit1<int(DigitList->size()); idigit1++ ){
  	RecoDigit* fdigit1 = (RecoDigit*)(DigitList->at(idigit1));
  	TString digit1Type = fdigit1->GetDigitType();
    for(int idigit2=idigit1+1; idigit2<int(DigitList->size()); idigit2++ ){
      RecoDigit* fdigit2 = (RecoDigit*)(DigitList->at(idigit2));
      TString digit2Type = fdigit2->GetDigitType();

      double dx = fdigit1->GetPosition().X() - fdigit2->GetPosition().X();
      double dy = fdigit1->GetPosition().Y() - fdigit2->GetPosition().Y();
      double dz = fdigit1->GetPosition().Z() - fdigit2->GetPosition().Z();
      double dt = fdigit1->GetCalTime() - fdigit2->GetCalTime();
      double drsq = dx*dx + dy*dy + dz*dz;
      
      if(digit1Type == RecoDigit::PMT8inch && digit2Type == RecoDigit::PMT8inch) {
        if( drsq>0.0
         && drsq<fPmtNeighbourRadius*fPmtNeighbourRadius
         && fabs(dt)<fPmtTimeWindowN ){
          numNeighbours[idigit1]++;
          numNeighbours[idigit2]++;
        }	
      }
      
      if(digit1Type == RecoDigit::lappd_v0 && digit2Type == RecoDigit::lappd_v0) {
        if( drsq>0.0
         && drsq<fLappdNeighbourRadius*fLappdNeighbourRadius
         && fabs(dt)<fLappdTimeWindowN ){
          numNeighbours[idigit1]++;
          numNeighbours[idigit2]++;
        }	
      }
      
      if(digit1Type == RecoDigit::PMT8inch && digit2Type == RecoDigit::lappd_v0) {
        if( drsq>0.0
            && drsq<fPmtNeighbourRadius*fPmtNeighbourRadius
            && fabs(dt)<fPmtTimeWindowN ){
          numNeighbours[idigit1]++;
        }	
        if( drsq>0.0
            && drsq<fLappdNeighbourRadius*fLappdNeighbourRadius
            && fabs(dt)<fLappdTimeWindowN ){
          numNeighbours[idigit2]++;
        }	   
      }

      if(digit1Type == RecoDigit::lappd_v0 && digit2Type == RecoDigit::PMT8inch) {
        if( drsq>0.0
            && drsq<fLappdNeighbourRadius*fLappdNeighbourRadius
            && fabs(dt)<fLappdTimeWindowN ){
          numNeighbours[idigit1]++;
        }	
        if( drsq>0.0
            && drsq<fPmtNeighbourRadius*fPmtNeighbourRadius
            && fabs(dt)<fPmtTimeWindowN ){
          numNeighbours[idigit2]++;
        }	   
      }
    }
  }

  // Select by number of neighbours
  // ==============================
  for(int idigit=0; idigit<int(DigitList->size()); idigit++){
    RecoDigit* fdigit = (RecoDigit*)(DigitList->at(idigit));
    //std::cout << "numNeighbours[" << idigit << "] = " << numNeighbours[idigit] << std::endl;
    if( numNeighbours[idigit]>=fPmtMinNeighbourDigits ){
      fSelectByNeighbours->push_back(fdigit);
    }
  }

  // delete array of neighbours
  // ==========================
  delete [] numNeighbours;

  // return vector of Selected digits
  // ================================
  if(verbosity>v_message) std::cout << name << "  Select by neighbours: " << fSelectByNeighbours->size() << std::endl;
  
  
  return fSelectByNeighbours;
}

std::vector<RecoDigit*>* ClusterSearcher::SelectByClusters(std::vector<RecoDigit*>* DigitList)
{
	std::string name = "ClusterSearcher::SelectByClusters() ";
  // clear vector of Selected digits
  // ===============================
  fSelectByClusters->clear();
  //fRecoClusters->clear();

  // run clustering algorithm
  // ========================
  std::vector<RecoCluster>* ClusterList = RecoClusters(DigitList);

  for(int icluster=0; icluster<int(ClusterList->size()); icluster++ ){
    RecoCluster Cluster = (ClusterList->at(icluster));
    fRecoClusters->push_back(Cluster);    

    for(int idigit=0; idigit<Cluster.GetNDigits(); idigit++ ){
      RecoDigit* Digit = new RecoDigit;
      *Digit=(Cluster.GetDigit(idigit));
      fSelectByClusters->push_back(Digit);
    }
  }
  
  // return vector of Selected digits
  // ================================
  if(verbosity>v_message) std::cout << name <<"  Select by clusters: " << fSelectByClusters->size() << std::endl;
  
  
  return fSelectByClusters;
}

std::vector<RecoCluster>* ClusterSearcher::RecoClusters(std::vector<RecoDigit*>* DigitList)
{  

  // delete cluster digits
  // =====================
  for(int i=0; i<int(vClusterDigitList.size()); i++ ){
    delete (RecoClusterDigit*)(vClusterDigitList.at(i));
  }
  vClusterDigitList.clear();

  // clear vector clusters
  // =====================
  fClusterList->clear();

  //Digit Selection
  SelectByPulseHeight(DigitList);
  SelectByNeighbours(fSelectByPulseHeight);

  // make cluster digits
  // ===================
  for(int idigit=0; idigit<int(fSelectByNeighbours->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(fSelectByNeighbours->at(idigit));
    Log("Check RC: Does the digit have a parent? "+to_string(recoDigit->GetParents().size()), v_debug, verbosity);
    RecoClusterDigit* clusterDigit = new RecoClusterDigit(recoDigit);
    vClusterDigitList.push_back(clusterDigit);
  }

  // run clustering algorithm
  // ========================
  for(int idigit1=0; idigit1<int(vClusterDigitList.size()); idigit1++){
  	RecoClusterDigit* fdigit1 = (RecoClusterDigit*)(vClusterDigitList.at(idigit1));
  	int digit1Type = fdigit1->GetDigitType();
    for(int idigit2=idigit1+1; idigit2<int(vClusterDigitList.size()); idigit2++ ){

      RecoClusterDigit* fdigit2 = (RecoClusterDigit*)(vClusterDigitList.at(idigit2));
      int digit2Type = fdigit2->GetDigitType();

      double dx = fdigit1->GetX() - fdigit2->GetX();
      double dy = fdigit1->GetY() - fdigit2->GetY();
      double dz = fdigit1->GetZ() - fdigit2->GetZ();
      double dt = fdigit1->GetTime() - fdigit2->GetTime();
      double drsq = dx*dx + dy*dy + dz*dz;
      if(digit1Type == RecoDigit::PMT8inch && digit2Type == RecoDigit::PMT8inch) {
        if( drsq>0.0
         && drsq<fPmtClusterRadius*fPmtClusterRadius
         && fabs(dt)<fPmtTimeWindowC ){
          fdigit1->AddClusterDigit(fdigit2);
          fdigit2->AddClusterDigit(fdigit1);
        }	
        if (drsq == 0 && fabs(dt) < fPmtTimeWindowC) {
            fdigit2->SetClustered();
        }
      }
      
      if(digit1Type == RecoDigit::lappd_v0 && digit2Type == RecoDigit::lappd_v0) {
        if( drsq>0.0
         && drsq<fLappdClusterRadius*fLappdClusterRadius
         && fabs(dt)<fLappdTimeWindowC ){
          fdigit1->AddClusterDigit(fdigit2);
          fdigit2->AddClusterDigit(fdigit1);
        }	
      }
      
      if(digit1Type == RecoDigit::PMT8inch && digit2Type == RecoDigit::lappd_v0) {
        if( drsq>0.0
         && drsq<fPmtClusterRadius*fPmtClusterRadius
         && fabs(dt)<fPmtTimeWindowC ){
          fdigit1->AddClusterDigit(fdigit2);
        }	
        if( drsq>0.0
         && drsq<fLappdClusterRadius*fLappdClusterRadius
         && fabs(dt)<fLappdTimeWindowC ){
          fdigit2->AddClusterDigit(fdigit1);
        }
      }
      
      if(digit1Type == RecoDigit::lappd_v0 && digit2Type == RecoDigit::PMT8inch) {
        if( drsq>0.0
         && drsq<fLappdClusterRadius*fLappdClusterRadius
         && fabs(dt)<fLappdTimeWindowC ){
          fdigit1->AddClusterDigit(fdigit2);
        }	
        if( drsq>0.0
         && drsq<fPmtClusterRadius*fPmtClusterRadius
         && fabs(dt)<fPmtTimeWindowC ){
          fdigit2->AddClusterDigit(fdigit1);
        }	
      }
      
    }
  }
  
  // collect up clusters
  // ===================
  Bool_t carryon = 0;
  for(int idigit=0; idigit<int(vClusterDigitList.size()); idigit++ ){
    RecoClusterDigit* fdigit = (RecoClusterDigit*)(vClusterDigitList.at(idigit));

    if( fdigit->IsClustered()==0 && fdigit->GetNClusterDigits()>0 ){
        Log("ClusterSearcher: valid fdigit index: "+to_string(idigit),v_debug,verbosity);
        Log("with time: "+to_string(fdigit->GetTime()),v_debug,verbosity);
        Log("And location: "+to_string(fdigit->GetX())+", "+to_string(fdigit->GetY())+", "+to_string(fdigit->GetZ()),v_debug,verbosity);
      vClusterDigitCollection.clear();
      vClusterDigitCollection.push_back(fdigit);
      fdigit->SetClustered();

      carryon = 1;
      while( carryon ){
        carryon = 0;
        for(int jdigit=0; jdigit<int(vClusterDigitCollection.size()); jdigit++ ){
	  //std::cout <<"jdigit = "<<jdigit<<", vClusterDigitCollection.size() = "<<vClusterDigitCollection.size()<<std::endl;
          RecoClusterDigit* cdigit = (RecoClusterDigit*)(vClusterDigitCollection.at(jdigit));
          TString digitType = cdigit->GetDigitType();
	        double nDigits = cdigit->GetNClusterDigits();
	        vNdigitsCluster.push_back(nDigits);	 
	               
	        if (digitType==RecoDigit::PMT8inch && cdigit->GetNClusterDigits() >= fPmtMinHitsPerCluster) {
                
             if( cdigit->IsAllClustered()==0 ){
               for( int kdigit=0; kdigit<cdigit->GetNClusterDigits(); kdigit++ ){
                 RecoClusterDigit* cdigitnew = (RecoClusterDigit*)(cdigit->GetClusterDigit(kdigit));
                 if( cdigitnew->IsClustered()==0 ){
                   vClusterDigitCollection.push_back(cdigitnew);
                   cdigitnew->SetClustered();
                   carryon = 1;
                 }
               }
             }           
	        }//end if min of # of hits per cluster
	        
	        if (digitType==RecoDigit::lappd_v0 && cdigit->GetNClusterDigits() >= fLappdMinHitsPerCluster) {
             if( cdigit->IsAllClustered()==0 ){
               for( int kdigit=0; kdigit<cdigit->GetNClusterDigits(); kdigit++ ){
                 RecoClusterDigit* cdigitnew = (RecoClusterDigit*)(cdigit->GetClusterDigit(kdigit));
                 if( cdigitnew->IsClustered()==0 ){
                   vClusterDigitCollection.push_back(cdigitnew);
                   cdigitnew->SetClustered();
                   carryon = 1;
                 }
               }
             }           
	        }//end if min of # of hits per cluster
	        
        }
      } 

	//std::cout <<"vClusterDigitCollection.size() == "<<vClusterDigitCollection.size()<<std::endl;
      if( (int)vClusterDigitCollection.size()>=fMinClusterDigits ){
        RecoCluster cluster;
        cluster.SetClusterMode(fClusterMode);
        
        Log("Adding Digits",v_debug,verbosity);
        vector<RecoDigit> ClusteredDigits;

        for(int jdigit=0; jdigit<int(vClusterDigitCollection.size()); jdigit++ ){

          RecoClusterDigit* cdigit = (RecoClusterDigit*)(vClusterDigitCollection.at(jdigit));

          RecoDigit* arecodigit=cdigit->GetRecoDigit();
          arecodigit->AddCluster(fClusterMode);
          RecoDigit brecodigit(arecodigit);
          ClusteredDigits.push_back(brecodigit);

          //cluster.AddDigit(*arecodigit);        
        }
        cluster.SetDigits(ClusteredDigits);
        //ClusteredDigits = nullptr;
        Log("Cluster has " + to_string(cluster.GetNDigits()) + " digits", v_debug, verbosity);
        Log("ready to caluclate parameters.",v_debug,verbosity);    
        cluster.CalcParameters();
        int parent = cluster.calcBestParent();
        Log("Cluster Parent: "+ to_string(parent),v_debug,verbosity);
        fClusterList->push_back(cluster);
        Log("ClusterSearcher: Clusters made: "+to_string(fClusterList->size()),v_debug,verbosity);
        
        
      }
    }
  }
  

  Log("Number of clusters = "+to_string(fClusterList->size()),v_message,verbosity);
  // return vector of clusters
  // =========================
  return fClusterList;
}

std::vector<RecoDigit*>* ClusterSearcher::SelectByTruthInfo(std::vector<RecoDigit*>* DigitList)
{
	std::string name = " ClusterSearcher::SelectByTruthInfo(() ";
  // clear vector of Selected digits
  // ===============================
  fSelectByTruthInfo->clear();
  
  //===================== true info.
  double x0 = fTrueVertex->GetPosition().X();
  double y0 = fTrueVertex->GetPosition().Y();
  double z0 = fTrueVertex->GetPosition().Z();
  double dirx = fTrueVertex->GetDirection().X();
  double diry = fTrueVertex->GetDirection().Y();
  double dirz = fTrueVertex->GetDirection().Z(); 
  double t0 = 0.0;
  	
  // Select by truth
  // ======================
  for(int idigit=0; idigit<int(DigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)DigitList->at(idigit);
    double x = recoDigit->GetPosition().X();
    double y = recoDigit->GetPosition().Y();
    double z = recoDigit->GetPosition().Z();
    double dx = x-x0;
    double dy = y-y0;
    double dz = z-z0;
    double ds = sqrt(dx*dx + dy*dy + dz*dz);
    double px = dx/ds;
    double py = dy/ds;
    double pz = dz/ds;
    double cosphi = px*dirx + py*diry + pz*dirz;
    double phideg = acos(cosphi)/3.14*180;
    if(phideg>38) fSelectByTruthInfo->push_back(recoDigit);   
  }

  // return vector of Selected digits
  // ================================
  if(verbosity>v_message) std::cout << name << "  Select by opening angle: " << fSelectByTruthInfo->size() << std::endl;
  
  return fSelectByTruthInfo;
}


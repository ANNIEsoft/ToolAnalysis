#include "HitCleaner.h"

static HitCleaner* fgHitCleaner = 0;

HitCleaner* HitCleaner::Instance()
{
  if( !fgHitCleaner ){
    fgHitCleaner = new HitCleaner();
  }

  if( !fgHitCleaner ){
    assert(fgHitCleaner);
  }

  if( fgHitCleaner ){

  }

  return fgHitCleaner;
}

HitCleaner::HitCleaner():Tool(){}
	
HitCleaner::~HitCleaner() {
  
}

bool HitCleaner::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  // Default cleaning parameters
  fConfig = HitCleaner::kPulseHeightAndClusters;
  fPmtMinPulseHeight = 5.0;     // minimum pulse height (PEs) //Ioana... initial 1.0
  fPmtNeighbourRadius = 50.0;  // clustering window (cm) //Ioana... intial 300.0
  fPmtMinNeighbourDigits = 2;   // minimum neighbouring digits //Ioana.... initial 2
  fPmtClusterRadius = 50.0;    // clustering window (cm) //Ioana... initia 300.0
  fMinClusterDigits = 2;    // minimum clustered digits
  fPmtTimeWindowN = 10;        // timing window for neighbours (ns)
  fPmtTimeWindowC = 10;        // timing window for clusters (ns)
  fPmtMinHitsPerCluster = -1;   //min # of hits per cluster //Ioana 
  fisMC = 1;			//default: MC 
 
  fLappdMinPulseHeight = -1.0;     // minimum pulse height (PEs) //Ioana... initial 1.0
  fLappdNeighbourRadius = 25.0;  // clustering window (cm) //Ioana... intial 300.0
  fLappdMinNeighbourDigits = 5;   // minimum neighbouring digits //Ioana.... initial 2
  fLappdClusterRadius = 25.0;    // clustering window (cm) //Ioana... initia 300.0
  fLappdTimeWindowN = 10;        // timing window for neighbours (ns)
  fLappdTimeWindowC = 10;        // timing window for clusters (ns)
  fLappdMinHitsPerCluster = 5;   //min # of hits per cluster //Ioana 
  
  /// Get the Tool configuration variables
  m_variables.Get("verbosity",verbosity);
  m_variables.Get("IsMC",fisMC);
  m_variables.Get("Config",fConfig);
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

  /// Fill map with settings of HitCleaner
  fHitCleaningParam = new std::map<std::string,double>;
  fHitCleaningParam->emplace("Config",fConfig);
  fHitCleaningParam->emplace("PmtMinPulseHeight",fPmtMinPulseHeight);
  fHitCleaningParam->emplace("PmtNeighbourRadius",fPmtNeighbourRadius);
  fHitCleaningParam->emplace("PmtMinNeighbourDigits",fPmtMinNeighbourDigits);
  fHitCleaningParam->emplace("PmtClusterRadius",fPmtClusterRadius);
  fHitCleaningParam->emplace("PmtTimeWindowN",fPmtTimeWindowN);
  fHitCleaningParam->emplace("PmtTimeWindowC",fPmtTimeWindowC);
  fHitCleaningParam->emplace("LappdMinPulseHeight",fLappdMinPulseHeight);
  fHitCleaningParam->emplace("LappdNeighbourRadius",fLappdNeighbourRadius);
  fHitCleaningParam->emplace("LappdMinNeighbourDigits",fLappdMinNeighbourDigits);
  fHitCleaningParam->emplace("LappdClusterRadius",fLappdClusterRadius);
  fHitCleaningParam->emplace("LappdTimeWindowN",fLappdTimeWindowN);
  fHitCleaningParam->emplace("LappdTimeWindowC",fLappdTimeWindowC);
  fHitCleaningParam->emplace("MinClusterDigits",fMinClusterDigits);

  if (fConfig!=0 && fConfig !=1 && fConfig !=2 && fConfig !=3 && fConfig !=4){
    Log("HitCleaner tool: Configuration <"+std::to_string(fConfig)+"> not recognized. Setting Config 3 (kPulseHeightAndClusters)",v_error,verbosity);
    fConfig = HitCleaner::kPulseHeightAndClusters;
  }

  if (!fisMC){
    ifstream file_singlepe(singlePEgains.c_str());
    unsigned long temp_chankey;
    double temp_gain;
    while (!file_singlepe.eof()){
      file_singlepe >> temp_chankey >> temp_gain;
      if (file_singlepe.eof()) break;
      pmt_gains.emplace(temp_chankey,temp_gain);
    }
    file_singlepe.close();
    m_data->CStore.Get("pmt_tubeid_to_channelkey",pmt_tubeid_to_channelkey);
  }

  // vector of filtered digits
  fFilterAll = new std::vector<RecoDigit*>;
  fFilterByPulseHeight = new std::vector<RecoDigit*>;
  fFilterByNeighbours = new std::vector<RecoDigit*>;
  fFilterByClusters = new std::vector<RecoDigit*>;
  // only for test 
  fFilterByTruthInfo = new std::vector<RecoDigit*>; 
  // vector of clusters
  fClusterList = new std::vector<RecoCluster*>;
  fHitCleaningClusters = new std::vector<RecoCluster*>;

  //Set hit cleaner parameters in the RecoEvent store
  m_data->Stores.at("RecoEvent")->Set("HitCleaningParameters", fHitCleaningParam);

  return true;
}


bool HitCleaner::Execute(){
	
  
  std::string name = "HitCleaner::Execute()";
  Log(name + ": Executing",v_error,verbosity);
	
  // print filtering parameters
  if(verbosity>v_message) this->PrintParameters();
	
  // see if "ANNIEEvent" exists
  auto get_annieevent = m_data->Stores.count("ANNIEEvent");
  if(!get_annieevent){
    Log(name + ": No ANNIEEvent store!",v_error,verbosity); 
    return false;
  };
	
  /// see if "RecoEvent" exists
  auto get_recoevent = m_data->Stores.count("RecoEvent");
  if(!get_recoevent){
    Log(name + ": No RecoEvent store!",v_error,verbosity); 
    return false;
  };

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
  // Reset Filter
  // ============
  for(int n=0; n<int(digits->size()); n++ ) {
    digits->at(n)->ResetFilter();
  }

  // Run Hit Cleaner
  // ================
  std::vector<RecoDigit*>* FilterDigitList = Run(digits);

  // Set Filter
  // ==========
  for(int n=0; n<int(FilterDigitList->size()); n++ ) {
  	RecoDigit* FilterDigit = (RecoDigit*)(FilterDigitList->at(n));
    FilterDigit->PassFilter();
  }
  
  // Hit cleaning done!
  // =====
  fIsHitCleaningDone = true;
  m_data->Stores.at("RecoEvent")->Set("HitCleaningDone", fIsHitCleaningDone); 
  m_data->Stores.at("RecoEvent")->Set("HitCleaningClusters", fHitCleaningClusters);

  delete digits; digits = 0;
  return true;
}

bool HitCleaner::Finalise(){
  //delete fHitCleaningParam; fHitCleaningParam = 0;      //Will be deleted by the store, don't manually delete
  delete fFilterAll; fFilterAll = 0;
  delete fFilterByPulseHeight; fFilterByPulseHeight = 0;
  delete fFilterByNeighbours; fFilterByNeighbours = 0;
  delete fFilterByClusters; fFilterByClusters = 0;
  //delete fHitCleaningClusters; fHitCleaningClusters = 0;    //Will be deleted by the store, don't manually delete
  delete fClusterList; fClusterList = 0;
  // for test
  delete fFilterByTruthInfo; fFilterByTruthInfo = 0;
  return true;
}

void HitCleaner::Config(int config)
{
  HitCleaner::Instance()->SetConfig(config);
}

void HitCleaner::PmtMinPulseHeight(double min)
{
  HitCleaner::Instance()->SetPmtMinPulseHeight(min);
}
  
void HitCleaner::PmtNeighbourRadius(double radius)
{
  HitCleaner::Instance()->SetPmtNeighbourRadius(radius);
}
  
void HitCleaner::PmtNeighbourDigits(int digits)
{
  HitCleaner::Instance()->SetPmtNeighbourDigits(digits);
}
  
void HitCleaner::PmtClusterRadius(double radius)
{
  HitCleaner::Instance()->SetPmtClusterRadius(radius);
}
  
void HitCleaner::MinClusterDigits(int digits)
{
  HitCleaner::Instance()->SetMinClusterDigits(digits);
}
  
void HitCleaner::PmtTimeWindowN(double windowN)
{
  HitCleaner::Instance()->SetPmtTimeWindowNeighbours(windowN);
}

void HitCleaner::PmtTimeWindowC(double windowC)
{
  HitCleaner::Instance()->SetPmtTimeWindowClusters(windowC);
}

void HitCleaner::PrintParameters()
{
  std::cout << " *** HitCleaner::PrintParameters() *** " << std::endl;
  
  std::cout << "  Data Cleaner Parameters: " << std::endl
            << "   Config = " << fConfig<< std::endl
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

void HitCleaner::Reset()
{

  return;
}

std::vector<RecoDigit*>* HitCleaner::Run(std::vector<RecoDigit*>* myDigitList)
{
  if(verbosity>v_debug) std::cout << " *** HitCleaner::Run(...) *** " << std::endl;
  
  // input digit list
  // ================
  std::vector<RecoDigit*>* myInputList = myDigitList;
  std::vector<RecoDigit*>* myOutputList = myDigitList;
  
  // filter all digits
  // =================
  myInputList = ResetDigits(myOutputList);
  myOutputList = (std::vector<RecoDigit*>*)(this->FilterAll(myInputList));
  myOutputList = FilterDigits(myOutputList);
  if( fConfig==HitCleaner::kNone ) return myOutputList;
  
  // filter by pulse height
  // ======================
  myInputList = ResetDigits(myOutputList);
  myOutputList = (std::vector<RecoDigit*>*)(this->FilterByPulseHeight(myInputList));
  myOutputList = FilterDigits(myOutputList);
  if( fConfig==HitCleaner::kPulseHeight ) return myOutputList;

  // filter using neighbouring digits
  // ================================
  myInputList = ResetDigits(myOutputList);
  myOutputList = (std::vector<RecoDigit*>*)(this->FilterByNeighbours(myInputList));
  myOutputList = FilterDigits(myOutputList);
  if( fConfig==HitCleaner::kPulseHeightAndNeighbours ) return myOutputList;
  	
  // filter using clustered digits
  // =============================
  myInputList = ResetDigits(myOutputList);
  myOutputList = (std::vector<RecoDigit*>*)(this->FilterByClusters(myInputList));
  myOutputList = FilterDigits(myOutputList);
  if( fConfig==HitCleaner::kPulseHeightAndClusters ) return myOutputList;
  	
  if (fisMC){
    // filter using truth information (for simulation test only)
    // =============================
    myInputList = ResetDigits(myOutputList);
    myOutputList = (std::vector<RecoDigit*>*)(this->FilterByTruthInfo(myInputList));
    myOutputList = FilterDigits(myOutputList);
    if( fConfig==HitCleaner::kPulseHeightAndTruthInfo ) return myOutputList;
  }

  // return vector of filtered digits
  // ================================
  return myOutputList;
}

std::vector<RecoDigit*>* HitCleaner::ResetDigits(std::vector<RecoDigit*>* myDigitList)
{
  for(int idigit=0; idigit<int(myDigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(myDigitList->at(idigit));
    recoDigit->ResetFilter();
  }

  return myDigitList;
}

std::vector<RecoDigit*>* HitCleaner::FilterDigits(std::vector<RecoDigit*>* myDigitList)
{
  for(int idigit=0; idigit<int(myDigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(myDigitList->at(idigit));
    recoDigit->PassFilter();
  }

  return myDigitList;
}

std::vector<RecoDigit*>* HitCleaner::FilterAll(std::vector<RecoDigit*>* myDigitList)
{
	std::string name = "HitCleaner::FilterAll() ";
  // clear vector of filtered digits
  // ==============================
  fFilterAll->clear();
  // filter all digits
  // =================
  for(int idigit=0; idigit<int(myDigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(myDigitList->at(idigit));
    fFilterAll->push_back(recoDigit);
  }

  // return vector of filtered digits
  // ================================
  if(verbosity>v_message) std::cout << name << "  filter all: " << fFilterAll->size() << std::endl;
  
  return fFilterAll;
}

std::vector<RecoDigit*>* HitCleaner::FilterByPulseHeight(std::vector<RecoDigit*>* myDigitList)
{
  std::string name = "HitCleaner::FilterByPulseHeight() ";
  // clear vector of filtered digits
  // ===============================
  fFilterByPulseHeight->clear();

  // filter by pulse height
  // ======================
  for(int idigit=0; idigit<int(myDigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(myDigitList->at(idigit));
    double qep = recoDigit->GetCalCharge();
    if (!fisMC){
      int pmtid = recoDigit->GetDetectorID();
      unsigned long chankey = pmt_tubeid_to_channelkey[pmtid];
      if (pmt_gains[chankey]>0) qep/=pmt_gains[chankey];
    }
    int detType = recoDigit->GetDigitType();
    if(detType == RecoDigit::lappd_v0) {
    	if( qep>fLappdMinPulseHeight ){
        fFilterByPulseHeight->push_back(recoDigit);
      }
    }
    else if(detType == RecoDigit::PMT8inch) {
    	if( qep>fPmtMinPulseHeight ){
        fFilterByPulseHeight->push_back(recoDigit);
      }
    }
    else Log(name + "<error:> detector type doesn't exist!",v_error,verbosity);
  }


  // return vector of filtered digits
  // ================================
  if(verbosity>v_message) std::cout <<name << "  filter by pulse height: " << fFilterByPulseHeight->size() << std::endl;
  
  return fFilterByPulseHeight;
}

std::vector<RecoDigit*>* HitCleaner::FilterByNeighbours(std::vector<RecoDigit*>* myDigitList)
{
	std::string name = "HitCleaner::FilterByNeighbours() ";
  // clear vector of filtered digits
  // ===============================
  fFilterByNeighbours->clear();

  // create array of neighbours
  // ==========================
  int Ndigits = myDigitList->size();

  if( Ndigits<=0 ){
    return fFilterByNeighbours;
  }

  int* numNeighbours = new int[Ndigits];

  for( int idigit=0; idigit<Ndigits; idigit++ ){
    numNeighbours[idigit] = 0;
  }

  // count number of neighbours
  // ==========================
  for(int idigit1=0; idigit1<int(myDigitList->size()); idigit1++ ){
  	RecoDigit* fdigit1 = (RecoDigit*)(myDigitList->at(idigit1));
  	TString digit1Type = fdigit1->GetDigitType();
    for(int idigit2=idigit1+1; idigit2<int(myDigitList->size()); idigit2++ ){
      RecoDigit* fdigit2 = (RecoDigit*)(myDigitList->at(idigit2));
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

  // filter by number of neighbours
  // ==============================
  for(int idigit=0; idigit<int(myDigitList->size()); idigit++){
    RecoDigit* fdigit = (RecoDigit*)(myDigitList->at(idigit));
    //std::cout << "numNeighbours[" << idigit << "] = " << numNeighbours[idigit] << std::endl;
    if( numNeighbours[idigit]>=fPmtMinNeighbourDigits ){
      fFilterByNeighbours->push_back(fdigit);
    }
  }

  // delete array of neighbours
  // ==========================
  delete [] numNeighbours;

  // return vector of filtered digits
  // ================================
  if(verbosity>v_message) std::cout << name << "  filter by neighbours: " << fFilterByNeighbours->size() << std::endl;
  
  
  return fFilterByNeighbours;
}

std::vector<RecoDigit*>* HitCleaner::FilterByClusters(std::vector<RecoDigit*>* myDigitList)
{
	std::string name = "HitCleaner::FilterByClusters() ";
  // clear vector of filtered digits
  // ===============================
  fFilterByClusters->clear();
  fHitCleaningClusters->clear();

  // run clustering algorithm
  // ========================
  std::vector<RecoCluster*>* myClusterList = (std::vector<RecoCluster*>*)(this->RecoClusters(myDigitList));

  for(int icluster=0; icluster<int(myClusterList->size()); icluster++ ){
    RecoCluster* myCluster = (RecoCluster*)(myClusterList->at(icluster));
    fHitCleaningClusters->push_back(myCluster);    

    for(int idigit=0; idigit<myCluster->GetNDigits(); idigit++ ){
      RecoDigit* myDigit = (RecoDigit*)(myCluster->GetDigit(idigit));
      fFilterByClusters->push_back(myDigit);
    }
  }
  
  // return vector of filtered digits
  // ================================
  if(verbosity>v_message) std::cout << name <<"  filter by clusters: " << fFilterByClusters->size() << std::endl;
  
  
  return fFilterByClusters;
}

std::vector<RecoCluster*>* HitCleaner::RecoClusters(std::vector<RecoDigit*>* myDigitList)
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

  // make cluster digits
  // ===================
  for(int idigit=0; idigit<int(myDigitList->size()); idigit++ ){
    RecoDigit* recoDigit = (RecoDigit*)(myDigitList->at(idigit));
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
	               
	        if (digitType==RecoDigit::PMT8inch && cdigit->GetNClusterDigits() > fPmtMinHitsPerCluster) {
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
	        
	        if (digitType==RecoDigit::lappd_v0 && cdigit->GetNClusterDigits() > fLappdMinHitsPerCluster) {
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
        RecoCluster* cluster = new RecoCluster();
        fClusterList->push_back(cluster);

        for(int jdigit=0; jdigit<int(vClusterDigitCollection.size()); jdigit++ ){
          RecoClusterDigit* cdigit = (RecoClusterDigit*)(vClusterDigitCollection.at(jdigit));
          RecoDigit* recodigit = (RecoDigit*)(cdigit->GetRecoDigit());
          cluster->AddDigit(recodigit);        
        }
      }
    }
  }

  //std::cout <<"fClusterList->size() = "<<fClusterList->size()<<std::endl;
  // return vector of clusters
  // =========================
  return fClusterList;
}

std::vector<RecoDigit*>* HitCleaner::FilterByTruthInfo(std::vector<RecoDigit*>* DigitList)
{
	std::string name = " HitCleaner::FilterByTruthInfo(() ";
  // clear vector of filtered digits
  // ===============================
  fFilterByTruthInfo->clear();
  
  //===================== true info.
  double x0 = fTrueVertex->GetPosition().X();
  double y0 = fTrueVertex->GetPosition().Y();
  double z0 = fTrueVertex->GetPosition().Z();
  double dirx = fTrueVertex->GetDirection().X();
  double diry = fTrueVertex->GetDirection().Y();
  double dirz = fTrueVertex->GetDirection().Z(); 
  double t0 = 0.0;
  	
  // filter by truth
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
    if(phideg>38) fFilterByTruthInfo->push_back(recoDigit);   
  }

  // return vector of filtered digits
  // ================================
  if(verbosity>v_message) std::cout << name << "  filter by opening angle: " << fFilterByTruthInfo->size() << std::endl;
  
  return fFilterByTruthInfo;
}

#include "PrintRecoEvent.h"

PrintRecoEvent::PrintRecoEvent():Tool(){}


bool PrintRecoEvent::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_variables.Get("verbosity",verbosity);
  m_variables.Get("isMC",isMC);
  

  bool hitcleaningparam_available = true;
  int get_ok;
  get_ok = m_data->Stores["RecoEvent"]->Get("HitCleaningParameters",hitcleaningparam);
  if (!get_ok) hitcleaningparam_available = false;

  if (hitcleaningparam_available){
    Log("PrintRecoEvent tool: HitCleaningParameters:",v_message,verbosity);
    for (std::map<std::string,double>::iterator it = hitcleaningparam->begin(); it != hitcleaningparam->end(); it++){
      Log("PrintRecoEvent tool: "+(it->first)+": "+std::to_string(int(it->second)),v_message,verbosity);
    }
  }

  return true;
}


bool PrintRecoEvent::Execute(){

  Log("PrintRecoEvent tool: Executing",v_message,verbosity);

  int recoeventexists = m_data->Stores.count("RecoEvent");
  if (!recoeventexists) std::cerr <<"No RecoEvent store!"<< std::endl;

  //Get RecoEvent objects
  int get_ok;
  Log("PrintRecoEvent tool: Getting contents of RecoEvent Store",v_message,verbosity);
  if (isMC) Log("PrintRecoEvent tool: Monte Carlo mode",v_message,verbosity);
  else Log("PrintRecoEvent tool: Data mode",v_message,verbosity);

  //Set default values for cases where information can't be read
  Position default_position(-999,-999,-999);
  Direction default_direction(-999,-999,-999);
  bool truevertex_available = false;
  bool truestopvertex_available = false;

  if (isMC){
  
    get_ok = m_data->Stores["RecoEvent"]->Get("MCPiPlusCount",mcpipluscount);
    if (!get_ok) mcpipluscount = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("MCPiMinusCount",mcpiminuscount);
    if (!get_ok) mcpiminuscount = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("MCPi0Count",mcpi0count);
    if (!get_ok) mcpi0count = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("MCKPlusCount",mckpluscount);
    if (!get_ok) mckpluscount = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("MCKMinusCount",mckminuscount);
    if (!get_ok) mckminuscount = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("MCK0Count",mck0count);
    if (!get_ok) mck0count = -1;

    truevertex_available = true;
    get_ok = m_data->Stores["RecoEvent"]->Get("TrueVertex",truevertex);
    if (!get_ok) truevertex_available = false;
    truestopvertex_available = true; 
    get_ok = m_data->Stores["RecoEvent"]->Get("TrueStopVertex",truestopvertex);
    if (!get_ok) truestopvertex_available = false;
    get_ok = m_data->Stores["RecoEvent"]->Get("TrueMuonEnergy",truemuonenergy);
    if (!get_ok) truemuonenergy = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("TrueTrackLengthInWater",truetracklengthinwater);
    if (!get_ok) truetracklengthinwater = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("TrueTrackLengthInMRD",truetracklengthinmrd);
    if (!get_ok) truetracklengthinmrd = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("SingleRingEvent",singleringevent);
    if (!get_ok) singleringevent = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("MultiRingEvent",multiringevent);
    if (!get_ok) multiringevent = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("PdgPrimary",pdgprimary);
    if (!get_ok) pdgprimary = -1;
    get_ok = m_data->Stores["RecoEvent"]->Get("NRings",nrings);
    if (!get_ok) nrings = -1;
    bool indexparticlesring_available = true;
    indexparticlesring.clear();
    get_ok = m_data->Stores["RecoEvent"]->Get("IndexParticlesRing",indexparticlesring);
    if (!get_ok) indexparticlesring_available = false; 
    get_ok = m_data->Stores["RecoEvent"]->Get("ProjectedMRDHit",projectedmrdhit);
    if (!get_ok) projectedmrdhit = -1;

  }

  get_ok = m_data->Stores["RecoEvent"]->Get("EventCutStatus",eventcutstatus);
  if (!get_ok) eventcutstatus = -1;
  get_ok = m_data->Stores["RecoEvent"]->Get("EventFlagApplied",eventflagapplied);
  if (!get_ok) eventflagapplied = -1;
  get_ok = m_data->Stores["RecoEvent"]->Get("EventFlagged",eventflagged);
  if (!get_ok) eventflagged = -1;

  bool recodigits_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("RecoDigit",recodigits);
  if (!get_ok) recodigits_available = false;
  
  get_ok = m_data->Stores["RecoEvent"]->Get("HitCleaningDone",hitcleaningdone);
  if (!get_ok) hitcleaningdone = -1;
  bool hitcleaningclusters_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("HitCleaningClusters",hitcleaningclusters);
  if (!get_ok) hitcleaningclusters_available = false;

  bool vseedvtxlist_available = true;
  vseedvtxlist.clear();
  get_ok = m_data->Stores["RecoEvent"]->Get("vSeedVtxList",vseedvtxlist);
  if (!get_ok) vseedvtxlist_available = false;
  bool vseedfomlist_available = true;
  vseedfomlist.clear();
  get_ok = m_data->Stores["RecoEvent"]->Get("vSeedFOMList",vseedfomlist);
  if (!get_ok) vseedfomlist_available = false;

  bool simpleposition_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("SimplePosition",simpleposition);
  if (!get_ok) simpleposition_available = false;
  bool pointposition_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("PointPosition",pointposition);
  if (!get_ok) pointposition_available = false;
  bool simpledirection_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("SimpleDirection",simpledirection);
  if (!get_ok) simpledirection_available = false;
  bool pointdirection_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("PointDirection",pointdirection);
  if (!get_ok) pointdirection_available = false;
  bool pointvertex_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("PointVertex",pointvertex);
  if (!get_ok) pointvertex_available = false;
  bool extendedvertex_available = true;
  get_ok = m_data->Stores["RecoEvent"]->Get("ExtendedVertex",extendedvertex);
  if (!get_ok) extendedvertex_available = false;

  if (isMC){
    Log("PrintRecoEvent tool: Particle Counts:",v_message,verbosity);
    Log("PrintRecoEvent tool: MCPiPlusCount: "+std::to_string(mcpipluscount),v_message,verbosity);
    Log("PrintRecoEvent tool: MCPiMinusCount: "+std::to_string(mcpiminuscount),v_message,verbosity);
    Log("PrintRecoEvent tool: MCPi0Count: "+std::to_string(mcpi0count),v_message,verbosity);
    Log("PrintRecoEvent tool: MCKPlusCount: "+std::to_string(mckpluscount),v_message,verbosity);
    Log("PrintRecoEvent tool: MCKMinusCount: "+std::to_string(mckminuscount),v_message,verbosity);
    Log("PrintRecoEvent tool: MCK0Count: "+std::to_string(mck0count),v_message,verbosity);
  
    Log("PrintRecoEvent tool: True event information:",v_message,verbosity);
    if (truevertex_available){
      Log("PrintRecoEvent tool: TrueVertex: ("+std::to_string(truevertex->GetPosition().X())+","+std::to_string(truevertex->GetPosition().Y())+","+std::to_string(truevertex->GetPosition().Z())+")",v_message,verbosity);
    } else {
      Log("PrintRecoEvent tool: TrueVertex not available in RecoEvent Store",v_message,verbosity);
    }
    if (truestopvertex_available){
      Log("PrintRecoEvent tool: TrueStopVertex: ("+std::to_string(truestopvertex->GetPosition().X())+","+std::to_string(truestopvertex->GetPosition().Y())+","+std::to_string(truestopvertex->GetPosition().Z())+")",v_message,verbosity);
    } else {
      Log("PrintRecoEvent tool: TrueStopVertex not available in RecoEvent Store",v_message,verbosity);
    }
    Log("PrintRecoEvent tool: TrueMuonEnergy: "+std::to_string(truemuonenergy),v_message,verbosity);
    Log("PrintRecoEvent tool: TrueTrackLengthInWater: "+std::to_string(truetracklengthinwater),v_message,verbosity);
    Log("PrintRecoEvent tool: TrueTrackLengthInMRD: "+std::to_string(truetracklengthinmrd),v_message,verbosity);
    Log("PrintRecoEvent tool: SingleRingEvent: "+std::to_string(singleringevent),v_message,verbosity);
    Log("PrintRecoEvent tool: MultiRingEvent: "+std::to_string(multiringevent),v_message,verbosity);
    Log("PrintRecoEvent tool: PdgPrimary: "+std::to_string(pdgprimary),v_message,verbosity);
    Log("PrintRecoEvent tool: NRings: "+std::to_string(nrings),v_message,verbosity);
    Log("PrintRecoEvent tool: IndexParticlesRing: Size: "+std::to_string(indexparticlesring.size()),v_message,verbosity);
    Log("PrintRecoEvent tool: ProjectedMRDHit: "+std::to_string(projectedmrdhit),v_message,verbosity);
  }  

  Log("PrintRecoEvent tool: EventCutStatus: "+std::to_string(eventcutstatus),v_message,verbosity);
  Log("PrintRecoEvent tool: EventFlagApplied: "+std::to_string(eventflagapplied),v_message,verbosity);
  Log("PrintRecoEvent tool: EventFlagged: "+std::to_string(eventflagged),v_message,verbosity);

  if (recodigits_available){
    Log("PrintRecoEvent tool: RecoDigit size: "+std::to_string(recodigits->size()),v_message,verbosity);
    for (int i=0; i< int(recodigits->size()); i++){
      RecoDigit temp_digit = recodigits->at(i);
      Log("PrintRecoEvent tool: RecoDigit entry "+std::to_string(i+1)+", Time: "+std::to_string(temp_digit.GetCalTime())+", Charge: "+std::to_string(temp_digit.GetCalCharge())+", DetectorID: "+std::to_string(temp_digit.GetDetectorID())+", FilterStatus: "+std::to_string(temp_digit.GetFilterStatus()),v_debug,verbosity);
    }
    if (recodigits->size() == 0) return true; //no need to display other objects if recodigits is empty
  } else {
    Log("PrintRecoEvent tool: RecoDigits not available in RecoEvent Store!",v_message,verbosity);
  }

  Log("PrintRecoEvent tool: HitCleaning done: "+std::to_string(hitcleaningdone),v_message,verbosity);


  if (hitcleaningclusters_available){
    Log("PrintRecoEvent tool: HitCleaningClusters size: "+std::to_string(hitcleaningclusters->size()),v_message,verbosity);
    for (int i=0; i<int(hitcleaningclusters->size()); i++){
      RecoCluster *thiscluster = hitcleaningclusters->at(i);
      Log("PrintRecoEvent tool: HitCleaningClusters entry "+std::to_string(i+1)+", digits in this cluster: "+std::to_string(thiscluster->GetNDigits()),v_debug,verbosity);
    }
  } else {
    Log("PrintRecoEvent tool: HitCleaningClusters not available in RecoEvent Store!",v_message,verbosity);
  }

  if (vseedvtxlist_available){
    Log("PrintRecoEvent tool: vSeedVtxList size: "+std::to_string(vseedvtxlist.size()),v_message,verbosity);
    for (int i=0; i< int(vseedvtxlist.size()); i++){
      RecoVertex *temp_vertex = vseedvtxlist.at(i);
      Position temp_pos = temp_vertex->GetPosition();
      Log("PrintRecoEvent tool: vSeedVtxList entry "+std::to_string(i+1)+", Position: ("+std::to_string(temp_pos.X())+","+std::to_string(temp_pos.Y())+","+std::to_string(temp_pos.Z())+")",v_debug,verbosity);
    }
  } else {
    Log("PrintRecoEvent tool: vSeedVtxList not available in RecoEvent Store!",v_message,verbosity);
  }

  if (vseedfomlist_available){
    Log("PrintRecoEvent tool: vSeedFOMList size: "+std::to_string(vseedfomlist.size()),v_message,verbosity);
    for (int i=0; i < int(vseedfomlist.size()); i++){
      Log("PrintRecoEvent tool: vSeedFOMList entry "+std::to_string(i+1)+", FOM: "+std::to_string(vseedfomlist.at(i)),v_debug,verbosity);
    }
  } else {
    Log("PrintRecoEvent tool: vSeedFOMList not available in RecoEvent Store!",v_message,verbosity);
  }
  
  if (simpleposition_available){
    Log("PrintRecoEvent tool: SimplePosition: ("+std::to_string(simpleposition->GetPosition().X())+","+std::to_string(simpleposition->GetPosition().Y())+","+std::to_string(simpleposition->GetPosition().Z())+")",v_message,verbosity);
  } else {
    Log("PrintRecoEvent tool: SimplePosition not available in RecoEvent Store",v_message,verbosity);
  }
  
  if (pointposition_available){
    Log("PrintRecoEvent tool: PointPosition: ("+std::to_string(pointposition->GetPosition().X())+","+std::to_string(pointposition->GetPosition().Y())+","+std::to_string(pointposition->GetPosition().Z())+")",v_message,verbosity);
  } else {
    Log("PrintRecoEvent tool: PointPosition not available in RecoEvent Store",v_message,verbosity);
  }

  if (simpledirection_available){
    Log("PrintRecoEvent tool: SimpleDirection: ("+std::to_string(simpledirection->GetDirection().X())+","+std::to_string(simpledirection->GetDirection().Y())+","+std::to_string(simpledirection->GetDirection().Z())+")",v_message,verbosity);
  } else {
    Log("PrintRecoEvent tool: SimpleDirection not available in RecoEvent Store",v_message,verbosity);
  }

  if (pointdirection_available){
    Log("PrintRecoEvent tool: PointDirection: ("+std::to_string(pointdirection->GetDirection().X())+","+std::to_string(pointdirection->GetDirection().Y())+","+std::to_string(pointdirection->GetDirection().Z())+")",v_message,verbosity);
  } else {
    Log("PrintRecoEvent tool: PointDirection not available in RecoEvent Store",v_message,verbosity);
  }

  if (pointvertex_available){
    Log("PrintRecoEvent tool: PointVertex: ("+std::to_string(pointvertex->GetPosition().X())+","+std::to_string(pointvertex->GetPosition().Y())+","+std::to_string(pointvertex->GetPosition().Z())+")",v_message,verbosity);
  } else {
    Log("PrintRecoEvent tool: PointVertex not available in RecoEvent Store",v_message,verbosity);
  }

  if (extendedvertex_available){
    Log("PrintRecoEvent tool: Extended vertex: ("+std::to_string(extendedvertex->GetPosition().X())+","+std::to_string(extendedvertex->GetPosition().Y())+","+std::to_string(extendedvertex->GetPosition().Z())+")",v_message,verbosity);
  } else {
    Log("PrintRecoEvent tool: Extended vertex not available in RecoEvent Store",v_message,verbosity);
  }

  return true;
}


bool PrintRecoEvent::Finalise(){

  Log("PrintRecoEvent tool: Finalising",v_message,verbosity);
  
  return true;
}

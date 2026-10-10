#include "ClassicalEnergyReco.h"

ClassicalEnergyReco::ClassicalEnergyReco():Tool(){}


bool ClassicalEnergyReco::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_variables.Get("verbosity",verbosity);
  cout<<"Classicverb: "<<verbosity<<endl;
  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool ClassicalEnergyReco::Execute(){
    cout<<"ClassicCheck"<<endl;
	//Get RecoVertex and direction.  Extrapolate track length.  Possible get track length from DNNTrackLengthPredict
    // check if event passes the cut
    bool EventCutstatus = false;
    auto get_evtstatus = m_data->Stores.at("RecoEvent")->Get("EventCutStatus", EventCutstatus);
    if (!get_evtstatus) {
        Log("Error: The ClassicalEnergyReco tool could not find the Event selection status", v_error, verbosity);
        return false;
    }

    if (!EventCutstatus) {
        Log("Message: This event doesn't pass the event selection. ", v_message, verbosity);
        return true;
    }

    bool get_clusters = m_data->Stores.at("RecoEvent")->Get("RecoClusters", fClusterList);
    if (not get_clusters) {
        Log("ClassicalEnergyReco Tool: Error retrieving RecoClusters, no clusters from the RecoEvent!", v_error, verbosity);
        return false;
    }

    for (int i = 0; i < fClusterList->size(); i++) {
        if (fClusterList->at(i).GetClusterMode() == 1 && fClusterList->at(i).GetTime() < 10000 && fClusterList->at(i).GetNDigits() > 0) {  //Todo: Set Time Window in config
        max_pe=fClusterList->at(i).GetCharge();
        muonClusterIndex = i;
        break;
        }
    }

    bool got_vtx = m_data->Stores.at("RecoEvent")->Get("ExtendedVertex",recoVertex);
    if (!got_vtx) {
        Log("ClassicalEnergyReco tool could not find Extended Vertex in selected event!  Aborting",v_error,verbosity);
        return false;
    }

    recoVtxX=recoVertex->GetPosition().X()/152;
    recoVtxY=recoVertex->GetPosition().Y()/152;
    recoVtxZ=recoVertex->GetPosition().Z()/152;
    recoVtxT=recoVertex->GetTime();
    recoDirX = recoVertex->GetDirection().X();
    recoDirY = recoVertex->GetDirection().Y();
    recoDirZ = recoVertex->GetDirection().Z();
    double TankRadius = 1.;
    double Extrapolation;


    double a=pow(recoDirX,2)+pow(recoDirZ,2);
    cout<<"Classica: "<<a<<endl;
    double b=2*(recoVtxX*recoDirX+recoVtxZ*recoDirZ);
    cout<<"Classicb: "<<b<<endl;
    double c=pow(recoVtxX,2)+pow(recoVtxZ,2)-pow(TankRadius,2);
    cout<<"Classicc: "<<c<<endl;

    Extrapolation= ( - b + sqrt(pow(b, 2) - 4 * a * c) )/(2*a);
    
    exitX=recoVtxX+Extrapolation*recoDirX;
    exitY=recoVtxY+Extrapolation*recoDirY;
    exitZ=recoVtxZ+Extrapolation*recoDirZ;
    if (exitZ < 0) {
        Extrapolation = (-b - sqrt(pow(b, 2) - 4 * a * c)) / (2 * a);

        exitX = recoVtxX + Extrapolation * recoDirX;
        exitY = recoVtxY + Extrapolation * recoDirY;
        exitZ = recoVtxZ + Extrapolation * recoDirZ;
    }

    if (exitZ < 0) {
        Log("ClassicalEnergyRecoTool: Something has gone wrong.  Both roots are negative in Z.",v_warning,verbosity);
    }

    recoTankTrackLength=pow(pow(exitX-recoVtxX,2)+pow(exitY-recoVtxY,2)+pow(exitZ-recoVtxZ,2),0.5)*1.52;


    GetANNIEEventVariables();

    ClassicEnergyReconstruction();

    m_data->Stores.at("RecoEvent")->Set("classicTankTrackLength",recoTankTrackLength);
    m_data->Stores.at("RecoEvent")->Set("classicRecoEnergy",ClassicRecoEnergy);
    m_data->Stores.at("RecoEvent")->Set("classicTransverseP",crTransverseP);

  return true;
}



bool ClassicalEnergyReco::Finalise(){
    //fClusterList=nullptr;
    //recoVertex=nullptr;
  return true;
}


bool ClassicalEnergyReco::GetANNIEEventVariables() {

    //Get MRD track reco variables + PMT information from BoostStores

    //MRD reco variables
    Position StartVertex;
    Position StopVertex;
    double TrackLength = -9999;
    double TrackAngle = -9999;
    double TrackAngleError = -9999;
    double PenetrationDepth = -9999;
    Position MrdEntryPoint;
    double EnergyLoss = -9999; //in MeV
    double EnergyLossError = -9999;
    double EntryPointRadius = -9999;
    bool IsMrdPenetrating;
    bool IsMrdStopped;
    bool IsMrdSideExit;
    int numtracksinev = 0;
    int TrackEventID = -1;
    std::vector<BoostStore>* theMrdTracks = nullptr;

    longesttrackEntryNumber=0;
    MaximumMRDTrackLength=0;

    bool get_annie = true;

    std::vector<std::vector<int>> MrdTimeClusters;
    get_annie = m_data->CStore.Get("MrdTimeClusters", MrdTimeClusters);
    if (!get_annie) {
        Log("ClassicalReconstruction tool: No MrdTimeClusters object in CStore! Did you run TimeClustering beforehand?", v_warning, verbosity);
        return false;
    }
    else if (MrdTimeClusters.size() == 0) {
        Log("ClassicalReconstruction tool: MrdTimeClusters object is empty! Don't proceed with reconstruction...", vv_debug, verbosity);
        return false;
    }

    bool get_MRD = m_data->Stores.at("MRDTracks")->Get("MRDTracks", theMrdTracks);
    bool get_numTracks = m_data->Stores.at("MRDTracks")->Get("NumMrdTracks", numtracksinev);

    if (!get_MRD || !get_numTracks) {
        Log("ClassicalEnergyReconstruction tool: Error retrieving MRDTracks",v_error,verbosity);
        return false;
    }

    if (numtracksinev > theMrdTracks->size()) {
        cout<<"Copilot was right.  This is a problem!"<<endl;
        return false;
    }

    int NumClusterTracks = 0;

    for (int tracki = 0; tracki < numtracksinev; tracki++) {
        BoostStore* thisTrackAsBoostStore = &(theMrdTracks->at(tracki));

        thisTrackAsBoostStore->Get("StartVertex", StartVertex);
        thisTrackAsBoostStore->Get("StopVertex", StopVertex);
        thisTrackAsBoostStore->Get("TrackAngle", TrackAngle);
        thisTrackAsBoostStore->Get("TrackAngleError", TrackAngleError);
        thisTrackAsBoostStore->Get("PenetrationDepth", PenetrationDepth);
        thisTrackAsBoostStore->Get("MrdEntryPoint", MrdEntryPoint);
        thisTrackAsBoostStore->Get("EnergyLoss", EnergyLoss);
        thisTrackAsBoostStore->Get("EnergyLossError", EnergyLossError);
        thisTrackAsBoostStore->Get("IsMrdPenetrating", IsMrdPenetrating);        // bool
        thisTrackAsBoostStore->Get("IsMrdStopped", IsMrdStopped);                // bool
        thisTrackAsBoostStore->Get("IsMrdSideExit", IsMrdSideExit);
        thisTrackAsBoostStore->Get("MrdSubEventID", TrackEventID);
        TrackLength = sqrt(pow((StopVertex.X() - StartVertex.X()), 2) + pow(StopVertex.Y() - StartVertex.Y(), 2) + pow(StopVertex.Z() - StartVertex.Z(), 2)) * 100.0;
        EntryPointRadius = sqrt(pow(MrdEntryPoint.X(), 2) + pow(MrdEntryPoint.Y(), 2)) * 100.0; // convert to cm
        PenetrationDepth = PenetrationDepth * 100.0;

        
        if (verbosity > 4) Log("ClassicalReconstruction Tool: NumClusterTracks: " + std::to_string(NumClusterTracks), vv_debug, verbosity);

        double mrdtracklength = sqrt(pow((StopVertex.X() - StartVertex.X()), 2) + pow(StopVertex.Y() - StartVertex.Y(), 2) + pow(StopVertex.Z() - StartVertex.Z(), 2)) * 100.0;

        if (MaximumMRDTrackLength < mrdtracklength) {
            MaximumMRDTrackLength = mrdtracklength;
            longesttrackEntryNumber = tracki;
            longestMRDEloss = EnergyLoss;
        }
    }

    return true;

}


bool ClassicalEnergyReco::ClassicEnergyReconstruction() {

    Log("ClassicalEnergyReco tool: ClassicEnergyReconstruction", v_message, verbosity);
    //Log("SimpleReconstruction tool: Conditions: dist (pmtvol - tank) = " + std::to_string(dist_pmtvol_tank) + " m, MRD Eloss = " + std::to_string(mrd_eloss) + " MeV, Qmax = " + std::to_string(max_pe) + " p.e.", v_message, verbosity);

    ClassicRecoEnergy = 87.3 + 200 * (recoTankTrackLength)+longestMRDEloss + max_pe * 0.08534;

    Log("ClassicalEnergyReco: Reconstructed muon energy: " + std::to_string(ClassicRecoEnergy), v_message, verbosity);
    cout<<"ClassicCheck Energy: "<<ClassicRecoEnergy<<endl;


    //Also calculate transverse muon momentum
    double ClassicRecoTotalEnergy = ClassicRecoEnergy + 105.6;
    crTransverseP = sqrt((1 - recoDirZ * recoDirZ) * (ClassicRecoTotalEnergy * ClassicRecoTotalEnergy - 105.6 * 105.6));

    return true;

}
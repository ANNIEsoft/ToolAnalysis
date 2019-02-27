#include "PrintGenieEvent.h"

PrintGenieEvent::PrintGenieEvent():Tool(){}

bool PrintGenieEvent::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Usefull header ///////////////////////
	if(configfile!="")  m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	return true;
}


bool PrintGenieEvent::Execute(){
	
	// Get the genie event
	
//	intptr_t thegenieinfoptr;
	//GenieInfo* thegenieinfoptr;
	//int get_ok = m_data->Stores.at("GenieInfo")->Get("TheGenieInfoPtr",thegenieinfoptr);
	GenieInfo thegenieinfo;
	int get_ok = m_data->Stores.at("GenieInfo")->Get("GenieInfo",thegenieinfo);
	
	if(!get_ok){
		Log("PrintGenieEvent Tool: Failed to retrieve TheGenieInfo rom GenieInfo BoostStore!",1,0);
		return false;
	}
	//GenieInfo* thegenieinfop = reinterpret_cast<GenieInfo*>(thegenieinfoptr);
	//GenieInfo thegenieinfo = (*thegenieinfoptr);
	
	// Print the info
	cout<<"This was a "<< thegenieinfo.procinfostring <<" (neut code "<<thegenieinfo.neutinteractioncode
		<<") interaction of a "
		<<thegenieinfo.probeenergy<<"GeV " << thegenieinfo.probepartname << " on a "; 
	
	if( thegenieinfo.targetnucleonpdg==2212 || thegenieinfo.targetnucleonpdg==2122 ){
		cout<<thegenieinfo.targetnucleonname<<" in a ";
	} else {
		cout<<"PDG-Code " << thegenieinfo.targetnucleonpdg<<" in a ";
	}
	
	if( thegenieinfo.targetnucleusname!="unknown"){ cout<<thegenieinfo.targetnucleusname<<" nucleus, "; }
	else { cout<<"Z=["<<thegenieinfo.targetnucleusZ<<","<<thegenieinfo.targetnucleusA<<"] nucleus, "; }
	
	if(thegenieinfo.remnantnucleusenergy>0){
		cout<<"producing a "<<thegenieinfo.remnantnucleusenergy<<"GeV "<<thegenieinfo.remnantnucleusname;
	} else { cout<<"with no remnant nucleus"; }	// DIS on 16O produces no remnant nucleus?!
	
	if(thegenieinfo.fsleptonenergy>0){
		cout<<" and a "<<thegenieinfo.fsleptonenergy<<"GeV "<<thegenieinfo.fsleptonname<<endl;
	} else{ cout<<" and no final state leptons"<<endl; }
	
	cout<<endl<<"Q^2 was "<<thegenieinfo.Q2<<"(GeV/c)^2, with final state lepton"
		<<" ejected at Cos(θ)="<<thegenieinfo.costhfsl<<endl;
	cout<<"Additional final state particles included "<<endl;
	cout<< " N(p) = "	 << thegenieinfo.numfsprotons
		<< " N(n) = "	 << thegenieinfo.numfsneutrons
		<< endl
		<< " N(pi^0) = "	<< thegenieinfo.numfspi0
		<< " N(pi^+) = "	<< thegenieinfo.numfspiplus
		<< " N(pi^-) = "	<< thegenieinfo.numfspiminus
		<<endl;
	
	return true;
}

bool PrintGenieEvent::Finalise(){
	return true;
}

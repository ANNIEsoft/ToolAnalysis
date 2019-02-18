/* vim:set noexpandtab tabstop=4 wrap */
#include "PrintANNIEEvent.h"

PrintANNIEEvent::PrintANNIEEvent():Tool(){}

bool PrintANNIEEvent::Initialise(std::string configfile, DataModel &data){
	
	if(verbose) cout<<"Initializing tool PrintANNIEEvent"<<endl;
	
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	m_variables.Get("verbose",verbose);
	//verbose=10;
	
	return true;
}

bool PrintANNIEEvent::Execute(){
	
	if(verbose) cout<<"executing PrintANNIEEvent"<<endl;
	int annieeventexists = m_data->Stores.count("ANNIEEvent");
	if(!annieeventexists){ cerr<<"no ANNIEEvent store!"<<endl; /*return false;*/};
	//m_data->Log->Log(logmessage.str(),1,verbose);
	//logmessage.str(""); ??
	
	// Get the contents to be printed
	if(verbose>1) cout<<"getting contents to be printed from ANNIEEvent at "<<m_data->Stores["ANNIEEvent"]<<endl;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);
	if(get_ok==0) RunNumber=-1;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber);
	if(get_ok==0) SubrunNumber=-1;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
	if(get_ok==0) EventNumber=-1;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("MCParticles",MCParticles);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("RecoParticles",RecoParticles);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("MCHits",MCHits);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",MCLAPPDHits);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("RawADCData",RawADCData);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",RawLAPPDData);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("CalibratedADCData",CalibratedADCData);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("CalibratedLAPPDData",CalibratedLAPPDData);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("TriggerData",TriggerData);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("MCFlag",MCFlag);
	if(get_ok==0) MCFlag=0;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("EventTime",EventTime);
	get_ok = m_data->Stores["ANNIEEvent"]->Get("MCEventNum",MCEventNum);
	if(get_ok==0) MCEventNum=-1;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("MCTriggernum",MCTriggernum);
	if(get_ok==0) MCTriggernum=-1;
	get_ok = m_data->Stores["ANNIEEvent"]->Get("MCFile",MCFile);
	if(get_ok==0) MCFile="";
	get_ok = m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus);
	
	cout<<"RunNumber : "<<RunNumber<<endl;
	cout<<"SubrunNumber : "<<SubrunNumber<<endl;
	cout<<"EventNumber : "<<EventNumber<<endl;
	if(MCParticles){
		cout<<"Num MCParticles : "<<MCParticles->size()<<endl;
		if(verbose>1){
			cout<<"MCParticles: {"<<endl;
			for(auto&& aparticle : *MCParticles) aparticle.Print();
			cout<<"}"<<endl;
		}
	} else {
		cout<<"No MCParticles"<<endl;
	}
	if(RecoParticles){
		cout<<"Num RecoParticles : "<<RecoParticles->size()<<endl;
		if(verbose>1){
			cout<<"RecoParticles: {"<<endl;
			for(auto&& aparticle : *RecoParticles) aparticle.Print();
			cout<<"}"<<endl;
		}
	} else {
		cout<<"No RecoParticles"<<endl;
	}
	if(MCHits){
		cout<<"Num MCHits : "<<MCHits->size()<<endl;
		if(verbose>2){
			cout<<"MCHits : {"<<endl;
			for(auto&& achannel : *MCHits){
				unsigned long chankey = achannel.first;
				auto& hits = achannel.second;
				cout<<"ChannelKey : "<<chankey<<endl;
				cout<<"Hits : "<<endl;
				for(auto&& ahit : hits) ahit.Print();
				cout<<endl;
			}
			cout<<"}"<<endl;
		}
	} else {
		cout<<"No MCHits"<<endl;
	}
	if(MCLAPPDHits){
		cout<<"Num MCLAPPDHits : "<<MCLAPPDHits->size()<<endl;
		if(verbose>2){
			cout<<"MCLAPPDHits : {"<<endl;
			for(auto&& achannel : *MCLAPPDHits){
				unsigned long chankey = achannel.first;
				auto& hits = achannel.second;
				cout<<"ChannelKey : "<<chankey<<endl;
				cout<<"Hits : "<<endl;
				for(auto&& ahit : hits) ahit.Print();
				cout<<endl;
			}
			cout<<"}"<<endl;
		}
	} else {
		cout<<"No MCLAPPDHits"<<endl;
	}
	if(TDCData){
		cout<<"Num TDCData Hits : "<<TDCData->size()<<endl;
		if(verbose>2){
			cout<<"TDCData : {"<<endl;
			for(auto&& achannel : *TDCData){
				unsigned long chankey = achannel.first;
				auto& hits = achannel.second;
				cout<<"ChannelKey : "<<chankey<<endl;
				cout<<"Hits : "<<endl;
				for(auto&& ahit : hits) ahit.Print();
				cout<<endl;
			}
			cout<<"}"<<endl;
		}
	} else {
		cout<<"No TDCData"<<endl;
	}
	if(RawADCData){
		cout<<"Num RawADCData Waveforms : "<<RawADCData->size()<<endl;
		if(verbose>1){
			cout<<"RawADCData : {"<<endl;
			for(auto&& achannel : *RawADCData){
				unsigned long chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "<<chankey<<endl;
				cout<<"Has "<<waveforms.size()<<" waveforms"<<endl;
				if(verbose>2){
					cout<<"Waveforms : "<<endl;
					for(auto&& awaveform : waveforms) awaveform.Print();
					cout<<endl;
				}
				cout<<"}"<<endl;
			}
		}
	} else {
		cout<<"No RawADCData"<<endl;
	}
	if(RawLAPPDData){
		cout<<"Num RawLAPPDData Waveforms : "<<RawLAPPDData->size()<<endl;
		if(verbose>1){
			cout<<"RawLAPPDData : {"<<endl;
			for(auto&& achannel : *RawLAPPDData){
				unsigned long chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "<<chankey<<endl;
				cout<<"Has "<<waveforms.size()<<" waveforms"<<endl;
				if(verbose>2){
					cout<<"Waveforms : "<<endl;
					for(auto&& awaveform : waveforms) awaveform.Print();
					cout<<endl;
				}
				cout<<"}"<<endl;
			}
		}
	} else {
		cout<<"No RawLAPPDData"<<endl;
	}
	if(CalibratedADCData){
		cout<<"Num CalibratedADCData Waveforms : "<<CalibratedADCData->size()<<endl;
		if(verbose>1){
			cout<<"CalibratedADCData : {"<<endl;
			for(auto&& achannel : *CalibratedADCData){
				unsigned long chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "<<chankey<<endl;
				cout<<"Has "<<waveforms.size()<<" waveforms"<<endl;
				if(verbose>2){
					cout<<"Waveforms : "<<endl;
					for(auto&& awaveform : waveforms) awaveform.Print();
					cout<<endl;
				}
				cout<<"}"<<endl;
			}
		}
	} else {
		cout<<"No CalibratedADCData"<<endl;
	}
	if(CalibratedLAPPDData){
		cout<<"Num CalibratedLAPPDData Waveforms : "<<CalibratedLAPPDData->size()<<endl;
		if(verbose>1){
			cout<<"CalibratedLAPPDData : {"<<endl;
			for(auto&& achannel : *CalibratedLAPPDData){
				unsigned long chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "<<chankey<<endl;
				cout<<"Has "<<waveforms.size()<<" waveforms"<<endl;
				if(verbose>2){
					cout<<"Waveforms : "<<endl;
					for(auto&& awaveform : waveforms) awaveform.Print();
					cout<<endl;
				}
				cout<<"}"<<endl;
			}
		}
	} else {
		cout<<"No CalibratedLAPPDData"<<endl;
	}
	if(TriggerData){
		cout<<"Num TriggerData Entries : "<<TriggerData->size()<<endl;
		if(verbose>1){
			cout<<"TriggerData : {"<<endl;
			for(auto&& atrigger : *TriggerData) atrigger.Print();
			cout<<"}"<<endl;
		}
	} else {
		cout<<"No TriggerData"<<endl;
	}
	cout<<"MCFlag : "<<MCFlag<<endl;
	if(EventTime) { cout<<"EventTime : "; EventTime->Print(); }
	else { cout<<"No EventTime"<<endl; }
	cout<<"MCEventNum : "<<MCEventNum<<endl;
	cout<<"MCFile : "<<MCFile<<endl;
	if(BeamStatus) { cout<<"BeamStatus : "; BeamStatus->Print(); }
	else { cout<<"No BeamStatus"<<endl; }
	
	return true;
}

bool PrintANNIEEvent::Finalise(){
	
	return true;
}

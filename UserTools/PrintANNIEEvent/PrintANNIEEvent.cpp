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
	if(verbose>1) cout<<"getting contents to be printed from ANNIEEvent at"<<m_data->Stores["ANNIEEvent"]<<endl;
	m_data->Stores["ANNIEEvent"]->Get("RunNumber",RunNumber);
	m_data->Stores["ANNIEEvent"]->Get("SubrunNumber",SubrunNumber);
	m_data->Stores["ANNIEEvent"]->Get("EventNumber",EventNumber);
	m_data->Stores["ANNIEEvent"]->Get("MCParticles",MCParticles);
	m_data->Stores["ANNIEEvent"]->Get("RecoParticles",RecoParticles);
	m_data->Stores["ANNIEEvent"]->Get("MCHits",MCHits);
	//if(m_data->Stores["ANNIEEvent"]->count("MCLAPPDHits")>0)
	//m_data->Stores["ANNIEEvent"]->Get("MCLAPPDHits",MCLAPPDHits);
	m_data->Stores["ANNIEEvent"]->Get("TDCData",TDCData);
	m_data->Stores["ANNIEEvent"]->Get("RawADCData",RawADCData);
	m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",RawLAPPDData);
	m_data->Stores["ANNIEEvent"]->Get("CalibratedADCData",CalibratedADCData);
	m_data->Stores["ANNIEEvent"]->Get("CalibratedLAPPDData",CalibratedLAPPDData);
	m_data->Stores["ANNIEEvent"]->Get("TriggerData Entries",TriggerData);
	m_data->Stores["ANNIEEvent"]->Get("MCFlag",MCFlag);
	m_data->Stores["ANNIEEvent"]->Get("EventTime",EventTime);
	m_data->Stores["ANNIEEvent"]->Get("MCEventNum",MCEventNum);
	m_data->Stores["ANNIEEvent"]->Get("MCTriggernum",MCTriggernum);
	m_data->Stores["ANNIEEvent"]->Get("MCFile",MCFile);
	m_data->Stores["ANNIEEvent"]->Get("BeamStatus",BeamStatus);
	
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
		if(verbose>1){
			cout<<"MCHits : {"<<endl;
			for(auto&& achannel : *MCHits){
				ChannelKey chankey = achannel.first;
				auto& hits = achannel.second;
				cout<<"ChannelKey : "; chankey.Print();
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
		if(verbose>1){
			cout<<"MCLAPPDHits : {"<<endl;
			for(auto&& achannel : *MCLAPPDHits){
				ChannelKey chankey = achannel.first;
				auto& hits = achannel.second;
				cout<<"ChannelKey : "; chankey.Print();
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
		if(verbose>1){
			cout<<"TDCData : {"<<endl;
			for(auto&& achannel : *TDCData){
				ChannelKey chankey = achannel.first;
				auto& hits = achannel.second;
				cout<<"ChannelKey : "; chankey.Print();
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
				ChannelKey chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "; chankey.Print();
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
				ChannelKey chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "; chankey.Print();
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
				ChannelKey chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "; chankey.Print();
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
				ChannelKey chankey = achannel.first;
				auto& waveforms = achannel.second;
				cout<<"ChannelKey : "; chankey.Print();
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

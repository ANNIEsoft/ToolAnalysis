#include "EventBuilder.h"


static EventBuilder* fgEventBuilder = 0;
EventBuilder* EventBuilder::Instance()
{
  if( !fgEventBuilder ){
    fgEventBuilder = new EventBuilder();
  }

  return fgEventBuilder;
}
EventBuilder::EventBuilder():Tool(){}

bool EventBuilder::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(fverbosity) cout<<"Initializing Tool EventBuilder"<<endl;
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  
  // Get the Tool configuration variables
	m_variables.Get("verbosity",fverbosity);
	Log("Initializing Tool EventBuilder",fv_message,fverbosity);
	
  return true;
}

bool EventBuilder::Execute(){
	Log("EventBuilder Tool: Executing",fv_debug,fverbosity);
	fget_ok = m_data->Stores.count("ANNIEEvent");
	if(!fget_ok){
		Log("EventBuilder Tool: No ANNIEEvent store!",fv_error,fverbosity);
		return false;
	};
  return true;
}

bool EventBuilder::Finalise(){
  return true;
}

#include "TestWaveForm.h"

TestWaveForm::TestWaveForm():Tool(){}


bool TestWaveForm::Initialise(std::string configfile, DataModel &data){
  std::cout << "Initialise TestWaveForm" << std::endl;
  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////
  Geometry* geom;
  bool testgeom = m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", geom);
  if (not testgeom)
  {
    std::cerr << "LAPPDSim Tool: Could not find Geometry in the ANNIEEvent!" << std::endl;
    return false;
  }

  std::ofstream _csvfile;
  _csvfile.open("/nfs/neutrino/data2/stenderm/PhD/ToolAnalysis/ToolAnalysis/UserTools/TestWaveForm/LAPPDGeometry.csv");
  _csvfile << "LEGEND_LINE" << "\n";
  _csvfile << "detector_num,detector_position_x,detector_position_y,detector_position_z,detector_direction_x,detector_direction_y,detector_direction_z,detector_type,detector_status"
           << ",channel_num,channel_position_x,channel_position_y,channel_position_z,channel_strip_side,channel_strip_num,channel_signal_crate"
           << ",channel_signal_card,channel_signal_channel,channel_level2_crate,channel_level2_card,channel_level2_channel"
           << ",channel_hv_crate,channel_hv_card,channel_hv_channel,channel_status" << "\n";
  _csvfile << "\n";
  _csvfile << "DATA_START" << "\n";
  std::map<std::string, std::map<unsigned long,Detector*> >* AllDetectors = geom->GetDetectors();
  std::map<std::string, std::map<unsigned long,Detector*> >::iterator itGeom;
  for(itGeom = AllDetectors->begin(); itGeom != AllDetectors->end(); ++itGeom){
    if(itGeom->first == "LAPPD"){
        std::map<unsigned long,Detector*> LAPPDDetectors = itGeom->second;
        std::map<unsigned long,Detector*>::iterator iterDetector;
        for(iterDetector = LAPPDDetectors.begin(); iterDetector != LAPPDDetectors.end(); ++iterDetector){
          Detector* itDetector = iterDetector->second;
          std::map<unsigned long,Channel>* Channels = iterDetector->second->GetChannels();
          std::map<unsigned long,Channel>::iterator iterChannel;
          for(iterChannel = Channels->begin(); iterChannel != Channels->end(); ++iterChannel){
            Channel itChannel = iterChannel->second;
              _csvfile << itDetector->GetDetectorID() << ","
                       << IsItZero(itDetector->GetDetectorPosition().X()) << "," << IsItZero(itDetector->GetDetectorPosition().Y()) << "," << IsItZero(itDetector->GetDetectorPosition().Z()) << ","
                       << IsItZero(itDetector->GetDetectorDirection().X()) << "," << IsItZero(itDetector->GetDetectorDirection().Y()) << "," << IsItZero(itDetector->GetDetectorDirection().Z()) << ",";
              _csvfile << itDetector->GetDetectorType() << "," << GetStatus(itDetector->GetStatus())  << "," << itChannel.GetChannelID() << ","
                       << IsItZero(itChannel.GetChannelPosition().X()) << ","  << IsItZero(itChannel.GetChannelPosition().Y()) << ","  << IsItZero(itChannel.GetChannelPosition().Z()) << ","
                       << itChannel.GetStripSide() << "," << itChannel.GetStripNum() << "," << itChannel.GetSignalCrate() << ","
                       << itChannel.GetSignalCard() << "," << itChannel.GetSignalChannel() << "," << itChannel.GetLevel2Crate() << ","
                       << itChannel.GetLevel2Card() << "," << itChannel.GetLevel2Channel() << "," << itChannel.GetHvCrate() << ","
                       << itChannel.GetHvCard() << "," << itChannel.GetHvChannel() << "," << GetStatus(itChannel.GetStatus()) << "\n";
          }
        }
    }
  }
  //_csvfile << "PMT," << radiusPmt << "," << phiPmt << "," <<  position_pmt[i_pmt].Y() << ","  <<  position_pmt[i_pmt].X() << "," <<  position_pmt[i_pmt].Y() << "," <<  position_pmt[i_pmt].Z() << "," << i << "\n";}
  _csvfile << "DATA_END" << "\n";

  return true;
}


bool TestWaveForm::Execute(){
  // bool testgeom = m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);
  // if (not testgeom)
  // {
  //   std::cerr << "LAPPDSim Tool: Could not find Geometry in the ANNIEEvent!" << std::endl;
  //   return false;
  // }
  //
  // bool testwav = m_data->Stores["ANNIEEvent"]->Get("LAPPDWaveforms", _waveforms);
  // if (not testwav)
  // {
  //   std::cerr << "TestWaveForm Tool: Could not find LAPPDWaveforms in the ANNIEEvent!" << std::endl;
  //   return false;
  // }
  //  std::map<unsigned long, Waveform<double> >::iterator itr;
  //  for(itr = _waveforms->begin(); itr != _waveforms->end(); itr++){
  //    std::cout << "Channelkey " << itr->first << std::endl;
  //    Waveform<double> Penis = itr->second;
  //    std::vector<double> * samples = Penis.GetSamples();
  //    double time = Penis.GetStartTime();
  //    std::cout << "Start time " << time << std::endl;
  //    for(int i = 0; i < samples->size(); i++){
  //      std::cout << "Sample " << i << " Voltage " << samples->at(i) << std::endl;
  //    }
  //  }


  return true;
}


bool TestWaveForm::Finalise(){

  return true;
}

std::string TestWaveForm::GetStatus(detectorstatus detectorStatus){
  switch(detectorStatus){
			case (detectorstatus::OFF): return "OFF"; break;
			case (detectorstatus::ON): return "ON"; break;
			case (detectorstatus::UNSTABLE): return "UNSTABLE"; break;
    }
}

std::string TestWaveForm::GetStatus(channelstatus channelStatus){
  switch(channelStatus){
  			case (channelstatus::OFF): return "OFF"; break;
  			case (channelstatus::ON): return "ON"; break;
  			case (channelstatus::UNSTABLE): return "UNSTABLE"; break;
  }
}

double TestWaveForm::IsItZero(double number){
  if(abs(number) < 0.00000000001){
    number = 0;
  }
  return number;
}

#include "ReadConfigInfo.h"

ReadConfigInfo::ReadConfigInfo():Tool(){}


bool ReadConfigInfo::Initialise(std::string configfile, DataModel &data){

	verbosity = 0;
	old_run_number = 0;
	old_subrun_number = 0;
	old_part_number = 0;
	filename = "";
	write_seperate_files = false;

	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); // loading config file
	//m_variables.Print();

	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////

	m_variables.Get("verbosity", verbosity);
	m_variables.Get("FileName", filename);

	if(filename == ""){
		write_seperate_files = true;
	}

	Log("write_seperate_files: "+std::to_string(write_seperate_files), v_message, verbosity);

	return true;
}


bool ReadConfigInfo::Execute(){
	if(write_seperate_files){ //Write a txt file with the correspondinf ConfigInfo for each part file
		int current_run_number;
		int current_subrun_number;
		int current_part_number;

		m_data->Stores["ANNIEEvent"]->Get("RunNumber", current_run_number);
		m_data->Stores["ANNIEEvent"]->Get("SubrunNumber", current_subrun_number);
		m_data->Stores["ANNIEEvent"]->Get("PartNumber", current_part_number);

		if(current_run_number != old_run_number || current_subrun_number != old_subrun_number ||  current_part_number != old_part_number){
			std::string config_info = "ConfigInfo does not exits in ANNIEEvent Header for R"+to_string(current_run_number)+"S"+to_string(current_subrun_number)+"p"+to_string(current_part_number) ;
			if(m_data->Stores["ANNIEEvent"]->Header->Get("ConfigInfo",config_info)){
				filename = "ConfigInfo_R" + std::to_string(current_run_number) + "S" + std::to_string(current_subrun_number) + "p" + std::to_string(current_part_number) + ".txt";
				outfile.open (filename.c_str());
				outfile << config_info;
				outfile.close();
				Log(filename, v_message, verbosity);
				Log(config_info, v_message, verbosity);
			}
			else{ //If ConfigInfo does not exists throw an error message
				Log(config_info, v_error, verbosity);
			}
			
			old_run_number = current_run_number;
			old_subrun_number = current_subrun_number;
			old_part_number = current_part_number;
		}
	}
	else{//Stop the loop if only a single file should be written
		m_data->vars.Set("StopLoop",1);
	}
	return true;
}


bool ReadConfigInfo::Finalise(){
	if(write_seperate_files == 0){ //Write a single file
		std::string config_info = "ConfigInfo does not exits in ANNIEEvent Header for filename: "+filename;
		if(m_data->Stores["ANNIEEvent"]->Header->Get("ConfigInfo",config_info)){
			outfile.open (filename.c_str());
			outfile << config_info;
			outfile.close();
			Log(filename, v_message, verbosity);
			Log(config_info, v_message, verbosity);
		}
		else{ //If ConfigInfo does not exists throw an error message
			Log(config_info, v_error, verbosity);
		}
	}
	return true;
}

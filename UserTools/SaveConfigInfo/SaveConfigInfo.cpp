#include "SaveConfigInfo.h"

SaveConfigInfo::SaveConfigInfo():Tool(){}


bool SaveConfigInfo::Initialise(std::string configfile, DataModel &data){

	verbosity = 0;
	outfilename = "configInfo.txt";
	config_info = "THIS IS NOT WORKING";
	full_depth = false;
	/////////////////// Useful header ///////////////////////
	if(configfile!="") m_variables.Initialise(configfile); // loading config file
	//m_variables.Print();

	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////

	m_variables.Get("verbosity",verbosity);
	m_variables.Get("OutFileName",outfilename);
	m_variables.Get("FullDepth",full_depth);
	// Include the output filename in vec_configfiles to avoid a double entry
	vec_configfiles.push_back(outfilename);

	// First we read the git commit hash from the Makefile
	buffer << "git commit hash saved in Makefile during compile time\n";
	buffer << VERSION;
	if( strstr(VERSION,"-dirty") ){
		buffer << "\ndirty: The working tree has local modification. This must not necessarliy impact the ToolChain which has been run just now.\n";
	}
	buffer << "\n------------------------------------------------\n\n";
	
	// First we look for the name of the Tools_File, which includes all other UserTool Config file names
	// This is saved in the vars Store
	std::string line;
	m_data->vars.Get("Tools_File", line);
	vec_configfiles.push_back(line);

	// Next we look through the Tools_File and save the names of all ConfigFiles
	buffer << vec_configfiles[1] << "\n";
	if( std::ifstream(vec_configfiles[1].c_str()) ){
		buffer << std::ifstream(vec_configfiles[1].c_str()).rdbuf();
		infile.open(vec_configfiles[1].c_str());
		std::string uniquename, toolname, configfile;
		while(getline(infile, line)){
		 	std::stringstream ss(line);
		 	ss >> uniquename >> toolname >> configfile;
		 	if(uniquename[0] != '#') vec_configfiles.push_back(configfile);
		}
		infile.close();
	}
	else{
		Log(vec_configfiles[1] + " does not exist!", v_error,verbosity);
		buffer << " - does not exist!";
	}
	buffer << "\n------------------------------------------------\n\n";
	
	// Last come the files defined in vec_configfiles[0]
	if(full_depth == false){
		for(int i=2; i<vec_configfiles.size(); i++){
			buffer << vec_configfiles[i] << "\n";
			if( std::ifstream(vec_configfiles[i].c_str()) ){
				buffer << std::ifstream(vec_configfiles[i].c_str()).rdbuf();
			}
			else{
				Log(vec_configfiles[i] + " does not exist!", v_error,verbosity);
				buffer << "does not exist!";
			}
			buffer << "\n------------------------------------------------\n\n";
		}
	}
	else{
		vector<int> vec_depth(vec_configfiles.size());
		for(int i=0; i<vec_configfiles.size(); i++){
			vec_depth[i] = 0;
		}
		for(int i=2; i<vec_configfiles.size(); i++){
			buffer << vec_configfiles[i] << "\n";
			infile.open(vec_configfiles[i].c_str());
			if( infile.is_open() ){
				std::string token;
				std::vector<std::string> vec_local_files;
				ifstream local_infile;
				while( getline(infile,line) ){

					// Skip lines, which are commented out
					if(line.empty()) continue;
					char firstchar = '0';
					for(char& achar : line){
					 	if(std::isspace(achar)) continue;
					 	firstchar = achar;
					 	break;
					}
					if(firstchar == '#') continue;

					// Now read te line and check if it contains another textfile that can be openend
					std::stringstream ss(line);
					while( !ss.eof() ){
						ss >> token;
						//Avoid reading in the RawData binaries
						if(token.find("RAWData") != std::string::npos) continue;
						//Avoid reading in ROOT files
						if(token.find(".root") != std::string::npos) continue;

						local_infile.open( token.c_str() );
						if( local_infile.is_open() ){
							// We need to avoid double entries and endless loops
							bool double_entry = false;
							for(int k=0; k<vec_configfiles.size(); k++){
								if( ( token.compare(vec_configfiles[k]) ) == 0){
									double_entry = true;
									break;
								}
							}
							if(double_entry == false) vec_local_files.push_back(token);
							local_infile.close();
						}
					}

				}
				infile.close();

				if(verbosity >= v_debug){
					std::cout << "Number of local files to open from " << vec_configfiles[i] << ": " << vec_local_files.size() << std::endl;
					for(int j=0; j<vec_local_files.size(); j++){
						std::cout << j << ": " << vec_local_files[j] << std::endl;
					}
				}

				for(int j=0; j<vec_local_files.size(); j++){
					bool double_entry = false; // We need to avoid double entries and endless loops
					for(int k=0; k<vec_configfiles.size(); k++){
						if( vec_local_files[j].compare(vec_configfiles[k]) == 0 || vec_local_files[j].compare("//") == 0 || vec_local_files[j].compare("./") == 0 ){
							double_entry = true; 
							break;
						}
					}
					if(double_entry == false){
						vec_configfiles.emplace(vec_configfiles.begin() + i + 1, vec_local_files[j]);
						vec_depth.emplace(vec_depth.begin() + i + 1, vec_depth[i]+1);							
					}
				}

				buffer << "Relative depth of file: " << vec_depth[i] << "\n\n";
				buffer << std::ifstream(vec_configfiles[i].c_str()).rdbuf();
			}
			else{
				Log(vec_configfiles[i] + " does not exist!", v_error,verbosity);
				buffer << "does not exist!";
			}
			buffer << "\n------------------------------------------------\n\n";
		}
	}

	outfile.open (outfilename.c_str());
	outfile << buffer.str();
	outfile.close();

	return true;
}


bool SaveConfigInfo::Execute(){
	if( m_data->CStore.Get("ConfigInfo",config_info) == 0){
		m_data->CStore.Set("ConfigInfo",buffer.str());
	}
 	return true;
}


bool SaveConfigInfo::Finalise(){

	if(m_data->CStore.Get("ConfigInfo",config_info)){
		Log("SaveConfigInfo: ConfigInfo" + config_info, v_debug,verbosity);
	}
	else{
		Log("SaveConfigInfo: ConfigInfo is missing! This is unexpected!", v_warning,verbosity);
	}
	return true;
}
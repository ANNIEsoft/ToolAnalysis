/* vim:set noexpandtab tabstop=2 wrap */
#include "TotalLightMap.h"
#include "TPolyMarker.h"
#include "TPointSet3D.h"

/*
make scatter plot (TPolyMarker2D) of the projected tank exit of pion/gammas/muons based on their initial momentum. 
Then also show scatter plot (or colour plot) of hits on tank walls from each species (might need special WCSim mod that saves true position of hit on PMT surface...)
*/

TotalLightMap::TotalLightMap():Tool(){}

bool TotalLightMap::Initialise(std::string configfile, DataModel &data){
	
	/////////////////// Useful header ///////////////////////
	
	if(verbosity) cout<<"Initializing Tool TotalLightMap"<<endl;
	
	if(configfile!="") m_variables.Initialise(configfile); //loading config file
	//m_variables.Print();
	
	m_data= &data; //assigning transient data pointer
	/////////////////////////////////////////////////////////////////
	
	// Get the Tool configuration variables
	// ====================================
	m_variables.Get("verbose",verbosity);
	m_variables.Get("plotDirectory",plotDirectory);
	m_variables.Get("drawHistos",drawHistos);
	
	// check the output directory exists and is suitable
	bool isdir=false, plotDirectoryExists=false;
	struct stat s;
	if(stat(plotDirectory.c_str(),&s)==0){
		plotDirectoryExists=true;
		if(s.st_mode & S_IFDIR){        // mask to extract if it's a directory
			isdir=true;                   //it's a directory
		} else if(s.st_mode & S_IFREG){ // mask to check if it's a file
			isdir=false;                  //it's a file
		} else {
			isdir=false;                  // neither file nor folder??
		}
	} else {
		plotDirectoryExists=false;
		//assert(false&&"stat failed on input path! Is it valid?"); // error
		// errors could also be because this is a file pattern: e.g. wcsim_0.4*.root
		isdir=false;
	}
	
	if(/*drawHistos &&*/ (!plotDirectoryExists || !isdir)){   // we need to either draw or save
		Log("TotalLightMap Tool: output directory "+plotDirectory+" does not exist or is not a writable directory; please check and re-run.",v_error,verbosity);
		return false;
	}
	
	// If we wish to show the histograms during running, we need a TApplication
	// There may only be one TApplication, so see if another tool has already made one
	// and register ourself as a user if so. Otherwise, make one and put a pointer in the
	// CStore for other Tools
	// ======================
	if(drawHistos){
		// create the ROOT application to show histograms
		int myargc=0;
		char *myargv[] = {(const char*)"mrdeff"};
		// get or make the TApplication
		intptr_t tapp_ptr=0;
		get_ok = m_data->CStore.Get("RootTApplication",tapp_ptr);
		if(not get_ok){
			Log("MrdEfficiency Tool: Making global TApplication",v_error,verbosity);
			rootTApp = new TApplication("rootTApp",&myargc,myargv);
			std::cout<<"rootTApp="<<rootTApp<<std::endl;
			tapp_ptr = reinterpret_cast<intptr_t>(rootTApp);
			m_data->CStore.Set("RootTApplication",tapp_ptr);
		} else {
			Log("MrdEfficiency Tool: Retrieving global TApplication",v_error,verbosity);
			rootTApp = reinterpret_cast<TApplication*>(tapp_ptr);
		}
		int tapplicationusers;
		get_ok = m_data->CStore.Get("RootTApplicationUsers",tapplicationusers);
		if(not get_ok) tapplicationusers=1;
		else tapplicationusers++;
		m_data->CStore.Set("RootTApplicationUsers",tapplicationusers);
	}
	
	// Get the anniegeom
	// =================
	get_ok = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",anniegeom);
	if(not get_ok){
		Log("TotalLightMap: Could not get the AnnieGeometry from ANNIEEvent!",v_error,verbosity);
		return false;
	}
	
	// Get properties of the detector we need
	tank_radius = geom.GetTankRadius();
	tank_height = geom.GetTankHalfheight();
	// in fact at present these are incorrect, and we'll need to override them manually:
	tank_radius = 1.277;  // [m]
	tank_height = 3.134;  // [m]
	// Get the detectors too
	Detectors = geom.GetDetectors();
	
	// define the hitmaps
	// we'll create maps of:
	// unbinned true position (for the best resolution) << Is this useful with PMTs? Use Sandbox?
	// binned true position   (so we can take bin differences and compare distributions)
	// and PMT maps           (we we have PMT distribution differences)
	
	TCanvas* lightMapCanv=nullptr;  // XXX this will need 3 TPads XXX
//	// polymarkers of unbinned true vertices of photon hits
//	lightmapmu = new TPolyMarker(); lightmapmu->SetTitle("lightmapmu");
//	lightmappip = new TPolyMarker(); lightmappip->SetTitle("lightmappip");
//	lightmappim = new TPolyMarker(); lightmappim->SetTitle("lightmappim");
//	lightmappigamma = new TPolyMarker(); lightmappigamma->SetTitle("lightmappigamma");
//	lightmapother = new TPolyMarker(); lightmapother->SetTitle("lightmapother");
	
	// polymarkers of unbinned true vertices of photon hits
	lightmapmu = new TPolyMarker(); lightmapmu->SetTitle("lightmapmu");
	lightmappip = new TPolyMarker(); lightmappip->SetTitle("lightmappip");
	lightmappim = new TPolyMarker(); lightmappim->SetTitle("lightmappim");
	lightmappigamma = new TPolyMarker(); lightmappigamma->SetTitle("lightmappigamma");
	lightmapother = new TPolyMarker(); lightmapother->SetTitle("lightmapother");
	TH2D* blightmapmu=nullptr, *blightmappip=nullptr, *blightmappim=nullptr, *blightmappigamma=nullptr,
		*blightmapother=nullptr;
	TPolyMarker2D *plightmapmu=nullptr, *plightmappip=nullptr, *plightmappim=nullptr, *plightmappigamma=nullptr,
		*plightmapother=nullptr;
	
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
	// CODE DUMP
	///////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////
	// make a map of polymarkers, which will mark the PMT locations
	// each marker collection will represent a different PMT type
	std::map<std::string, TPointSet3D*> pmt_polymarkers;
		// make the marker collection if it doesn't exist
		if(pmt_polymarkers.count(pmt_type_name)==0){
			std::cout<<"new PMT type: "<<pmt_type_name<<std::endl;
			if(pmt_type_name=="INDEX_OUT_OF_BOUNDS"){
				std::cerr<<"PMT "<<pmti<<" with tubeNo "<<thepmt.GetTubeNo()
								 <<", tube type index "<<geo->GetTubeIndex(thepmt.GetTubeNo())
								 <<", type name (from PMT object) "<<thepmt.GetName()
								 <<", type name (from geometry type index) "
								 <<geo->GetWCPMTNameAt(geo->GetTubeIndex(thepmt.GetTubeNo()))
								 <<" and position ("
								 <<thepmt.GetPosition(0)<<", "
								 <<thepmt.GetPosition(1)<<", "
								 <<thepmt.GetPosition(2)<<")"
								 <<" has no type!"<<std::endl;
			}
			pmt_polymarkers.emplace(pmt_type_name, new TPointSet3D());
			pmt_polymarkers.at(pmt_type_name)->SetTitle(pmt_type_name.c_str());
			pmt_counts.emplace(pmt_type_name,0);
		}
		// add this pmt to the marker collection
		float pmtx = thepmt.GetPosition(0);
		float pmty = thepmt.GetPosition(1);
		float pmtz = thepmt.GetPosition(2);
		//std::cout<<"adding a pmt of type "<<pmt_type_name<<std::endl;
		pmt_polymarkers.at(pmt_type_name)->SetNextPoint(pmtx,pmty, pmtz);
		pmt_counts.at(pmt_type_name)++;
		
	}
	
	// give all polymarker sets a unique colour and draw them
	ColourWheel thecolourwheel = ColourWheel();
	for(auto& amarkerset : pmt_polymarkers){
		std::cout<<"drawing markers for collection "<<amarkerset.first<<std::endl;
		TPointSet3D* themarkers = amarkerset.second;
		themarkers->SetMarkerStyle(20);
		themarkers->SetMarkerColor(thecolourwheel.GetNextColour());
		themarkers->Draw("same");
		cout<<"We have "<<pmt_counts.at(amarkerset.first)<<" PMTs of type "<<amarkerset.first<<endl;
		//N.B. themarkers->GetN() returns the ALLOCATED SPACE and is NOT the number of points!!
	}
	thecanvas.BuildLegend();
	
	
	lightmapmu = new TPolyMarker(
	
	
	return true;
}


bool TotalLightMap::Execute(){
	
	return true;
}


bool TotalLightMap::Finalise(){
	
	return true;
}

//////////////////////////////////////////////////
//////////////////////////////////////////////////
// Stolen code, from Michael Nielsony's EventDisplay tool
//////////////////////////////////////////////////
//////////////////////////////////////////////////


void TotalLightMap::make_gui(){
	
	delete_canvas_contents(); // XXX how often is this called?
	
	//draw top circle
	p1 = (TPad*) canvas_ev_display->cd(1);
	canvas_ev_display->Draw();
	gPad->SetFillColor(0);
	p1->Range(0,0,1,1);
	
	top_circle = new TEllipse(0.5,0.5+(tank_height/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
	top_circle->SetFillColor(1);
	top_circle->SetLineColor(1);
	top_circle->SetLineWidth(1);
	top_circle->Draw();
	//draw bulk
	TBox *box = new TBox(0.5-TMath::Pi()*size_top_drawing,0.5-tank_height/tank_radius*size_top_drawing,0.5+TMath::Pi()*size_top_drawing,0.5+tank_height/tank_radius*size_top_drawing);
	box->SetFillColor(1);
	box->SetLineColor(1);
	box->SetLineWidth(1);
	box->Draw();
	//draw lower circle
	TEllipse *bottom_circle = new TEllipse(0.5,0.5-(tank_height/tank_radius+1)*size_top_drawing,size_top_drawing,size_top_drawing);
	bottom_circle->SetFillColor(1);
	bottom_circle->SetLineColor(1);
	bottom_circle->SetLineWidth(1);
	bottom_circle->Draw();
	
	canvas_ev_display->Modified();
	canvas_ev_display->Update();
	
}

///////////////////////////////////////
/////// what does this do
void EventDisplay::draw_lappd_legend(){
	for (int co=0; co<255; co++) {
		float yc = 0.5-tank_height/tank_radius*size_top_drawing+0.03+0.7*2*tank_height/tank_radius*size_top_drawing*co/255;
		TMarker *colordot = new TMarker(0.08,yc,21);
		colordot->SetMarkerColor(Bird_Idx+co);
		colordot->SetMarkerSize(3.);
		colordot->Draw();
	}
	
////color palettes (kBird color palette)
//	const int n_colors=255;
//	double alpha=1.;  //make colors opaque, not transparent
//	Int_t Bird_palette[255];
//	Int_t Bird_Idx;
//	Double_t stops[9] = { 0.0000, 0.1250, 0.2500, 0.3750, 0.5000, 0.6250, 0.7500, 0.8750, 1.0000};
//	Double_t red[9]   = { 0.2082, 0.0592, 0.0780, 0.0232, 0.1802, 0.5301, 0.8186, 0.9956, 0.9764};
//	Double_t green[9] = { 0.1664, 0.3599, 0.5041, 0.6419, 0.7178, 0.7492, 0.7328, 0.7862, 0.9832};
//	Double_t blue[9]  = { 0.5293, 0.8684, 0.8385, 0.7914, 0.6425, 0.4662, 0.3499, 0.1968, 0.0539};
}

///////////////////////////////////////

void EventDisplay::draw_event_PMTs(){
	
	std::cout <<"drawing PMT hits..."<<std::endl;
	marker_pmts_top.clear();
	marker_pmts_bottom.clear();
	marker_pmts_wall.clear();
	
	// Loop over PMTs
	for (std::map<ChannelKey,Detector>::iterator it = Detectors->begin(); it != Detectors->end();it++){
		// if it's not a tank PMT, ignore it
		if( (it->second.GetDetectorElement()!="Tank") || (it->Second.GetChannels().size()>1) ){
			continue;
		}
		Position PMT_position = it->second.GetPositionInTank();  // this is PMT position relative the tank centre
		
		double markerx=0.,markery=0.;
		// calculate position on canvas
		if (it->second.GetTankLocation()=="TopCap"){
			// top cap
			markerx=0.5-size_top_drawing*PMT_position.X()/tank_radius;
			markery=0.5+(tank_height/tank_radius+1)*size_top_drawing-size_top_drawing*PMT_position.Z()/tank_radius;
			theset = *marker_pmts_top;
		} else if (it->second.GetTankLocation()=="BottomCap"){
			// bottom cap
			markerx=0.5-size_top_drawing*PMT_position.Z()/tank_radius;
			markery=0.5-(tank_height/tank_radius+1)*size_top_drawing+size_top_drawing*PMT_position.Z()/tank_radius;
			theset = &marker_pmts_bottom;
		} else if (it->second.GetTankLocation()=="Barrel"){
			// wall
			markerx=0.5+it->second.GetPhi()*size_top_drawing;
			markery=0.5+y_pmt[i_pmt]/tank_height*tank_height/tank_radius*size_top_drawing;
			theset = &marker_pmts_wall;
		} else {
			Log("TotalLightMap Tool: Unrecognised cylinder location of tank PMT: "
					+(it->second.GetTankLocation()), v_warning, verbosity);
		}
		
		// Make the Polymarker
		TPolyMarker* marker = new TPolyMarker(1,&markerx,&markery,"");
		
		// set the colour based on the time / charge of the hit
		if (mode == "Charge")
			// colour based on charge
			color_marker = Bird_Idx+int(charge[i_pmt]/maximum_pmts*254);
		else if (mode == "Time" && threshold_time_high == -999 && threshold_time_low == -999)
			// if we have no time window in effect, colour based on time within event
			color_marker = Bird_Idx+int((time[i_pmt]-min_time_overall)/(maximum_time_overall-min_time_overall)*254);
		else if (mode == "Time")
			// else if we have a time window, colour based on time within window
			color_marker = Bird_Idx+int((time[i_pmt]-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
		
		// Set the marker properties
		marker->SetMarkerColor(color_marker);
		marker->SetMarkerStyle(8);
		marker->SetMarkerSize(1);
		// Add to the relevant set
		theset->push_back(marker);
	}
	
	// draw all the markers
	for (int i_marker=0;i_marker<marker_pmts_top.size();i_marker++){
		marker_pmts_top.at(i_marker)->Draw();
	}
	for (int i_marker=0;i_marker<marker_pmts_bottom.size();i_marker++){
		marker_pmts_bottom.at(i_marker)->Draw();
	}
	for (int i_marker=0;i_marker<marker_pmts_wall.size();i_marker++){
		marker_pmts_wall.at(i_marker)->Draw();
	}
	
}

///////////////////////////////////////

	void EventDisplay::draw_event_LAPPDs(){
	
	std::cout <<"Drawing LAPPD hits..."<<std::endl;
	
	marker_lappds.clear();
	
	double phi;
	double x[1];
	double y[1];
	double charge_single=1.;  //FIXME when charge is implemented in LoadWCSimLAPPD
	
	// loop over LAPPDs with a hit
	for (int i_lappd_hits=0;i_lappd_hits<hits_LAPPDs.size();i_lappd_hits++){
		// Lop over hits on this LAPPD
		for (int i_single_lappd=1; i_single_lappd<hits_LAPPDs.at(i_lappd_hits).size(); i_single_lappd++){ 
			//start at 1 to avoid the default fill of the first entry with zero vector XXX ??? huh??
			
			// get time of this hit
			double time_lappd_single = time_lappd.at(i_lappd_hits).at(i_single_lappd-1);
			
			// skip drawing this LAPPD hit if the time/charge is outside the plot range...
			if( (charge_single<threshold_lappd) ||
				  (threshold_time_high !=-999 && time_lappd_single>threshold_time_high) ||
				  (threshold_time_low  !=-999 && time_lappd_single<threshold_time_low )  ) continue;
			
			// get LAPPD position
			double x_lappd = hits_LAPPDs.at(i_lappd_hits).at(i_single_lappd).X();
			double y_lappd = hits_LAPPDs.at(i_lappd_hits).at(i_single_lappd).Y();
			double z_lappd = hits_LAPPDs.at(i_lappd_hits).at(i_single_lappd).Z();
			x_lappd-=tank_center_x;
			y_lappd-=tank_center_y;
			z_lappd-=tank_center_z;
			
			// calculate LAPPD phi
			if (x_lappd>0 && z_lappd>0) phi = atan(z_lappd/x_lappd)+TMath::Pi()/2;
			else if (x_lappd>0 && z_lappd<0) phi = atan(x_lappd/-z_lappd);
			else if (x_lappd<0 && z_lappd<0) phi = 3*TMath::Pi()/2+atan(z_lappd/x_lappd);
			else if (x_lappd<0 && z_lappd>0) phi = TMath::Pi()+atan(-x_lappd/z_lappd);
			if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
			phi-=TMath::Pi();
			if (phi < - TMath::Pi()) phi = -TMath::Pi();
			if (phi < - TMath::Pi() || phi>TMath::Pi())
				std::cout <<"Drawing LAPPD event: Phi out of bounds! "
									<<", x= "<<x_lappd
									<<", y="<<y_lappd
									<<", z="<<z_lappd
									<<", phi="<<phi
									<<std::endl;
			
			// calculate position on canvas
			x[0]=0.5+phi*size_top_drawing;
			y[0]=0.5+y_lappd/tank_height*tank_height/tank_radius*size_top_drawing;
			
			// make the polymarker
			TPolyMarker *marker_lappd = new TPolyMarker(1,x,y,"");
			// set the colour based on the time / charge of the hit
			if (mode == "Charge")
				// colour based on charge (placeholder; LAPPD hits do not have charge)
				color_marker = Bird_Idx+254;
			else if (mode == "Time" && threshold_time_high == -999 && threshold_time_low == -999) {
				// if we have no time window in effect, colour based on time within event
				if (time_lappd_single > maximum_time_overall || time_lappd_single < min_time_overall){
					std::cout <<"lappd time: "<<time_lappd_single
										<<", min time: "<<min_time_overall
										<<", max time: "<<maximum_time_overall
										<<std::endl;
				}
				color_marker = Bird_Idx+int((time_lappd_single-min_time_overall)/(maximum_time_overall-min_time_overall)*254);
			}
			else if (mode == "Time")
				// else if we have a time window, colour based on time within window
				color_marker = Bird_Idx+int((time_lappd_single-threshold_time_low)/(threshold_time_high-threshold_time_low)*254);
			
			// Set the marker properties
			marker_lappd->SetMarkerColor(color_marker);
			marker_lappd->SetMarkerStyle(8);
			marker_lappd->SetMarkerSize(0.4);
			// Add to the relevant set
			marker_lappds.push_back(marker_lappd);
		}
	}
	
	// draw the polymarkers
	for (int i_draw=0; i_draw<marker_lappds.size();i_draw++){
		marker_lappds.at(i_draw)->Draw();
	}
	
}


//////////////////////////////////////////

	void EventDisplay::draw_true_vertex(){
	
	find_projected_xyz(truevtx_x, truevtx_y, truevtx_z, truedir_x, truedir_y, truedir_z, vtxproj_x, vtxproj_y, vtxproj_z);
	std::cout <<"projected vtx on wall: ( "<< vtxproj_x<<" , "<<vtxproj_y<<" , "<<vtxproj_z<< " )"<<std::endl;
	std::cout <<"drawing vertex on EventDisplay.... "<<std::endl;
			
	double xTemp, yTemp, phiTemp;
	int status_temp;
	translate_xy(vtxproj_x,vtxproj_y,vtxproj_z,xTemp,yTemp, status_temp, phiTemp);
	
	double x_vtx[1] = {xTemp};
	double y_vtx[1] = {yTemp};
	
	TPolyMarker *marker_vtx = new TPolyMarker(1,x_vtx,y_vtx,"");
	marker_vtx->SetMarkerColor(2);
	marker_vtx->SetMarkerStyle(22);
	marker_vtx->SetMarkerSize(1.);
	marker_vtx->Draw();
	
	}


//////////////////////////////////////////

void EventDisplay::find_projected_xyz(double vtxX, double vtxY, double vtxZ, double dirX, double dirY, double dirZ, double &projected_x, double &projected_y, double &projected_z){

	double time_top, time_wall, time_wall1, time_wall2, time_min;
	double a,b,c; //for calculation of time_wall

	time_top = (dirY > 0)? (max_y-vtxY)/dirY : (min_y - vtxY)/dirY;
	a = dirX*dirX + dirZ*dirZ;
	b = 2*vtxX*dirX + 2*vtxZ*dirZ;
	c = vtxX*vtxX+vtxZ*vtxZ-tank_radius*tank_radius;
	time_wall1 = (-b+sqrt(b*b-4*a*c))/(2*a);
	time_wall2 = (-b-sqrt(b*b-4*a*c))/(2*a);
	if (time_wall2>=0 && time_wall1>=0) time_wall = (time_wall1<time_wall2) ? time_wall1 : time_wall2;
	else if (time_wall2 < 0 ) time_wall = time_wall1;
	else if (time_wall1 < 0 ) time_wall = time_wall2;

	time_min = (time_wall < time_top && time_wall >=0 )? time_wall : time_top;

	projected_x = vtxX+dirX*time_min;
	projected_y = vtxY+dirY*time_min;
	projected_z = vtxZ+dirZ*time_min;

}

//////////////////////////////////////////

void EventDisplay::translate_xy(double vtxX, double vtxY, double vtxZ, double &xWall, double &yWall, int &status_hit, double &phi_calc){
	
	if (fabs(vtxY-max_y)<0.01){            //draw vtx projection on the top of tank
		
		xWall=0.5-size_top_drawing*vtxX/tank_radius;
		yWall=0.5+(tank_height/tank_radius+1)*size_top_drawing-size_top_drawing*vtxZ/tank_radius;
		status_hit = 1;
		phi_calc = -999;
		if (sqrt(vtxX*vtxX+vtxZ*vtxZ)>tank_radius) status_hit = 4;
	} else if (fabs(vtxY-min_y)<0.01){            //draw vtx projection on the top of tank
		xWall=0.5-size_top_drawing*vtxX/tank_radius;
		yWall=0.5-(tank_height/tank_radius+1)*size_top_drawing+size_top_drawing*vtxZ/tank_radius;
		status_hit = 2;
		phi_calc = -999;
		if (sqrt(vtxX*vtxX+vtxZ*vtxZ)>tank_radius) status_hit = 4;
	}else {
		double phi;
		if (vtxX>0 && vtxZ>0) phi = atan(vtxZ/vtxX)+TMath::Pi()/2;
		else if (vtxX>0 && vtxZ<0) phi = atan(vtxX/-vtxZ);
		else if (vtxX<0 && vtxZ<0) phi = 3*TMath::Pi()/2+atan(vtxZ/vtxX);
		else if (vtxX<0 && vtxZ>0) phi = TMath::Pi()+atan(-vtxX/vtxZ);
		if (phi>2*TMath::Pi()) phi-=(2*TMath::Pi());
		phi-=TMath::Pi();
		if (phi < - TMath::Pi()) phi = -TMath::Pi();
		xWall=0.5+phi*size_top_drawing;
		yWall=0.5+vtxY/tank_height*tank_height/tank_radius*size_top_drawing;
		status_hit = 3;
		phi_calc = phi;
		if (vtxY>max_y && vtxY<min_y) status_hit = 4;
		
	}
	
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////


	std::map<ChannelKey,Detector> *Detectors = anniegeom->GetDetectors();
	ChannelKey ck;
	Position position_PMT;
	int detector_id;
	max_y = -100.;
	min_y =  100.;
	
	for (std::map<ChannelKey,Detector>::iterator it = Detectors->begin(); it != Detectors->end();it++){
		ck = it->first;
		
		// if it's an MRD PMT record it's position
		if (it->second.GetDetectorElement() == "MRD"){
			position_PMT = it->second.GetDetectorPosition();
			detector_id = it->second.GetDetectorId();
			x_MRD[detector_id]=position_PMT.X()-tank_center_x;
			y_MRD[detector_id]=position_PMT.Y()-tank_center_y;
			z_MRD[detector_id]=position_PMT.Z()-tank_center_z;
		}
		
		// if it's a Tank detector...
		if(it->second.GetDetectorElement() =="Tank"){
			// if it's an LAPPD, just increment the counter and then continue XXX redundant
			if (it->second.GetDetectorType()=="lappd_v1"){
				n_lappds++;
				continue;
			}
			// if it's a tank PMT, record it's position, and get the maximum extents
			// for tank PMTs
			position_PMT = it->second.GetDetectorPosition();
			detector_id = it->second.GetDetectorId();
			x_pmt[detector_id]=position_PMT.X()-tank_center_x;
			y_pmt[detector_id]=position_PMT.Y()-tank_center_y;
			z_pmt[detector_id]=position_PMT.Z()-tank_center_z;
			if (y_pmt[detector_id]>max_y) max_y = y_pmt[detector_id];
			if (y_pmt[detector_id]<min_y) min_y = y_pmt[detector_id];
		}
	} // loop over detectors


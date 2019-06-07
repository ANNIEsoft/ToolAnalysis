#include "FTBFAnalysis.h"

using namespace std;

FTBFAnalysis::FTBFAnalysis():Tool(){}


bool FTBFAnalysis::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // setup the output files
  TString OutFile = "lappdout.root";	 //default input file
  m_variables.Get("outfile", OutFile);
  tff = new TFile(OutFile,"RECREATE");

  return true;
}


bool FTBFAnalysis::Execute(){
  int event;
  int board;
  m_variables.Get("eventno", event);
  m_variables.Get("boardno", board);
  //HeatmapEvent(event, board);
  //PlotSeparateChannels(event, board);
  

  return true;
}


//write's a heatmap of an event to 
//the rootfile
void FTBFAnalysis::HeatmapEvent(int event, int board){
	map<int, map<int, map<int, Waveform<double>>>> rawData;
    m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData_ftbf",rawData);
    vector<float> sampletimes;
    map<int, map<string, vector<float>>> caldata;
    m_data->Stores["ANNIEEvent"]->Get("psec4caldata", caldata);
    sampletimes = caldata[board]["sampletimes"];

    int n_cells;
    int n_chs;
    m_data->Stores["ANNIEEvent"]->Get("num_cells", n_cells);
    m_data->Stores["ANNIEEvent"]->Get("num_chs", n_chs);

    double maxtime = sampletimes.at(0);
    for(vector<float>::iterator it = sampletimes.begin(); it != sampletimes.end(); ++it)
    {
    	if(*it > maxtime) maxtime = *it;
    }

    TH2D *VoltageMap = new TH2D("VoltageMap","VoltageMap",n_cells,0.5,maxtime+0.5,n_chs,0.5,n_chs+0.5);
    map<int, Waveform<double>>::iterator itr;
    Waveform<double> tempwav;
    for(itr = rawData[event][board].begin(); itr != rawData[event][board].end(); ++itr)
    {
    	//check to make sure that this
    	//waveform has as many samples as
    	//the size of the time array
    	int ch = itr->first;
    	tempwav = itr->second; 
    	if(tempwav.GetSamples()->size() != sampletimes.size())
    	{
    		cout << "Mismatch between sample times and loaded waveforms. Check calibration file" << endl;
    		return;
    	}

		for(int sp = 0; sp < n_cells; sp++)
		{
			VoltageMap->Fill(sampletimes.at(sp), ch, tempwav.GetSample(sp));
		}
    	

    }

    //label the figure
    string title = "heatmap event " + to_string(event) + " board " + to_string(board);
    int ntemp = title.length();
    char chartitle[ntemp + 1];
    strcpy(chartitle, title.c_str());
    VoltageMap->SetNameTitle(chartitle, chartitle);
    VoltageMap->SetMaximum(20);
    VoltageMap->SetMinimum(-50);

    TCanvas* c1 = new TCanvas(chartitle, chartitle);
    c1->cd();
    VoltageMap->Draw("COLZ");
    c1->Modified(); c1->Update();
    c1->Write();
    return;

}

//write's a heatmap of an event to 
//the rootfile. Currently not working
//because it only draws one channel? 
void FTBFAnalysis::PlotSeparateChannels(int event, int board){
	//get data and sample times for each channel
	map<int, map<int, map<int, Waveform<double>>>> rawData;
    m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData_ftbf",rawData);
    vector<float> sampletimes;
    map<int, map<string, vector<float>>> caldata;
    m_data->Stores["ANNIEEvent"]->Get("psec4caldata", caldata);
    sampletimes = caldata[board]["sampletimes"];

    int n_cells;
    int n_chs;
    m_data->Stores["ANNIEEvent"]->Get("num_cells", n_cells);
    m_data->Stores["ANNIEEvent"]->Get("num_chs", n_chs);

    //find max time for determining binning of hist
    double maxtime = sampletimes.at(0);
    for(vector<float>::iterator it = sampletimes.begin(); it != sampletimes.end(); ++it)
    {
    	if(*it > maxtime) maxtime = *it;
    }


    //create title for a canvas
    string cantitle ="event " + to_string(event) + " board " + to_string(board);
   	int ntempcan = cantitle.length();
    char chartitlecan[ntempcan + 1];
    strcpy(chartitlecan, cantitle.c_str());
    TCanvas* cdivided = new TCanvas(chartitlecan, chartitlecan, 1000, 950);

    //divide canvas into subplots by channel
    cdivided->Divide(6, 5);

    vector<TH1D*> WaveHists;
    for(int i = 0; i < n_chs; i++)
    {
    	cantitle = to_string(i+1);
    	ntempcan = cantitle.length();
    	char whtitle[ntempcan+1];
    	strcpy(whtitle, cantitle.c_str());
    	WaveHists.push_back(new TH1D(whtitle, whtitle, n_cells, 0.5, 0.5+maxtime));
    }

    map<int, Waveform<double>>::iterator itr;
    Waveform<double> tempwav;
    for(itr = rawData[event][board].begin(); itr != rawData[event][board].end(); ++itr)
    {
    	//check to make sure that this
    	//waveform has as many samples as
    	//the size of the time array
    	int ch = itr->first;
    	tempwav = itr->second; 
    	if(tempwav.GetSamples()->size() != sampletimes.size())
    	{
    		cout << "Mismatch between sample times and loaded waveforms. Check calibration file" << endl;
    		return;
    	}

		for(int sp = 0; sp < n_cells; sp++)
		{
			WaveHists.at(ch-1)->Fill(sampletimes.at(sp), tempwav.GetSample(sp));
		}
		string title ="channel " + to_string(ch);
   		int ntemp = title.length();
    	char chartitle[ntemp + 1];
    	strcpy(chartitle, title.c_str());
		WaveHists.at(ch-1)->SetNameTitle(chartitle, chartitle);
		//select the subplot
		cdivided->cd(ch-1);
		WaveHists.at(ch-1)->Draw();
    }

   	cdivided->cd();
    cdivided->Write();

    return;

}

bool FTBFAnalysis::Finalise(){
  tff->Close();
  return true;
}

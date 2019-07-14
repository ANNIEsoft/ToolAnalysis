#include "FTBFAnalysis.h"

using namespace std;

FTBFAnalysis::FTBFAnalysis():Tool(),_event_counter(0),_file_number(0),_display_config(0),_display(nullptr){}


bool FTBFAnalysis::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // setup the output files
  std::string OutFile = "lappdout.root";	 //default input file
  m_variables.Get("OutputFile", OutFile);
  OutFile.erase(OutFile.size()-5);
  OutFile = OutFile + "00.root";
  cout << "FTBF filename: " << OutFile << endl;


  m_variables.Get("EventDisplay", _display_config);

  if (_display_config > 0)
  {
        _display = new LAPPDDisplay(OutFile, _display_config);
  }


  return true;
}


bool FTBFAnalysis::Execute(){

    //The files become too large, if one tries to save all WCSim events into one file.
    //Every 100 events get a new file.
    if(_event_counter == (20 * (_file_number + 1)))
    {
        _display->OpenNewFile(_file_number);
        _file_number++;
    }
    //Initialise the histogram for displaying all LAPPDs at once
    if (_display_config > 0)
    {
        _display->InitialiseHistoAllLAPPDs(_event_counter);
    }

    bool testval = false; //test for checking the store
    
    testval = m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData", LAPPDWaveforms);
    /*
    if(testval)
    {
        PlotRawHists();
    }
    */
    testval = m_data->Stores["ANNIEEvent"]->Get("nnls_solution", NNLSsoln);
    if(testval)
    {
        cout << "got the objects" << endl;
        for(int i = 0; i < NNLSsoln.size(); i++)
        {
            NnlsSolution tmp = NNLSsoln.at(i);
            tmp.Print();
        }
    }
    


    if (_display_config > 0)
    {
        _display->FinaliseHistoAllLAPPDs();
    }

    return true;

}


void FTBFAnalysis::PlotNNLSandRaw()
{

    return; 

}   


void FTBFAnalysis::PlotRawHists()
{
    int tube_no = 1; //temporary
    vector<Waveform<double>> vwavs; 
    if(_display_config > 0)
    {
        for(int ch = 0; ch < LAPPDWaveforms.size(); ch++)
        {
            vwavs.push_back(LAPPDWaveforms[ch].front());
        }

        _display->RecoDrawing(_event_counter, tube_no, vwavs);
        if (_display_config == 2)
        {
            do
            {
                std::cout << "Press a key to continue..." << std::endl;
            } while (cin.get() != '\n');

            std::cout << "Continuing" << std::endl;
        }

    }
    return;
}


//write's a heatmap of an event to 
//the rootfile
void FTBFAnalysis::HeatmapEvent(int event, int board)
{
    return;
}

//write's a heatmap of an event to 
//the rootfile. Currently not working
//because it only draws one channel? 
void FTBFAnalysis::PlotSeparateChannels(int event, int board)
{
    return;
}

bool FTBFAnalysis::Finalise()
{
  return true;
}

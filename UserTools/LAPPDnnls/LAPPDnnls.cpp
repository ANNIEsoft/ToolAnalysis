#include "LAPPDnnls.h"


LAPPDnnls::LAPPDnnls():Tool(){}


bool LAPPDnnls::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  return true;
}


bool LAPPDnnls::Execute(){

	int event;
	int board;
	m_variables.Get("eventno", event);
	m_variables.Get("boardno", board);


	// get raw lappd data
	map<int, map<int, map<int, Waveform<double>>>> rawData;
	m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData_ftbf",rawData);
	map<int, map<string, vector<float>>> caldata;
	vector<float> sampletimes;
    m_data->Stores["ANNIEEvent"]->Get("psec4caldata", caldata);
    sampletimes = caldata[board]["sampletimes"];

	//This variable controls the sampling depth of the
	//nnls algorithm and allows the template to have
	//more samples than the signal waveform. The template
	//waveform is reduced to (nsamples template)/samplingFactor
	//number of samples. The timestep between template
	//and signal waveform is made equal by expanding the
	//signal waveform to a larger number of total samples. 
	double samplingFactor;
	m_variables.Get("sampling_factor", samplingFactor);

	//maximum number of nnls iterations
	int maxiter;
	m_variables.Get("maxiter", maxiter);

	//you can use any template you want for the 
	//algorithm. It assumes that it is a histogram
	//in a root file named ""
	TString tempfilename = "dummy.root";
	m_variables.Get("tempfilename", tempfilename);
	TString temphistname = "dummy";
	m_variables.Get("temphistname", temphistname);

	//get the template waveform. This assumes
	//that the file is .root file with a TH1D
	//named templatehistname
	TFile* tempfile = new TFile(tempfilename, "READ");
	TH1D* temphist = (TH1D*)tempfile->Get(temphistname);
	Waveform<double> tempwave; //signal of template
	vector<double> temptimes;  //times of template
	int nbins = temphist->GetNbinsX(); 
	double binwidth = temphist->GetBinWidth(0); //should be in ns
	double starttime = temphist->GetBinLowEdge(0); //first time in template histogram, used to set template to start at 0ns
	double endtime = temphist->GetBinLowEdge(nbins-1);
	double newtimestep = binwidth*samplingFactor*1000; //in ps

	//fill the template waveform and time vectors 
	//at a downsampled/upsampled time sampling using
	//the root built in linear interpolation
	for(double t = starttime; t <= endtime; t+=newtimestep)
	{
		tempwave.PushSample(temphist->Interpolate(t));
		temptimes.push_back((t - starttime)*1000); //zeroed time vector, in ps just like signal wave
	}


	//initialize variables used in nnls 
	//library to solve nnls problem
	nsNNLS::matrix* A;
	nsNNLS::vector* b;
	nsNNLS::vector* x;
	nsNNLS::nnls*   solver;

	//the number of rows for the matrix
	//and the number of samples in the signal waveform
	//is set to match the timesteps of the NEW template
	//waveform and the signal waveform. Here, we get
	//the signal waveform and expand it to that nrows sampling rate
	map<int, Waveform<double>> signalwaves = rawData[event][board]; //map[channel] = Waveform<double>
	map<int, Waveform<double>>::iterator itr; //iterator for channels
    Waveform<double> signalwave; 
    size_t nrows;
    int flag; //error flag on nnls solver
    //loop over each channel
    for(itr = signalwaves.begin(); itr != signalwaves.end(); ++itr)
    {
    	int ch = itr->first; //channel of this waveform in the loop
    	cout << "On channel " << ch << endl;
    	signalwave = itr->second; 
    	b = BuildWaveformVector(signalwave, sampletimes, newtimestep);
    	nrows = b->length(); //size of the vector will be square size of the matrix
    	cout << "doing a " << nrows << " x " << nrows << " = " << nrows*nrows << " matrix " << endl;

    	//The denseMatrix class is used here and is constructed
		nsNNLS::matrix* A = new nsNNLS::denseMatrix(nrows, nrows); //initialization without data.
    	
    	//A = BuildTemplateMatrix(tempwave, nrows);
    	/*
    	solver = new nsNNLS::nnls(A, b, maxiter);
    	flag = solver->optimize();
    	if(flag < 0)
    	{
    		cout << "NNLS solver terminated with an error flag" << endl;
    	}
    	x = solver->getSolution();
    	*/

    }


	return true;
}

//makes the nnls matrix A given a root template file. 
//The algorithm requires a careful accounting of the
//number of samples in both the template and the signal waveform.
//Furthermore, to preserve conclusions about the resulting signals,
//one needs to maintain that the signal and the template matrix
//have the same time-step. The variable nrows is the number of rows in the end matrix, 
//and is determined by the samplingFactor from Execute and the number of samples in the template. 
//Both here and in BuildWaveformVector, the template and signal waveform
//are over/undersampled to match timestamps and number of samples.
nsNNLS::matrix* LAPPDnnls::BuildTemplateMatrix(Waveform<double> tempwave, size_t nrows)
{
	
	//The denseMatrix class is used here and is constructed
	nsNNLS::matrix* A = new nsNNLS::denseMatrix(nrows, nrows); //initialization without data. 
	int temp_size = tempwave.GetSamples()->size();
	

	//the nnls matrix consists of:
	//each row is a version of the template waveform
	//inserted starting at a sample indexed by the column number. 
	//the next column then has another template waveform inserted
	//one sample further in time. The elements below the diagonal
	//are 0, and any elements that are not part of the template
	//are 0. 
	for(int row = 0; row < nrows; row++)
	{
		for(int col = 0; col < nrows; col++)
		{
			//elements below diagonal are zero
			if(col < row)
			{
				A->set(row, col, 0.0);
			}
			//on the upper diagonal, the template
			//waveform is written to a row, starting
			//with the column = row
			else if((col - row) < temp_size)
			{
				A->set(row, col, tempwave.GetSample(col - row));
			}
			//otherwise (outside bounds of template)
			//set to 0. 
			else {A->set(row, col, 0.0);}
		}
	}

	return A;
} 

//formats the waveform into the vector format expected by nnls algo.
//see comment above BuildTemplateMatrix for explanation of nrows
nsNNLS::vector* LAPPDnnls::BuildWaveformVector(Waveform<double> wave, vector<float> times, double template_timestep)
{

	//make a new signal vector that is
	//NOT interpolated, but has more samples
	// so that the timesteps of the template
	// and signal waveform are equal. 
	vector<double> newwave;
	float t0;
	float t1;
	for(double t = times.front(); t < times.back(); t+=template_timestep)
	{
		//find value of waveform at this time by 
		//finding closest sample times. Assumes the
		//times vector is ordered
		for(int i = 0; i < times.size() - 1; i++)
		{
			t0 = times.at(i);
			t1 = times.at(i+1);
			if(t >= t0 and t < t1)
			{
				newwave.push_back(wave.GetSample(i));
				break;
			}
		}
	}


	
	//create a double array for the
	//nnls vector initionalization
	size_t vsize = newwave.size();
	const size_t vsize_const = newwave.size();
	double dvec[vsize_const];
	for(int i = 0; i < vsize; i++)
	{
		dvec[i] = newwave.at(i);
	}

	nsNNLS::vector* v = new nsNNLS::vector(vsize, dvec);
	return v;
	
} 




bool LAPPDnnls::Finalise(){

  return true;
}

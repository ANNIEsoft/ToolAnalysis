/*
This tool is a waveform processing tool that uses the Lawson and Hanson
non-negative least squares deconvolution algorithm to find signals
with a known "template" in a noisy waveform. The libraries found
in this tool directory are taken from: http://suvrit.de/work/soft/nnls.html

The algorithm numerically solves the matrix equation Ax = b by
minimizing 0.5*|Ax - b|^2 . In the context of a template fit,
the matrix A represents information about the template, b is
a vector form of the noisy waveform where the vector index is a time
index, and x is a "solution vector". Each row of A is a time series
(indexed by the column) that is a template waveform. Subsequent rows of
A have the template waveform shifted in time by one sample. Therefore, each
element of solution vector "x" represents how many template waveforms should
be placed at a particular index in time. The sum of all such components is
the final fitted solution, A*x .

Expects raw data, named in the config file, of the form map<int, vector<Waveform<double>>>.
Expects a template that is a TH1D coming from a root file. The template often has a much
higher sampling rate than the raw data. This algorithm can handle that and can use it as
extra information. There is a parameter "samplingFactor" that controls how to up/downsample
the template and raw data so that they match timescales. A list of sample times is also
needed to place the solution components in the correct position relative to the rawdata
time-series.

-Evan Angelico, 7/11/2019
*/

/*
The algorithm and the nnls solver are from WavefromNNLS Tool, optimized for fitting multipule
photon data.

Structure:
resample the template waveform based on the sampling samplingFactor, get tempwave and temptimes
resample the rawdata to the template timestep


*/



#include "LAPPDnnlsPeak.h"
#include <TCanvas.h>
#include <math.h>
#include <string>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>




LAPPDnnlsPeak::LAPPDnnlsPeak():Tool(){}


bool LAPPDnnlsPeak::Initialise(std::string configfile, DataModel &data){
    cout<<"start nnls"<<endl;
  //cout<<"Start initialise nnls peak"<<endl;
  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  //cout<<"nnls load variables"<<endl;
  TString IWL;
  m_variables.Get("nnlsInputWavLabel",IWL);
  InputWavLabel = IWL;

  TString OWL;
  m_variables.Get("nnlsOutputWavLabel",OWL);
  OutputWavLabel = OWL;

  //sampling factor.
  //The template waveform is reduced by this sampling factor, for instance, (1000 template)/10 = 100
	m_variables.Get("sampling_factor", samplingFactor);
  //template file read
	m_variables.Get("tempfilename", tempfilename);
	m_variables.Get("temphistname", temphistname);
  m_variables.Get("nnlsVerbosityLevel",nnlsVerbosityLevel);

  //get the template waveform. This assumes
	//that the file is .root file with a TH1D
	//named templatehistname
  tempfile = new TFile(tempfilename, "READ"); // if I declear a TFile in .h, should I use "new" here?
	temphist = (TH1D*)tempfile->Get(temphistname);
	//Waveform<double> tempwave; //signal of template in .h
	//vector<double> temptimes;  //times of template in .h
	nbins = temphist->GetNbinsX();
	binwidth = temphist->GetBinWidth(0); //should be in ns
	starttime = temphist->GetBinLowEdge(0); //first time in template histogram, used to set template to start at 0ps
	endtime = temphist->GetBinLowEdge(nbins-1);
  //this new timestep will be the new timestep for the
	//raw data. Even if the raw data is sampled at a non-constant
	//rate (for example, well calibrated LAPPD electronics), the raw
	//waveform is re-sampled (not interpolated) at this new timestep
	newtimestep = binwidth*samplingFactor; //in ps

  //fill the template waveform and time vectors
	//at a downsampled/upsampled time sampling using
	//linear interpolation. Interpolation not necessary, it
	//can just be re-sampled. One method may perform better than another
	for(double t = starttime; t <= endtime; t+=newtimestep)
	{
		tempwave.PushSample(temphist->Interpolate(t));
		//zero the time vector, in ps just like signal wave
		temptimes.push_back((t - starttime));
    if (nnlsVerbosityLevel>1) {
      cout << "interpolate  time bin: " << t << ", value " << temphist->Interpolate(t) <<  endl;
    }
	}
  //use the new time step to create an interpolated new histogram, same waveform, different step


  m_variables.Get("maxiter",maxiter);
  //max iter of solving, lagrer will reduce error
  m_variables.Get("multiThread",multiThread);
  m_variables.Get("threadNumber",threadNumber);
  m_variables.Get("timeCount",timeCount);
  m_variables.Get("nnlsPrintOption",nnlsPrintOption);

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);
  cout<<"end nnls"<<endl;
  return true;
}


bool LAPPDnnlsPeak::CompactPeakExe(std::map<unsigned long, vector<Waveform<double>>> &lappddata,
                    std::map<int,NnlsSolution> &soln,
                    std::map<unsigned long, vector<Waveform<double>>> &FittedPulseNumber,
                    size_t nrows,
                    Waveform<double> tempwave,
                    vector<double> temptimes,
                    vector<float> sampletimes,
                    double newtimestep,
                    int maxiter,
                    vector<double> newsignaltimes,
                    int nnlsVerbosityLevel,
                    vector<int> keymap,
                    int start,
                    int end,
                    int threadCount){

  if (nnlsVerbosityLevel>0) cout<<"thread "<<threadCount<<" here, channel between "<<start<<", "<<end <<endl;
  std::map<unsigned long, vector<Waveform<double>>> :: iterator itr;
  for (int i = start;i<end;i++){
    itr = lappddata.find(keymap.at(i));

    Waveform<double> signalwave;
    // the size of rawdata waveform in newtimestep
    int flag;

    if (nnlsVerbosityLevel>0) {
      cout << "doing a " << nrows << " x " << nrows << " = " << nrows*nrows << " matrix " << endl;
    }

    nnlsvector* b = new nnlsvector(nrows);
    nnlsvector* x = new nnlsvector(nrows);
    nnlsmatrix* A = new denseMatrix(nrows, nrows); //modify this matrix A
    BuildTemplateMatrix(A, tempwave, nrows); //make the template matrix

    int ch = itr->first;
    soln[ch].SetTemplate(tempwave,temptimes);   //save the template waveform for each channel in this map
    //looping in all vectors in this waveform, currently only one signal, 2022.5.23
    // if there are more vectors, looping should start early.
    vector<Waveform<double>> Vwavs = itr->second;
    vector<Waveform<double>> plotWaveVector;
    unsigned long channelNo = ch;
    for (int i = 0; i<Vwavs.size(); i++){
      signalwave = Vwavs.at(i);
      BuildWaveformVector(b,signalwave,sampletimes,newtimestep);
      nnls* solver = new nnls(A, b, maxiter); //here get the solution
      flag = solver->optimize();
      if(flag<0){
        cout << "NNLS solver terminated with an error flag" << endl;
      }
      x = solver->getSolution();
      SaveNNLSOutput(&soln[ch],A, x, newsignaltimes);
      Waveform<double> plotWave;
      for (int j=0; j<soln[ch].GetNumberOfComponents(); j++){
        plotWave.PushSample(-soln[ch].GetComponentScale(j));
      }
        plotWaveVector.push_back(plotWave);
        delete solver;
      }
    FittedPulseNumber.insert(pair<unsigned long, vector<Waveform<double>>>(channelNo, plotWaveVector));

    delete A; //free the memory
    delete b;
    delete x;

  }//end vector looping
  return true;
}




bool LAPPDnnlsPeak::Execute(){

auto start = std::chrono::high_resolution_clock::now();

  std::map<unsigned long, vector<Waveform<double>>> lappddata;
  m_data->Stores["ANNIEEvent"]->Get(InputWavLabel, lappddata);


  // resample the rawdata timestep to the template timestep
  vector<float> sampletimes;
  //assume constant sampling rate at all channels
  //assume constant sampling rate.
  double dt = (1.0/(256*40*1e6))*1e12; //ps
  for(int i = 0; i < 256; i++){
    sampletimes.push_back(i*dt);
  } //event sampling time

  //create a newe signal times list based on the new timestep of the templatepulse
  vector<double> newsignaltimes;
	for(double t = sampletimes.front(); t <= sampletimes.back(); t+=newtimestep)
	{
		newsignaltimes.push_back(t);
	}
  size_t nrows = newsignaltimes.size();



  std::map<unsigned long, vector<Waveform<double>>> :: iterator itr; //lappddata iterator
  std::map<int,NnlsSolution> soln; //nnls solution map
  std::map<unsigned long, vector<Waveform<double>>> FittedPulseNumber; //pure fitted pulse number map, for plots

  //std::thread t(&ToolName::functionName, this, j);

  if(multiThread ==1){ //if go with multi threads
    vector<int> keymap; //build a key map, to seperate channel looping to multi threads
    for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
      keymap.push_back(itr->first);
    }
    int total = lappddata.size(); //calcualte how many loops should be in one thread
    int firstPart= ((int)total/threadNumber)+1;
    //if (total%threadNumber ==0) firstPart-=1;
    int secondPart = ((int)total/threadNumber);
    int firstN = total%threadNumber;
    std::vector<std::thread> threadExecute(threadNumber);  //contain variable numbers of threads
    for (int j = 0;j<threadNumber;j++){ //fill the threads
      int start;
      int end;
      if (j<firstN){
         start = j*firstPart;
         end = (j+1)*firstPart;
      }else{
         start = firstN*firstPart+(j-firstN)*secondPart;
         end = firstN*firstPart+(j-firstN+1)*secondPart;}
        cout<<"on thread "<<j<<" doing from start "<<start<<" to end "<<end<<endl;
      auto t = thread ( &LAPPDnnlsPeak::CompactPeakExe, this,
                                  std::ref(lappddata),
                                  std::ref(soln),
                                  std::ref(FittedPulseNumber),
                                  nrows,
                                  tempwave,
                                  temptimes,
                                  sampletimes,
                                  newtimestep,
                                  maxiter,
                                  newsignaltimes,
                                  nnlsVerbosityLevel,
                                  keymap,
                                  start,
                                  end,
                                  j);


        threadExecute.emplace_back(move(t));
    }

    for (std::thread & th : threadExecute){//execute threads
        if (th.joinable())th.join();
    }

  }

if (nnlsPrintOption ==1 ){
  cout<<"print raw data here:"<<endl;
  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
    int ch = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;
      Waveform<double> signalwave = Vwavs.at(0);
      cout<<ch<<", ";
      for (int i =0; i<signalwave.GetSamples()->size();i++){
        if(i==signalwave.GetSamples()->size()-1){
          cout<<signalwave.GetSample(i)<<endl;
        }else{
          cout<<signalwave.GetSample(i)<<", ";
        }
      }
  }
}

if(multiThread == 0){  //if not multiThread
  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){

    Waveform<double> signalwave;
    size_t nrows = newsignaltimes.size();// the size of rawdata waveform in newtimestep
    int flag;

    if (nnlsVerbosityLevel>0) {
      cout << "doing a " << nrows << " x " << nrows << " = " << nrows*nrows << " matrix " << endl;
    }

    nnlsvector* b = new nnlsvector(nrows);
    nnlsvector* x = new nnlsvector(nrows);
    nnlsmatrix* A = new denseMatrix(nrows, nrows); //modify this matrix A
    BuildTemplateMatrix(A, tempwave, nrows); //make the template matrix
    int ch = itr->first;
    if(nnlsVerbosityLevel>0) cout<<"size "<<tempwave.GetSamples()->size()<<" size "<<temptimes.size()<<endl;
    soln[ch].SetTemplate(tempwave,temptimes);   //save the template waveform for each channel in this map
    //looping in all vectors in this waveform, currently only one signal, 2022.5.23
    // if there are more vectors, looping should start early.
    vector<Waveform<double>> Vwavs = itr->second;
    vector<Waveform<double>> plotWaveVector;
    unsigned long channelNo = ch;
    for (int i = 0; i<Vwavs.size(); i++){
      signalwave = Vwavs.at(i);
      BuildWaveformVector(b,signalwave,sampletimes,newtimestep);
      nnls* solver = new nnls(A, b, maxiter); //here get the solution
      flag = solver->optimize();
      if(flag<0){
        cout << "NNLS solver terminated with an error flag" << endl;
      }
      x = solver->getSolution();
      SaveNNLSOutput(&soln[ch],A, x, newsignaltimes);
      Waveform<double> plotWave;
      bool Zero = false;
      //cout<<"channel "<<ch;
      for (int j=0; j<soln[ch].GetNumberOfComponents(); j++){
        plotWave.PushSample(-soln[ch].GetComponentScale(j));
        //cout<<-soln[ch].GetComponentScale(j)<<", ";
        if(-soln[ch].GetComponentScale(j)<-10) Zero = true;
      }//cout<<endl;
      //special zero if there is only a huge negative peak

      if(Zero){
        Waveform<double> zeroWave;
        for (int j=0; j<soln[ch].GetNumberOfComponents(); j++){zeroWave.PushSample(0);}
        plotWaveVector.push_back(zeroWave);
      }else{
        plotWaveVector.push_back(plotWave);
      }
      delete solver;
    }


    FittedPulseNumber.insert(pair<unsigned long, vector<Waveform<double>>>(channelNo, plotWaveVector));

    delete A;
    delete b;
    delete x;




  }//end looping all channels

}


if (nnlsPrintOption ==1 ){
  cout<<"print nnls result here:"<<endl;
  for (itr = FittedPulseNumber.begin(); itr != FittedPulseNumber.end(); ++itr){
    int ch = itr->first;
    vector<Waveform<double>> Vwavs = itr->second;
      Waveform<double> signalwave = Vwavs.at(0);
      cout<<ch<<", ";
      for (int i =0; i<signalwave.GetSamples()->size();i++){
        if(i==signalwave.GetSamples()->size()-1){
          cout<<signalwave.GetSample(i)<<endl;
        }else{
          cout<<signalwave.GetSample(i)<<", ";
        } }}}



  m_data->Stores["ANNIEEvent"]->Set("nnls_solution",soln);
  m_data->Stores["ANNIEEvent"]->Set(OutputWavLabel, FittedPulseNumber);


  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);
  cout << "time "<<duration.count() << endl;

  return true;
} //end execute





//BuileWaveFormVector and SaveNNLSOutput are directly copied from WaveformNNLS tool.


void LAPPDnnlsPeak::BuildTemplateMatrix(nnlsmatrix* A, Waveform<double> tempwave, size_t nrows)
{

	int temp_size = tempwave.GetSamples()->size();
  int tsh = (int) temp_size/2;

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
			if(col < row-tsh)
			{
				A->set(row, col, 0.0);
			}
			//on the upper diagonal, the template
			//waveform is written to a row, starting
			//with the column = row
			else if((col - row) < tsh )
			{
				A->set(row, col, tempwave.GetSample(col - row + tsh));
			}
			//otherwise (outside bounds of template)
			//set to 0.
			else {A->set(row, col, 0.0);}
		}
	}


}

//formats the waveform into the vector format expected by nnls algo.
//see comment above BuildTemplateMatrix for explanation of nrows
void LAPPDnnlsPeak::BuildWaveformVector(nnlsvector* b, Waveform<double> wave, vector<float> times, double template_timestep)
{

	//make a new signal vector that is
	//NOT interpolated, but has more samples
	// so that the timesteps of the template
	// and signal waveform are equal.
	vector<double> newwave;
	float t0;
	float t1;
	double current_time;
	for(int j = 0; j < b->length(); j++)
	{
		//current time iterating through
		//times scaled for nnls matrix
		current_time = times.front() + j*template_timestep;

		//find value of waveform at this time by
		//finding closest sample times. Assumes the
		//times vector is ordered
		for(int i = 0; i < times.size() - 1; i++)
		{
			t0 = times.at(i);
			t1 = times.at(i+1);
			if(current_time >= t0 and current_time < t1)
			{
				b->set(j, wave.GetSample(i));
				break;
			}
		}
	}

}


//want to save the nnls output such that you have access to the
//full comprehensive fit (A*x = bsolution) as well as the
//individual template components contained in the vector x.
//Each element of x represents a template waveform scaled by
//the magnitude of the element and placed at a time "t"
//(read off of the index of the element)
void LAPPDnnlsPeak::SaveNNLSOutput(NnlsSolution* soln, nnlsmatrix* A, nnlsvector* x, vector<double> signaltimes)
{

	//first, the fully composed (full fit) solution
	nnlsvector* bsolv = new nnlsvector(x->length());
	A->dot(false, x, bsolv); //now bsolv is the fitted vector waveform
	//turn vector into waveform
	Waveform<double> ff; //waveform version of nnls full (summed) solution
	for(int i = 0; i < bsolv->length(); i++)
	{
		ff.PushSample(bsolv->get(i));

		//now, the decomposed pulses, one for every
		//element of x that is non-zero.

		//(1e-5 is for possible floating point error)
		//ignore scales that are close to 0
		if(x->get(i) <= 1e-5)
		{
      soln->AddComponent(signaltimes.at(i), x->get(i));
      //continue;
		}
		//store the scale and time for each component
		//of the nnls-solved waveform
		else
		{
			soln->AddComponent(signaltimes.at(i), x->get(i));
		}

	}
	soln->SetFullSoln(ff);
}

bool LAPPDnnlsPeak::Finalise(){

  return true;
}

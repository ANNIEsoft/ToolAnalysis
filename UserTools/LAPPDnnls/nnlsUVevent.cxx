//----------APS Analysis
//----------Author's Name:Jingbo WANG
//----------Copyright:Those valid for ANL
//----------Modified:22/07/2014
#include "RVersion.h"
#include "TRandom.h"
#include "TDirectory.h"
#include "TProcessID.h"
#include "TMath.h"
#include "nnlsUVevent.h" 

#include <ctime>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <cstdarg>

#include "nnls.h"
#include "TH1D.h"
#include "TFile.h"
#include "TCanvas.h"

#include <iostream>


//using namespace nsNNLS;

nsNNLS::matrix* load (const char* fname, char type);
nsNNLS::vector* load_vector(const char* fil, size_t sz, char dos);
int     write_vector(const char* fil, nsNNLS::vector* v);
int     write_binvector(const char* fil, nsNNLS::vector* v);



// ------------------------------------------------------ 
// nnlsUVevent 
// ------------------------------------------------------
nnlsUVevent::nnlsUVevent() 
{ 
  // Create an nnlsUVevent object.
   // When the constructor is invoked for the first time, the class static
   // variable fgTracks is 0 and the TClonesArray fgTracks is created.
	fnwav = 0;
	WavDimSize = 256;
}

nnlsUVevent::~nnlsUVevent() {
	Clear();
}  

void nnlsUVevent::Clear(Option_t * /*option*/)
{
   fnwav = 0;
   t.clear();
   fz.clear();
//	for(int i=0;i<60;i++) {wav[i].Clear();}  

//   delete fwav;
   
}

//main constructor that takes in a digitized signal
nnlsWaveform::nnlsWaveform()
{

    hwav = 0;
	hwav_raw = 0;
	xvector = 0;
	nnlsoutput = 0;
	hbg =0;
    hcfd = 0;
	hpedhist = 0;
	DimSize = 256;
	baseline = 0;
	CutoffFrequency = 5e8;
	Fraction_CFD = 0.5;
	Delay_CFD = 100;
	DisWindow_min = 0;
	DisWindow_max = DimSize*Deltat;
	FitWindow_min = 0;
	FitWindow_max = DimSize*Deltat;
	GaussRange_min = 0;
	GaussRange_max = DimSize*Deltat;

	npeaks = 0;
	MinimumTot = 100;
	DoFFT = 0;
	IfDynamicWindow = 0;
	
	for(int i=0; i<10; i++){
	  vtime[i]=0;
	  vamp[i]=0;
	  vcharge[i]=0;
	  vFWPM[i]=0;
	  vchi2[i]=0;
	  vLowB[i]=0;
	  vHiB[i]=0;
	}
}



nnlsWaveform::~nnlsWaveform() {
	Clear();
}

void nnlsWaveform::Clear(Option_t * /*option*/)
{
   // Note that we intend on using TClonesArray::ConstructedAt, so we do not
   // need to delete any of the arrays.
	delete hwav; hwav = 0;
	delete hbg; hbg = 0;
	delete hwav_raw; hwav_raw = 0;
	delete xvector; xvector = 0;
	delete nnlsoutput; nnlsoutput = 0;
	delete hcfd; hcfd = 0;
	delete hpedhist; hpedhist = 0;
	bg.clear();
	vol.clear();
	vol_raw.clear();
	vol_fft.clear();
	re_fft.clear();
	im_fft.clear();
}

//loads a list of nnlsWaveform samples
// into the nnlsWaveform arrays
void nnlsWaveform::SetWave(float* fvol)
{


	//Set values for the nnlsWaveform members
  steps = 5; //how finely the template is binned
  maxiter = 10; //the maximum number of iterations

  sprintf(WavName, "wav_ch%d_%d", chno, evtno);
  sprintf(WavNameRaw, "wavraw_ch%d_%d", chno, evtno);
  sprintf(xvectorname, "xvec_ch%d_%d", chno, evtno);
  sprintf(nnlsoutputname, "nnlso_ch%d_%d", chno, evtno);

  hwav = new TH1D(WavName, WavName, DimSize, 0, DimSize*Deltat);
  hwav_raw = new TH1D(WavNameRaw,WavNameRaw, DimSize, 0, DimSize*Deltat);
  xvector = new TH1D(xvectorname, xvectorname, (DimSize+60)*steps, 0, (DimSize+60)*steps);
  nnlsoutput = new TH1D(nnlsoutputname,nnlsoutputname, DimSize*steps, 0, DimSize*Deltat);
  
  for(int i=0;i<DimSize;i++) {
    vol_raw.push_back(*(fvol+i));
    vol.push_back(*(fvol+i));
    hwav_raw->SetBinContent(i+1, vol_raw[i]);
    hwav->SetBinContent(i+1,vol[i]);
    //hwav_raw->SetBinError(i+1,0.3); if this is needed, set to electronics noise value
  }

  AmpThreshold = 60;
}


void nnlsWaveform::Setup_nnls() {

  //creates matrixA array and xvector histogram

  m = (DimSize+60)*steps;
  n = (DimSize+60)*steps;
  double* matrixA2 = new double[m*n];
  
  //fill matrixA (array version of A)
  TFile* tt = new TFile("pulsecharacteristics.root");
  TH1D* tempp = (TH1D*)tt->Get("templatepulse;1");
  int bincount = 1;
  for(int i=0; i<m; i++)
    {
      bincount = 1;
      for(int j=0; j<m; j++)
	{
	  if(bincount>0 && bincount<=30*steps && j>=i && j<=(i+30*steps))
	    {
	      if(tempp->GetBinContent((100/steps)*(bincount))<0) 
		{
		  matrixA2[m*i+j] = tempp->GetBinContent((100/steps)*(bincount));
		}
	      else 
		{
		  matrixA2[m*i+j] = 0;
		}
	      bincount++;
	    }
	  else
	    {
	      matrixA2[m*i+j] = 0;
	    }
	}
    }

  nsNNLS::denseMatrix* matA = new nsNNLS::denseMatrix(m,n,matrixA2);

  double* vv = new double[m];

  //fill vv (array version of b)
  for(int i=0; i<m; i++)
    {
      if(i<30*steps || i>(m-30*steps)) 
	{
	  vv[i]=0;
	}
      else
	{
	  vv[i]=hwav_raw->GetBinContent((i-30*steps)/steps);
	}
    }
  nsNNLS::vector* vecB = new nsNNLS::vector(m,vv);
  nsNNLS::vector* vecX;

  nsNNLS::nnls* solver = new nsNNLS::nnls(matA,vecB,maxiter);
  solver->optimize();

  vecX = solver->getSolution();

  //fill xvector histogram using vecX (vector version of x)
  for(int i=0; i<m; i++)
    {
      if(i+1-28*steps>0 && i>30*steps && i<(m-30*steps)) {xvector->SetBinContent(i+1-28*steps,vecX->getData()[i]);}
    }

  //create output based on templates and xvector
  double outputa[m];
  for(int i=0; i<m; i++) outputa[i]=0;

  for(int i=0; i<matA->nrows(); i++)
    {
      for(int j=0; j<matA->ncols(); j++)
	{
	  outputa[j]+=(xvector->GetBinContent(i+1))*(matA->get(i,j));
	}
    }

  for(int i=0; i<matA->ncols(); i++)
    {
      if(i>30*steps && i<(m-30*steps)) nnlsoutput->SetBinContent(i+1-30*steps,outputa[i]);
    }

  delete tt;
  delete solver;
  delete vecX;
  delete vecB;
  delete matA;
  delete[] vv;
  tempp = 0;
  delete[] matrixA2;

  cout<<"made it"<<endl;
}



int nnlsWaveform::Calculate_Peaks_nnls()
{
  nnpeaks = 0;

  HighBound.clear();
  LowBound.clear();

  double threshold = 2; //based on best guess after looking at xvector
  double vol = 0;
  int nbin = (int)m; //bins in xvector
  int length = 0;
  int MinimumTotBin = 10; //minimum length of peak to characterize a peak (again based on guess)
  for(int i=30*steps;i<nbin-30*steps;i++) {
    vol = TMath::Abs(xvector->GetBinContent(i+1));
    if(vol>threshold) {length++;}
    else {
      if(length<MinimumTotBin) {length=0;}
      else {
	nnpeaks++;
	TH1D *NEWhwav = (TH1D*)xvector->Clone();
	NEWhwav->GetXaxis()->SetRange((i+1-length),(i+1));
	Int_t MaxBin = NEWhwav->GetMaximumBin()-30*steps; //subtracting the extra 30 buffer bins (we started the loop at 30*steps)
	MaxBin = MaxBin+12*steps; //difference in bins between the beginning of the template and the peak on the template
	cout<<"peaktime: "<<MaxBin*Deltat/steps<<endl;
	LowBound.push_back(i+1-length-30*steps+10*steps);
	HighBound.push_back(i+1-17*steps);

	if(nnpeaks<=10) vtime[nnpeaks-1]=MaxBin*Deltat/steps;
	//old way of setting boundaries
	/*
	if (MaxBin+8*steps<nbin)
	  {
	    HighBound.push_back(MaxBin+8*steps);
	    vHiB[nnpeaks]=MaxBin+8;
	  }
	else
	  {
	    HighBound.push_back(nbin);
	    vHiB[nnpeaks]=nbin;
	  }
	if (MaxBin-8*steps>0)
	  {
	    LowBound.push_back(MaxBin-8*steps);
	    vLowB[nnpeaks]=MaxBin-8*steps;
	  }
	else
	  {
	    LowBound.push_back(0);
	    vLowB[nnpeaks]=0;
	  }
	*/
	delete NEWhwav;
	length = 0;
      }	
    }
  }

  for(int i=0; i<HighBound.size(); i++)
    {
      cout<<"Bounds: "<<LowBound[i]<<" "<<HighBound[i]<<endl;
    }

  return nnpeaks;
}

void nnlsWaveform::Calculate_Variables_nnls(int Npulses) {
  
  int pulses;
  if(Npulses<=10) 
    {
      pulses=Npulses;
    }
  else
    {
      pulses=10;
    }

  //BEGIN~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  TH1D *REBINNEDhwav = new TH1D("rebinned","rebinned",DimSize*steps,0,DimSize*Deltat);
  for(int i=0; i<DimSize*steps; i++)
    {
      REBINNEDhwav->SetBinContent(i+1,hwav_raw->GetBinContent((i+1)/(steps-0.01))); //replace with formula that works with other step numbers besides 2
    }

  for(int p=0; p<pulses; p++)
    {
      //CALCULATING CHI2~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      vchi2[p] = 0;
      if(trigno>=0)
	{
	  for(int i=0; i<m-60*steps; i++)
	    {
	      if(LowBound[p]<i && HighBound[p]>i)
		{
		  if(REBINNEDhwav->GetBinContent(i+1)!=0) 
		    {
		      vchi2[p]+=TMath::Abs(((nnlsoutput->GetBinContent(i+1)-(REBINNEDhwav->GetBinContent(i+1)))*(nnlsoutput->GetBinContent(i+1)-(REBINNEDhwav->GetBinContent(i+1)))/(REBINNEDhwav->GetBinContent(i+1))));
		    }
		}
	    }
	  cout<<"chi2 for pulse "<<p+1<<": "<<vchi2[p]<<endl;
	}
      
      
      //CALCULATING TIMING~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //done in calculate_peaks_nnls

      //CALCULATING CHARGE~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      vcharge[p]=0;
      if(trigno>=0)
	{
	  vcharge[p]=TMath::Abs(nnlsoutput->Integral(LowBound[p],HighBound[p]));
	  cout<<"Charge for pulse "<<p+1<<": "<<vcharge[p]<<endl;
	}

      //CALCULATING AMP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      vamp[p]=0;
      if(trigno>=0)
	{
	  double min = 0;
	  for(int i=LowBound[p]; i<HighBound[p]; i++)
	    {
	      if(nnlsoutput->GetBinContent(i)<min) min=nnlsoutput->GetBinContent(i);
	    }
	  vamp[p]=TMath::Abs(min);
	  cout<<vamp[p]<<endl;
	}
      
      //CALCULATING FWPM~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      if(trigno>=0)
	{
	  double frac = 0.2; //choose what fraction you want FWPM to be at

	  int MinBin = steps*vtime[p]/Deltat;
	  bool lside=false;
	  bool rside=false;
	  float fh = frac*vamp[p];
	  
	  int i=0;
	  int j=0;
	  
	  while(!lside)
	    {
	      i++;
	      if( fabs(nnlsoutput->GetBinContent(MinBin-i)) < fh ) lside=true;
	    }
	  
	  while(!rside)
	    { 
	      j++;
	      if( fabs(nnlsoutput->GetBinContent(MinBin+j)) < fh ) rside=true;
	    }       
	  
	  float slopeL = (nnlsoutput->GetBinContent(MinBin-i+1)-nnlsoutput->GetBinContent(MinBin-i))/((MinBin-i+1)*Deltat/steps-(MinBin-i)*Deltat/steps);
	  float slopeR = (nnlsoutput->GetBinContent(MinBin+j)-nnlsoutput->GetBinContent(MinBin+j-1))/((MinBin+j)*Deltat/steps-(MinBin+j-1)*Deltat/steps);
	  
	  float interceptL = nnlsoutput->GetBinContent(MinBin-i)-slopeL*(MinBin-i)*Deltat/steps;
	  float interceptR = nnlsoutput->GetBinContent(MinBin+j)-slopeR*(MinBin+j)*Deltat/steps;
	  
	  float timeL = (fh-interceptL)/slopeL;
	  float timeR = (fh-interceptR)/slopeR;
	  
	  float thewidth = timeR-timeL;     
	  
	  vFWPM[p]=thewidth;
	  cout<<"fwpm for pulse "<<p+1<<": "<<vFWPM[p]<<endl;
	  
	}

      
      
    }
  delete REBINNEDhwav;
}

void nnlsWaveform::Analyze() {
	DisWindow_min = 0;
	DisWindow_max = DimSize*Deltat;
	hwav->GetXaxis()->SetRange(DisWindow_min/Deltat,DisWindow_max/Deltat);
	

    cout<<"Running NNLS algorithm on nnlsWaveform"<<endl;
	Setup_nnls();
	int pulses = Calculate_Peaks_nnls();
	Calculate_Variables_nnls(pulses);



}




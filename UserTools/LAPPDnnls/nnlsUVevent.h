//----------APS Analysis 
//----------Author's Name:Jingbo WANG
//----------Copyright:Those valid for ANL
//----------Modified:22/07/2014
#ifndef NNLSUVevent_H 
#define NNLSUVevent_H

#include <TObject.h>
#include <TFile.h>
#include <TF1.h>
#include <TROOT.h>
#include <TH1F.h>
#include <vector>

using namespace std;

class nnlsUVevent: public TObject { 

private: 
	int				WavDimSize;
	
public: 

  nnlsUVevent(); 
  virtual ~nnlsUVevent(); 
  void          Clear(Option_t *option ="");

  int			evtno;
	int			fnwav;            //Number of nnlsWaveforms
	int			ClusterSize;			//Number of fired strips

 
	int			cutflag;		//
	vector<float>	t;
	vector<float>	fz;
	float			ttrans;			//
	float			tdiff;
	float			x;		//Parallel postion
	float			y;		//Transverse position

	int				Setevtno(int fevtno)			{ evtno = fevtno; return evtno; }
	int				SetWavDimSize(int fsize)		{WavDimSize = fsize; return WavDimSize;}
	int				SetCutflag(int flag)			{ cutflag = flag; return cutflag;}
	float			SetTransitTime(float ftrans)		{ ttrans = ftrans; return ttrans;}
	float			SetDifferentialTime(float ftdiff)		{ tdiff = ftdiff; return tdiff;}
	int				SetWavNumber(int fn)			{ fnwav = fn; return fnwav;}
	int				Getevtno()						const { return evtno; }
	int				GetWavNumber()					const { return fnwav;}
  

}; 

class nnlsWaveform: public TObject {
private:

public: 
		nnlsWaveform();
		~nnlsWaveform();
		void 		SetWave(float* fvol);
		void        Clear(Option_t *option="");
		int			Setevtno(int fevtno)		{evtno = fevtno; return evtno;}
		int			Setchno(int fchno)				{chno = fchno; return fchno;}
		int			SetDimSize(int fDimSize)		{DimSize = fDimSize; return DimSize;}
		float		SetTimeStamp(float ft)		{TimeStamp = ft; return TimeStamp;}
		double		SetAmpThreshold(double fth) {AmpThreshold = fth; return AmpThreshold;}
		double		SetFraction_CFD(double fraction) {Fraction_CFD = fraction; return Fraction_CFD;}
		int			SetDelay_CFD(double delay) {Delay_CFD = delay; return Delay_CFD;}
		int			EnableFFT()						{DoFFT = 1; return DoFFT;}
		int			EnableDynamicWindow()			{IfDynamicWindow = 1; return IfDynamicWindow;}
		float		SetCutoffFrequency(float f) {CutoffFrequency = f; return CutoffFrequency;};
		int			SetMinimumTot(int fdt)		{MinimumTot = fdt; return MinimumTot;}
		float		SetTotThreshold(float thr)		{TotThreshold = thr; return TotThreshold;}
		
		void        Setup_nnls();
		int			Calculate_Peaks_nnls();
		void        Calculate_Variables_nnls(int Npulses);
		void		Analyze();
		int			Getevtno()				const { return evtno; }
		float		GetTimeStamp()			const { return TimeStamp;}
		int			GetDimSize()			const { return DimSize; }
		TH1D        *GetWavHist()			const { return hwav; }
		int			DoFFT;
		float		Deltat;			//time per sample
		int			DimSize;		//Number of samples
		float		baseline;		//Base line of this nnlsWaveform
	    float           goodbl;
		//float           istrig;
		float           bnoise;                 //noise on the baseline
		float		qfast;			//fast charge
		float		qtot;			//total charge
		float		gain;
		float		amp;		//amp of the pulse
		float           FWHM;    //full width at half max
		float           FW20pM;   //full width at 20 percent max
		double		time;	//leading edge
        double      risingtime; 
        int			MinimumTot;
	float       TotThreshold;
		int			npeaks;
		int			evtno;		//nnlsUVevent number
		int			chno;
		int			WavID;		//each nnlsWaveform must have an ID, ID = 100*evtno + Channel_Number
		double        gmean;
		double        gsigma;
		double        gpeak;
		double        gtime;
		double        grisetime;
		double        gchi2; 
		double        gdegfree;
		double        goffset;     //Gaussian parameters

		double        ggmean;
		double        ggsigma;
		double        ggpeak;
		double        ggmean2;
		double        ggsigma2;
		double        ggpeak2;
		double        ggtime;
		double        ggtime2;
		double        ggrisetime;
		double        ggchi2;
		double        ggdegfree;
		double        ggoffset;     //Double-Gaussian parameters

		double		ttime;
		double		tscale;
		double          tamp;
		double          tcharge;
		double		tchi2;
		double          tdegfree;
		double          tNS;       //Template Fit Parameters

		//NNLS parameters
		int         nnpeaks;
		double      vtime[10];       // array of the times for each pulse
		double      vamp[10];        // array of the amplitudes for each pulse
		double      vcharge[10];     // array of charges for each pulse
		double      vFWPM[10];       // array of FW20pMs for each pulse
		double      vchi2[10];
		double      vLowB[10];
		double      vHiB[10];

		char		WavName[100];
		char		WavNameRaw[100];
		char            xvectorname[100];
		char            nnlsoutputname[100];
		char		BgName[100];
		char		PEDName[100];
		float		TimeStamp;	//Time stamp of this nnlsWaveform
		
		vector<float>	vol;
		vector<float>   vol_raw;
		vector<float>   bg;
		vector<float>	vol_fft;
		vector<float>	re_fft;
		vector<float>	im_fft;

//		float		vol[256];		//Y value of the samples
//		float		vol_fft[256];	//reconstruced Y value
//		float		re_fft[256];
//		float		im_fft[256];
		TH1D*		hwav;			//nnlsWaveform plot
		TH1D*           hwav_raw;
		TH1D*           xvector;                //nnls output vector (weights)
		TH1D*           nnlsoutput;
		int             steps;
		int             maxiter;
		int          m;
		int          n;
		int             trigno;
		TH1D*           hbg;
		TH1D*		hcfd;			//histogram for CFD
		TH1D*           hpedhist;
		int			IfDynamicWindow;
		double		FitWindow_min;	//Xmin for SplineFit
		double		FitWindow_max;	//Xmax for SplineFit
		double		DisWindow_min;
		double		DisWindow_max;
		double		GaussRange_min;	//Xmin for DoubleGaussFit
		double		GaussRange_max;	//Xmax for DoubleGaussFit

		float		CutoffFrequency;
		double		AmpThreshold;
		double		Fraction_CFD;	//fraction of Attenuation 
		int			Delay_CFD;		//
		

		  float slopeL;
		float slopeR;
		float interceptL;
		float interceptR;
		float timeL;
		float timeR;
		
		vector<int> HighBound;
		vector<int> LowBound;
		
};


#endif  

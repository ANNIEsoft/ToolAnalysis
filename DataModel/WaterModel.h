#ifndef WATERMODEL_H
#define WATERMODEL_H

#include <cmath>
#include "TMath.h"
#include "TH1.h"
#include "TGraph.h"

class TH1D;
class TH1F;

class WaterModel {

  public:
    static WaterModel* Instance();
    //static WaterModel* Instance(Int_t op1, Int_t op2, Int_t op3); //op1 is absorption length, op2 is qe, op3 is index; And value=0 is default table, 1 is input table, 2 is parametera
    void Reset();
    
    struct waterM{
	Double_t velocity;
	Double_t finalSpectrum;
    };

    Double_t evalGraphs(Double_t flmda,char paramType);
    Double_t N_Index(Double_t flmda);
    //Double_t N_Index_d(Double_t dist);
    //Double_t N_Index(Double_t flmda, Double_t temp);
    Double_t Vg(Double_t flmda);
    Double_t TimeMu(Double_t mudist);
    Double_t InitSpect(Double_t flmda);
    Double_t AbsLength(Double_t flmda);
    Double_t ChereAngle(Double_t mudist);
    //void SetOPabsl(Int_t op){fOP = op;}
    void SetOPabsl(Int_t op){ fopa = op;}
    void SetOPphotv(Int_t op){ fopv = op;}
    void SetOPindex(Int_t op){ fopn = op;}
    void SetAbsTable(TGraph* mytg);
    Double_t Atten(Double_t flmda, Double_t fdist);
    //void SetOPqe(Int_t op){fOP = op;}
    void SetQETable(TGraph* mytg);
    Double_t QE(Double_t flmda);
    Double_t FinlSpect(Double_t flmda, Double_t fdist);
    //Double_t NormInit();
    
    waterM getParamsWM(Double_t v, Double_t initi, Double_t absl, Double_t sctl, Double_t qe, Double_t fdist);

    TH1D *FinlTimeSpect(TH1D *dhist);

  private:
    //WaterModel(Int_t op1, Int_t op2, Int_t op3);
    WaterModel();
    ~WaterModel();

    Double_t fdist;

    Double_t ftest;
    Double_t findex;
    Double_t fvelo;
    Double_t finitintensity;
    Double_t fabslength;
    Double_t fatten;
    Double_t fqe;
    Double_t ffinlintensity;
    TH1D *ffinltime;
    TGraph* fIndexLg;
    TGraph* fAbsLg;
    TGraph* fSctLg;
    TGraph* fQEg;
    TGraph* fVeloLg;
    TGraph* fInitLg;
    Int_t fOP, fopa, fopv, fopn;

    Double_t Temp; //Celsius Degree
    Double_t r5; //kg*m^-3
    Double_t r1; //Celsius Degree
    Double_t r2;
    Double_t r3;
    Double_t r4;
    Double_t a0;
    Double_t a1;
    Double_t a2;
    Double_t a3;
    Double_t a4;
    Double_t a5;
    Double_t a6;
    Double_t a7;
    Double_t lambdauv;
    Double_t lambdair;  
    Double_t C; // cm/ns
    Double_t N;
    Double_t dndl;
    Int_t numD;
    Double_t minDist;
    Double_t width;
    Double_t Dist;
    Double_t numPh;
    Double_t TimePh;
    Int_t TimeBin;
    Double_t Intensity;
    Double_t rho;
    Double_t A0;
    Double_t A1;
    Double_t A2;
    Double_t A3;
    Double_t A4;
    Double_t A5;
    Double_t A6;
    Double_t A7;
    Double_t A;

};

#endif

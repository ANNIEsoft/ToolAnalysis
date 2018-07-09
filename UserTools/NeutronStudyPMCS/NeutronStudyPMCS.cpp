#include "NeutronStudyPMCS.h"

NeutronStudyPMCS::NeutronStudyPMCS():Tool(){}


bool NeutronStudyPMCS::Initialise(std::string configfile, DataModel &data){

  /////////////////// Usefull header ///////////////////////
  if(configfile!="")  m_variables.Initialise(configfile); //loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // i should change the random seed here
  ttr = new TRandom3();
  trr = new TRandom3();

  // currently hard coded energy smearing
  muEsmear = 100.; //in MeV
  muAngsmear = 0.087; // in radians

  return true;
}


bool NeutronStudyPMCS::Execute(){

  // input variables
  int primneut,totneut,ispi;
  double nuE,muE,muAngle,mupx,mupy,mupz,piE,piAngle,q2,recoE,recoE_nosmear;

  m_data->Stores["NeutrinoEvent"]->Get("Nprimaryneutrons",primneut);
  m_data->Stores["NeutrinoEvent"]->Get("Ntotalneutrons",totneut);
  m_data->Stores["NeutrinoEvent"]->Get("isPion",ispi);
  m_data->Stores["NeutrinoEvent"]->Get("trueNeutrinoEnergy",nuE);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonEnergy",muE);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonAngle",muAngle);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonPx",mupx);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonPy",mupy);
  m_data->Stores["NeutrinoEvent"]->Get("trueMuonPz",mupz);
  m_data->Stores["NeutrinoEvent"]->Get("truePionEnergy",piE);
  m_data->Stores["NeutrinoEvent"]->Get("truePionAngle",piAngle);
  m_data->Stores["NeutrinoEvent"]->Get("trueQ2",q2);
  m_data->Stores["NeutrinoEvent"]->Get("recoNeutrinoEnergy",recoE);

  // Output variables
  int isgoodmuon,isPismeared,Ntotneutsmeared,Nprimneutsmeared,Nbkgdneutrons,Nbkgdneutrons_high;
  int passedselection;
  double smearedMuE,smearedMuangle,myRecoE,myRecoE_unsmeared,unsmearedMuangle,muonefficiency;

  bool passevent=false;

  // smear the reconstructed muon energy
  smearedMuE = MuEsmear(muE,muEsmear); // smearing
  smearedMuangle = MuAnglesmear(mupx,mupy,mupz,muAngsmear); // 5 deg res
  //calculate unsmeared muon angle from px,py,pz
  unsmearedMuangle = MuAnglesmear(mupx,mupy,mupz,0);

  // kinematic reconstruction of the neutrino energy with smeared muon parameters
  double reconstructedE = RecoE(smearedMuE,smearedMuangle);
  // kinematic reconstruction of the neutrino energy with unsmeared muon parameters
  double reconstructedE_nosmear = RecoE(muE,unsmearedMuangle);
  myRecoE = (reconstructedE/1000.);
  myRecoE_unsmeared = (reconstructedE_nosmear/1000.);

  // criteria for an experimentally CCQE-like event

  // set initial values for parameters
  isgoodmuon=0;
  isPismeared=ispi;

  //efficiency for detecting events based on ANNIE acceptance cuts
  double mueffic = MuonEfficiency(muE,unsmearedMuangle);
  muonefficiency = mueffic;
  double mroll = ttr->Rndm();
  //did the muon pass the ANNIE acceptance cut
  if(mroll<mueffic) {isgoodmuon=1;}

  //if(i%10000==0) cout<<"nuE "<<nuE<<" mu effic "<<mueffic<<" mroll "<<mroll<<endl;

  // did this event have a pion and, if so did it sneak by?
  if(ispi){
    // calculate angle between pion and muon
    double openingangle=0;
    // determine the
    double inefficiency = PionInefficiency(piE,piAngle);
    // pull a random number from 0 to 1
    double froll = 1.0;
    // is the pion undetected
    if(froll<inefficiency){isPismeared=0;}
  } else{
    // no pion and a good muon passes
    isPismeared=0;
  }

  // CCQE-like event has a muon that passes cuts and no detectred pion
  passevent = ((isgoodmuon==1) && (isPismeared==0));
  if(passevent) passedselection=1;
  else passedselection=0;

  //Apply efficiencies to neutron detection, apply bkgd
  Ntotneutsmeared=DetectedNeutrons(totneut);
  Nprimneutsmeared=DetectedNeutrons(primneut);
  // nbackground neutrons
  Nbkgdneutrons = BkgNeutrons(0.2);
  Nbkgdneutrons_high = BkgNeutrons(0.5);

  passedselection=0;
  if(passevent) passedselection=1;

  // store the output variables

  m_data->Stores["NeutrinoEvent"]->Set("smearedMuonAngle",smearedMuangle);
  m_data->Stores["NeutrinoEvent"]->Set("smearedMuonEnergy",smearedMuE);
  m_data->Stores["NeutrinoEvent"]->Set("originalrecoNeutrinoEnergy",recoE);
  m_data->Stores["NeutrinoEvent"]->Set("myrecoNeutrinoEnergy",myRecoE);
  m_data->Stores["NeutrinoEvent"]->Set("myrecoNuEunsmeared",myRecoE_unsmeared);
  m_data->Stores["NeutrinoEvent"]->Set("isGoodMuon",isgoodmuon);
  m_data->Stores["NeutrinoEvent"]->Set("smearedIsPion",isPismeared);
  m_data->Stores["NeutrinoEvent"]->Set("smearedNtotalNeutrons",Ntotneutsmeared);
  m_data->Stores["NeutrinoEvent"]->Set("smearedNprimaryNeutrons",Nprimneutsmeared);
  m_data->Stores["NeutrinoEvent"]->Set("NbackgroundNeutrons",Nbkgdneutrons);
  m_data->Stores["NeutrinoEvent"]->Set("NbackgroundNeutrons_high",Nbkgdneutrons_high);
  m_data->Stores["NeutrinoEvent"]->Set("PassedSelectionCuts",passedselection);
  m_data->Stores["NeutrinoEvent"]->Set("MuonEfficiency",muonefficiency);

  return true;
}


bool NeutronStudyPMCS::Finalise(){

  return true;
}


double NeutronStudyPMCS::nuEEfficiency(double nuE)
{
  double theefficiency=exp( -((nuE-1.39)*(nuE-1.39))/(2*0.395*0.395) );

  return theefficiency;
}


double NeutronStudyPMCS::MuonEfficiency(double mu_E, double mu_angle)
{
  double theefficiency=1.0;

  double MeanE = 775.0;
  double SigE = 277.5;
  double MeanAng = 0.16;
  double SigAng = 0.513;

  theefficiency = exp(-((((mu_E-MeanE)*(mu_E-MeanE))/(2*SigE*SigE)) + (((mu_angle-MeanAng)*(mu_angle-MeanAng))/(2*SigAng*SigAng))));

  if(mu_E<200.) theefficiency=0.;

  return theefficiency;
}


double NeutronStudyPMCS::PionInefficiency(double pi_E, double pi_angle)
{
  double thefakerate=0.;

  return thefakerate;
}


double NeutronStudyPMCS::MuEsmear(double mu_E, double Eres)
{
  double thesmearedE;
  double sv = trr->Gaus(0.,Eres);
  thesmearedE=mu_E + sv;

  return thesmearedE;
}

double NeutronStudyPMCS::MuAnglesmear(double mu_px, double mu_py, double mu_pz, double angsmear)
{

  TVector3 muvect(mu_px,mu_py,mu_pz);
  TVector3 beamdir(1.,0.,0.);
  double mu_angle = muvect.Angle(beamdir);

  //cout<<mu_angle<<endl;
  double thesmearedAngle;
  double sv = trr->Gaus(0.,angsmear);

  thesmearedAngle=mu_angle + sv;

  //  cout<<thesmearedAngle<<" "<<mu_angle<<" "<<mu_px<<" "<<mu_py<<endl;

  return thesmearedAngle;
}

double NeutronStudyPMCS::RecoE(double mu_E,double mu_angle)
{
  double theRE=0;
  double Mneut = 939.57-30;
  //double Mneut = 939.57;
  double Mprot = 938.27;
  double Mmu = 105.7;
  double cosT = cos(mu_angle);
  double muE = mu_E+Mmu;

  //theRE = (Mneut-(Mmu*Mmu/2))/(Mneut-muE+(cosT*sqrt(muE*muE-Mmu*Mmu)));


  theRE = ((2*Mneut*muE) - (Mneut*Mneut + Mmu*Mmu - Mprot*Mprot))/(2*(Mneut-muE+(cosT*sqrt(muE*muE-Mmu*Mmu))));
  return theRE;
}

int NeutronStudyPMCS::DetectedNeutrons(int totneut)
{
  int detneut=0;
  double neutdeteffic = 0.7;
  for(int i=0; i<totneut; i++){
    double rolln = ttr->Rndm();
    if(rolln<neutdeteffic) detneut++;
  }

  //return detneut;
  return detneut;
}


int NeutronStudyPMCS::BkgNeutrons(double prob)
{
  int bgneut=0;
  double rollbn=ttr->Rndm();
  double neutbgrate=prob;
  if(rollbn<neutbgrate) bgneut=1;

  //bgneut=0;
  return bgneut;
}

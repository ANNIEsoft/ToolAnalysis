#include "LAPPDMakePeds.h"

LAPPDMakePeds::LAPPDMakePeds():Tool(){}


bool LAPPDMakePeds::Initialise(std::string configfile, DataModel &data){

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  m_data->Stores["ANNIEEvent"]->Header->Get("AnnieGeometry", _geom);

  TString IWL;
  //RawInputWavLabel;
  m_variables.Get("PedWavLabel",IWL);
  InputWavLabel = IWL;

  m_variables.Get("NChannels",NChannels);

  pedhists = new std::map<unsigned long, vector<TH1D>>;
  //pedrootfile = new TFile("PedHist.root","RECREATE");
  eventcount=0;

  return true;
}


bool LAPPDMakePeds::Execute(){
  bool firstevent = false;
  // get raw lappd data
  std::map<unsigned long,vector<Waveform<double>>> lappddata;

  if(eventcount%100==0) cout<<"In MakePeds Execute, Event: "<<eventcount<<endl;
  //m_data->Stores["ANNIEEvent"]->Get("RawLAPPDData",rawlappddata);
  m_data->Stores["ANNIEEvent"]->Get(InputWavLabel,lappddata);

  // loop over all channels

  std::map<unsigned long, vector<Waveform<double>>> :: iterator itr;
  for (itr = lappddata.begin(); itr != lappddata.end(); ++itr){
      unsigned long channelNo = itr->first;
      vector<Waveform<double>> Vwavs = itr->second;
      //cout<< "Vwavs size "<< Vwavs.size()<<endl;
      //cout<<"Looping, on channel: "<<(int)channelNo<<endl;

      Channel* mychannel = _geom->GetChannel(channelNo);
      //figure out the stripline number
      int stripno = mychannel->GetStripNum();
      //figure out the side of the stripline
      int stripside = mychannel->GetStripSide();

    //  cout<<"here"<<endl;
    if((pedhists->count(channelNo))==0)
    {
      firstevent=true;
      //cout<<"FIRST EVENT ON CHANNEL"<<endl;
    }


    //  cout<<"here 2"<<endl;

      for(int i=0; i<Vwavs.size(); i++)//number of waves? how many?
      {
        if(firstevent)
        {

        //  TH1D thepeds(hname,hname,1000,0,1000);
          std::vector<TH1D> histvect;
          //cout<<"new vector"<<endl;
      //  pedhists= new map<unsigned long, vector<TH1D>>(channelNo,thepeds);
          pedhists->insert (std::pair<unsigned long, vector<TH1D>>(channelNo,histvect));
        }
          Waveform<double> bwav = Vwavs.at(i);
          int nbins = bwav.GetSamples()->size();

          for(int j=0; j<nbins; j++)// number of samples 256?
          {
            double sampleval = bwav.GetSamples()->at(j);
            if(firstevent)
            {
              TString hname;
              hname+="pedch_";
              hname+=channelNo;
              hname+="_";
              hname+=i;
              hname+="_";
              hname+= j;
              TH1D thepeds(hname,hname,1000,0,3000);
            //  cout<<"new TH1D"<<endl;
              (pedhists->find(channelNo))->second.push_back(thepeds);
            }
            (pedhists->find(channelNo))->second.at(j).Fill(sampleval/0.3);
            //cout<<"sample added_"<<pedhists->find(channelNo)->second.at(j).GetMean()<< endl;
          }

        }
      }

  eventcount++;
  return true;
}


bool LAPPDMakePeds::Finalise(){

  int PlotPedChannel;
  TFile ptf("pedhists.root","RECREATE");
  m_variables.Get("PlotPedChannel",PlotPedChannel);

  string a, b, fileName1, fileName2;
  vector<string> datavalues1;
  vector<string> datavalues2;
  cout << "ENTER FILE 1 NAME";
  cin >> fileName1;
  cout << "ENTER FILE 2 NAME";
  cin >> fileName2;
  fileName1 = fileName1 + ".txt";
  fileName2 = fileName2 + ".txt";
  ofstream txtOut;
  txtOut.open (fileName1);
  bool firstchannel=true;

/*
  TH1D* means = new TH1D("means","means",60,-0.5,59.5);
  TH1D* rmss = new TH1D("rmss","rmss",60,-0.5,59.5);
  TH1D* mus = new TH1D("mus","mus",60,-0.5,59.5);
  TH1D* sigmas = new TH1D("sigmas","sigmas",60,-0.5,59.5);
*/

  TH1D** means = new TH1D*[pedhists->size()];
  TH1D** rmss = new TH1D*[pedhists->size()];
  TH1D** mus = new TH1D*[pedhists->size()];
  TH1D** sigmas = new TH1D*[pedhists->size()];

/*
  ("means","means",60,-0.5,59.5);
  TH1D** rmss = new TH1D("rmss","rmss",60,-0.5,59.5);
  TH1D** mus = new TH1D("mus","mus",60,-0.5,59.5);
  TH1D** sigmas = new TH1D("sigmas","sigmas",60,-0.5,59.5);
*/

  std::map<unsigned long, vector<TH1D>> :: iterator itr;
  int ccount=0;
  for (itr = pedhists->begin(); itr != pedhists->end(); ++itr)
  {
    unsigned long channelno = itr-> first;
    vector<TH1D> hists = itr->second;
    int nhists = hists.size();
    cout<<"CHANNEL NUMBER"<<channelno<<endl;

    TString hmeanname;
    hmeanname+="means_";
    hmeanname+=ccount;
    means[ccount] = new TH1D(hmeanname,hmeanname,256,-0.5,255.5);

    TString hrmsname;
    hrmsname+="rmss_";
    hrmsname+=ccount;
    rmss[ccount] = new TH1D(hrmsname,hrmsname,256,-0.5,255.5);

    TString hmuname;
    hmuname+="mus_";
    hmuname+=ccount;
    mus[ccount] = new TH1D(hmuname,hmuname,256,-0.5,255.5);

    TString hsigname;
    hsigname+="sigmas_";
    hsigname+=ccount;
    sigmas[ccount] = new TH1D(hsigname,hsigname,256,-0.5,255.5);

    if (channelno==30)
    {
      firstchannel=true;
    }
    for(int i=0; i<nhists; i++)
    {

      if(PlotPedChannel==(int)channelno){ptf.cd();   itr->second.at(i).Write();}
      TF1 *f1 = new TF1("f1","gaus",0,3000);
      double max = itr->second.at(i).GetMaximum();
      double mean =  itr->second.at(i).GetMean();
      double maxbin = itr->second.at(i).GetMaximumBin();
      double maxloc = itr->second.at(i).GetBinCenter(maxbin);
      double rms =  itr->second.at(i).GetRMS();
      if(rms>5.) rms = 3.;
      f1->SetParameters(max,maxloc,rms);
      
      std::vector<int> bins;
      for(int s=1; s <= hists.at(i).GetNbinsX() ; s++)
      {   
         double val = hists.at(i).GetBinContent(s);
         if(val > 0.0) bins.push_back(s);
      }   

      double low_lim = hists.at(i).GetXaxis()->GetBinCenter(bins[0]);
      double up_lim  = hists.at(i).GetXaxis()->GetBinCenter(bins[bins.size()-1]);
    
      bins.clear();
      bins.shrink_to_fit();

      hists.at(i).Fit("f1","Q","",low_lim, up_lim);
            
      double mu = f1->GetParameters()[1];
      double gaussigma = f1->GetParameters()[2];

      means[ccount]->SetBinContent(i,mean);
      rmss[ccount]->SetBinContent(i,rms);
      mus[ccount]->SetBinContent(i,mu);
      sigmas[ccount]->SetBinContent(i,gaussigma);


      if(fabs(mu-mean)>10) cout<<"Means are different! "<<channelno<<" "<<i<<" "<<mu<<" "<<mean<<endl;
      //cout<<"avg ped value= "<<(itr->second).at(i).GetMean()<<endl;
      //cout<<"avg ped value with gaussian"<<f1->GetParameters()[1]<<endl;
      //pedrootfile->cd();
      //hists.at(i).Write();
/*
      means->SetBinContent((int)channelno,justmean);
      rmss->SetBinContent((int)channelno,rms);
      mus->SetBinContent((int)channelno,gausmean);
      sigmas->SetBinContent((int)channelno,gaussigma);
*/

      string mutext = std::to_string(mu);
      if (channelno<30)
      {
        if (firstchannel)
        {
          string line;
          line =mutext;
          datavalues1.push_back(line);
        }
        else
        {
          string templine =datavalues1.at(i);
          templine = templine +" " + mutext;
          datavalues1.at(i)=templine;
        }
      }
      else
      {
        if (firstchannel)
        {
          string line;
          line =mutext;
          datavalues2.push_back(line);
        }
        else
        {
          string templine =datavalues2.at(i);
          templine = templine +" " + mutext;
          datavalues2.at(i)=templine;
        }
      }
    }
    firstchannel=false;
    ccount++;
  }
  cout<<datavalues1.size()<<endl;
  cout<<datavalues2.size()<<endl;
  for (int j=0; j<datavalues1.size(); j++)
  {
    txtOut << datavalues1.at(j)<<endl;
  }
  txtOut.close();
  txtOut.open(fileName2);
  for (int j=0; j<datavalues2.size(); j++)
  {
    txtOut << datavalues2.at(j)<<endl;
  }

  for (int j=0; j<ccount; j++)
  {
    means[j]->Write();
    rmss[j]->Write();
    mus[j]->Write();
    sigmas[j]->Write();
  }

/*
  means->Write();
  rmss->Write();
  mus->Write();
  sigmas->Write();
*/
//  pedhists->clear();
  ptf.Close();
  txtOut.close();
//  pedrootfile->Close();
  return true;
}

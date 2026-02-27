#ifndef _MCFLUXEVENTWEIGHT_H_
#define _MCFLUXEVENTWEIGHT_H_

#include <vector>
#include <string>

namespace evwgh {
  struct MCEventWeight
  {
    std::map<std::string, std::vector<double> > fWeight;
  };

/*  struct xsecconfig{
    std::string title = "";
    std::string type = "";
    int random_seed = 0;
    std::vector<std::string> parameter_list;
    std::vector<float> parameter_sigma;
    std::vector<float> parameter_min;
    std::vector<float> parameter_max;
    std::string mode = "";
    int number_of_multisims = 0;
  };
*/
  struct fluxconfig{
    std::string title = "";
    std::string type = "";
    std::string CentralValue_hist_file = "";                //FluxUnisim only
    std::string PositiveSystematicVariation_hist_file = ""; //FluxUnisim only
    std::string NegativeSystematicVariation_hist_file = ""; //FluxUnisim only
    std::string cv_hist_file = "";   //FluxHist only
    std::string rw_hist_file = "";   //FluxHist only
    std::vector<std::string> parameter_list;
    int parameter_sigma = 0;        //Primary Calcs only
    int scale_factor = 0;           //Primary Calcs only
    int random_seed = 0;
    double scale_factor_pos = 0.0;       //FluxUnisim only
    double scale_factor_neg = 0.0;       //FluxUnisim only
    int PrimaryHadronGeantCode = 0; //Primary Calcs only
    std::vector<int> PrimaryHadronGeantCode_SW; //SanfordWang Calc only
    std::string ExternalData = "";   //Primary Calcs only
    std::string ExternalFit = "";    //Primary Calcs only
    std::string weight_calculator = "";
    std::string mode = "";
    int number_of_multisims = 0;
    bool use_MiniBooNE_random_numbers;
  };

  struct event{
    int run;
    int entryno;
    int evtno;
    double tpx;
    double tpy;
    double tpz;
    double vx;
    double vy;
    double vz;
    int tptype;
    int ptype;
    int ntype;
    double nimpwt;
    double nenergyn;
    double nenergyf;
    double ndecay;
    double ppmedium;
    double pdpx;
    double pdpy;
    double pdpz;
    double ppdxdz;
    double ppdydz;
    double pppz;
  };
}
#endif //_MCFLUXEVENTWEIGHT_H_

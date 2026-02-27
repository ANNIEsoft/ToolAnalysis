#ifndef _MCXSECEVENTWEIGHT_H_
#define _MCXSECEVENTWEIGHT_H_

#include <vector>
#include <string>

namespace xsecevwgh {

  struct xsecconfig{
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

}
#endif //_MCXSECEVENTWEIGHT_H_

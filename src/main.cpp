#include <string>
#include "ToolChain.h"
#include "DummyTool.h"

int main(int argc, char* argv[]){

  std::string conffile;
  if (argc==1)conffile="configfiles/Dummy/ToolChainConfig";
  else conffile=argv[1];

  ToolChain tools(conffile);

  //DummyTool dummytool;

  //tools.Add("DummyTool",&dummytool,"configfiles/DummyToolConfig");

  //int portnum=24000;
  //  tools.Remote(portnum);
  //tools.Interactive();

  //  tools.Initialise();
  // tools.Execute();
  //tools.Finalise();


  return 0;

}

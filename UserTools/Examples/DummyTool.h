#ifndef DummyTool_H
#define DummyTool_H

#include <string>
#include <iostream>

#include "Tool.h"

/**
 * \class DummyTool
 *
 * This is a simple dummy Tool designed to show operation of a Tool. It also provides a default Tool for the Default ToolChain.
*
* $Author: B.Richards $
* $Date: 2019/05/28 10:44:00 $
* Contact: b.richards@qmul.ac.uk
*/
class DummyTool: public Tool {


 public:

  DummyTool(); ///< Constructor
  bool Initialise(std::string configfile,DataModel &data); ///< Assigns verbosity from config file and creates a log message.
  bool Execute(); ///< Creates a log message.
  bool Finalise(); ///< Does nothing


 private:

  int m_verbose;

};


#endif

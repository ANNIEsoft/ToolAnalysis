#ifndef FACTORY_H
#define FACTORY_H

#include <string>
#include "Tool.h"
#include "Unity.h"

/**
 * Global Factory function for creating Tools.
 @param tool Name of the Tool class to create.
*/

Tool* Factory(std::string tool);

#endif

#include "../Unity.cpp"

Tool* Factory(std::string tool){
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="Logger") ret=new Logger;

  if (tool=="ServiceAdd") ret=new ServiceAdd;
return ret;
}


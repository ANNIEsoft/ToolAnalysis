#ifndef PYTHON_API
#define PYTHON_API

#include "BoostStore.h"
#include <Python.h>

static BoostStore* gstore;

static PyObject* GetStoreInt(PyObject *self, PyObject *args){
  const char *command;
  if (!PyArg_ParseTuple(args, "s", &command)) return NULL;
  int ret=0;
  gstore->Get(command,ret);
  return Py_BuildValue("i", ret);

}

static PyObject* GetStoreDouble(PyObject *self, PyObject *args){  
  const char *command;
  if (!PyArg_ParseTuple(args, "s", &command)) return NULL;
  double ret=0;
  gstore->Get(command,ret);
  return Py_BuildValue("d", ret);

}

static PyObject* GetStoreString(PyObject *self, PyObject *args){
  const char *command;
  if (!PyArg_ParseTuple(args, "s", &command)) return NULL;
  std::string ret="";
  gstore->Get(command,ret);
  return Py_BuildValue("s", ret.c_str());

}

static PyObject* SetStoreInt(PyObject *self, PyObject *args){
  const char *command;
  int a;
  if (!PyArg_ParseTuple(args, "si", &command, &a)) return NULL;
  gstore->Set(command,a);
  return Py_BuildValue("i", a);

}

static PyObject* SetStoreDouble(PyObject *self, PyObject *args){
  const char *command;
  double b;
  if (!PyArg_ParseTuple(args, "sd", &command, &b)) return NULL;  
  gstore->Set(command,b);
  return Py_BuildValue("d", b);

}

static PyObject* SetStoreString(PyObject *self, PyObject *args){
  const char *command;
  const char *s;
  if (!PyArg_ParseTuple(args, "ss", &command, &s)) return NULL;
  std::string a(s);
  gstore->Set(command,a);
  return Py_BuildValue("s", s);

}


static PyMethodDef StoreMethods[] = {
  {"GetInt", GetStoreInt, METH_VARARGS,
   "Return the value of an int in the store"},
  {"GetDouble", GetStoreDouble, METH_VARARGS,
   "Return the value of an int in the store"},
  {"GetString", GetStoreString, METH_VARARGS,
   "Return the value of an int in the store"},
  {"SetInt", SetStoreInt, METH_VARARGS,
   "Return the value of an int in the store"},
  {"SetDouble", SetStoreDouble, METH_VARARGS,
   "Return the value of an int in the store"},
  {"SetString", SetStoreString, METH_VARARGS,
   "Return the value of an int in the store"},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef StoreModule = {
  PyModuleDef_HEAD_INIT,
  "Store",   /* name of module */
  "Module for accessing BoostStores from python tools", /* module documentation, may be NULL */
  -1,       /* size of per-interpreter state of the module,
	       or -1 if the module keeps state in global variables. */
    StoreMethods
};


#endif

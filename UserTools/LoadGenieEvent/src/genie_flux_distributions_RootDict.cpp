// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME srcdIgenie_flux_distributions_RootDict

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "RConfig.h"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// Since CINT ignores the std namespace, we need to do so in this file.
namespace std {} using namespace std;

// Header files passed as explicit arguments

// Header files passed via #pragma extra_include

namespace {
  void TriggerDictionaryInitialization_genie_flux_distributions_RootDict_Impl() {
    static const char* headers[] = {
0
    };
    static const char* includePaths[] = {
"/cvmfs/annie.opensciencegrid.org/products/root/v6_06_08/Linux64bit+2.6-2.12-e10-nu-debug/include",
"/grid/fermiapp/products/larsoft/genie/v2_12_0a/Linux64bit+2.6-2.12-e10-r6-debug/GENIE_R2120/../include/GENIE",
"/cvmfs/annie.opensciencegrid.org/products/root/v6_06_08/Linux64bit+2.6-2.12-e10-nu-debug/include",
"/annie/app/users/moflaher/ToolAnalysis/ToolAnalysis/UserTools/LoadGenieEvent/",
0
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "genie_flux_distributions_RootDict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_Autoloading_Map;
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "genie_flux_distributions_RootDict dictionary payload"

#ifndef G__VECTOR_HAS_CLASS_ITERATOR
  #define G__VECTOR_HAS_CLASS_ITERATOR 1
#endif

#define _BACKWARD_BACKWARD_WARNING_H

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[]={
nullptr};

    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("genie_flux_distributions_RootDict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_genie_flux_distributions_RootDict_Impl, {}, classesHeaders);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_genie_flux_distributions_RootDict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_genie_flux_distributions_RootDict() {
  TriggerDictionaryInitialization_genie_flux_distributions_RootDict_Impl();
}

ifndef CXXFLAGS
  CXXFLAGS = -std=c++1y -O3
endif

BoostLib= -L $(BOOST_LIB) -lboost_date_time -lboost_serialization -lboost_iostreams 
BoostInclude= -I $(BOOST_INC)

all: libRawViewer.so

# Skip lots of initialization if all we want is "make clean"
ifneq ($(MAKECMDGOALS),clean)
  # Use g++ as the default compiler
  CXX = g++
  CXXFLAGS += -Wall -Wextra -Wpedantic
  CXXFLAGS += -Werror -Wno-error=unused-parameter -Wcast-align
  
  # Add extra compiler flags for recognized compilers (currently just gcc
  # and clang)
  CXXVERSION = $(shell $(CXX) --version)
  COMPILER_VERSION := $(word 3, $(CXXVERSION))
  ifneq (,$(findstring clang,$(CXXVERSION)))
    # clang
    $(info Compiling using version $(COMPILER_VERSION) of clang)
  
    # The ROOT headers trigger clang's no-keyword-macro warning, so disable it.
    CXXFLAGS += -Wno-keyword-macro
  else
    ifneq (,$(or $(findstring GCC,$(CXXVERSION)), $(findstring g++,$(CXXVERSION))))
      # gcc
      $(info Compiling using version $(COMPILER_VERSION) of GCC)
      ifneq (,$(findstring $(COMPILER_VERSION), 4.9.))
        # g++ 4.9 gives many false positives for -Wshadow, so disable it
        # for now.
        CXXFLAGS += -Wno-shadow
      endif
      # Linking to ROOT libraries can be problematic on distributions (e.g.,
      # Ubuntu) that set the g++ flag -Wl,--as-needed by default (see
      # http://www.bnikolic.co.uk/blog/gnu-ld-as-needed.html for details), so
      # disable this behavior on Linux.
      ifneq ($(UNAME_S),Darwin)
        CXXFLAGS += -Wl,--no-as-needed
      endif
    endif
  endif
  
  ROOTCONFIG := $(shell command -v root-config 2> /dev/null)
  # prefer rootcling as the dictionary generator executable name, but use
  # rootcint if you can't find it
  ROOTCLING := $(shell command -v rootcling 2> /dev/null)
  ifndef ROOTCLING
    ROOTCLING := $(shell command -v rootcint 2> /dev/null)
  endif
  ROOT := $(shell command -v root 2> /dev/null)
  
  ifndef ROOTCONFIG
    $(error Could not find a valid ROOT installation.)
  else
    ROOT_VERSION := $(shell $(ROOTCONFIG) --version)
    $(info Found ROOT version $(ROOT_VERSION) in $(ROOT))
    ROOT_CXXFLAGS := $(shell $(ROOTCONFIG) --cflags)
    ROOT_LDFLAGS := $(shell $(ROOTCONFIG) --ldflags --glibs)
    ROOT_LIBDIR := $(shell $(ROOTCONFIG) --libdir)
    ROOT_LDFLAGS += -L$(ROOT_LIBDIR) -lCore -lRIO -lHist -lTree -lGraf
    ifeq ($(UNAME_S),Linux)
      ROOT_LDFLAGS += -rdynamic
    endif
  endif

endif

dictionary: viewer_linkdef.hh
	@echo "making $@"
	$(RM) dict.* dict*.pcm
	$(ROOTCLING) -f dict.cc -c RawViewer.h viewer_linkdef.hh

dict.o: dictionary dict.cc
	@echo "making $@"
	$(CXX) $(CXXFLAGS) -fPIC -o $@ -I../../include -L../lib $(ROOT_CXXFLAGS) \
	$(ROOT_LDFLAGS) -c dict.cc

%.o: %.cc
	@echo "making $@"
	$(CXX) $(ROOT_CXXFLAGS) $(CXXFLAGS) -fPIC -I../../include  $(BoostInclude) $(BoostLib) -o $@ -c $^

libRawViewer.so: dict.o RawViewer.o
	@echo "making $@"
	$(CXX) $(CXXFLAGS) -fPIC --shared -L../lib -I../../include \
	   $(ROOT_CXXFLAGS) $(ROOT_LDFLAGS) \
	  -Wl,-rpath -Wl,$(libdir):$(shell pwd) dict.o RawViewer.o \
	  -o $@

.INTERMEDIATE: dict.o RawViewer.o dict.cc

.PHONY: clean

clean:
	$(RM) viewer dict.* dict*.pcm *.o

# =============================== 
# Makefile for UVevent 
# =============================== 
# #
#  make clean and make install of this makefile imply that you define the system variable
#ROOTDEV. Exactly as ROOTSYS is a pointer towards the directory containing the include
#files, the libraries and the shared libraries of ROOT, ROOTDEV points towards a directory intended
#to contain the include files, the libraries and the shared libraries of all developments made
#above ROOT, like SplineFit, or the programs you may have developed yourself.
#  $(ROOTDEV) must contain at least 3 subdirectories: bin, lib and include.
#  Only by this way will you be able to write modular code, allowing one of your module
#to call entries of an other of your modules or entries of SplineFit.
#  If you have write access to $(ROOTSYS), you can choose ROOTDEV=ROOTSYS, but this mixing
#of your code with the code of ROOT is to my mind inelegant and the choice of a separate
#ROOTDEV is surely better.
# $(ROOTDEV)/bin  has to be added to PATH
# $(ROOTDEV)/lib  has to be added to LD_LIBRARY_PAT
#

ObjSuf        = o
SrcSuf        = cxx
ExeSuf        = 
DllSuf        = so
OutPutOpt     = -o # keep whitespace after "-o"

ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLIBS      = $(shell root-config --libs) 
ROOTGLIBS     = $(shell root-config --glibs)

# Linux with egcs
CXX           = g++
CXXFLAGS      = -g -Wall -fPIC
LD            = g++
LDFLAGS       = -O
SOFLAGS       = -shared

LIBUVHIT       = libUVevent

CXXFLAGS     += $(ROOTCFLAGS)
LIBS          = $(ROOTLIBS) $(SYSLIBS) 
GLIBS         = $(ROOTGLIBS) $(SYSLIBS) 

#------------------------------------------------------------------------------


HDRS          = UVevent.h vector.h matrix.h sparseMatrix.h denseMatrix.h nnls.h 
SRCS          = UVevent.$(SrcSuf) nnls.$(SrcSuf) denseMatrix.$(SrcSuf) sparseMatrix.$(SrcSuf) 
OBJS          = UVevent.$(ObjSuf) nnls.$(ObjSuf) sparseMatrix.$(ObjSuf) denseMatrix.$(ObjSuf) 

UVHITSO = $(LIBUVHIT).$(DllSuf)
PROGRAM = lappd$(ExeSuf)

all:        $(UVHITSO) $(PROGRAM)

$(UVHITSO): $(OBJS)
		@echo "Creating library $(UVHITSO) ..."
		@echo "root libs: $(ROOTLIBS)"
		$(LD) $(SOFLAGS) $(LDFLAGS) $(ROOTFLAGS) $(ROOTLIBS) $^ $(OutPutOpt) $@
		@echo "$(UVHITSO) done"

$(PROGRAM): lappd.$(ObjSuf) $(OBJS)
		@echo "Linking $(PROGRAM) ..."
		$(LD) $(LDFLAGS) lappd.$(ObjSuf) $(OBJS) $(LIBS) $(OutPutOpt) $(PROGRAM)
		@echo "$(PROGRAM) done"

clean:
		@rm -f $(OBJS) lappd.$(ObjSuf) 
		@rm -f $(PROGRAM) $(PROGRAMSO)
		@rm -f $(ROOTDEV)/bin/$(PROGRAM)
		@rm -f $(ROOTDEV)/include/UVevent.h
		@rm -f $(ROOTDEV)/include/vector.h #added
		@rm -f $(ROOTDEV)/include/matrix.h #added
		@rm -f $(ROOTDEV)/include/denseMatrix.h #added
		@rm -f $(ROOTDEV)/include/sparseMatrix.h #added
		@rm -f $(ROOTDEV)/include/nnls.h #added
		@rm -f UVevent.o libUVevent.so nnls.o sparseMatrix.o denseMatrix.o #added

install:
		@rm -f $(ROOTDEV)/include/UVevent.h	
		@rm -f $(ROOTDEV)/bin/$(PROGRAM)
		@rm -f $(ROOTDEV)/include/vector.h #added
		@rm -f $(ROOTDEV)/include/matrix.h #added
		@rm -f $(ROOTDEV)/include/denseMatrix.h #added
		@rm -f $(ROOTDEV)/include/sparseMatrix.h #added
		@rm -f $(ROOTDEV)/include/nnls.h #added
		@cp UVevent.h $(ROOTDEV)/include/UVevent.h
		@cp $(PROGRAM) $(ROOTDEV)/bin/$(PROGRAM)
		@cp UVevent.h $(ROOTDEV)/include/UVevent.h
		@cp vector.h $(ROOTDEV)/include/vector.h #added
		@cp vector.h $(ROOTDEV)/include/matrix.h #added
		@cp vector.h $(ROOTDEV)/include/denseMatrix.h #added
		@cp vector.h $(ROOTDEV)/include/sparseMatrix.h #added
		@cp vector.h $(ROOTDEV)/include/nnls.h #added
		@echo "libraries, shared libraries and includes copied to $(ROOTDEV)"

###
UVevent.$(ObjSuf):     UVevent.h
denseMatrix.$(ObjSuf):     denseMatrix.h #added
sparseMatrix.$(ObjSuf):     sparseMatrix.h #added
nnls.$(ObjSuf):     nnls.h vector.h matrix.h #added



.SUFFIXES: .$(SrcSuf)

.$(SrcSuf).$(ObjSuf):
	$(CXX) $(CXXFLAGS) -c $<


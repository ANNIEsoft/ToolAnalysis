# BDTMuonEnergy Tool script
# ------------------
from Tool import *
import sys
import numpy as np
import pandas as pd
import tensorflow as tf
import tempfile
import random
import csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from array import array
from sklearn import datasets
from sklearn import metrics
from sklearn import model_selection
from sklearn import preprocessing
import sklearn
from sklearn.utils import shuffle
from sklearn import linear_model, ensemble
from sklearn.metrics import mean_squared_error
import pickle
import ROOT

class BDTMuonEnergyPredict(Tool):
    
    # declare member variables here
    weightsfilename = std.string("")
    
    def Initialise(self):
        self.m_log.Log(__file__+" Initialising", self.v_debug, self.m_verbosity)
        self.m_variables.Print()
        self.m_variables.Get("BDTMuonModelFile", self.weightsfilename)
        return 1
    
    def Execute(self):
        self.m_log.Log(__file__+" Executing", self.v_debug, self.m_verbosity)
        #Load Data
        EnergyRecoBoostStore=cppyy.gbl.BoostStore(True, 2)#define the energy boost store class object to load the variables for the BDT training
        EnergyRecoBoostStore = self.m_data.Stores.at("EnergyReco")
        #Retrieve the required variables from the Store
        EnergyRecoBoostStore.Print(False)
        get_ok = EnergyRecoBoostStore.Has("num_pmt_hits")
        num_pmt_hits=ctypes.c_int(0)
        if not get_ok:
            print("BDTMuonEnergyPredict Tool: There is no entry in Energy Reco boost store.")
            return 1
        if get_ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry num_pmt_hits: ", get_ok)
            print("BDTMuonEnergyPredict Tool: type of num_pmt_hits entry is :",EnergyRecoBoostStore.Type("num_pmt_hits"))
            print("BDTMuonEnergyPredict Tool: Getting num_pmt_hits from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("num_pmt_hits",num_pmt_hits)
        print("BDTMuonEnergyPredict Tool: Number of pmt hits is: ", num_pmt_hits.value)
        ok = EnergyRecoBoostStore.Has("num_lappd_hits")
        num_lappd_hits=ctypes.c_int(0)
        if ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry num_lappd_hits: ",ok)
            print("BDTMuonEnergyPredict Tool: type of num_lappd_hits entry is :",EnergyRecoBoostStore.Type("num_lappd_hits"))
            print("BDTMuonEnergyPredict Tool: Getting num_lappd_hits from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("num_lappd_hits",num_lappd_hits)
        print("BDTMuonEnergyPredict Tool: Number of lappd hits is: ", num_lappd_hits.value)
        ok = EnergyRecoBoostStore.Has("DNNRecoLength")
        DNNRecoLength=ctypes.c_double(0)
        if ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry DNNRecoLength: ",ok)
            print("BDTMuonEnergyPredict Tool: type of DNNRecoLength entry is :",EnergyRecoBoostStore.Type("DNNRecoLength"))
            print("BDTMuonEnergyPredict Tool: Getting DNNRecoLength from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("DNNRecoLength",DNNRecoLength)
        print("BDTMuonEnergyPredict Tool: The reconstructed track length in the water by the DNN is: ", DNNRecoLength.value)
        ok = EnergyRecoBoostStore.Has("recoTrackLengthInMrd")
        recoTrackLengthInMrd=ctypes.c_double(0)
        if ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry recoTrackLengthInMrd: ",ok)
            print("BDTMuonEnergyPredict Tool: type of recoTrackLengthInMrd entry is :",EnergyRecoBoostStore.Type("recoTrackLengthInMrd"))
            print("BDTMuonEnergyPredict Tool: Getting recoTrackLengthInMrd from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("recoTrackLengthInMrd",recoTrackLengthInMrd)
        print("BDTMuonEnergyPredict Tool: The reconstructed track length in the MRD is: ", recoTrackLengthInMrd.value)
        ok = EnergyRecoBoostStore.Has("diffDirAbs")
        diffDirAbs=ctypes.c_float(0)
        if ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry diffDirAbs: ",ok)
            print("BDTMuonEnergyPredict Tool: type of diffDirAbs entry is :",EnergyRecoBoostStore.Type("diffDirAbs"))
            print("BDTMuonEnergyPredict Tool: Getting diffDirAbs from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("diffDirAbs",diffDirAbs)
        print("BDTMuonEnergyPredict Tool: DiffDirAbs is: ", diffDirAbs.value)
        ok = EnergyRecoBoostStore.Has("recoDWallR")
        recoDWallR=ctypes.c_float(0)
        if ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry recoDWallR: ",ok)
            print("BDTMuonEnergyPredict Tool: type of recoDWallR entry is :",EnergyRecoBoostStore.Type("recoDWallR"))
            print("BDTMuonEnergyPredict Tool: Getting recoDWallR from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("recoDWallR",recoDWallR)
        print("BDTMuonEnergyPredict Tool: RecoDWallR is: ", recoDWallR.value)
        ok = EnergyRecoBoostStore.Has("recoDWallZ")
        recoDWallZ=ctypes.c_float(0)
        if(ok):
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry recoDWallZ: ",ok)
            print("BDTMuonEnergyPredict Tool: type of recoDWallZ entry is :",EnergyRecoBoostStore.Type("recoDWallZ"))
            print("BDTMuonEnergyPredict Tool: Getting recoDWallZ from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("recoDWallZ",recoDWallZ)
        print("BDTMuonEnergyPredict Tool: RecoDWallZ is: ", recoDWallZ.value)
        ok = EnergyRecoBoostStore.Has("vtxVec")
        vtx_position=cppyy.gbl.Position()
        if ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry vtxVec: ",ok)
            print("BDTMuonEnergyPredict Tool: type of vtxVec entry is :", EnergyRecoBoostStore.Type("vtxVec"))
            print("BDTMuonEnergyPredict Tool: Getting vtxVec from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("vtxVec", vtx_position)
        vtxX=vtx_position.X()
        print("BDTMuonEnergyPredict Tool: VtxX is: ", vtxX)
        vtxY=vtx_position.Y()
        print("BDTMuonEnergyPredict Tool: VtxY is: ", vtxY)
        vtxZ=vtx_position.Z()
        print("BDTMuonEnergyPredict Tool: VtxZ is: ", vtxZ)
        ok = EnergyRecoBoostStore.Has("trueMuonEnergy")
        trueMuonEnergy=ctypes.c_double(0)
        if ok:
            print("BDTMuonEnergyPredict Tool: EnergyRecoBoostStore has entry trueMuonEnergy: ",ok)
            print("BDTMuonEnergyPredict Tool: type of trueMuonEnergy entry is :",EnergyRecoBoostStore.Type("trueMuonEnergy"))
            print("BDTMuonEnergyPredict Tool: Getting trueMuonEnergy from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("trueMuonEnergy",trueMuonEnergy)
        print("BDTMuonEnergyPredict Tool: The MC muon energy is: ", trueMuonEnergy.value)

        #Create features and labels and preprocess data for the model
        features_list=[]
        features_list.append(DNNRecoLength.value/600.)
        features_list.append(recoTrackLengthInMrd.value/200.)
        features_list.append(diffDirAbs.value)
        features_list.append(recoDWallR.value)
        features_list.append(recoDWallZ.value)
        features_list.append(num_lappd_hits.value/200.)
        features_list.append(num_pmt_hits.value/200.)
        features_list.append(vtxX/150.)
        features_list.append(vtxY/200.)
        features_list.append(vtxZ/150.)
        features=np.array(features_list).reshape(1,10)
        labels=np.array([trueMuonEnergy.value])

        # load the model from disk
        print("BDTMuonEnergyPredict Tool: Loading model")
        loaded_model = pickle.load(open(str(self.weightsfilename), 'rb'))

        #predicting...
        print("BDTMuonEnergyPredict Tool: Predicting")
        recoEnergy = loaded_model.predict(features)

        #Set the BDTMuonEnergy in the EnergyReco boost store to be loaded by other tools
        BDTMuonEnergy=float(recoEnergy[0])
        EnergyRecoBoostStore.Set("BDTMuonEnergy", BDTMuonEnergy)
        self.m_data.Stores.at("ANNIEEvent").Set("RecoMuonEnergy", BDTMuonEnergy/1000.)#Set the BDTMuonEnergy in the ANNIEEvent store in [GeV]
        
        EnergyRecoBoostStore.Save("EnergyRecoStore.bs")#Append each entry to file in case we want to make plots

        return 1
    
    def Finalise(self):
        self.m_log.Log(__file__+" Finalising", self.v_debug, self.m_verbosity)
        return 1

###################
# ↓ Boilerplate ↓ #
###################

thistool = BDTMuonEnergyPredict()

def SetToolChainVars(m_data_in, m_variables_in, m_log_in):
    return thistool.SetToolChainVars(m_data_in, m_variables_in, m_log_in)

def Initialise():
    return thistool.Initialise()

def Execute():
    return thistool.Execute()

def Finalise():
    return thistool.Finalise()

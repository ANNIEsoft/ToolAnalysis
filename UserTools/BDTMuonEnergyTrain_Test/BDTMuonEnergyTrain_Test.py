# BDTMuonEnergyTrain_Test Tool script# ------------------
import sys
import numpy as np
import pandas as pd
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
from Tool import *

class BDTMuonEnergyTrain_Test(Tool):
    
    # declare member variables here
    weightsfilename=std.string("")
    inputBoostStorepathname = std.string("")
    
    def Initialise(self):
        self.m_log.Log(__file__+" Initialising", self.v_debug, self.m_verbosity)
        self.m_variables.Print()
        self.m_variables.Get("BDTMuonEnergyWeightsFile", self.weightsfilename)
        self.m_variables.Get("BDTMuonEnergyTrainingInputBoostStoreFile", self.inputBoostStorepathname)
        return 1
    
    def Execute(self):
        self.m_log.Log(__file__+" Executing", self.v_debug, self.m_verbosity)

        #Set seed for reproducibility
        seed=150
        
        EnergyRecoBoostStore=cppyy.gbl.BoostStore(True, 2)#define the energy boost store class object to load the variables for the BDT training
        ok=EnergyRecoBoostStore.Initialise(self.inputBoostStorepathname)#read from disk
        print("BDTMuonEnergyTrain_Test Tool: Initiliased boost store successfully",ok)
        total_entries = ctypes.c_ulong(0)
        get_ok = EnergyRecoBoostStore.Header.Get("TotalEntries",total_entries)
        print("BDTMuonEnergyTrain_Test Tool: Get num of entries of Energy Reco Store: ",get_ok,", entries: ",total_entries.value)
        ievt=ctypes.c_ulong(0)
        while True:
            get_ok=EnergyRecoBoostStore.GetEntry(ievt.value)
            print("BDTMuonEnergyTrain_Test Tool: There is an entry in the BoostStore",get_ok)
            if not get_ok:
                break;
            #When there is no other entry GetEntry() returns false so the while loop stops
            #Retrieve the required variables from this entry
            EnergyRecoBoostStore.Print(False)
            ok = EnergyRecoBoostStore.Has("num_pmt_hits")
            num_pmt_hits=ctypes.c_int(0)
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry num_pmt_hits: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of num_pmt_hits entry is :",EnergyRecoBoostStore.Type("num_pmt_hits"))
             print("BDTMuonEnergyTrain_Test Tool: Getting num_pmt_hits from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("num_pmt_hits",num_pmt_hits)
            print("BDTMuonEnergyTrain_Test Tool: Number of pmt hits is: ", num_pmt_hits.value)
            ok = EnergyRecoBoostStore.Has("num_lappd_hits")
            num_lappd_hits=ctypes.c_int(0)
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry num_lappd_hits: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of num_lappd_hits entry is :",EnergyRecoBoostStore.Type("num_lappd_hits"))
             print("BDTMuonEnergyTrain_Test Tool: Getting num_lappd_hits from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("num_lappd_hits",num_lappd_hits)
            print("BDTMuonEnergyTrain_Test Tool: Number of lappd hits is: ", num_lappd_hits.value)
            ok = EnergyRecoBoostStore.Has("recoTrackLengthInMrd")
            recoTrackLengthInMrd=ctypes.c_double(0)
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry recoTrackLengthInMrd: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of recoTrackLengthInMrd entry is :",EnergyRecoBoostStore.Type("recoTrackLengthInMrd"))
             print("BDTMuonEnergyTrain_Test Tool: Getting recoTrackLengthInMrd from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("recoTrackLengthInMrd",recoTrackLengthInMrd)
            print("BDTMuonEnergyTrain_Test Tool: The reconstructed track length in the MRD is: ", recoTrackLengthInMrd.value)
            ok = EnergyRecoBoostStore.Has("diffDirAbs")
            diffDirAbs=ctypes.c_float(0)
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry diffDirAbs: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of diffDirAbs entry is :",EnergyRecoBoostStore.Type("diffDirAbs"))
             print("BDTMuonEnergyTrain_Test Tool: Getting diffDirAbs from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("diffDirAbs",diffDirAbs)
            print("BDTMuonEnergyTrain_Test Tool: DiffDirAbs is: ", diffDirAbs.value)
            ok = EnergyRecoBoostStore.Has("recoDWallR")
            recoDWallR=ctypes.c_float(0)
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry recoDWallR: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of recoDWallR entry is :",EnergyRecoBoostStore.Type("recoDWallR"))
             print("BDTMuonEnergyTrain_Test Tool: Getting recoDWallR from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("recoDWallR",recoDWallR)
            print("BDTMuonEnergyTrain_Test Tool: RecoDWallR is: ", recoDWallR.value)
            ok = EnergyRecoBoostStore.Has("recoDWallZ")
            recoDWallZ=ctypes.c_float(0)
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry recoDWallZ: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of recoDWallZ entry is :",EnergyRecoBoostStore.Type("recoDWallZ"))
             print("BDTMuonEnergyTrain_Test Tool: Getting recoDWallZ from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("recoDWallZ",recoDWallZ)
            print("BDTMuonEnergyTrain_Test Tool: RecoDWallZ is: ", recoDWallZ.value)
            ok = EnergyRecoBoostStore.Has("vtxVec")
            vtx_position=cppyy.gbl.Position()
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry vtxVec: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of vtxVec entry is :", EnergyRecoBoostStore.Type("vtxVec"))
             print("BDTMuonEnergyTrain_Test Tool: Getting vtxVec from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("vtxVec", vtx_position)
            vtxX=vtx_position.X()
            print("BDTMuonEnergyTrain_Test Tool: VtxX is: ", vtxX)
            vtxY=vtx_position.Y()
            print("BDTMuonEnergyTrain_Test Tool: VtxY is: ", vtxY)
            vtxZ=vtx_position.Z()
            print("BDTMuonEnergyTrain_Test Tool: VtxZ is: ", vtxZ)
            ok = EnergyRecoBoostStore.Has("trueMuonEnergy")
            trueMuonEnergy=ctypes.c_double(0)
            if ok:
             print("BDTMuonEnergyTrain_Test Tool: EnergyRecoBoostStore has entry trueMuonEnergy: ",ok)
             print("BDTMuonEnergyTrain_Test Tool: type of trueMuonEnergy entry is :",EnergyRecoBoostStore.Type("trueMuonEnergy"))
             print("BDTMuonEnergyTrain_Test Tool: Getting trueMuonEnergy from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("trueMuonEnergy",trueMuonEnergy)
            print("BDTMuonEnergyTrain_Test Tool: The MC muon energy is: ", trueMuonEnergy.value)
            #Create features and labels and preprocess data for the model
            features_list=[]
            features_list.append(recoTrackLengthInMrd.value/200.)
            features_list.append(diffDirAbs.value)
            features_list.append(recoDWallR.value)
            features_list.append(recoDWallZ.value)
            features_list.append(num_lappd_hits.value/200.)
            features_list.append(num_pmt_hits.value/200.)
            features_list.append(vtxX/150.)
            features_list.append(vtxY/200.)
            features_list.append(vtxZ/150.)
            #make the features and labels numpy array for this entry
            featuresforthisentry=np.array(features_list)
            labelsforthisentry=np.array([trueMuonEnergy.value])
        #vstack each entry
            if ievt.value==0:
                features=featuresforthisentry
                labels=labelsforthisentry
            else:
                features=np.vstack([features,featuresforthisentry])
                labels=np.vstack([labels,labelsforthisentry])
            ievt.value+=1

        print(features)
        print(features.shape)
        print(labels)
        print(labels.shape)

        Dataset=np.concatenate((features,labels),axis=1)

        print(Dataset)

        np.random.seed(seed)
        np.random.shuffle(Dataset)

        print(Dataset)

        features, labels = np.split(Dataset,[9],axis=1)
        
        num_events, num_pixels = features.shape
        
        print(num_events, num_pixels)
        np.random.seed(0)
        #select the events for which we have the reconstructed track length from the DNN
        if len(labels) % 2==0:
            a=int(len(labels)/2)
        else:
            a=int((len(labels)+1)/2)
        features= features[a:]
        labels = labels[a:]

        num_events, num_pixels = features.shape
        print("BDTMuonEnergyTrain_Test Tool: Events with DNNRecoLength: ", num_events, num_pixels)
        
        #Get the DNN reconstructed track length in the water tank from RecoLength boost store
        #Get the RecoLength Boost Store from DataModel
        DNNRecoLengthBoostStore = self.m_data.Stores.at("RecoLength")
        ok = DNNRecoLengthBoostStore.Has("DNNRecoLength")
        DNNRecoLength_vector=std.vector[float](range(num_events))
        if ok:
            print("BDTMuonEnergyTrain_Test Tool: DNNRecoLengthBoostStore has entry DNNRecoLength: ",ok)
            print("BDTMuonEnergyTrain_Test Tool: type of DNNRecoLength entry is :", DNNRecoLengthBoostStore.Type("DNNRecoLength"))
            print("BDTMuonEnergyTrain_Test Tool: Getting DNNRecoLength from DNNRecoLengthBoostStore")
            DNNRecoLengthBoostStore.Get("DNNRecoLength", DNNRecoLength_vector)
            print("BDTMuonEnergyTrain_Test Tool: The DNNRecoLength for the first event: ", DNNRecoLength_vector.at(0))
        DNNRecoLength_list=[]
        for i in range(DNNRecoLength_vector.size()):
                 DNNRecoLength_list.append(DNNRecoLength_vector.at(i)/600.)
        DNNRecoLength=np.array(DNNRecoLength_list).reshape(len(DNNRecoLength_list),1)
        #Place the DNNRecoLength in the dataset
        features=np.concatenate((DNNRecoLength, features), axis=1)
        print(features)
        print("BDTMuonEnergyTrain_Test Tool: Features shape after adding DNNRecoLength",features.shape)

        DNNRecoLength_new=features[:,0]
        #This loop excludes any events with reconstructed length >1000 as not well reconstructed
        j=0
        b=[]
        for k in DNNRecoLength_new:
             if k>1000:
                print("RecoLength:",k,"Event:",j)
                b.append(j)
             j+=1
        features=np.delete(features,b,axis=0)
        labels=np.delete(labels,b,axis=0)

        ########### BDTG ############                                                                                                                                                                                      
        n_estimators=500
        params = {'n_estimators':n_estimators, 'max_depth': 50,
          'learning_rate': 0.025, 'loss': 'absolute_error'}
        
        #--- select 70% of sample for training and 30% for testing:                                                                                                                                                        
        offset = int(features.shape[0] * 0.7)
        train_X  = features[:offset] # train sample
        train_Y = labels[:offset].reshape(-1)#reshape for model
        test_X = features[offset:] # test sample
        test_Y = labels[offset:].reshape(-1)#reshape for model

        print("BDTMuonEnergyTrain_Test Tool: train shape: ", train_X.shape," label: ", train_Y.shape)
        print("BDTMuonEnergyTrain_Test Tool: test shape: ", test_X.shape," label: ", test_Y.shape)

        print("BDTMuonEnergyTrain_Test Tool: Training BDTG...")
        net_hi_E = ensemble.GradientBoostingRegressor(**params)
        model = net_hi_E.fit(train_X, train_Y)
        net_hi_E

        # save the model to disk                                                                                                                                                                                           
        filename = str(self.weightsfilename)
        pickle.dump(model, open(filename, 'wb'))

        mse = mean_squared_error(test_Y, net_hi_E.predict(test_X))
        print("BDTMuonEnergyTrain_Test Tool: MSE: %.4f" % mse)
        print("BDTMuonEnergyTrain_Test Tool: events at training & test samples: ", len(labels))
        print("BDTMuonEnergyTrain_Test Tool: events at train sample: ", len(train_Y))
        print("BDTMuonEnergyTrain_Test Tool: events at test sample: ", len(test_Y))

        test_score = np.zeros((params['n_estimators'],), dtype=np.float64)

        for i, y_pred in enumerate(net_hi_E.staged_predict(test_X)):
             test_score[i] = net_hi_E.loss_(test_Y, y_pred)

        fig,ax=plt.subplots(ncols=1, sharey=True)
        ax.plot(np.arange(params['n_estimators']) + 1, net_hi_E.train_score_, 'b-', label='Training Set Error')
        ax.plot(np.arange(params['n_estimators']) + 1, test_score, 'r-', label='Test Set Error')
        ax.set_ylim(0.,500.)
        ax.set_xlim(0.,n_estimators)
        ax.legend(loc='upper right')
        ax.set_ylabel('Absolute Errors [MeV]')
        ax.set_xlabel('Number of Estimators')
        ax.yaxis.set_label_coords(-0.1, 0.6)
        ax.xaxis.set_label_coords(0.85, -0.08)
        plt.savefig("error_train_test.png")
        return 1
    
    def Finalise(self):
        self.m_log.Log(__file__+" Finalising", self.v_debug, self.m_verbosity)
        return 1

###################
# ↓ Boilerplate ↓ #
###################

thistool = BDTMuonEnergyTrain_Test()

def SetToolChainVars(m_data_in, m_variables_in, m_log_in):
    return thistool.SetToolChainVars(m_data_in, m_variables_in, m_log_in)

def Initialise():
    return thistool.Initialise()

def Execute():
    return thistool.Execute()

def Finalise():
    return thistool.Finalise()

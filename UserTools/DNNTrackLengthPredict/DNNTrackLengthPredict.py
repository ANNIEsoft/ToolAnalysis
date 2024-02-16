##### DNNTrackLengthPredict Tool Script
import numpy
import tensorflow
import random
import sys
import glob
import numpy as np
import pandas #as pd
import tempfile
import csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot #as plt
from array import array
from sklearn import datasets
from sklearn import metrics
from sklearn import model_selection
from sklearn import preprocessing
from tensorflow import keras
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.callbacks import ModelCheckpoint
from tensorflow.keras.wrappers.scikit_learn import KerasRegressor
from tensorflow.keras import backend as K
import pprint
#import Store and other modules
from Tool import *

class DNNTrackLengthPredict(Tool):

    # declare member variables here
    weightsfilename = std.string("")
    ScalingVarsBoostStorepathname = std.string("")
        
    def Initialise(self):
        self.m_log.Log(__file__+" Initialising", self.v_debug, self.m_verbosity)
        self.m_variables.Print()
        self.m_variables.Get("TrackLengthWeightsFile", self.weightsfilename)
        self.m_variables.Get("ScalingVarsBoostStoreFile", self.ScalingVarsBoostStorepathname)
        return 1
        
    def Execute(self):
        self.m_log.Log(__file__+" Executing", self.v_debug, self.m_verbosity)
        # Load Data from EnergyReco Boost Store
        #-----------------------------
        EnergyRecoBoostStore=cppyy.gbl.BoostStore(True, 2)#define the energy boost store class object to load the variables for the DNN training
        EnergyRecoBoostStore = self.m_data.Stores.at("EnergyReco")
        #Retrieve the required variables from the Store
        EnergyRecoBoostStore.Print(False)
        get_ok = EnergyRecoBoostStore.Has("MaxTotalHitsToDNN")
        MaxTotalHitsToDNN=ctypes.c_int(0)
        if not get_ok:
            print("DNNTrackLengthPredict Tool: There is no entry in Energy Reco boost store.")
            return 1
        if get_ok:
            print("DNNTrackLengthPredict Tool: EnergyRecoBoostStore has entry MaxTotalHitsToDNN: ",get_ok)
            print("DNNTrackLengthPredict Tool: type of MaxTotalHitsToDNN entry is :",EnergyRecoBoostStore.Type("MaxTotalHitsToDNN"))
            print("DNNTrackLengthPredict Tool: Getting MaxTotalHitsToDNN from EnergyRecoBoostStore")#we are going to use it to instantiate the lambda and digit times vectors
            EnergyRecoBoostStore.Get("MaxTotalHitsToDNN",MaxTotalHitsToDNN)
        print("DNNTrackLengthPredict Tool: MaxTotalHitsToDNN is: ", MaxTotalHitsToDNN.value)
        ok = EnergyRecoBoostStore.Has("lambda_vec")
        lambda_vector=std.vector['double'](range(MaxTotalHitsToDNN.value))
        if ok:
            print("DNNTrackLengthPredict Tool: EnergyRecoBoostStore has entry lambda_vec: ",ok)
            print("DNNTrackLengthPredict Tool: type of lambda_vec entry is :", EnergyRecoBoostStore.Type("lambda_vec"))
            print("DNNTrackLengthPredict Tool: Getting lambda_vec from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("lambda_vec", lambda_vector)
        print("DNNTrackLengthPredict Tool: The lambda for the first digit is: ", lambda_vector.at(0))
        ok = EnergyRecoBoostStore.Has("digit_ts_vec")
        digit_ts_vector=std.vector['double'](range(MaxTotalHitsToDNN.value))
        if ok:
            print("DNNTrackLengthPredict Tool: EnergyRecoBoostStore has entry digit_ts_vec: ",ok)
            print("DNNTrackLengthPredict Tool: type of digit_ts_vec entry is :",EnergyRecoBoostStore.Type("digit_ts_vec"))
            print("DNNTrackLengthPredict Tool: Getting digit_ts_vec from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("digit_ts_vec", digit_ts_vector)
        print("DNNTrackLengthPredict Tool: The digit time for the first digit is: ", digit_ts_vector.at(0))
        ok = EnergyRecoBoostStore.Has("lambda_max")
        lambda_max=ctypes.c_double(0)
        if ok:
            print("DNNTrackLengthPredict Tool: EnergyRecoBoostStore has entry lambda_max: ",ok)
            print("DNNTrackLengthPredict Tool: type of lambda_max entry is :",EnergyRecoBoostStore.Type("lambda_max"))
            print("DNNTrackLengthPredict Tool: Getting lambda_max from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("lambda_max",lambda_max)
        print("DNNTrackLengthPredict Tool: Lambda_max is: ", lambda_max.value)
        ok = EnergyRecoBoostStore.Has("num_pmt_hits")
        num_pmt_hits=ctypes.c_int(0)
        if ok:
            print("DNNTrackLengthPredict Tool: EnergyRecoBoostStore has entry num_pmt_hits: ",ok)
            print("DNNTrackLengthPredict Tool: type of num_pmt_hits entry is :",EnergyRecoBoostStore.Type("num_pmt_hits"))
            print("DNNTrackLengthPredict Tool: Getting num_pmt_hits from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("num_pmt_hits",num_pmt_hits)
        print("DNNTrackLengthPredict Tool: Number of pmt hits is: ", num_pmt_hits.value)
        ok = EnergyRecoBoostStore.Has("num_lappd_hits")
        num_lappd_hits=ctypes.c_int(0)
        if ok:
            print("DNNTrackLengthPredict Tool: EnergyRecoBoostStore has entry num_lappd_hits: ",ok)
            print("DNNTrackLengthPredict Tool: type of num_lappd_hits entry is :",EnergyRecoBoostStore.Type("num_lappd_hits"))
            print("DNNTrackLengthPredict Tool: Getting num_lappd_hits from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("num_lappd_hits",num_lappd_hits)
        print("DNNTrackLengthPredict Tool: Number of lappd hits is: ", num_lappd_hits.value)
        ok = EnergyRecoBoostStore.Has("TrueTrackLengthInWater")
        TrueTrackLengthInWater=ctypes.c_float(0)
        if ok:
            print("DNNTrackLengthPredict Tool: EnergyRecoBoostStore has entry TrueTrackLengthInWater: ",ok)
            print("DNNTrackLengthPredict Tool: type of TrueTrackLengthInWater entry is :",EnergyRecoBoostStore.Type("TrueTrackLengthInWater"))
            print("DNNTrackLengthPredict Tool: Getting TrueTrackLengthInWater from EnergyRecoBoostStore")
            EnergyRecoBoostStore.Get("TrueTrackLengthInWater",TrueTrackLengthInWater)
        print("DNNTrackLengthPredict Tool: TrueTrackLengthInWater is: ", TrueTrackLengthInWater.value)
            
        #Create features and labels and preprocess data for the model
        features_list=[]
        for i in range(lambda_vector.size()):
            features_list.append(lambda_vector.at(i))
        for j in range(digit_ts_vector.size()):
            features_list.append(digit_ts_vector.at(j))
        features_list.append(lambda_max.value)
        features_list.append(num_pmt_hits.value)
        features_list.append(num_lappd_hits.value)
        #make the features and labels numpy array
        features=np.array(features_list)
        labels=np.array([TrueTrackLengthInWater.value])
        
        print(features)
        print(features.shape)
        print(labels)
        print(labels.shape)

        #Load scaling parameters
        ScalingVarsStore=cppyy.gbl.BoostStore(True, 0)
        ok=ScalingVarsStore.Initialise(self.ScalingVarsBoostStorepathname)
        features_mean_values_vec=std.vector['double'](range(len(features)))
        features_std_values_vec=std.vector['double'](range(len(features)))
        ScalingVarsStore.Get("features_mean_values",features_mean_values_vec)
        ScalingVarsStore.Get("features_std_values",features_std_values_vec)

        #Transform data
        test_x=[]

        for i in range(len(features)):
            test_x.append((features[i]-features_mean_values_vec.at(i))/features_std_values_vec.at(i))
        test_X=np.array(test_x).reshape(1,2203)

        print("DNNTrackLengthPredict Tool: Defining the model")
        model = Sequential()
        print("DNNTrackLengthPredict Tool: Adding layers")
        model.add(Dense(25, input_dim=2203, kernel_initializer='normal', activation='relu'))
        model.add(Dense(25, kernel_initializer='normal', activation='relu'))
        model.add(Dense(1, kernel_initializer='normal', activation='relu'))

        # load weights                                                                                                                                                                                                 
        print("DNNTrackLengthPredict Tool: Loading weights from file ", self.weightsfilename)
        model.load_weights(str(self.weightsfilename))

        # Compile model                                                                                                                                                                                                
        print("DNNTrackLengthPredict Tool: Compiling model")
        model.compile(loss='mean_squared_error', optimizer='Adamax', metrics=['accuracy'])
        print("DNNTrackLengthPredict Tool: Created model and loaded weights from file", self.weightsfilename)

        # Score accuracy / Make predictions                                                                                                                                                                            
        #----------------------------------                                                                                                                                                                            
        print('DNNTrackLengthPredict Tool: Predicting...')
        y_predicted = model.predict(test_X)
        print(y_predicted.shape)

        # estimate accuracy on dataset using loaded weights                                                                                                                                                            
        print("DNNTrackLengthPredict Tool: Evalulating model on test")
        scores = model.evaluate(test_X, labels, verbose=0)
        print("%s: %.2f%%" % (model.metrics_names[1], scores[1]*100))

        # Score with sklearn.
        print("scoring sk mse")
        score_sklearn = metrics.mean_squared_error(y_predicted, labels)
        print('MSE (sklearn): {0:f}'.format(score_sklearn))

        #Set the DNNRecoLength in the EnergyReco boost store for next tools
        DNNRecoLength=float(y_predicted[0])
        EnergyRecoBoostStore.Set("DNNRecoLength", DNNRecoLength)
        self.m_data.Stores.at("ANNIEEvent").Set("RecoTrackLengthInTank", DNNRecoLength/100.)#Set the DNNRecoLength in the ANNIEEvent store in [m]
        return 1
        
    def Finalise(self):
        self.m_log.Log(__file__+" Finalising", self.v_debug, self.m_verbosity)
        return 1

###################
# ↓ Boilerplate ↓ #
###################

thistool = DNNTrackLengthPredict()

def SetToolChainVars(m_data_in, m_variables_in, m_log_in):
    return thistool.SetToolChainVars(m_data_in, m_variables_in, m_log_in)

def Initialise():
    return thistool.Initialise()

def Execute(): 
    return thistool.Execute()

def Finalise():
    return thistool.Finalise()

# DNNTrackLengthTrain_Test Tool script
# ------------------
import sys
import glob
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
from sklearn.utils import shuffle
from tensorflow import keras
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.callbacks import ModelCheckpoint
from tensorflow.keras.wrappers.scikit_learn import KerasRegressor
import ROOT
import matplotlib.pylab as pylab
params = {'legend.fontsize': 'x-large',
          'figure.figsize': (9, 7),
          'axes.labelsize': 'x-large',
          'axes.titlesize':'x-large',
          'xtick.labelsize':'x-large',
          'ytick.labelsize':'x-large'}
pylab.rcParams.update(params)
from Tool import *

class DNNTrackLengthTrain_Test(Tool):
    
    # declare member variables here
    weightsfilename = std.string("")
    inputBoostStorepathname = std.string("")
    ScalingVarsBoostStorepathname = std.string("")
    
    def Initialise(self):
        self.m_log.Log(__file__+" Initialising", self.v_debug, self.m_verbosity)
        self.m_variables.Print()
        self.m_variables.Get("TrackLengthOutputWeightsFile", self.weightsfilename)
        self.m_variables.Get("TrackLengthTrainingInputBoostStoreFile", self.inputBoostStorepathname)
        self.m_variables.Get("ScalingVarsBoostStoreFile", self.ScalingVarsBoostStorepathname)
        return 1
    
    def Execute(self):
        self.m_log.Log(__file__+" Executing", self.v_debug, self.m_verbosity)
        #Set seed for reproducibility

        seed=150
        
        EnergyRecoBoostStore=cppyy.gbl.BoostStore(True, 2)#define the energy boost store class object to load the variables for the DNN training
        ok=EnergyRecoBoostStore.Initialise(self.inputBoostStorepathname)#read from disk
        print("DNNTrackLengthTrain_Test Tool: Initiliased boost store successfully")
        total_entries = ctypes.c_ulong(0)
        get_ok = EnergyRecoBoostStore.Header.Get("TotalEntries",total_entries)
        print("DNNTrackLengthTrain_Test Tool: Get num of entries of Energy Reco Store: ",get_ok,", entries: ",total_entries.value)
        ievt=ctypes.c_ulong(0)
        while True:
            get_ok=EnergyRecoBoostStore.GetEntry(ievt.value)
            print("DNNTrackLengthTrain_Test Tool: There is an entry in the BoostStore",get_ok)
            if not get_ok:
                break;
            #When there is no other entry GetEntry() returns false so the while loop stops
            #Retrieve the required variables from this entry
            EnergyRecoBoostStore.Print(False)
            ok = EnergyRecoBoostStore.Has("MaxTotalHitsToDNN")
            MaxTotalHitsToDNN=ctypes.c_int(0)
            if ok:
             print("DNNTrackLengthTrain_Test Tool: EnergyRecoBoostStore has entry MaxTotalHitsToDNN: ",ok)
             print("DNNTrackLengthTrain_Test Tool: type of MaxTotalHitsToDNN entry is :",EnergyRecoBoostStore.Type("MaxTotalHitsToDNN"))
             print("DNNTrackLengthTrain_Test Tool: Getting MaxTotalHitsToDNN from EnergyRecoBoostStore")#we are going to use it to instantiate the lambda and digit times vectors
             EnergyRecoBoostStore.Get("MaxTotalHitsToDNN",MaxTotalHitsToDNN)
            print("DNNTrackLengthTrain_Test Tool: MaxTotalHitsToDNN is: ", MaxTotalHitsToDNN.value)
            ok = EnergyRecoBoostStore.Has("lambda_vec")
            lambda_vector=std.vector['double'](range(MaxTotalHitsToDNN.value))
            if ok:
             print("DNNTrackLengthTrain_Test Tool: EnergyRecoBoostStore has entry lambda_vec: ",ok)
             print("DNNTrackLengthTrain_Test Tool: type of lambda_vec entry is :", EnergyRecoBoostStore.Type("lambda_vec"))
             print("DNNTrackLengthTrain_Test Tool: Getting lambda_vec from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("lambda_vec", lambda_vector)
            print("DNNTrackLengthTrain_Test Tool: The lambda for the first digit is: ", lambda_vector.at(0))
            ok = EnergyRecoBoostStore.Has("digit_ts_vec")
            digit_ts_vector=std.vector['double'](range(MaxTotalHitsToDNN.value))
            if ok:
             print("DNNTrackLengthTrain_Test Tool: EnergyRecoBoostStore has entry digit_ts_vec: ",ok)
             print("DNNTrackLengthTrain_Test Tool: type of digit_ts_vec entry is :",EnergyRecoBoostStore.Type("digit_ts_vec"))
             print("DNNTrackLengthTrain_Test Tool: Getting digit_ts_vec from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("digit_ts_vec", digit_ts_vector)
            print("DNNTrackLengthTrain_Test Tool: The digit time for the first digit is: ", digit_ts_vector.at(0))
            ok = EnergyRecoBoostStore.Has("lambda_max")
            lambda_max=ctypes.c_double(0)
            if ok:
             print("DNNTrackLengthTrain_Test Tool: EnergyRecoBoostStore has entry lambda_max: ",ok)
             print("DNNTrackLengthTrain_Test Tool: type of lambda_max entry is :",EnergyRecoBoostStore.Type("lambda_max"))
             print("DNNTrackLengthTrain_Test Tool: Getting lambda_max from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("lambda_max",lambda_max)
            print("DNNTrackLengthTrain_Test Tool: Lambda_max is: ", lambda_max.value)
            ok = EnergyRecoBoostStore.Has("num_pmt_hits")
            num_pmt_hits=ctypes.c_int(0)
            if ok:
             print("DNNTrackLengthTrain_Test Tool: EnergyRecoBoostStore has entry num_pmt_hits: ",ok)
             print("DNNTrackLengthTrain_Test Tool: type of num_pmt_hits entry is :",EnergyRecoBoostStore.Type("num_pmt_hits"))
             print("Getting num_pmt_hits from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("num_pmt_hits",num_pmt_hits)
            print("DNNTrackLengthTrain_Test Tool: Number of pmt hits is: ", num_pmt_hits.value)
            ok = EnergyRecoBoostStore.Has("num_lappd_hits")
            num_lappd_hits=ctypes.c_int(0)
            if ok:
             print("DNNTrackLengthTrain_Test Tool: EnergyRecoBoostStore has entry num_lappd_hits: ",ok)
             print("DNNTrackLengthTrain_Test Tool: type of num_lappd_hits entry is :",EnergyRecoBoostStore.Type("num_lappd_hits"))
             print("DNNTrackLengthTrain_Test Tool: Getting num_lappd_hits from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("num_lappd_hits",num_lappd_hits)
            print("DNNTrackLengthTrain_Test Tool: Number of lappd hits is: ", num_lappd_hits.value)
            ok = EnergyRecoBoostStore.Has("TrueTrackLengthInWater")
            TrueTrackLengthInWater=ctypes.c_float(0)
            if ok:
             print("DNNTrackLengthTrain_Test Tool: EnergyRecoBoostStore has entry TrueTrackLengthInWater: ",ok)
             print("DNNTrackLengthTrain_Test Tool: type of TrueTrackLengthInWater entry is :",EnergyRecoBoostStore.Type("TrueTrackLengthInWater"))
             print("DNNTrackLengthTrain_Test Tool: Getting TrueTrackLengthInWater from EnergyRecoBoostStore")
             EnergyRecoBoostStore.Get("TrueTrackLengthInWater",TrueTrackLengthInWater)
            print("DNNTrackLengthTrain_Test Tool: TrueTrackLengthInWater is: ", TrueTrackLengthInWater.value)
            
            #Create features and labels and preprocess data for the model
            features_list=[]
            for i in range(lambda_vector.size()):
                 features_list.append(lambda_vector.at(i))
            for j in range(digit_ts_vector.size()):
                 features_list.append(digit_ts_vector.at(j))
            features_list.append(lambda_max.value)
            features_list.append(num_pmt_hits.value)
            features_list.append(num_lappd_hits.value)
            #make the features and labels numpy array for this entry
            featuresforthisentry=np.array(features_list)
            labelsforthisentry=np.array([TrueTrackLengthInWater.value])
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

        #shuffle the data in order to avoid any bias in training
        np.random.seed(seed)
        np.random.shuffle(Dataset)

        print(Dataset)

        features, labels = np.split(Dataset,[2203],axis=1)
        
        #split events in train/test samples:
        
        num_events, num_pixels = features.shape
        print(num_events, num_pixels)
        np.random.seed(0)
        #split in half if the number of events is even or take one more event for training when odd
        if len(labels) % 2==0:
            a=int(len(labels)/2)
        else:
            a=int((len(labels)+1)/2)
        train_x = features[:a]
        train_y = labels[:a]
        test_x = features[a:]
        test_y = labels[a:]

        print("DNNTrackLengthTrain_Test Tool: train sample features shape: ", train_x.shape," DNNTrackLengthTrain_Test Tool: train sample label shape: ", train_y.shape)
        print("DNNTrackLengthTrain_Test Tool: test sample features shape: ", test_x.shape," DNNTrackLengthTrain_Test Tool: test sample label shape: ", test_y.shape)

        # Scale data (training set) to 0 mean and unit standard deviation.                                                                                                                                             
        scaler = preprocessing.StandardScaler()
        train_x = scaler.fit_transform(train_x)

        #Save the scaling parameters from fit_transform for later use in the predict toolchain
        features_mean_values=scaler.mean_
        features_std_values=scaler.scale_
        
        #Turn into std.vectors() to store in the ScalingVars BoostStore
        features_mean_values_vec=std.vector['double'](range(len(features_mean_values)))
        for i in range(features_mean_values_vec.size()):
            features_mean_values_vec[i]=features_mean_values[i]
        features_std_values_vec=std.vector['double'](range(len(features_std_values)))
        for j in range(features_std_values_vec.size()):
            features_std_values_vec[j]=features_std_values[j]
        #Create a boost store to store these values
        ScalingVarsStore=cppyy.gbl.BoostStore(True, 0)
        print("DNNTrackLengthTrain_Test Tool: Constructed: ",type(ScalingVarsStore))
        ScalingVarsStore.Set("features_mean_values",features_mean_values_vec)
        ScalingVarsStore.Set("features_std_values",features_std_values_vec)
        #Save BoostStore locally for later use in EnergyRecoPredict toolchain
        print("DNNTrackLengthTrain_Test Tool: Saving boost store with scaling parameters locally for later use")
        ScalingVarsStore.Save(self.ScalingVarsBoostStorepathname)
        def create_model():
            # create model                                                                                                                                                                                             
            model = Sequential()
            model.add(Dense(25, input_dim=2203, kernel_initializer='normal', activation='relu'))
            model.add(Dense(25, kernel_initializer='normal', activation='relu'))
            model.add(Dense(1, kernel_initializer='normal', activation='relu'))
            # Compile model                                                                                                                                                                                            
            model.compile(loss='mean_squared_error', optimizer='Adamax', metrics=['accuracy'])
            return model

        estimator = KerasRegressor(build_fn=create_model, epochs=10, batch_size=2, verbose=0)

        # checkpoint                                                                                                                                                                                                   
        filepath=str(self.weightsfilename)
        checkpoint = ModelCheckpoint(filepath, monitor='val_loss', verbose=1, save_best_only=True, save_weights_only=True, mode='auto')
        callbacks_list = [checkpoint]
        # Fit the model                                                                                                                                                                                           

        history = estimator.fit(train_x, train_y, validation_split=0.33, epochs=12, batch_size=1, callbacks=callbacks_list, verbose=0)
        #-----------------------------                                                                                                                                                                                 
        # summarize history for loss                                                                                                                                                                                   
        fig, ax = plt.subplots(1,1)
        ax.plot(history.history['loss'])
        ax.plot(history.history['val_loss'])
        ax.set_title('Model Loss')
        ax.set_ylabel('Performance')
        ax.set_xlabel('Epochs')
        ax.set_xlim(0.,12.)
        ax.legend(['training loss', 'validation loss'], loc='upper left')
        plt.savefig("keras_train_test.pdf")
        plt.close(fig)

        # create model to test the weights
        model = Sequential()
        model.add(Dense(25, input_dim=2203, kernel_initializer='normal', activation='relu'))
        model.add(Dense(25, kernel_initializer='normal', activation='relu'))
        model.add(Dense(1, kernel_initializer='normal', activation='relu'))

        # load weights
        model.load_weights(filepath)

        # Compile model
        model.compile(loss='mean_squared_error', optimizer='Adamax', metrics=['accuracy'])
        print("DNNTrackLengthTrain_Test Tool: Created model and loaded weights from"+filepath+"for testing")

        ## Predict.
        print('DNNTrackLengthTrain_Test Tool: Predicting...')
        x_transformed = scaler.transform(test_x)
        y_predicted = model.predict(x_transformed)

        scores = model.evaluate(x_transformed, test_y, verbose=0)
        print("%s: %.2f%%" % (model.metrics_names[1], scores[1]*100))

        # Score with sklearn.
        score_sklearn = metrics.mean_squared_error(y_predicted, test_y)
        print('MSE (sklearn): {0:f}'.format(score_sklearn))

        #-----------------------------
        print("shapes: ", test_y.shape, ", ", y_predicted.shape)
        #Creating a BoostStore to store the DNN reconstructed track length in the water in the DataModel to be loaded for the BDT trainining
        RecoLengthStore = cppyy.gbl.BoostStore(True,0)
        DNNRecoLength=std.vector[float](range(len(y_predicted)))
        for k in range(len(y_predicted)):
            DNNRecoLength[k]=float(y_predicted[k])
        print(DNNRecoLength.at(0),y_predicted[0])
        RecoLengthStore.Set("DNNRecoLength", DNNRecoLength)
        self.m_data.Stores["RecoLength"] = RecoLengthStore
        
        #Make some plots for visualization purposes
        fig0, ax0 = plt.subplots()
        ax0.scatter(test_y,y_predicted)
        ax0.plot([test_y.min(),test_y.max()],[test_y.min(),test_y.max()],'k--',lw=3)
        ax0.set_xlabel('MC Tracklength in water(cm)')
        ax0.set_ylabel('Predicted Reconstucted Length from DNN(cm)')
        fig0.savefig('recolength.png')
        plt.close(fig0)
        
        data = abs(y_predicted-test_y)
        lambda_max=test_x[:,[2200]].tolist()
        dataprev = abs(lambda_max-test_y)
        nbins=np.arange(0.,400.,5)
        fig1,ax1=plt.subplots(ncols=1, sharey=False)
        f0=ax1.hist(data, nbins, histtype='step', fill=False, color='blue',alpha=0.75) 
        f1=ax1.hist(dataprev, nbins, histtype='step', fill=False, color='red',alpha=0.75)
        ax1.set_xlabel('$\Delta R = |L_{Reco}-L_{MC}|$ [cm]')
        ax1.legend(('NEW','Previous'))
        ax1.xaxis.set_ticks(np.arange(0., 425., 25))
        ax1.tick_params(axis='x', which='minor', bottom=False)
        xmin, xmax = plt.xlim()
        title = "mean = %.2f, std = %.2f, Prev: mean = %.2f, std = %.2f " % (data.mean(), data.std(),dataprev.mean(), dataprev.std())
        plt.title(title)
        fig1.savefig('resol_distr2l_WCSim.png')
        plt.close(fig1)

        canvas = ROOT.TCanvas()
        canvas.cd(1)
        th2f = ROOT.TH2F("True_RecoLength", "; MC Track Length [cm]; Reconstructed Track Length [cm]", 50, 0, 400., 50, 0., 400.)
        for i in range(len(test_y)):
             th2f.Fill(test_y[i], y_predicted[i])
        line = ROOT.TLine(0.,0.,400.,400.)
        th2f.SetStats(0)
        th2f.Draw("ColZ")
        line.SetLineColor(2)
        line.Draw("same")
        canvas.SaveAs("MClength_newrecolength.png")
        return 1
    
    def Finalise(self):
        self.m_log.Log(__file__+" Finalising", self.v_debug, self.m_verbosity)
        return 1

###################
# ↓ Boilerplate ↓ #
###################

thistool = DNNTrackLengthTrain_Test()

def SetToolChainVars(m_data_in, m_variables_in, m_log_in):
    return thistool.SetToolChainVars(m_data_in, m_variables_in, m_log_in)

def Initialise():
    return thistool.Initialise()

def Execute():
    return thistool.Execute()

def Finalise():
    return thistool.Finalise()

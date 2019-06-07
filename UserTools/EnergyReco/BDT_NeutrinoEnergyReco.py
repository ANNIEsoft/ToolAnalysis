##### Script for Muon Energy Reconstruction in the water tank
import Store
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
#import seaborn as sns

#import ROOT
#ROOT.gROOT.SetBatch(True)
#from ROOT import TFile, TNtuple
#from root_numpy import root2array, tree2array, fill_hist

#-------- File with events for reconstruction:
#--- evts for training:
infile = "../LocalFolder/vars_Ereco.csv"
#--- evts for prediction:
infile2 = "../LocalFolder/vars_Ereco.csv"
#----------------

def Initialise():
    return 1

def Finalise():
 #   ROOT.gSystem.Exit(0)
    return 1

def Execute():
    # Set TF random seed to improve reproducibility
    seed = 180
    np.random.seed(seed)

    E_threshold = 2.
    E_low=0
    E_high=2000
    div=100
    bins = int((E_high-E_low)/div)
    print('bins: ', bins)

    print( "--- opening file with input variables!") 
    #--- events for training ---
    filein = open(str(infile))
    print("evts for training in: ",filein)
    df00=pd.read_csv(filein)
    df0=df00[['totalPMTs','totalLAPPDs','TrueTrackLengthInWater','neutrinoE','trueKE','diffDirAbs','TrueTrackLengthInMrd','recoDWallR','recoDWallZ','dirX','dirY','dirZ','vtxX','vtxY','vtxZ','DNNRecoLength']]
    dfsel=df0.loc[df0['neutrinoE'] < E_threshold]

    #print to check:
    print("check training sample: ",dfsel.head())
#    print(dfsel.iloc[5:10,0:5])
    #check fr NaN values:
    assert(dfsel.isnull().any().any()==False)

    #--- events for predicting ---
    filein2 = open(str(infile2))
    print(filein2)
    df00b = pd.read_csv(filein2)
    df0b=df00b[['totalPMTs','totalLAPPDs','TrueTrackLengthInWater','neutrinoE','trueKE','diffDirAbs','TrueTrackLengthInMrd','recoDWallR','recoDWallZ','dirX','dirY','dirZ','vtxX','vtxY','vtxZ','DNNRecoLength']]
    dfsel_pred=df0b.loc[df0b['neutrinoE'] < E_threshold]
    #print to check:
    print("check predicting sample: ",dfsel_pred.shape," ",dfsel_pred.head())
#    print(dfsel_pred.iloc[5:10,0:5])
    #check fr NaN values:
    assert(dfsel_pred.isnull().any().any()==False)

    #--- normalisation-training sample:
    dfsel_n = pd.DataFrame([ dfsel['DNNRecoLength']/600., dfsel['TrueTrackLengthInMrd']/200., dfsel['diffDirAbs'], dfsel['recoDWallR']/152.4, dfsel['recoDWallZ']/198., dfsel['totalLAPPDs']/1000., dfsel['totalPMTs']/1000., dfsel['vtxX']/150., dfsel['vtxY']/200., dfsel['vtxZ']/150. ]).T
    print("chehck normalisation: ", dfsel_n.head())
    #--- normalisation-sample for prediction:
    dfsel_pred_n = pd.DataFrame([ dfsel_pred['DNNRecoLength']/600., dfsel_pred['TrueTrackLengthInMrd']/200., dfsel_pred['diffDirAbs'], dfsel_pred['recoDWallR']/152.4, dfsel_pred['recoDWallZ']/198., dfsel_pred['totalLAPPDs']/1000., dfsel_pred['totalPMTs']/1000., dfsel_pred['vtxX']/150., dfsel_pred['vtxY']/200., dfsel_pred['vtxZ']/150. ]).T

    #--- prepare training & test sample for BDT:
    arr_hi_E0 = np.array(dfsel_n[['DNNRecoLength','TrueTrackLengthInMrd','diffDirAbs','recoDWallR','recoDWallZ','totalLAPPDs','totalPMTs','vtxX','vtxY','vtxZ']])
    arr3_hi_E0 = 1000.*np.array(dfsel[['neutrinoE']])
 
    #---- random split of events ----
    rnd_indices = np.random.rand(len(arr_hi_E0)) < 0.50
    #--- select events for training/test:
    arr_hi_E0B = arr_hi_E0[rnd_indices]
    arr2_hi_E_n = arr_hi_E0B #.reshape(arr_hi_E0B.shape + (-1,))
    arr3_hi_E = arr3_hi_E0[rnd_indices]
    #--- select events for prediction: -- in future we need to replace this with data sample!
    evts_to_predict = arr_hi_E0[~rnd_indices]
    evts_to_predict_n = evts_to_predict #.reshape(evts_to_predict.shape + (-1,))
    test_data_trueKE_hi_E = arr3_hi_E0[~rnd_indices]

    #printing..
    print('events for training: ',len(arr3_hi_E),' events for predicting: ',len(test_data_trueKE_hi_E)) 
    print('initial train shape: ',arr3_hi_E.shape," predict: ",test_data_trueKE_hi_E.shape)

    ########### BDTG ############
    n_estimators=1000
    params = {'n_estimators':n_estimators, 'max_depth': 50,
              'learning_rate': 0.01, 'loss': 'lad'} 
    
    print("arr2_hi_E_n.shape: ",arr2_hi_E_n.shape)
    #--- select 70% of sample for training and 30% for testing:
    offset = int(arr2_hi_E_n.shape[0] * 0.7) 
    arr2_hi_E_train, arr3_hi_E_train = arr2_hi_E_n[:offset], arr3_hi_E[:offset].reshape(-1)  # train sample
    arr2_hi_E_test, arr3_hi_E_test   = arr2_hi_E_n[offset:], arr3_hi_E[offset:].reshape(-1)  # test sample
 
    print("train shape: ", arr2_hi_E_train.shape," label: ",arr3_hi_E_train.shape)
    print("test shape: ", arr2_hi_E_test.shape," label: ",arr3_hi_E_test.shape)
    
    print("training BDTG...")
    net_hi_E = ensemble.GradientBoostingRegressor(**params)
    net_hi_E.fit(arr2_hi_E_train, arr3_hi_E_train)
    net_hi_E

    mse = mean_squared_error(arr3_hi_E_test, net_hi_E.predict(arr2_hi_E_test)) 
    print("MSE: %.4f" % mse)
    print("events at training & test samples: ", len(arr_hi_E0))
    print("events at train sample: ", len(arr2_hi_E_train))
    print("events at test sample: ", len(arr2_hi_E_test))
 
    test_score = np.zeros((params['n_estimators'],), dtype=np.float64)
 
    for i, y_pred in enumerate(net_hi_E.staged_predict(arr2_hi_E_test)):
        test_score[i] = net_hi_E.loss_(arr3_hi_E_test, y_pred)

#    fig,ax=plt.subplots(ncols=1, sharey=True)
#    ax.plot(np.arange(params['n_estimators']) + 1, net_hi_E.train_score_, 'b-',
#             label='Training Set Deviance')
#    ax.plot(np.arange(params['n_estimators']) + 1, test_score, 'r-',
#             label='Test Set Deviance')
#     ax.set_ylim(0.,500.)
#     ax.set_xlim(0.,n_estimators)
#     ax.legend(loc='upper right')
#     ax.set_ylabel('Least Absolute Deviations [MeV]')
#     ax.set_xlabel('Number of Estimators')
#     ax.yaxis.set_label_coords(-0.1, 0.6)
#     ax.xaxis.set_label_coords(0.85, -0.08)
#     plt.savefig("deviation_train_test.png")

    #predicting...
    print("events for energy reco: ", len(evts_to_predict_n)) 
    BDTGoutput_E = net_hi_E.predict(evts_to_predict_n)

    Y=[0 for j in range (0,len(test_data_trueKE_hi_E))]
    for i in range(len(test_data_trueKE_hi_E)):
        Y[i] = 100.*(test_data_trueKE_hi_E[i]-BDTGoutput_E[i])/(1.*test_data_trueKE_hi_E[i])
#        print("MC Energy: ", test_data_trueKE_hi_E[i]," Reco Energy: ",BDTGoutput_E[i]," DE/E[%]: ",Y[i])

    df1 = pd.DataFrame(test_data_trueKE_hi_E,columns=['NeutrinoEnergy'])
    df2 = pd.DataFrame(BDTGoutput_E,columns=['RecoE'])
    df_final = pd.concat([df1,df2],axis=1)
 
    #-logical tests:
    print("checking..."," df0.shape[0]: ",df1.shape[0]," len(y_predicted): ", len(BDTGoutput_E)) 
    assert(df1.shape[0]==len(BDTGoutput_E))
    assert(df_final.shape[0]==df2.shape[0])

    #save results to .csv:  
    df_final.to_csv("ErecoENEU_results.csv", float_format = '%.3f')

#    nbins=np.arange(-100,100,2)
#    fig,ax0=plt.subplots(ncols=1, sharey=True)#, figsize=(8, 6))
#    cmap = sns.light_palette('b',as_cmap=True)
#    f=ax0.hist(np.array(Y), nbins, histtype='step', fill=True, color='gold',alpha=0.75)
#    ax0.set_xlim(-100.,100.)
#    ax0.set_xlabel('$\Delta E/E$ [%]')
#    ax0.set_ylabel('Number of Entries')
#    ax0.xaxis.set_label_coords(0.95, -0.08)
#    ax0.yaxis.set_label_coords(-0.1, 0.71)
#    title = "mean = %.2f, std = %.2f " % (np.array(Y).mean(), np.array(Y).std())
#    plt.title(title)
#    plt.savefig("DE_E.png")
 
    #write in output .root file:
    #vtxX=dfsel_n['vtxX'][~rnd_indices]
    #vtxY=dfsel_n['vtxY'][~rnd_indices]
    #vtxZ=dfsel_n['vtxZ'][~rnd_indices]
    #outputFile = ROOT.TFile.Open("TESTNEWOUTRecoMuonMRD.root", "RECREATE")
    #outputTuple = ROOT.TNtuple("tuple", "tuple", "neutrinoE:recoKE:vtxX:vtxY:vtxZ:DE_E")
    #for i in range(len(test_data_trueKE_hi_E)):
    #    outputTuple.Fill(test_data_trueKE_hi_E[i],BDTGoutput_E[i],vtxX[i],vtxY[i],vtxZ[i],Y[i])
    #outputTuple.Write()
    #outputFile.Close()

    return 1



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
from tensorflow import keras
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.callbacks import ModelCheckpoint

#--------- File with events for reconstruction:
#--- evts for training:
infile = "../LocalFolderD/data_forRecoLength_06082019.csv"
#--- evts for prediction:
infile2 = "../LocalFolderD/data_forRecoLength_06082019.csv"
#

# Set TF random seed to improve reproducibility
seed = 150
np.random.seed(seed)

print( "--- opening file with input variables!")
#--- events for training - MC events
filein = open(str(infile))
print("evts for training in: ",filein)
Dataset = np.array(pd.read_csv(filein))
features, lambdamax, labels, rest = np.split(Dataset,[2203,2204,2205],axis=1)

#--- events for predicting
filein2 = open(str(infile2))
print("events for prediction in: ",filein2)
Dataset2 = np.array(pd.read_csv(filein2))
features2, lambdamax2, labels2, rest2 = np.split(Dataset2,[2203,2204,2205],axis=1)
print( "lambdamax2 ", lambdamax2[:2], labels2[:2])
print(features2[0])

#split events in train/test samples:
num_events, num_pixels = features.shape
print(num_events, num_pixels)
np.random.seed(0)
train_x = features[:1000]
train_y = labels[:1000]
test_x = features2[1000:]
test_y = labels2[1000:]
#    print("len(train_y): ",len(train_y)," len(test_y): ", len(test_y))
print("test sample features shape: ", test_x.shape," test sample label shape: ", test_y.shape)

# Scale data (training set) to 0 mean and unit standard deviation.
scaler = preprocessing.StandardScaler()
train_x = scaler.fit_transform(train_x)

# create model
model = Sequential()
model.add(Dense(25, input_dim=2203, kernel_initializer='normal', activation='relu'))
model.add(Dense(25, kernel_initializer='normal', activation='relu'))
model.add(Dense(1, kernel_initializer='normal', activation='relu'))

# load weights
model.load_weights("/ToolAnalysis/UserTools/DNNTrackLength/stand_alone/weights/weights_bets.hdf5")

# Compile model
model.compile(loss='mean_squared_error', optimizer='Adamax', metrics=['accuracy'])
print("Created model and loaded weights from file")

## Predict.
print('predicting...')
x_transformed = scaler.transform(test_x)
y_predicted = model.predict(x_transformed)

# Score with sklearn.
score_sklearn = metrics.mean_squared_error(y_predicted, test_y)
print('MSE (sklearn): {0:f}'.format(score_sklearn))

#-----------------------------
print(" saving .csv file with energy variables..")
print("shapes: ", test_y.shape, ", ", y_predicted.shape)

data = np.concatenate((test_y, y_predicted),axis=1)
df=pd.DataFrame(data, columns=['TrueTrackLengthInWater','DNNRecoLength'])

#---read .csv file containing predict
filein2 = open(str(infile2))
df0 = pd.read_csv(filein2)[1000:]
#    print(df0.head())
#    df0= pd.read_csv("../LocalFolder/data_forRecoLength_9.csv")
print("df0.shape: ",df0.shape," df.shape: ",df.shape)
print("df0.head(): ",df0.head())
print("df.head(): ", df.head())
#df_final = pd.concat([df0,df], axis=1).drop(['lambda_max.1'], axis=1)
df_final = df0
print("-- Prev: df_final.columns: ",df_final.columns)
df_final.insert(2217, 'TrueTrackLengthInWater', df['TrueTrackLengthInWater'].values, allow_duplicates="True")
df_final.insert(2218, 'DNNRecoLength', df['DNNRecoLength'].values, allow_duplicates="True")
print("df_final.head(): ",df_final.head())
print("--- df_final.shape: ",df_final.shape)
print("-- After: df_final.columns: ",df_final.columns)

#-logical tests:
print("checking..."," df0.shape[0]: ",df0.shape[0]," len(y_predicted): ", len(y_predicted))
assert(df0.shape[0]==len(y_predicted))
print("df_final.shape[0]: ",df_final.shape[0]," df.shape[0]: ",df.shape[0])
assert(df_final.shape[0]==df.shape[0])

df_final.to_csv("outputs_ener/vars_Ereco.csv", float_format = '%.3f')
df_final[:1000].to_csv("outputs_ener/vars_Ereco_train.csv", float_format = '%.3f')
df_final[1000:].to_csv("outputs_ener/vars_Ereco_pred.csv", float_format = '%.3f')

#df_final.to_csv("vars_Ereco_06082019CC0pi.csv", float_format = '%.3f')
#df_final[:1000].to_csv("vars_Ereco_train_06082019CC0pi.csv", float_format = '%.3f')
#df_final[1000:].to_csv("vars_Ereco_pred_06082019CC0pi.csv", float_format = '%.3f')

#---if asserts fails check dimensions with these print outs:
#print("df: ",df.head())
#print(df.iloc[:,2200:])
#print(df0.head())
#print(df0.shape)
#print(df0.iloc[:,2200:])
#print(df_final.shape)
#print(df_final.iloc[:,2200:])

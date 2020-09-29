import numpy as np 
import pandas as pd
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
from sklearn import datasets
from sklearn import preprocessing

from sklearn.metrics import accuracy_score
from sklearn.metrics import classification_report
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap

from sklearn import tree
from sklearn.metrics import roc_curve, auc
from sklearn.ensemble import RandomForestClassifier
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.svm import SVC
from sklearn import neighbors
from sklearn.linear_model import SGDClassifier
from sklearn.multiclass import OneVsRestClassifier
from sklearn.naive_bayes import GaussianNB
from sklearn.neural_network import MLPClassifier
from sklearn.linear_model import LogisticRegression

import pickle #For loading/saving models

import argparse #For user input


#------- Parse user arguments ----

parser = argparse.ArgumentParser(description='PID classification - Overview')
parser.add_argument("--input", default="/annie/app/users/mnieslon/MyToolAnalysis6/classification_data/R1613S0p1_Full.csv", help = "The input file containing the classification variables [csv-format]")
parser.add_argument("--output", default="/annie/app/users/mnieslon/MyToolAnalysis6/R1613_pid_predictions.txt", help = "The output file containing the predictions of the classifier")
parser.add_argument("--variable_names", default="VariableConfig_PMTOnly_Q_PID.txt", help = "File containing the list of classification variables")
parser.add_argument("--model",default="models/pid_model_beamlikev2_PMTOnly_Q_PID_MLP.sav",help="Path to classification model")
args = parser.parse_args()
input_file = args.input
output_file = args.output
variable_file = args.variable_names
model_file = args.model

print('PID classification initialization: Input_file: '+input_file+', variable file: '+variable_file+', model_file: '+model_file)

#----------- Load data-----------

data = pd.read_csv(input_file,header=0)

#---------- Load only specific variables of data ---------

variable_file_path = "variable_config/"+variable_file
with open(variable_file_path) as f:
    subset_variables = f.read().splitlines()

data = data[subset_variables]

# --------- Compare model variables ---------------------

loaded_model, feature_vars_model, scaler_model = pickle.load(open(model_file,'rb'))

print("Feature variables of loaded_model: ")
print(*feature_vars_model, sep = ", ") 
print("Feature variables used in data: ")
print(*subset_variables, sep = ", ")

do_prediction = True


# ------------- Number of variables check --------------

if len(feature_vars_model)-1 != len(subset_variables):
	print('Number of variables does not match in model & data (Model: '+str(len(feature_vars_model)-1)+', Data: '+str(len(subset_variables))+'). Abort')
	do_prediction = False

if do_prediction:
	for i in range(len(feature_vars_model)):
		if i is len(feature_vars_model)-1:		#Omit particleType column
			continue

		if feature_vars_model[i] != subset_variables[i]:
			print('Variables at index '+str(i)+' do not match in model & data (Model: '+feature_vars_model[i]+', Data: '+subset_variables[i]+'). Abort')
			do_prediction = False

# ----------------- Do prediction -----------------------

if do_prediction:

	X = data.loc[:,data.columns!='particleType']
	X = pd.DataFrame(scaler_model.transform(X))
	#X.to_csv("X_load.csv")

	Y_pred = loaded_model.predict(X)
	Y_pred_prob = loaded_model.predict_proba(X)

	with open(output_file,'w') as f:
		for item in range(len(Y_pred)):
			print(str(Y_pred[item])+','+str(Y_pred_prob[item]),file=f)


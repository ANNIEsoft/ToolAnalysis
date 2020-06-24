import numpy as np 
import pandas as pd
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
from sklearn import datasets
from sklearn import preprocessing
from sklearn.model_selection import train_test_split

from sklearn.metrics import accuracy_score
from sklearn.metrics import classification_report
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap

from sklearn import tree
from sklearn.metrics import roc_curve, auc, precision_recall_curve, f1_score
from sklearn.ensemble import RandomForestClassifier
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.svm import SVC
from sklearn import neighbors
from sklearn.linear_model import SGDClassifier
from sklearn.multiclass import OneVsRestClassifier
from sklearn.naive_bayes import GaussianNB
from sklearn.neural_network import MLPClassifier
from sklearn.linear_model import LogisticRegression
from xgboost import XGBClassifier

import pickle #for saving models

import argparse

#------- Parse user arguments ----

parser = argparse.ArgumentParser(description='Ring Classification training - Overview')
parser.add_argument("--input", default="data_new.nosync/beam_DigitThr10_0_4996_Full.csv", help = "The electron input file [csv-format]")
parser.add_argument("--status_suffix", default="Full.csv", help = "The strings of the input file to be replaced when looking at the status file.")
parser.add_argument("--variable_names", default="VariableConfig_Full.txt", help = "File containing the list of classification variables")
parser.add_argument("--model_name",default="MLP",help="Classififier to use for training. Options: RandomForest, XGBoost, SVM, SGD, MLP, GradientBoosting, All")
parser.add_argument("--dataset_name",default="beam",help="Keyword describing dataset name (used to label output files)")
parser.add_argument("--balance_data",default=True,help="Should the dataset be made even for the training process?")
parser.add_argument("--plot_roc",default=False,help="Shall ROC curve be drawn?")
parser.add_argument("--plot_prec_recall",default=False,help="Shall Precision-Recall curve be drawn?")
parser.add_argument("--plot_correlation",default=False,help="Shall variable correlation plots be drawn?")
parser.add_argument("--require_mrd",default=False,help="Only look at events that have MRD hits?")
parser.add_argument("--require_muon",default=False,help="Only look at events that have a muon?")

args = parser.parse_args()
input_file = args.input
status_suffix = args.status_suffix
variable_file = args.variable_names
model_name = args.model_name
dataset_name = args.dataset_name
balance_data = args.balance_data
do_plot_roc = args.plot_roc
do_plot_precision_recall = args.plot_prec_recall
do_plot_correlation = args.plot_correlation
do_require_mrd = args.require_mrd
do_require_muon = args.require_muon

print('Ring Classification initialization: Input_file: '+input_file+', variable file: '+variable_file+', model_name: '+model_name)


#------- Read in .csv file -------

data = pd.read_csv(input_file, header = 0)   #first row is header

#------ Load variable config --------

variable_file_path = "variable_config/"+variable_file
with open(variable_file_path) as f:
    subset_variables = f.read().splitlines()
subset_classification_vars = subset_variables
print('Subset of variables: ',subset_variables)
data = data[subset_variables]

variable_config = variable_file[15:variable_file.find('.txt')]
print('Variable configuration name: ',variable_config)


# ------- Load additional information -------

input_file_additional = input_file[0:input_file.find(status_suffix)]+"status.csv"
print('input_file_additional: ',input_file_additional)

data_additional = pd.read_csv(input_file_additional, header = 0)
print('data_additional (preview): ',data_additional.head())

feature_labels_additional = list(data_additional.columns)
feature_labels_additional.remove("MCMultiRing")
print('feature_labels_additional: ',feature_labels_additional)


# ------- Combine data and additional_data to keep label information -------

print('shape data: ',data.shape)
print('shape data_additional: ',data_additional.shape)

data_additional = pd.concat([data,data_additional], axis = 1, sort = False)
print('balanced_data_additional preview: ',data_additional.head())

data_additional['MCMultiRing'] = data_additional['MCMultiRing'].astype('str')
data_additional.replace({'MCMultiRing':{'0':'1-ring',str(1):'multi-ring'}},inplace=True)

subset_variables.append('MCMultiRing')

# ------- Load pion energy information --------

input_file_pion = input_file[0:input_file.find(status_suffix)]+"pion_energies.csv"
print('input_file_pions: ',input_file_pion)

data_pion = pd.read_csv(input_file_pion, header = 0)
print('data_pion (preview): ',data_pion.head())

feature_labels_pion = list(data_pion.columns)
print('feature_labels_pion: ',feature_labels_pion)

data_additional = pd.concat([data_additional,data_pion], axis = 1, sort = False)


# ------ Remove unwanted events (if specified) -------------------------
if do_require_muon:
    data_additional.drop(data_additional[data_additional.MCPDG !=13].index, inplace=True)

if do_require_mrd:
    data_additional.drop(data_additional[data_additional.MrdLayers > 0].index, inplace=True)


# ----- Balance data (if specified) --------

if balance_data:
    # Balance data to be 50% single ring, 50% multi ring
    balanced_data = data_additional.groupby('MCMultiRing')
    balanced_data = (balanced_data.apply(lambda x: x.sample(balanced_data.size().min()).reset_index(drop=True)))
else:
    balanced_data = data_additional

# -------- Populate arrays for classification ----------

X = balanced_data.loc[:, balanced_data.columns!='MCMultiRing']
y = balanced_data.iloc[:, balanced_data.columns=='MCMultiRing']
print("Preview X (classification variables): ",X.head())
print("Preview Y (class names): ",y.head())


# ------------ Build train & test dataset --------------

X_train0, X_test0, y_train, y_test = train_test_split(X, y, test_size = 0.4, random_state = 100)


# --------- Save additional info about events --------

X_test_indices = X_test0.index.get_level_values(1)
print("Test set indices: ",X_test_indices)
index_file = open("predictions/RingClassification/RingClassification_Indices_"+dataset_name+"_"+variable_config+".dat","w")
for index in X_test_indices:
    index_file.write("%i\n" % index)
index_file.close()

X_test0.to_csv('predictions/RingClassification/RingClassification_AddEvInfo_'+dataset_name+"_"+variable_config+'.csv')

# After saving the information, drop the additional variables from the training & test data


X_train0.drop(columns=feature_labels_additional,axis=1,inplace=True)
X_test0.drop(columns=feature_labels_additional,axis=1,inplace=True)
X_train0.drop(columns=feature_labels_pion,axis=1,inplace=True)
X_test0.drop(columns=feature_labels_pion,axis=1,inplace=True)
print("Shape of X_train0 after dropping additional variables: ",X_train0.shape,", shape of X_test0 after dropping additional variables: ",X_test0.shape)
print("Preview of X_train0 after dropping additional variables: ",X_train0.head())


# Get feature labels of remaining X_train0-dataframe

feature_labels = list(X_train0.columns)

print("Length of feature_labels: %i" %(len(feature_labels)))
num_plots = int(len(feature_labels)/2)
if (len(feature_labels)%2 == 0):
    num_plots =num_plots - 1
print("Feature labels: ",feature_labels)


# Scale data (training set) to 0 mean and unit standard deviation.
scaler = preprocessing.StandardScaler()
X_train = pd.DataFrame(scaler.fit_transform(X_train0))
X_test = pd.DataFrame(scaler.transform(X_test0))
print("type(X_train) ",type(X_train)," type(X_train0): ",type(X_train0))
y_train2 = np.array(y_train).ravel() #Return a contiguous flattened 1-d array
print("X_train.shape: ",X_train.shape," y_train2.shape: ",y_train2.shape, "X_test.shape: ",X_test.shape," y_test.shape: ",y_test.shape)


# Setup additional data sets with labels 0/1 instead of 1-ring/multi-ring
y_test_binary = y_test
np.where(y_test_binary == '1-ring', 0, 1)

fig=[]
for plot in range(num_plots):
    if model_name != "All":
        fig.append(plt.figure(plot,figsize=(15,10)))
    else:
        fig.append(plt.figure(plot,figsize=(15,20)))

if model_name != "All":
    fig_roc = plt.figure(num_plots+1,figsize=(15,10))
    fig_pr = plt.figure(num_plots+2,figsize=(15,10))
    fig_trash = plt.figure(num_plots+3,figsize=(15,10))
else:
    fig_roc = plt.figure(num_plots+1,figsize=(15,20))
    fig_pr = plt.figure(num_plots+2,figsize=(15,20))
    fig_trash = plt.figure(num_plots+3,figsize=(15,20))

# ------------------------------------------------------------------
# ---- run_model: Train the classifier on the training data set ----
# ------------------------------------------------------------------

# Predictions on the test dataset are saved to csv-files

def run_model(model, alg_name):
    
    model.fit(X_train, y_train2)
    y_pred = model.predict(X_test)
    y_pred_binary = y_pred
    np.where(y_pred_binary == '1-ring', 0, 1)
    accuracy =  accuracy_score(y_test, y_pred) * 100 #returns the fraction of correctly classified samples
    print("Algorithm name: ",alg_name)
    file = open("predictions/RingClassification/RingClassification_Accuracy_"+dataset_name+"_"+variable_config+".dat","a")
    file.write("%s \t %1.3f \n" % (alg_name, accuracy))
    file.close()

    predictions = pd.concat([y_test.reset_index(drop=True) ,pd.DataFrame(y_pred,columns=['Prediction'])], axis=1)

    if alg_name=="XGBoost":
       predictions.to_csv('predictions/RingClassification/RingClassification_XGBoost_predictions_'+dataset_name+'_'+variable_config+'.csv')
    elif alg_name=="Random Forest":
        predictions.to_csv('predictions/RingClassification/RingClassification_RandomForest_predictions_'+dataset_name+'_'+variable_config+'.csv')
    elif alg_name=="SVM Classifier":
        predictions.to_csv('predictions/RingClassification/RingClassification_SVM_predictions_'+dataset_name+'_'+variable_config+'.csv')
    elif alg_name=="MLP Neural network":
        predictions.to_csv('predictions/RingClassification/RingClassification_MLP_predictions_'+dataset_name+'_'+variable_config+'.csv')
    elif alg_name=="GradientBoostingClassifier":
        predictions.to_csv('predictions/RingClassification/RingClassification_GradientBoosting_predictions_'+dataset_name+'_'+variable_config+'.csv')
    elif alg_name=="SGD Classifier":
        predictions.to_csv('predictions/RingClassification/RingClassification_SGD_predictions_'+dataset_name+'_'+variable_config+'.csv')
        
    report = classification_report(y_test, y_pred) 
    print(report)

# ------------------------------------------------------------------
# -------- save_model: Save the trained model to disk --------------
# ------------------------------------------------------------------

# In addition to the model, also the used variable configuration and the scaler properties are saved

def save_model(model, alg_name):

    if alg_name == "XGBoost":
            filename = 'models/RingClassification/ringcounting_model_'+dataset_name+'_'+variable_config+'_XGB.sav'
    elif alg_name=="Random Forest":
            filename = 'models/RingClassification/ringcounting_model_'+dataset_name+'_'+variable_config+'_RandomForest.sav'
    elif alg_name == "SVM Classifier":
            filename = 'models/RingClassification/ringcounting_model_'+dataset_name+'_'+variable_config+'_SVM.sav'
    elif alg_name == "MLP Neural network":
            filename = 'models/RingClassification/ringcounting_model_'+dataset_name+'_'+variable_config+'_MLP.sav'
    elif alg_name == "GradientBoostingClassifier":
            filename = 'models/RingClassification/ringcounting_model_'+dataset_name+'_'+variable_config+'_GradientBoosting.sav'
    elif alg_name == "SGD Classifier":
            filename = 'models/RingClassification/ringcounting_model_'+dataset_name+'_'+variable_config+'_SGD.sav'

    model_objects = (model, subset_classification_vars, scaler)
    pickle.dump(model_objects, open(filename, 'wb'))

# ------------------------------------------------------------------
# -------- plot_correlation: Plot correlation of variables ---------
# ------------------------------------------------------------------

#Always two variables next to each other in the variable list are drawn in the same plot (to be improved)

def plot_correlation(model, alg_name, plot_index, it):
    y_pred = model.predict(X_test)
    accuracy =  accuracy_score(y_test, y_pred) * 100 
    predictions = pd.concat([y_test.reset_index(drop=True) ,pd.DataFrame(y_pred,columns=['Prediction'])], axis=1)
    color_code = {'1-ring':'red', 'multi-ring':'blue'}


    if model_name == "All":
        ax = fig[it].add_subplot(3,2,plot_index) #nrows, ncols, index
    else:
        ax = fig[it].add_subplot(1,1,1)


    colors = [color_code[x] for x in y_test.iloc[:,0]]
    ax.scatter(X_test.iloc[:,2*it], X_test.iloc[:,2*it+1], color=colors, marker='.', label='Circle = Ground truth')
    colors = [color_code[x] for x in y_pred]
    ax.scatter(X_test.iloc[:,2*it], X_test.iloc[:,2*it+1], color=colors, marker='o', facecolors='none', label='Dot = Prediction')
    ax.legend(loc="lower right")
    leg = plt.gca().get_legend()
    ax.set_title(alg_name + ". Accuracy: " + str(accuracy))
    ax.set_xlabel(feature_labels[2*it])
    ax.set_ylabel(feature_labels[2*it+1])

# ------------------------------------------------------------------
# ------------ plot_roc: Calculate & plot ROC curve ----------------
# ------------------------------------------------------------------

def plot_roc(model, alg_name, plot_index):
   
    model.probability = True
    probas = model.predict_proba(X_test)
    fpr, tpr, thresholds = roc_curve(y_test, probas[:, 0], pos_label ="1-ring") #assumes single ring
    roc_auc  = auc(fpr, tpr)
 
    if model_name != "All":
        ax_roc = fig_roc.add_subplot(1,1,1)
    else:
        ax_roc = fig_roc.add_subplot(3,2,plot_index)

    ax_roc.plot(fpr, tpr, label='%s ROC (area = %0.2f)' % (alg_name, roc_auc))
    ax_roc.plot([0, 1], [0, 1], 'k--')          #no skill as comparison
    ax_roc.set_xlabel('False Positive Rate')
    ax_roc.set_ylabel('True Positive Rate')
    ax_roc.legend(loc=0, fontsize='small')

# ------------------------------------------------------------------
# ----------- plot_precision_recall: Plot PrecRecall curve ---------
# ------------------------------------------------------------------

def plot_precision_recall(model, alg_name, plot_index):

    model.probability = True
    probas = model.predict_proba(X_test)
    y_pred = model.predict(X_test)
    precision, recall, _ = precision_recall_curve(y_test, probas[:,0],pos_label = "1-ring")
    pr_auc = auc(recall,precision)
    pr_f1 = f1_score(y_test,y_pred,pos_label="1-ring")

    if model_name != "All":
        ax_pr = fig_pr.add_subplot(1,1,1)
    else:
        ax_pr = fig_pr.add_subplot(3,2,plot_index)
    
    ax_pr.plot(recall,precision,label='AUC = %0.2f, f1_score = %0.2f' % (pr_auc,pr_f1))
    ax_pr.set_title('%s Ring Classification' % (alg_name))
    ax_pr.plot([0,1],[1,0.5],'k--')     #no skill as comparison
    ax_pr.set_xlabel('Recall')
    ax_pr.set_ylabel('Precision')
    ax_pr.legend()


#------ Test different models:


# ----- Random Forest ---------------
if model_name is "RandomForest" or model_name == "All":
    model = RandomForestClassifier(n_estimators=100)
    run_model(model, "Random Forest")
    save_model(model, "Random Forest")
    if do_plot_correlation:
        for plot in range(num_plots):
            plot_correlation(model,"Random Forest",1,plot)
    if do_plot_roc:
        plot_roc(model, "Random Forest",1)
    if do_plot_precision_recall:
        plot_precision_recall(model, "Random Forest",1)



# ------ XGBoost ----------------------
if model_name is "XGBoost" or model_name == "All":
    model = XGBClassifier(subsample=0.6, n_estimators=100, min_child_weight=5, max_depth=4, learning_rate=0.15, gamma=0.5, colsample_bytree=1.0)
    run_model(model, "XGBoost")
    save_model(model, "XGBoost")
    if do_plot_correlation:
        for plot in range(num_plots):
            plot_correlation(model, "XGBoost",2,plot)
    if do_plot_roc:
        plot_roc(model, "XGBoost",2)
    if do_plot_precision_recall:
        plot_precision_recall(model, "XGBoost",2)



# ------ SVM Classifier ----------------
if model_name is "SVM" or model_name == "All":
    model = SVC(probability=True)
    run_model(model, "SVM Classifier")
    save_model(model, "SVM Classifier")
    if do_plot_correlation:
        for plot in range(num_plots):
            plot_correlation(model, "SVM Classifier",3,plot)
    if do_plot_roc:
        plot_roc(model, "SVM Classifier",3)
    if do_plot_precision_recall:
        plot_precision_recall(model, "SVM Classifier",3)



# ---------- SGD Classifier -----------------
if model_name is "SGD" or model_name == "All":
    model = OneVsRestClassifier(SGDClassifier(loss="log", max_iter=1000))
    run_model(model, "SGD Classifier")
    save_model(model, "SGD Classifier")
    if do_plot_correlation:
        for plot in range(num_plots):
            plot_correlation(model, "SGD Classifier",4,plot)
    if do_plot_roc:
        plot_roc(model, "SGD Classifier",4)
    if do_plot_precision_recall:
        plot_precision_recall(model, "SGD Classifier",4)


# ----------- Neural network - Multi-layer Perceptron  ------------
if model_name is "MLP" or model_name == "All":
    model = MLPClassifier(hidden_layer_sizes= 100, activation='relu')
    run_model(model, "MLP Neural network")
    save_model(model, "MLP Neural network")
    if do_plot_correlation:
        for plot in range(num_plots):
            plot_correlation(model,"MLP Neural network",5,plot)
    if do_plot_roc:
        plot_roc(model, "MLP Neural network",5)
    if do_plot_precision_recall:
        plot_precision_recall(model, "MLP Neural network",5)


#----------- GradientBoostingClassifier -----------
if model_name is "GradientBoosting" or model_name == "All":
    model = GradientBoostingClassifier(learning_rate=0.01, max_depth=5, n_estimators=200)
    run_model(model, "GradientBoostingClassifier")
    save_model(model, "GradientBoostingClassifier")
    if do_plot_correlation:
        for plot in range(num_plots):
            plot_correlation(model,"GradientBoostingClassifier",6,plot)
    if do_plot_roc:
        plot_roc(model, "GradienBoostingClassifier",6)
    if do_plot_precision_recall:
        plot_precision_recall(model, "GradientBoostingClassifier",6)



#---------Finish plots & save to png-files ----------

if do_plot_correlation:
    for plot in range(num_plots):
        fig[plot].savefig("plots/RingClassification/Correlation/RingClassification_"+model_name+"_"+dataset_name+"_"+variable_config+"_"+feature_labels[2*plot]+"_"+feature_labels[2*plot+1]+".png")

if do_plot_roc:
    fig_roc.savefig("plots/RingClassification/ROC/RingClassification_ROCcurve_"+model_name+"_"+dataset_name+"_"+variable_config+".png")

if do_plot_precision_recall:
    fig_pr.savefig("plots/RingClassification/ROC/RingClassification_PrecRecallcurve_"+model_name+"_"+dataset_name+"_"+variable_config+".png")




######################################################
########## NOT SO WELL PERFORMING MODELS--> BACKUP####
######################################################

# ---- Decision Tree -----------

#if model_name is "DecisionTree" or model_name == "All":
#    model = tree.DecisionTreeClassifier(max_depth=10)
#    run_model(model, "Decision Tree")
#    save_model(model, "Decision Tree")
#    if do_plot_correlation:
#        for plot in range(num_plots):
#            plot_correlation(model,"Decision Tree",7,plot)
#    if do_plot_roc:
#        plot_roc(model, "Decision Tree",7)
#    if do_plot_precision_recall:
#        plot_precision_recall(model, "Decision Tree",7)

#----------- LogisticRegression ------------

#if model_name is "LogisticRegression" or model_name == "All":
#    model = LogisticRegression(penalty='l1', tol=0.01) 
#    run_model(model, "Logistic Regression")
#    save_model(model, "Logistic Regression")
#    if do_plot_correlation:
#        for plot in range(num_plots):
#            plot_correlation(model,"Logistic Regression",8,plot)
#    if do_plot_roc:
#        plot_roc(model, "Logistic Regression",8)
#    if do_plot_precision_recall:
#        plot_precision_recall(model, "Logistic Regression",8)

# --------- Gaussian Naive Bayes ---------

#if model_name is "GaussianNaiveBayes" or model_name == "All":
#    model = GaussianNB()
#    run_model(model, "Gaussian Naive Bayes")
#    save_model(model, "Gaussian Naive Bayes")
#    if do_plot_correlation:
#        for plot in range(num_plots):
#            plot_correlation(model, "Gaussian Naive Bayes",9,plot)
#    if do_plot_roc:
#        plot_roc(model, "Gaussian Naive Bayes",9)
#    if do_plot_precision_recall:
#        plot_precision_recall(model, "Gaussian Naive Bayes",9)

# -------- Nearest Neighbors ----------

#if model_name is "KNN" or model_name == "All":
#    model = neighbors.KNeighborsClassifier(n_neighbors=10)
#    run_model(model, "Nearest Neighbors Classifier")
#    save_model(model, "Nearest Neighbors Classifier")
#    if do_plot_correlation:
#        for plot in range(num_plots):
#            plot_correlation(model, "Nearest Neighbors Classifier",10,plot)
#    if do_plot_roc:
#        plot_roc(model, "Nearest Neighbors Classifier",10)
#    if do_plot_precision_recall:
#        plot_precision_recall(model, "Nearest Neighbors Classifier",10)

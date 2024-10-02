# coding: utf-8

# # Import modules
import pandas as pd
import json


# # Prepare data into pandas DataFrame
dataX = pd.read_csv("/home/jhe/annie/analysis/Muon_vertex/X.txt",sep=',',header=None,names=['id','ai','eta'])   #ai is track segment
dataY = pd.read_csv("/home/jhe/annie/analysis/Muon_vertex/Y.txt",sep=',',header=None,names=['id','truetracklen'])
# dataX['combine'] = dataX[['X','Y']].values.tolist()


# ### Preview dataframes
dataX.head(5)
dataY.head(5)


# # Aggregate data and filter out extremely long and negative tracks
grouped = dataX.groupby('id').agg(list).reset_index()
data = pd.merge(grouped, dataY, on='id')
print("after merge: " + str(len(data)))
criteria = data['truetracklen'] > 1000
data = data[~criteria]
print("after first filter (>1000): " + str(len(data)))
critiera = data['truetracklen'] < 0
data = data[~criteria]
print("after second filter (<0): " + str(len(data)))
print(data.columns)
data.head(10)


# # Prepare data into json format
json_data = data.to_json(orient='index')


# # Save json data into .json file
file_path = './data.json'
with open(file_path, 'w') as json_file:
    json_file.write(json_data)


# # Save pandas DataFrame as csv file
data.to_csv('./data.csv', index=False)


# # Save data into h5 file << USE THIS ONE
data.to_hdf("./data.h5",key='df')


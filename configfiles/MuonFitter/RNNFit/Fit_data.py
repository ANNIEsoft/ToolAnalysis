# coding: utf-8
import numpy as np
import pandas as pd
import torch
from torch.utils.data import DataLoader
from torch.utils.data import Dataset
import torch.nn as nn
import torch.optim as optim
from sklearn.model_selection import train_test_split
import matplotlib.pyplot as plt
import time
import sys

if (len(sys.argv) != 2):
  print(" @@@@@ MISSING ev_ai_eta_R{RUN}.txt FILE !! @@@@@ ")
  print("   syntax: python3 Fit_data.py ev_ai_eta_R{RUN}.txt")
  print("   path: ~/annie/analysis/FILE")
  exit(-1)

DATAFILE = sys.argv[-1]

# ## Extract run number from filename
# ## NOTE: Assumes filename has this structure: ev_ai_eta_R{RUN}.txt
RUN = DATAFILE[12:-4]

# ## Define ManyToOneRNN class
class ManyToOneRNN(nn.Module):
    def __init__(self, input_size, hidden_size, num_layers, output_size):
        super(ManyToOneRNN, self).__init__()
        self.hidden_size = hidden_size
        self.num_layers = num_layers
        self.rnn = nn.RNN(input_size, hidden_size, num_layers, batch_first=True,nonlinearity='relu')
        self.fc = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        # Initialize hidden state with zeros
        h0 = torch.zeros(self.num_layers, x.size(0), self.hidden_size).to(x.device)
        # Forward propagate the RNN
        out, _ = self.rnn(x,h0)

        # Decode the hidden state of the last time step
        out = self.fc(out[:, -1, :])
        return out


# ## Load model
model = torch.load('model.pth')
model.eval()


# ## Load data (needs to be in Tensor format)
data = pd.read_csv(DATAFILE, header=None, names=['evid','cluster_time','ai','eta'])
print(data.head(5))

data = data.groupby(['evid', 'cluster_time']).agg(list).reset_index()
print(data.head(5))

print(data.iloc[0,2:])

# ## Do the fit
# open output file
OUTFILENAME = "tanktrackfitfile_r" + RUN + "_RNN.txt"
out_f = open(OUTFILENAME, "a")

for idx in range(len(data)):
    dataT = torch.tensor(data.iloc[idx,2:]).t()
    dataT.unsqueeze_(0)
    out = model(dataT)
    print(data.iloc[idx,0], out)
    out_f.write(str(data.iloc[idx,0]) + "," + str(data.iloc[idx,1]) + "," + str(out.data.numpy()[0][0]) + "\n")

# close output file
out_f.close()

############################################################
############### EOF 
############################################################

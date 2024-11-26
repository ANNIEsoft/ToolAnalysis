# coding: utf-8

# ## Import modules
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


# ## Load data, using half of the dataset to train
data = pd.read_hdf("data.h5", 'df')
train_df, test_df = train_test_split(data, test_size=0.5)
test_df, CV_df = train_test_split(test_df, test_size=0.5)
# validation used to keep track of training set; monitor accuracy

# ### Preview data
print("len train_df: " + str(len(train_df)))
train_df.head(5)

print("len test_df: " + str(len(test_df)))
test_df.head(5)

print("len CV_df: " + str(len(CV_df)))
CV_df.head(5)

print(data[data['truetracklen'] < 0.])


# ## Define MyDataset class
class MyDataset(Dataset):
    def __init__(self, dataframe):
        self.data = dataframe

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        evid = self.data.iloc[idx,0]    #added to include evid in output file
        features = torch.tensor(self.data.iloc[idx, 1:-1], dtype=torch.float32)
        features = features.t()
        target = torch.tensor(self.data.iloc[idx, -1], dtype=torch.float32)
        return evid, features, target   #added evid as return value

# ### Prepare data for training
batch_size = 1 # Adjust batch size as needed
# sequence_length = 3  # Adjust sequence length as needed
shuffle = True  # Shuffle the data during training (recommended)
dataS = MyDataset(data)
train = MyDataset(train_df)
test = MyDataset(test_df)
CVS = MyDataset(CV_df)
trainloader = DataLoader(train, batch_size=1, shuffle=shuffle) #how much data to train per epoch
testloader = DataLoader(test)
dataloader = DataLoader(dataS, batch_size=1, shuffle=shuffle)
CVloader = DataLoader(CVS)

print(dataloader)


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

# ### Set parameters for training model
cost_list = []
CVcost_list = []
input_size = 2
hidden_size = 4
num_layers = 1
output_size = 1
learning_rate = 0.001   #can tune this for better model
num_epochs = 10000   #default:10000

model = ManyToOneRNN(input_size, hidden_size, num_layers, output_size)
criterion = nn.MSELoss()
optimizer = optim.Adam(model.parameters(), lr=learning_rate)

N = 100    #for printing progress


# ## Define training
def train():
    model.train()

    for epoch in range(num_epochs):
        COST=0
        CVCost = 0
        i = 0
        for ev,dat,target in trainloader:  # Iterate in batches over the training dataset.; EDIT: added ev
            out = model(dat)  # Perform a single forward pass.
            loss = criterion(out, target)  # Compute the loss.
            loss.backward()  # Derive gradients.
            optimizer.step()  # Update parameters based on gradients.
            optimizer.zero_grad()  # Clear gradients.
            COST += loss.data
            i += 1
            # if epoch % 100 == 0 and i ==1 :
                # print("loss is {}".format(loss.data))
                # print("target value is {}".format(target))
                # print("out is {}".format(out))

        cost_list.append(COST)

        #perform a prediction on the validation  data
        for CVID,CVD,CVT in CVloader:

            with torch.no_grad():
               CVout = model(CVD)
            loss = criterion(CVout, CVT)

            CVCost += loss

        CVcost_list.append(CVCost)

        if epoch%N == 0:
            print("epoch number:{}".format(epoch))
            print("validation MSE is {}".format(COST))
            print("train MSE is {}".format(CVCost))


# ## Train model
tstart=time.time()
print("start time={}".format(tstart))
train()
torch.save(model, 'model.pth')    # save model

print((time.time()-tstart))


# ### Plot the loss and accuracy
fig, ax1 = plt.subplots()
color = 'tab:red'
ax1.plot(cost_list[200::100], color=color,label="Train")
ax1.plot(CVcost_list[200::100], color='tab:blue',label="Validation")
ax1.set_xlabel('epoch', color=color)
ax1.set_ylabel('Cost', color=color)
ax1.tick_params(axis='y', color=color)
ax1.legend()
plt.savefig("loss.png", dpi=300)

# want red and blue to be close and cost to be low


# ## Test model
def test(loader):
    model.eval()

    #open output file for fitted track length
    out_f = open("fitbyeye_wcsim_RNN.txt", "a")
    
    diff_list = []
    for evid,data,target in loader:  # Iterate in batches over the training/test dataset.
        with torch.no_grad():
            out = model(data)   # just need this for data
            diff = out - target  # Use the class with highest probability.
            #print(evid[0], out.data.numpy()[0][0], target.data.numpy()[0])   #evid, fit, truelen
            #print(diff)
            out_f.write(str(evid[0]) + "," + str(out.data.numpy()[0][0]) + "\n")
            diff_list.append(diff.data.numpy()[0][0])  # Check against ground-truth labels.
            # diff_list.append(diff.data.numpy())  # Check against ground-truth labels.
    out_f.close()
    return diff_list  # Derive ratio of correct predictions.

diff_list = test(dataloader)


# ### Plot difference btwn model fit and truth info
plt.hist(diff_list)

mean = np.mean(diff_list)
std = np.std(diff_list)

# custom_labels = ['Mean is {}'.format(mean), 'Std is {}'.format(std)]

# Add a legend with custom labels
plt.text(20, 40, f'Mean: {mean:.2f}', fontsize=12, color='red')
plt.text(20, 35, f'Std: {std:.2f}', fontsize=12, color='green')


plt.xlabel("y - yhat")
plt.ylabel("Number of Event")
plt.title("RNN Muon Vetex Reconstruction Performance")
# plt.legend(custom_labels)
plt.savefig("RNN.png")

print("mean: ",mean)
print("std: " + str(std))


# ## Save the model
#torch.save or model.save
#load model in another script and just use


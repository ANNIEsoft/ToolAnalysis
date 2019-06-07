import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import ROOT

infile = "vars_Ereco_04202019.csv"

filein = open(str(infile))
print("number of events: ",filein)
df00=pd.read_csv(filein)
df0=df00[['TrueTrackLengthInWater','DNNRecoLength','lambda_max']]
lambdamax_test = df0['lambda_max']
test_y = df0['TrueTrackLengthInWater']
y_predicted = df0['DNNRecoLength']
#print(type(test_y)," , ", type(y_predicted))

fig, ax = plt.subplots()
ax.scatter(test_y,y_predicted)
ax.plot([test_y.min(),test_y.max()],[test_y.min(),test_y.max()],'k--',lw=3)
ax.set_xlabel('Measured')
ax.set_ylabel('Predicted')
plt.show()
#plt.savefig("plotsRecolengthB/test_recolength.png")

fig, ax = plt.subplots()
ax.scatter(test_y,y_predicted)
ax.plot([test_y.min(),test_y.max()],[test_y.min(),test_y.max()],'k--',lw=3)
ax.set_xlabel('Measured')
ax.set_ylabel('Predicted')
ax.set_xlim(-50.,400.)
ax.set_ylim(-50.,400.)
plt.show()
#plt.savefig("plotsRecolengthB/test_recolength_ZOOM.png")

##plt.hist2d(test_y,y_predicted, bins=100, cmap='Blues')
#fig, ax = plt.subplots()
#h = ax.hist2d(test_y, y_predicted, bins=8800, cmap='Blues')
#plt.colorbar(h[3])
##cb = plt.colorbar()
##cb.set_label('counts in bin')
#ax.set_xlim(50.,200.)
#ax.set_ylim(50.,200.)
#plt.show()

fig, ax = plt.subplots()
ax.scatter(test_y,lambdamax_test)
ax.plot([test_y.min(),test_y.max()],[test_y.min(),test_y.max()],'k--',lw=3)
ax.set_xlabel('Measured')
ax.set_ylabel('Predicted (lambdamax_test)')
ax.set_xlim(-50.,400.)
ax.set_ylim(-50.,400.)
plt.show()
#plt.savefig("plotsRecolengthB/MYrecolength.png")

data = abs(y_predicted-test_y)
dataprev = abs(lambdamax_test-test_y)
#nbins=np.arange(0,200,10)
nbins=np.arange(0,400,20)
##n, bins, patches = plt.hist(data, 100, alpha=1,normed='true')
fig,ax=plt.subplots(ncols=1, sharey=False)#, figsize=(8, 6))
f0=ax.hist(data, nbins, histtype='step', fill=False, color='blue',alpha=0.75) 
f1=ax.hist(dataprev, nbins, histtype='step', fill=False, color='red',alpha=0.75)
#ax.set_xlim(0.,200.)
ax.set_xlabel('$\Delta R = L_{Reco}-L_{MC}$ [cm]')
ax.legend(('NEW','Previous'))
##ax.set_ylabel('Number of Entries [%]')
##ax.xaxis.set_label_coords(0.95, -0.08)
##ax.yaxis.set_label_coords(-0.1, 0.71)

#from scipy.stats import norm
## Fit a normal distribution to the data:
##mu, std = norm.fit(data)

xmin, xmax = plt.xlim()
##x = np.linspace(xmin, xmax, 100)
##p = norm.pdf(x, mu, std)
##plt.plot(x, p, 'k', linewidth=2)
title = "mean = %.2f, std = %.2f, Prev: mean = %.2f, std = %.2f " % (data.mean(), data.std(),dataprev.mean(), dataprev.std())
plt.title(title)
#plt.savefig("plotsRecolengthB/resol_distr2l_WCSim.png")
plt.show()

print("checking..."," len(test_y): ",len(test_y)," len(y_predicted): ", len(y_predicted))
canvas = ROOT.TCanvas()
canvas.cd(1)
th2f = ROOT.TH2F("True_RecoLength", "; MC Track Length [cm]; Reconstructed Track Length [cm]", 40, 0, 400., 40, 0., 400.)
for i in range(len(test_y)):
    th2f.Fill(test_y[i], y_predicted[i])
line = ROOT.TLine(0.,400.,0.,400.)
th2f.SetStats(0)
th2f.Draw("ColZ")
line.SetLineColor(2)
line.Draw("same")
canvas.Draw()
canvas.SaveAs("MClength_newrecolength.png")



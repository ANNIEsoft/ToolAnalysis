import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
#import ROOT
import matplotlib.pylab as pylab
params = {'legend.fontsize': 'x-large',
#          'figure.figsize': (9, 7),
         'axes.labelsize': 'x-large',
         'axes.titlesize':'x-large',
         'xtick.labelsize':'x-large',
         'ytick.labelsize':'x-large'}
pylab.rcParams.update(params)

infile = "outputs_ener/Ereco_results.csv"

filein = open(str(infile))
print("number of events: ",filein)
df00=pd.read_csv(filein)
print("df00.head() ",df00.head())
Etrue = df00['MuonEnergy (MeV)']
Ereco = df00['RecoE (MeV)']

fig, ax = plt.subplots()
ax.scatter(Etrue,Ereco)
ax.plot([Etrue.min(),Etrue.max()],[Etrue.min(),Etrue.max()],'k--',lw=3)
ax.set_xlabel('Measured')
ax.set_ylabel('Predicted')
#plt.show()
fig.savefig('outputs_ener/Etrue_Erecoscatter.png') 
plt.close(fig)

nbins=np.arange(-100,100,2)
data = 100.*((Etrue-Ereco)/(1.*Etrue))
with plt.style.context("seaborn-white"):
#    fig, ax = plt.subplots()
#    Se_data.plot(kind="barh", ax=ax, title="With Border")
     fig0,ax0=plt.subplots(ncols=1, sharey=True)#, figsize=(8, 6))
#cmap = sns.light_palette('b',as_cmap=True) 
     f=ax0.hist(data, nbins, histtype='step', fill=True, color='darkred',alpha=0.75)
     ax0.set_xlim(-100.,100.)
     ax0.set_xlabel('$\Delta E/E$ [%]')
     ax0.set_ylabel('Number of Entries')
     ax0.xaxis.set_label_coords(0.95, -0.08)
     ax0.yaxis.set_label_coords(-0.1, 0.71)
     title = "mean = %.2f, std = %.2f " % (data.mean(), data.std())
     plt.title(title)
     plt.show()
     fig0.savefig('outputs_ener/DE_E_feat.png')   # save the figure to file
     plt.close(fig0)

nbins2=np.arange(0,2000,20)
fig1,ax1=plt.subplots(ncols=1, sharey=False)#, figsize=(8, 6))
f0=ax1.hist(Etrue, nbins2, histtype='step', fill=False, color='blue',alpha=0.75, log=True)
f1=ax1.hist(Ereco, nbins2, histtype='step', fill=False, color='red',alpha=0.75, log=True)
ax1.set_xlim(0.,2000.)
ax1.set_xlabel('Muon Energy [MeV]')
ax1.legend(('MC','Reco'))
plt.show()
fig1.savefig('outputs_ener/resol_Energy.png')   # save the figure to file
plt.close(fig1)

print("--------- Reco Events Profile --------")
count_bad = 0
count_med = 0
count_good = 0
for i,r in data.items():
    if abs(r)>=20.:
       #print("data: ",r," recoVtxFOM: ",recoVtxFOM[i]," deltaVtxR: ",deltaVtxR[i]," deltaAngle: ",deltaAngle[i])
       count_bad +=1    
    if abs(r)>=10. and abs(r)<20.:
       count_med +=1
    if abs(r)<10.:
       count_good +=1
print("Percentage of good events: ", 100.*count_good/len(data))
print("Percentage of med events: ", 100.*count_med/len(data))
print("Percentage of bad events: ", 100.*count_bad/len(data))

print("checking..."," len(Etrue): ",len(Etrue)," len(Ereco): ", len(Ereco))

#canvas = ROOT.TCanvas()
#canvas.cd(1)
#th2f = ROOT.TH2F("True_Reco_Energy", ";  E_{MC} [MeV]; E_{reco} [MeV]", 100, 0, 2000., 100, 0., 2000.)
#for i in range(len(Etrue)):
#    th2f.Fill(Etrue[i], Ereco[i])
#line = ROOT.TLine(0.,0.,2000.,2000.)
#th2f.SetStats(0)
#th2f.Draw("ColZ")
#line.SetLineColor(2)
#canvas.Draw()
#line.Draw("same")
#canvas.SaveAs("outputs_ener/MCE_recoE.png")


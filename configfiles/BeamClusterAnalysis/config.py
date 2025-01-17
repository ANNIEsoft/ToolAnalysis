import sys

run = sys.argv[1]
pi = sys.argv[2]
pf = sys.argv[3]
run_type = sys.argv[4]

# Create TreeMaker config and ToolsConfig based on run type

# -------------------------------------------
file = open('ANNIEEventTreeMakerConfig', "w")

file.write('ANNIEEventTreeMakerVerbosity 0\n')
file.write('\n')
file.write('fillAllTriggers 1\n')
file.write('fill_singleTrigger 0\n')
file.write('fill_singleTriggerWord 0\n')
file.write('fillLAPPDEventsOnly 0\n')
file.write('TankCluster_fill 1\n')
file.write('cluster_TankHitInfo_fill 1\n')
file.write('\n')
file.write('OutputFile BeamCluster_' + str(run) + '_' + str(pi) + '_' + str(pf) + '.ntuple.root\n')
file.write('TankClusterProcessing 1\n')
file.write('TriggerProcessing 1\n')
file.write('TankHitInfo_fill 1\n')
file.write('\n')

if run_type == 'beam' or run_type == 'cosmic' or run_type == 'beam_39':
    file.write('MRDClusterProcessing 1\n')
    file.write('MRDHitInfo_fill 1\n')
    file.write('MRDReco_fill 1\n')
else:
    file.write('MRDClusterProcessing 0\n')
    file.write('MRDHitInfo_fill 0\n')
    file.write('MRDReco_fill 0\n')
file.write('\n')

file.write('SiPMPulseInfo_fill 0\n')
file.write('fillCleanEventsOnly 0\n')
file.write('MCTruth_fill 0\n')
file.write('Reco_fill 1\n')
file.write('TankReco_fill 0\n')
file.write('RecoDebug_fill 0\n')
file.write('muonTruthRecoDiff_fill 0\n')
file.write('isData 1\n')
file.write('HasGenie 0\n')
file.write('LAPPD_MC_fill 0\n')
file.write('\n')

if run_type == 'beam' or run_type == 'laser' or run_type == 'beam_39':
    file.write('LAPPDData_fill 1\n')
    file.write('LAPPDReco_fill 1\n')
    file.write('LAPPD_PPS_fill 1\n')
    file.write('LAPPD_Waveform_fill 1\n')
else:
    file.write('LAPPDData_fill 0\n')
    file.write('LAPPDReco_fill 0\n')
    file.write('LAPPD_PPS_fill 0\n')
    file.write('LAPPD_Waveform_fill 0\n')

if run_type == 'beam' or run_type == 'beam_39':
    file.write('RWMBRF_fill 1\n')
else:
    file.write('RWMBRF_fill 0\n')

file.close()


# -------------------------------------------
file2 = open('ToolsConfig', "w")

file2.write('myLoadANNIEEvent LoadANNIEEvent ./configfiles/BeamClusterAnalysis/LoadANNIEEventConfig\n')
file2.write('myLoadGeometry LoadGeometry ./configfiles/LoadGeometry/LoadGeometryConfig\n')

if run_type == 'beam' or run_type == 'cosmic' or run_type == 'beam_39':
    file2.write('myTimeClustering TimeClustering configfiles/BeamClusterAnalysis/TimeClusteringConfig\n')
    file2.write('myFindMrdTracks FindMrdTracks configfiles/BeamClusterAnalysis/FindMrdTracksConfig\n')

file2.write('myClusterFinder ClusterFinder ./configfiles/BeamClusterAnalysis/ClusterFinderConfig\n')
file2.write('myClusterClassifiers ClusterClassifiers ./configfiles/BeamClusterAnalysis/ClusterClassifiersConfig\n')
file2.write('myEventSelector EventSelector ./configfiles/BeamClusterAnalysis/EventSelectorConfig\n')

if run_type == 'beam' or run_type == 'laser' or run_type == 'beam_39':
    file2.write('LAPPDLoadStore LAPPDLoadStore configfiles/LAPPDProcessedAna/Configs\n')
    file2.write('LAPPDStoreReorder LAPPDStoreReorder configfiles/LAPPDProcessedAna/ConfigStoreReadIn\n')
    file2.write('LAPPDTimeAlignment LAPPDTimeAlignment configfiles/LAPPDProcessedAna/ConfigPreProcess\n')
    file2.write('LAPPDBaseline LAPPDBaseline configfiles/LAPPDProcessedAna/ConfigPreProcess\n')
    file2.write('LAPPDThresReco LAPPDThresReco configfiles/LAPPDProcessedAna/ConfigPlot\n')

if run_type == 'beam' or run_type == 'beam_39':
    file2.write('FitRWMWaveform FitRWMWaveform ./configfiles/BeamClusterAnalysis/FitRWMWaveformConfig\n')

file2.write('ANNIEEventTreeMaker ANNIEEventTreeMaker ./configfiles/BeamClusterAnalysis/ANNIEEventTreeMakerConfig\n')

file2.close()

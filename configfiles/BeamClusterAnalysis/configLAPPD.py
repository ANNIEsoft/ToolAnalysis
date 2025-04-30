import sys

run = sys.argv[1]
pi = sys.argv[2]
pf = sys.argv[3]
run_type = sys.argv[4]

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
file.write('OutputFile BeamCluster_LAPPD_' + str(run) + '_' + str(pi) + '_' + str(pf) + '.lappd.root\n')
file.write('TankClusterProcessing 1\n')
file.write('TriggerProcessing 1\n')
file.write('TankHitInfo_fill 1\n')
file.write('\n')

# if we are running the LAPPDs at this stage, it is either a laser or beam run. Only beam runs need MRD info
if run_type == 'beam' or run_type == 'beam_39':
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

file.write('LAPPDData_fill 1\n')
file.write('LAPPDReco_fill 1\n')
file.write('LAPPD_PPS_fill 1\n')
file.write('LAPPD_Waveform_fill 1\n')

if run_type == 'beam' or run_type == 'beam_39':
    file.write('RWMBRF_fill 1\n')
else:
    file.write('RWMBRF_fill 0\n')

file.close()

#include "DummyTool.h"
#include "ExampleGenerateData.h"
#include "ExampleSaveStore.h"
#include "ExampleSaveRoot.h"
#include "ExampleloadStore.h"
#include "ExamplePrintData.h"
#include "ExampleLoadRoot.h"
#include "PythonScript.h"
#include "LAPPDBaselineSubtract.h"
#include "LAPPDcfd.h"
#include "TSplineFit.h"
#include "TPoly3.h"
#include "TZigZag.h"
#include "TOnePadDisplay.h"
#include "TBandedLE.h"
#include "LAPPDFilter.h"
#include "LAPPDFindPeak.h"
#include "LAPPDIntegratePulse.h"
#include "LAPPDParseACC.h"
#include "LAPPDParseScope.h"
#include "LAPPDSaveROOT.h"
#include "SaveANNIEEvent.h"
#include "LAPPDSim.h"
#include "LAPPDresponse.h"
#include "BeamTimeAna.h"
#include "BeamTimeTreeMaker.h"
#include "BeamTimeTreeReader.h"
#include "FindMrdTracks.h"
#include "LoadWCSim.h"
#include "wcsimT.h"
#include "PrintANNIEEvent.h"
#include "GenerateHits.h"
#include "NeutronStudyReadSandbox.h"
#include "NeutronStudyPMCS.h"
#include "NeutronStudyWriteTree.h"
#include "RawLoader.h"
#include "HeftyTreeReader.h"
#include "Unity_recoANNIE.h"
#include "ADCCalibrator.h"
#include "ADCHitFinder.h"
#include "BeamChecker.h"
#include "IFBeamDBInterface.h"
#include "BeamFetcher.h"
#include "FindTrackLengthInWater.h"
#include "LoadANNIEEvent.h"
#include "PhaseITreeMaker.h"
#include "MrdPaddlePlot.h"
#include "LoadWCSimLAPPD.h"
#include "LAPPDTree.h"
#include "WCSimDemo.h"
#include "DigitBuilder.h"
#include "VtxSeedGenerator.h"
#include "VtxPointPositionFinder.h"
#include "LAPPDlasertestHitFinder.h"
#include "RawLoadToRoot.h"
#include "MRDPulseFinder.h"
#include "LAPPDAnalysis.h"
#include "ExampleOverTool.h"
#include "PhaseIITreeMaker.h"
#include "VertexGeometryCheck.h"
#include "LikelihoodFitterCheck.h"
#include "EventSelector.h"
#include "SaveRecoEvent.h"
#include "VtxExtendedVertexFinder.h"
#include "VtxPointDirectionFinder.h"
#include "VtxPointVertexFinder.h"
#include "LoadCCData.h"
#include "MRDTree.h"
#include "PMTData.h"
//#include "LoadCCData/RunInformation.h"
//#include "LoadCCData/TrigData.h"
#include "HitCleaner.h"
#include "HitResiduals.h"
#include "MonitorReceive.h"
#include "MonitorSimReceive.h"
#include "EventSelectorDoE.h"
#include "DigitBuilderDoE.h"
#include "MonitorMRDTime.h"
#include "MonitorMRDLive.h"
#include "PulseSimulation.h"
#include "PlotLAPPDTimesFromStore.h"
#include "CheckDetectorCounts.h"
#include "MrdDistributions.h"
#include "MCParticleProperties.h"
#include "DigitBuilderROOT.h"
#include "MrdEfficiency.h"
#include "EventDisplay.h"
#include "TankCalibrationDiffuser.h"
#include "TotalLightMap.h"
#include "MrdDiscriminatorScan.h"
#include "MCRecoEventLoader.h"
#include "MonitorMRDEventDisplay.h"
#include "LoadGeometry.h"
#include "LoadRATPAC.h"
#include "WaveformNNLS.h"
#include "TimeClustering.h"
#include "GracefulStop.h"
#include "PhaseIIADCHitFinder.h"
#include "TrackCombiner.h"
#include "SimulatedWaveformDemo.h"
#include "CNNImage.h"
#include "MonitorTankTime.h"
#include "PhaseIIADCCalibrator.h"
#include "MCHitToHitComparer.h"
#include "MCPropertiesToTree.h"
#include "CalcClassificationVars.h"
#include "StoreClassificationVars.h"
//#include "LoadGenieEvent.h"
//#include "PrintGenieEvent.h"
#include "PlotWaveforms.h"
#include "PMTDataDecoder.h"
#include "ANNIEEventBuilder.h"
#include "MRDDataDecoder.h"
#include "PrintADCData.h"
#include "ClusterFinder.h"
#include "RunValidation.h"
#include "AmBeRunStatistics.h"
#include "SimpleTankEnergyCalibrator.h"
#include "BeamClusterPlots.h"
#include "PrintRecoEvent.h"
#include "MrdPaddleEfficiencyPreparer.h"
#include "MrdPaddleEfficiencyCalc.h"
#include "FMVEfficiency.h"
#include "LoadRawData.h"
#include "TriggerDataDecoder.h"
#include "ClusterClassifiers.h"
#include "MRDLoopbackAnalysis.h"
#include "VetoEfficiency.h"
#include "MonitorTrigger.h"
#include "EventClassification.h"
#include "DataSummary.h"
#include "MaxPEPlots.h"
#include "StoreDecodedTimestamps.h"
#include "PlotDecodedTimestamps.h"
#include "LAPPDReorderData.h"
#include "LAPPDStoreReorderData.h"
#include "LAPPDStoreReadIn.h"
#include "LAPPDPlotWaveForms.h"
#include "LAPPDFindT0.h"
#include "LAPPDStoreFindT0.h"
#include "ClusterDummy.h"
#include "LAPPDMakePeds.h"
#include "LAPPDCluster.h"
#include "LAPPDClusterTree.h"
#include "LAPPDPlotWaveForms2D.h"
#include "LAPPDGausBaselineSubtraction.h"
#include "LAPPDASCIIReadIn.h"
#include "BeamDecoder.h"
#include "LoadRunInfo.h"
#include "ApplyMRDEff.h"
#include "MonitorDAQ.h"

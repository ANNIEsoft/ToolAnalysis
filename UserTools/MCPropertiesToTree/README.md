# MCPropertiesToTree

MCPropertiesToTree is a tool to display basic MC Truth variables in a ROOT-based output format. Properties of the simulation file like the direction, energy and positions of primary particles can be accessed via histograms. Furthermore, the detector and particle properties are also stored in a tree to enable a more flexible selection and plotting of data.

## Data

MCPropertiesToTree takes the properties from the `MCParticles` object. To enable a comparison with the detector response, it also accesses the properties `MCHits` (tank PMTs), `MCLAPPDHits` (tank LAPPDs) and `TDCData` (MRD PMTs).

The following information is stored in the `mcproperties` tree:

* E_true (`vector<double>`): True Energies of the particles within the event
* PDG (`vector<int>`): PDG numbers of particles within the event
* ParentPDG (`vector<int>`): PDG numbers of parent particles within the event
* Flag (`vector<int>`): Flag of particles within the event
* NTriggers (`int`): Number of triggers for specific event, specifically the size of the `TriggerData` object
* EventNr (`int`): Event Number
* ParticlePosX (`vector<double>`): True x start positions for particles within the event
* ParticlePosY (`vector<double>`): True y start positions for particles within the event
* ParticlePosZ (`vector<double>`): True z start positions for particles within the event
* ParticleStopPosX (`vector<double>`): True x stop positions for particles within the event
* ParticleStopPosY (`vector<double>`): True y stop positions for particles within the event
* ParticleStopPosZ (`vector<double>`): True z stop positions for particles within the event
* ParticleDirX (`vector<double>`): True x directions for particles within the event
* ParticleDirY (`vector<double>`): True y directions for particles within the event
* ParticleDirZ (`vector<double>`): True z directions for particles within the event
* PDGPrimaries (`vector<int>`): PDG numbers for primary particles
* PDGSecondaries (`vector<int>`): PDG numbers for secondary particles
* NumPrimaries (`int`): Number of primary particles
* NumSecondaries (`int`): Number of secondary particles
* PMTQ (`vector<double>`): Single charges recorded by the PMTs for the event
* PMTT (`vector<double>`): Single times recorded by the PMTs for the event
* PMTQtotal (`double`): Summed charge recorded by all the PMTs for the event
* LAPPDQ (`vector<double>`): Single charges recorded by the LAPPDs for the event
* LAPPDT (`vector<double>`): Single times recorded by the LAPPDs for the event
* LAPPDQtotal (`double`): Total charge recorded by the LAPPDs for the event
* PMTHits (`int`): Number of PMTs that were hit
* LAPPDHits (`int`): Number of hits on LAPPDs
* MRDPaddles (`int`): Number of MRD paddles that were hit
* MRDLayers (`int`): Number of MRD layers that were hit
* MRDClusters (`int`): Number of MRD clusters that were identified by the `TimeClustering` tool
* Prompt (`bool`): Is the current event a prompt trigger (`true`) or a delayed trigger (`false`)
* TriggerTime (`ULong64_t`): The trigger time (in MC with respect to prompt trigger time of `0 ns`)
* NRings (`int`): Number of rings for the event
* MRDStop (`bool`): Did the primary particle stop in the MRD?
* FV (`bool`): Did the event originate in the FV (as defined by the `EventSelector` tool)?
* PMTVol (`bool`): Did the event originate in the PMTVol (as defined by the `EventSelector` tool)?

## Configuration

MCPropertiesToTree needs the following configuration variables

```
verbose 1                 #the verbosity setting of the tool
OutFile mcproperties.root #define the output root filename
```

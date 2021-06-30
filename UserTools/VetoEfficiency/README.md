# VetoEfficiency #

The `VetoEfficiency` tool calculates veto paddle efficiencies for phase I data. It looks for coincidences between one layer of the veto and either a certain number of tank hits or a hit in both MRD layers. If a coincidence is found, it is evaluated whether the corresponding adjacent veto channel in the other layer also saw a hit. In this way, a relative efficiency for all veto channels is calculated.

## Input data ##

`VetoEfficiency` uses the `TDCData` object created by the `LoadCCData` tool and the `RecoADCHits` object from the `RawLoader`,`ADCCalibrator` and `ADCHitFinder` tool as main input variables. It gets information about the minibuffers from the `MinibufferLabels`, `MinibufferTimestamps` objects and hefty information from the `HeftyInfo` object, if available. The object `BeamStatusses` provided by the `BeamChecker` tool is used to select good beam events.

**TDCData** `map<unsigned long, vector<Hit>>`
* Data structure that contains all hits registered by TDCs.

**RecoADCHits** `map<unsigned long, vector<vector<ADCPulse>>>`
* Data structure that contains all hits registered by the ADCs (water PMTs).

**MinibufferLabels** `vector<MinibufferLabel>`
* Vector of minibuffer labels.

**MinibufferTimestamps** `vector<TimeClass> `
* Vector containing the Minibuffer timestamps.

**HeftyInfo** `HeftyInfo`
* Object containing additional information for Hefty runs.

**BeamStatusses** `vector<BeamStatus>`
* `BeamStatus` objects for event beam quality checks.

## Output data ## 

The output information is stored in a ROOT-file in a tree-structure. There are two trees stored in this output file, the `veto_efficiency` tree and the `summary_tree`. While the `veto_efficiency` tree stores more detailed information on an event-by-event basis, the latter `summary_tree` stores information on a file-by-file basis. They have the following variables:

**veto_efficiency**
* hefty_mode `bool`: Is the data in Hefty-mode?
* hefty_trigger_mask `int`: Hefty trigger mask
* event_label `uint8_t`: Event label
* run_number `uint32_t`: Run number
* subrun_number `uint32_t`: Subrun number
* event_number `uint32_t`: Event number
* minibuffer_number `uint32_t`: Minibuffer number
* spill_number `uint32_t`: Spill number
* veto_l1_ids `vector<unsigned long>`: IDs of upstream veto PMTs in this event
* veto_l2_ids `vector<unsigned long>`: IDs of downstream veto PMTs in this event
* event_time_ns `double`: Event time in nanoseconds
* num_veto_l1_hits `int`: Number of hits in upstream veto layer
* num_veto_l2_hits `int`: Number of hits in downstream veto layer
* num_mrdl1_hits `int`: Number of hits in MRD layer 1
* num_mrdl2_hits `int`: Number of hits in MRD layer 2
* tank_charge `double`: Total charge collected by PMTs
* num_unique_water_pmts `int`: Number of unique water PMTs hit in this event
* coincidence_layer `int`: Veto layer that recorded the coincidence with the MRD/tank PMTs

**summary_tree**
* total_POT `ULong64_t`: Total Protons on target for file.
* num_beam_spills `uint32_t`: Number of beam spills
* num_hits_on_l1_PMTs `vector<unsigned long>`: IDs for veto layer 1 IDs
* coincident_hits_on_l2_PMTs `vector<unsigned long>`: IDs for coincident layer 2 IDs
* l2efficiencies `vector<double>`: Efficiencies for layer 2 veto IDs
* num_hits_on_l2_PMTs `vector<unsigned long>`: IDs for veto layer 2 IDs
* coincident_hits_on_l1_PMTs `vector<unsigned long>`: IDs for coincident layer 1 IDs`
* l1efficiencies `vector<double>`: Efficiencies for layer 1 veto IDs

## Configuration ##

VetoEfficiency has the following configuration parameters:

```
verbosity 1
drawHistos 1                  #Should debug histograms be drawn?
plotDirectory ./              #Where should plots/files be saved?
min_unique_water_pmts 10      #How many unique water PMTs need to fire for coincidence condition
min_tank_charge 0.5           #in nC, minimum total charge registered by PMTs for coincidence condition
min_mrdl1_pmts                #minimum number of MRD hits in first layer for coincidence condition
min_mrdl2_pmts                #minimum number of MRD hits in second layer for coincidence condition
coincidence_tolerance 200     #Time window in which coinciences are accepted (in ns, default: 200)
pre_trigger_ns 0              # Time before trigger
useTApplication 0             # Should TApplication be launched?
```

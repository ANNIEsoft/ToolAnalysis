# ClusterSearcher

ClusterSearcher
Creates list of RecoClusters according to provided time- and neighboring- parameters.  Clusters are tagged with an int ClusterMode variable for use to differentiate different forms of clusters, created from multiple iterations of the tool within the ToolChain, using separate configfiles (e.g. ClusterSearcherConfig, ClusterSearcherConfig2, etc., using Clustermode 1, 2, etc. respectively)

## Data

Describe any data formats ClusterSearch creates, destroys, changes, or analyzes. E.G.

**pmt_tubeid_to_chanelkey** from CStore
* Identifies individual PMTs in order to find effective charge in p.e.

**ClusteringParameters** from RecoEvent
* Compares execution parameters to other iterations of this tool within the toolchain

**TrueVertex** from RecoEvent
* True vertex position, used for 

**RecoClusters** Into and from RecoEvent
* List of RecoClusters found based on the configuration parameters provided
* Any RecoClusters created by previous iteration(s) of ClusterSearcher in the toolchain, so that new clusters can be added to the singular list




## Configuration


verbosity                   5
ClusterMode		    1			#searching track-like clusters	
Config                      3			#config type: 1 = pulse height cut, 2 = +neighbour cut, 3 = +cluster cut							
PmtMinPulseHeight           20		#minimum pulse height
PmtNeighbourRadius          60		#digit neighbouring distance [cm]
PmtMinNeighbourDigits       2			#minimum neighbour digits
PmtClusterRadius            60		#digit clustering distance [cm]      
PmtTimeWindowN              10		#neighbouring time window [ns]    
PmtTimeWindowC              100		#clustering time window [ns]           
PmtMinHitsPerCluster        4			#number of hits per cluster								                         
LappdMinPulseHeight         0			#minimum pulse height                             
LappdNeighbourRadius        25              #digit neighbouring distance [cm]              
LappdMinNeighbourDigits     20              #minimum neighbour digits                      
LappdClusterRadius          25              #digit clustering distance [cm]                                    
LappdTimeWindowN            1               #neighbouring time window [ns]                 
LappdTimeWindowC            1               #clustering time window [ns]                   
LappdMinHitsPerCluster      5			#number of digits per cluster	
MinClusterDigits            50		#minimum clustered digits	(LAPPD+PMT)		
MinClusterDigits            4			#minimum clustered digits	(PMT-only)	
FirstRun		    1		#Debug input for differentiating first iteration from later iterations

```

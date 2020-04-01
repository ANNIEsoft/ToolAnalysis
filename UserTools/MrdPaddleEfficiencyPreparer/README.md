# MrdPaddleEfficiencyPreparer

MrdPaddleEfficiencyPreparer utilizes the information provided by the `FindMrdTracks` tool to compute expected and observed event numbers for the single MRD paddles based on whether the paddle activity coincided with the fitted tracks or not. Information is saved in form of ROOT-files and is later further processed by the `MrdPaddleEfficiency` tool.

## Input Data

The input data provided by the `FindMrdTracks` tool is stored in the `MRDTracks` BoostStore:

**m_data->Stores["MRDTracks"]->Get("MRDTracks",theMrdTracks)** `vector<BoostStore>*`
The MrdTracks BoostStore then contains the following information that is accessed
* `Position StartVertex`
* `Position StopVertex`
* `vector<int> PMTsHit`

## Output Data

Based on the start and stop vertex of the tracks, the expected intercepted paddles are calculated alongside the position where the paddle was expected to be hit. It is evaluated whether there was actually an observed hit in the paddle, the information for observed/expected hits is then stored in the following histograms:

* expected_MRDHits (`vector<TH1D>`): The expected paddle hits due to the fitted tracks. Each paddle constitutes one entry in the vector, the single entries in the histogram represent a spatial resolution on the given paddle.
* observed_MRDHits (`vector<TH1D>`): The observed paddle hits. Each paddle constitutes one entry in the vector, the single entries in the histogram represent spatial coordinates on the given paddle.

## Configuration

The main configuration variable is the name of the ROOT-file in which the observed and expected MRD paddle information is about to be stored.

```
OutputFile MRDFile_efficiency
verbosity 1
```

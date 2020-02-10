# MrdPaddlePlot

MrdPaddlePlot uses the `MrdTrackLib` libraries to create MRD event displays. In addition to displaying the hit times for the single paddles in a color-coded way, it also draws reconstructed and true tracks on top of the paddle plot.

## Input

MrdPaddlePlot uses the subevent output from the `FindMrdTracks` tool and then calls SubEvent internal functions to draw the paddle plot, the reconstructed tracks and the true tracks. The subevents are passed on in a `TClonesArray` from `FindMrdTracks` in the CStore.

**MrdSubEventTClonesArray**

The paddle plot images can either be saved in a ROOT-file or saved as images or shown within a TApplication window. There is furthermore the possibility to draw the tracks based on the `annie_v04.gdml` file.

## Configuration

Describe any configuration variables for MrdPaddlePlot.

```
verbosity 1
gdmlpath ../WCSim/annie_v04.gdml
## because we're only interested in drawing tracks over the gdml subvolume representing the hall, 
## (not the world entirely), but we want the coordinates to match those from WCSim
## (which match the world), we need to offset our WCSim positions by the position of the hall within the world
#gdmlpath /annie/app/users/moflaher/wcsim/root_work/annie_v04_aligned.gdml ## has buildingoffsets included
## units cm
buildingoffsetx 206.82
buildingoffsety 54.3225
buildingoffsetz 243.43894

# save images of hit paddles in each track identified. ONLY FOR SMALL NUM TRACKS AS DEBUG!
drawPaddlePlot 1        # draw the top/side view paddle plot of tracks
drawGdmlOverlay 0       # draw a 3D representation of the ANNIE detector overlaid with MRD tracks
drawStatistics 1        # histograms plotting some debug statistics about the track reconstruction
saveimages 0            # save the paddle plots as images.
saverootfile 0          # save the paddle plots within a rootfile.
plotDirectory ./MrdPlots  # where to save plot images
printTClonesTracks 0    # verbose printout of track information from TClonesArray
useTApplication 0       # launch a TApplication to directly show the plots (slow on cluster)
OutputROOTFile MRDPaddlePlots_MRDTest28_cluster30ns
DrawOnlyTracks 1        # should subevents only be drawn if there was a track found?
```

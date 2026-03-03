#!/bin/bash

#singularity shell --cleanenv -B/pnfs:/pnfs,/cvmfs:/cvmfs,/annie/data:/annie/data,/annie/app:/annie/app /containers/toolanalysis_latest_29_08_19_sandbox

singularity shell -B/pnfs:/pnfs,/cvmfs:/cvmfs,/exp/annie/data:/exp/annie/data,/exp/annie/app:/exp/annie/app /cvmfs/singularity.opensciencegrid.org/anniesoft/toolanalysis\:latest/


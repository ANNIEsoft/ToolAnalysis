# Configure files

***********************
#Description
**********************

Shows an interactive viewer that allows the user to view the minibuffer waveforms. 
The user can select which card, and which minibuffer, will be shown.
Ported from the recoANNIE RawViewer by Steven Gardiner.

************************
#Useage
************************

Double-click the minibuffer from the list on the RHS to change the displayed waveform.
Click 'next readout' to allow the ToolChain to proceed to the next readout.
Click 'close viewer' to close the Waveform viewer and allow the ToolChain to process the remaining events.

***********************
#Compilation
***********************

This tool may require recompilation of the RawViewer library in new environments:
  source Setup.sh
  cd UserTools/PlotWaveforms
  source setup_builder.sh
  make
then proceed to build ToolAnalysis as normal

***********************
#Container notes!
***********************

When running this tool from within a container, some additional setup may be required.
Your host X server must be configured to accept indirect GL content.
Find the appropriate location for xorg.conf.d files on your distribution (e.g. /usr/share/X11/xorg.conf.d)
and within this directory create a file named `10-glx.conf` with the following contents:

  Section "ServerFlags"
      Option "IndirectGLX" "on"
  EndSection

Restart the X server to allow the changes come into effect.
Next, in the container, export the following environmental variable:

  export LIBGL_ALWAYS_INDIRECT=1

Run the ToolChain with the PlotWaveforms viewer and the tool should function as expected.

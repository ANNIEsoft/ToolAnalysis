# ROOT scripts

The following root scripts were used for the neutron multiplicity analysis before the ToolAnalysis implementation. The single root scripts can be executed via

`root -l 'script_name.C("input_file.root","output_file.root",verbose_bool)'`

in the terminal.

The different scripts will produce different diagrams in the output files:

* `plot_neutrons_data_beam.C`: Regular neutron multiplicity plots on data (beam events)
* `plot_neutrons_data_dirt.C`: Regular neutron multiplicity plots on data (dirt events)
* `plot_neutrons_data_mc.C`: Regular neutron multiplicity plots on MC (simulated beam events)
* `neutrino_selection.C`: Histograms which show the time and charge distributions after the various neutrino candidate selection cuts
* `reconstruction_migration.C`: Migration plots for the different reconstructed variables (energy, cos(theta))
* `visible_energy_vertex.C`: Histograms of the visible energy as a function of the vertex coordinates

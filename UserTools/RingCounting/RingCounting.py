########################################################################################################################
#
# 8888888b.  d8b                         .d8888b.                             888    d8b
# 888   Y88b Y8P                        d88P  Y88b                            888    Y8P
# 888    888                            888    888                            888
# 888   d88P 888 88888b.   .d88b.       888         .d88b.  888  888 88888b.  888888 888 88888b.   .d88b.
# 8888888P"  888 888 "88b d88P"88b      888        d88""88b 888  888 888 "88b 888    888 888 "88b d88P"88b
# 888 T88b   888 888  888 888  888      888    888 888  888 888  888 888  888 888    888 888  888 888  888
# 888  T88b  888 888  888 Y88b 888      Y88b  d88P Y88..88P Y88b 888 888  888 Y88b.  888 888  888 Y88b 888
# 888   T88b 888 888  888  "Y88888       "Y8888P"   "Y88P"   "Y88888 888  888  "Y888 888 888  888  "Y88888
#                              888                                                                     888
#                         Y8b d88P                                                                Y8b d88P
#                          "Y88P"                                                                  "Y88P"
# --------------------------------------------------------------------------------------------------------------------
#                  Daniel Tobias Schmid, Feb. 2023, dschmid@fnal.gov / d.schmid@students.uni-mainz.de
# --------------------------------------------------------------------------------------------------------------------
#
# The Cherenkov-ring-counting tool is used to classify events as single- or multi-ring by analyzing PMT hit maps loaded
#   from a CSV file of CNNImage displays. It uses a keras based neural network. To use this tool in a ToolChain, users
#   must populate the the corresponding "/configfiles/YourToolChain/RingCountingConfig" file with the following
#   information:
#     (Square brackets imply a variable in the config file)
#     1. The path to a file containing a list of files of the PMT data to be used.Must be in CNNImage format.
#        -> defined by setting [[files_to_load]]
#     2. The model version
#        -> defined by setting [[version]]
#        Check the documentation!
#     3. The model directory path (not including the model's filename, but including last slash)
#        -> defined by setting [[model_path]]
#     4. Which PMT mask to use (some PMTs have been turned off in the training); check documentation for which model
#        requires what mask.
#       -> defined by setting [[pmt_mask]]
#     5. Where to save the predictions.
#       -> defined by setting [[save_to]]
#   An example config file can also be found in in the RingCountingStore/documentation/ folders mentioned below.
#
# Documentation on the tool, model versions and performance can be found in (anniegpvm-machine):
#     /pnfs/annie/persistent/users/dschmid/RingCountingStore/documentation/ **TODO**
# All models are located in (anniegpvm-machine):
#     /pnfs/annie/persistent/users/dschmid/RingCountingStore/models/
#
########################################################################################################################
from Tool import *

import numpy as np
import tensorflow as tf


class RingCountingGlobals:
    # The reason to mask some PMTs to 0 is to disable PMTs that differ in the MC and experimental datasets.
    #   If a model has been trained with a specific mask toggled on, it is crucial that when evaluating experimental
    #   data with this model, that the same specific mask is also toggled on. This is due to the model not having
    #   encountered these PMTs being enabled during training, and thus probably producing worse predictions with
    #   the un-encountered PMTs enabled when predicting.
    PMT_MASKS = {
        "none": [],
        # From per-PMT p.e. distribution histograms -> which PMTs have different MC / data curves
        "pe_curve_divergent_november_22": [56, 58, 84, 85, 87, 88, 89, 100, 118, 120, 121, 137, 138, 139],
        # Difference in MC and data on specific datasets used for training
        "active_divergent_november_22": [22, 124],
        # Both november 22 lists
        "november_22": [22, 56, 58, 84, 85, 87, 88, 89, 100, 118, 120, 121, 124, 137, 138, 139],
        # Load from config TODO
        "custom": []
    }


class RingCounting(Tool, RingCountingGlobals):
    # ----------------------------------------------------------------------------------------------------
    # Data stuff
    cnn_image_pmt = None  # To be loaded with PMT data

    # ----------------------------------------------------------------------------------------------------
    # Config stuff
    files_to_load = std.string()  # List of files to be loaded (must be in CNNImage format)
    version = std.string()  # Model version
    model_path = std.string()  # Path to model directory
    pmt_mask = std.string()  # See RingCountingGlobals
    save_to = std.string()  # Where to save the predictions to

    # ----------------------------------------------------------------------------------------------------
    # Model stuff
    model = None
    predicted = None

    def Initialise(self):
        """ Initialise RingCounting tool object in following these steps:
        1. Load necessary config
        2. Load data
          2.1 Modify data
        3. Load model

        For specific details, see compartmentalised function docstrings.
        """
        # ----------------------------------------------------------------------------------------------------
        # Debug area
        self.m_log.Log(__file__ + " Initialising", self.v_debug, self.m_verbosity)

        # ----------------------------------------------------------------------------------------------------
        # Config area
        self.m_variables.Get("files_to_load", self.files_to_load)
        self.files_to_load = str(self.files_to_load)  # cast to str since std.string =/= str
        self.m_variables.Get("version", self.version)
        self.m_variables.Get("model_path", self.model_path)
        self.m_variables.Get("pmt_mask", self.pmt_mask)
        self.m_variables.Get("save_to", self.save_to)
        self.save_to = str(self.save_to)  # cast to str since std.string =/= str
        self.pmt_mask = self.PMT_MASKS[self.pmt_mask]

        # ----------------------------------------------------------------------------------------------------
        # Loading data
        self.load_data()
        self.mask_pmts()

        # ----------------------------------------------------------------------------------------------------
        # Loading model
        self.load_model()

        return 1

    def load_data(self):
        """ Load data in the CNNImage format.

        The files to load are specified in a txt file which filepath is defined in the RingCountingConfig file under
        [[files_to_load]]. The CNNImage format is defined in the CNNImage tool. In short, a 160-entry list of numbers
        (cast-able to numpy float-types) in comma separated format, where events are separated by newlines, is expected.

        Uncommenting a filepath in the [[files_to_load]] file is supported by placing "#" as the first character.

        Handles empty files by skipping and logging a warning.
        """
        self.cnn_image_pmt = np.array([])
        # Removes trailing and leading whitespace and "\n"
        self.files_to_load = self.files_to_load.strip(" \n")
        with open(self.files_to_load, "r") as file:
            for data_path in file:
                # Removes trailing and leading whitespace and "\n"
                data_path = data_path.strip(" \n")
                # Supports commenting out a file
                if data_path[0] == "#":
                    continue

                arr = np.loadtxt(data_path, delimiter=",")
                if arr.shape == (160,):
                    # Explicitly adding the extra dimension using np.array([]) is done to ensure the data is
                    #   reshaped into the shape (-1, 160) in case of having only a single entry.
                    arr = np.array([arr])
                arr = np.reshape(arr, (-1, 160))

                if len(self.cnn_image_pmt) > 0:
                    self.cnn_image_pmt = np.concatenate([arr, self.cnn_image_pmt], axis=0)
                    self.m_log.Log(__file__ + f" Successfully loaded CNNImage file {data_path} with {len(arr)} events.",
                                   self.v_debug, self.m_verbosity)
                elif len(arr) > 0:
                    self.m_log.Log(__file__ + f" Successfully loaded CNNImage file {data_path} with {len(arr)} events.",
                                   self.v_debug, self.m_verbosity)
                    self.cnn_image_pmt = arr
                else:
                    self.m_log.Log("WARNING: " + __file__ + f" Attempted to load an empty PMT-datafile (CNNImage). "
                                                            f"({data_path})",
                                   self.v_debug, self.m_verbosity)

    def save_data(self):
        """ Save the data to the specified [[save_to]]-file. """
        np.savetxt(self.save_to, self.predicted, delimiter=",")

    def mask_pmts(self):
        """ Mask PMTs to 0. The PMTs to be masked is given as a list of indices, defined by setting [[pmt_mask]].
        For further details check the RingCountingGlobals class.
        """
        for event in self.cnn_image_pmt:
            np.put(event, self.pmt_mask, 0, mode='raise')

    def load_model(self):
        """ Load the specified model [[version]]."""
        self.model = tf.keras.models.load_model(self.model_path + f"RC_model_v{self.version}.model")

    def predict(self):
        """
        Classify events in single- and multi-ring events using a keras model. Save a list of 2-dimensional predictions
        (same order as input) to self.predicted. Predictions are given as [MR prediction, SR prediction].
        """
        print("Predicting...")
        print(self.cnn_image_pmt)
        self.predicted = self.model.predict(np.reshape(self.cnn_image_pmt, newshape=(-1, 10, 16, 1)))

    def Execute(self):
        """ Execute the tool by generating model predictions on the supplied data. """
        self.m_log.Log(__file__ + " Executing", self.v_debug, self.m_verbosity)
        self.predict()

        return 1

    def Finalise(self):
        """ Finalise the tool by saving the predictions. """
        self.m_log.Log(__file__ + " Finalising", self.v_debug, self.m_verbosity)
        self.save_data()

        return 1


###################
# ↓ Boilerplate ↓ #
###################


thistool = RingCounting()


def SetToolChainVars(m_data_in, m_variables_in, m_log_in):
    return thistool.SetToolChainVars(m_data_in, m_variables_in, m_log_in)


def Initialise():
    return thistool.Initialise()


def Execute():
    return thistool.Execute()


def Finalise():
    return thistool.Finalise()

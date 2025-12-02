"""
HDF5 Manager module.

This module provides interfaces for writing Tudat simulation results to HDF5 files
with XDMF visualization support for ParaView.

Classes
-------
HDF5OutputFile
    Class for writing simulation results to HDF5 files.
XDMFAttributeType
    Enumeration of XDMF attribute types (Scalar, Vector, Tensor, Matrix).
XDMFAttributeConfig
    Configuration for an attribute on an XDMF grid.
TrajectoryConfig
    Configuration for a trajectory in XDMF output.
ObservationXDMFConfig
    Configuration for an observation set in XDMF output.

Example
-------
>>> from tudatpy.data.hdf5_manager import HDF5OutputFile
>>>
>>> # Create HDF5 output file
>>> hdf5_file = HDF5OutputFile("/path/to/output.h5", overwrite=True)
>>>
>>> # Add observation collection
>>> hdf5_file.add_observation_collection(obs_collection, "my_observations")
>>>
>>> # Generate XDMF for ParaView visualization
>>> hdf5_file.generate_observation_xdmf("/path/to/output.xdmf")
>>>
>>> # Close the file
>>> hdf5_file.close()
"""

from tudatpy.kernel.data.hdf5_manager import *

__all__ = [
    "HDF5OutputFile",
    "XDMFAttributeType",
    "XDMFAttributeConfig",
    "TrajectoryConfig",
    "ObservationXDMFConfig",
]

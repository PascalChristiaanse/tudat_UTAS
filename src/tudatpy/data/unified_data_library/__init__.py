"""
Unified Data Library reader module.

This module provides interfaces for reading and processing VLBI observation data
from the Unified Data Library (UDL) format, particularly UTAS VLBI observations.

Classes
-------
BatchVLBI
    Convenience class for loading batch VLBI observations from JSON files.
UTASObservationCollection
    Collection of UTAS observation sets organized by target and station pairs.
UTASObservationSet
    Single UTAS observation set with metadata and time series data.
GeodeticPosition
    Structure representing geodetic coordinates (longitude, latitude, altitude).
UDLObservationMetadata
    Metadata for observation sets (stations, target, frequency, etc.).
UDLTimeSeries
    Time series data (epochs, TDOA, FDOA, uncertainties).

Example
-------
>>> from tudatpy.data.unified_data_library import BatchVLBI
>>> from tudatpy.numerical_simulation.environment_setup import SystemOfBodies
>>>
>>> # Load VLBI observations
>>> vlbi = BatchVLBI(["/path/to/observations.json"])
>>>
>>> # Convert to Tudat format (creates ground stations automatically)
>>> obs_collection = vlbi.to_tudat(bodies, station_body="Earth")
"""

from tudatpy.kernel.data.unified_data_library import *

__all__ = [
    "GeodeticPosition",
    "UDLObservationMetadata",
    "UDLTimeSeries",
    "UTASObservationSet",
    "UTASObservationCollection",
    "BatchVLBI",
]
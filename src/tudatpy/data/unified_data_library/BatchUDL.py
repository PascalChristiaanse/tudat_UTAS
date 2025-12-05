"""
Pure Python wrapper for BatchVLBI providing access to C++ VLBI observation loading.

This module provides the BatchVLBI class which wraps the C++ UTASObservationCollection
and UTASTudatFormatter for conversion to Tudat observation format.
"""

from typing import List, Optional

from tudatpy.kernel.data.unified_data_library import (
    UTASObservationCollection,
    UTASTudatFormatter,
)


class BatchVLBI:
    """
    Convenience class for loading and converting batch VLBI observations.

    Combines UTASObservationCollection and UTASTudatFormatter for easy use.
    This is the recommended interface for loading UDL VLBI data.

    Example
    -------
    >>> from tudatpy.data.unified_data_library import BatchVLBI
    >>>
    >>> # Load observations from JSON files
    >>> vlbi = BatchVLBI(["/path/to/obs1.json", "/path/to/obs2.json"])
    >>>
    >>> # Convert to Tudat observation collection (also creates ground stations)
    >>> obs_collection = vlbi.to_tudat(bodies)

    Parameters
    ----------
    file_paths : list[str]
        List of paths to JSON files containing VLBI observation data.
    """

    def __init__(self, file_paths: List[str]):
        """
        Construct from a list of JSON file paths.

        Parameters
        ----------
        file_paths : list[str]
            List of paths to JSON files containing VLBI observation data.
        """
        self._collection = UTASObservationCollection(file_paths)
        self._formatter = UTASTudatFormatter()

    def to_tudat(
        self,
        bodies,
        included_targets: Optional[List[str]] = None,
        station_body: str = "Earth",
    ):
        """
        Convert to Tudat observation collection.

        Creates ground stations on the specified body and returns observations
        in Tudat format for use with estimation.

        Parameters
        ----------
        bodies : SystemOfBodies
            System of bodies (will be modified to add ground stations).
        included_targets : list[str], optional
            List of target IDs to include. If empty or None, all targets are included.
        station_body : str, default="Earth"
            Name of the body on which to place ground stations.

        Returns
        -------
        ObservationCollection
            Tudat observation collection ready for use with estimation.
        """
        if included_targets is None:
            included_targets = []

        return self._formatter.to_tudat(
            self._collection, bodies, included_targets, station_body
        )

    def get_collection(self) -> UTASObservationCollection:
        """
        Get the underlying observation collection.

        Returns
        -------
        UTASObservationCollection
            The raw observation collection.
        """
        return self._collection

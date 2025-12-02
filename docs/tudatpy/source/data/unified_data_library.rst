.. _unified_data_library:

``unified_data_library``
========================
Interface for loading observation data from the Unified Data Library (UDL) format.

This module provides classes for reading VLBI (Very Long Baseline Interferometry)
observation data from JSON files in the UDL format. It supports TDOA (Time Difference
of Arrival) and FDOA (Frequency Difference of Arrival) measurements, and can convert
these observations to Tudat's native observation format for use in orbit determination.

.. note:: The primary entry point for most users is the :class:`BatchVLBI` class,
          which handles file loading, parsing, and conversion to Tudat format in
          a single convenient interface.


Notes
-----

The UDL format stores VLBI observations in JSON files with the following structure:

- **Metadata**: Station identifiers, positions, target ID, frequency, etc.
- **Time Series**: Epochs (seconds since J2000 TDB), TDOA/FDOA values and uncertainties

When converting to Tudat format using :meth:`BatchVLBI.to_tudat`:

1. Ground stations are automatically created on the specified body (default: Earth)
2. Link definitions are created for each station-target combination
3. Observations are packaged into a Tudat :class:`ObservationCollection`

+-------------------+-----------------------------------------------------------+
| Observable Type   | Description                                               |
+-------------------+-----------------------------------------------------------+
| TDOA              | Time Difference of Arrival between two receivers (s)     |
+-------------------+-----------------------------------------------------------+
| FDOA              | Frequency Difference of Arrival between two receivers (Hz)|
+-------------------+-----------------------------------------------------------+



Classes
-------
.. currentmodule:: tudatpy.data.unified_data_library

.. autosummary::

   BatchVLBI

   UTASObservationCollection

   UTASObservationSet

   UDLObservationMetadata

   UDLTimeSeries

   GeodeticPosition



.. autoclass:: tudatpy.data.unified_data_library.BatchVLBI
   :members:
   :special-members: __init__

.. autoclass:: tudatpy.data.unified_data_library.UTASObservationCollection
   :members:
   :special-members: __init__

.. autoclass:: tudatpy.data.unified_data_library.UTASObservationSet
   :members:

.. autoclass:: tudatpy.data.unified_data_library.UDLObservationMetadata
   :members:

.. autoclass:: tudatpy.data.unified_data_library.UDLTimeSeries
   :members:
   :special-members: __len__

.. autoclass:: tudatpy.data.unified_data_library.GeodeticPosition
   :members:
   :special-members: __init__

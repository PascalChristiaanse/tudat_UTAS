.. _hdf5_output:

``hdf5_manager``
================
Interface for writing simulation results to HDF5 files with XDMF visualization support.

This module provides the :class:`HDF5OutputFile` class for exporting propagation results,
observation data, and related simulation outputs to HDF5 format. The exported files are
compatible with XDMF3, enabling direct visualization in ParaView.

.. note:: The primary class is :class:`HDF5OutputFile`, which handles file creation,
          data export, and XDMF generation in a unified interface.


Notes
-----

The HDF5 output structure follows a hierarchical organization:

**Trajectory Data** (``/Trajectories/``)

- ``SingleArcSimulationResults/<BodyName>/`` - State and dependent variable histories
- ``VariationalEquationsSimulationResults/<BodyName>/`` - Including STM and sensitivity matrices

**Observation Data** (``/Observations/``)

- ``SingleObservationSets/<SetName>/`` - Individual observation sets
- ``ObservationCollections/<CollectionName>/`` - Collections organized by observable type

Each observation set contains:

+----------------------+-------------------------------------------------------+
| Dataset              | Description                                           |
+----------------------+-------------------------------------------------------+
| observations         | Observation values (2D array)                         |
+----------------------+-------------------------------------------------------+
| times                | Observation epochs (1D array)                         |
+----------------------+-------------------------------------------------------+
| weights              | Observation weights (2D array)                        |
+----------------------+-------------------------------------------------------+
| residuals            | Observation residuals (2D array)                      |
+----------------------+-------------------------------------------------------+
| dependent_variables  | Optional dependent variables (2D array)               |
+----------------------+-------------------------------------------------------+

XDMF files can be generated for ParaView visualization:

- Trajectory XDMF shows trajectories as polylines with velocity vectors
- Observation XDMF shows observations as point clouds with time as geometry



Classes
-------
.. currentmodule:: tudatpy.data.hdf5_manager

.. autosummary::

   HDF5OutputFile

   XDMFAttributeType

   XDMFAttributeConfig

   TrajectoryConfig

   ObservationXDMFConfig



.. autoclass:: tudatpy.data.hdf5_manager.HDF5OutputFile
   :members:
   :special-members: __init__

.. autoclass:: tudatpy.data.hdf5_manager.XDMFAttributeType
   :members:
   :undoc-members:

.. autoclass:: tudatpy.data.hdf5_manager.XDMFAttributeConfig
   :members:

.. autoclass:: tudatpy.data.hdf5_manager.TrajectoryConfig
   :members:

.. autoclass:: tudatpy.data.hdf5_manager.ObservationXDMFConfig
   :members:

/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rights reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#define PYBIND11_DETAILED_ERROR_MESSAGES
#include "expose_hdf5_manager.h"

#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>

#include "scalarTypes.h"

#include <tudat/io/hdf5Manager.h>
#include <tudat/io/xdmfGenerator.h>
#include <tudat/basics/timeType.h>

namespace py = pybind11;
namespace tio = tudat::io;

namespace tudatpy
{
namespace data
{
namespace hdf5_manager
{

void expose_hdf5_manager( py::module& m )
{
    // =========================================================================
    // XDMFAttributeType enum
    // =========================================================================
    py::enum_< tio::XDMFAttributeType >( m, "XDMFAttributeType",
            R"doc(

Type of attribute for XDMF visualization.

Used to specify how data should be interpreted in ParaView.

)doc" )
            .value( "scalar_type", tio::XDMFAttributeType::Scalar,
                    R"doc(Scalar attribute (single value per point).)doc" )
            .value( "vector_type", tio::XDMFAttributeType::Vector,
                    R"doc(Vector attribute (3 values per point).)doc" )
            .value( "tensor_type", tio::XDMFAttributeType::Tensor,
                    R"doc(Tensor attribute.)doc" )
            .value( "matrix_type", tio::XDMFAttributeType::Matrix,
                    R"doc(Matrix attribute.)doc" )
            .export_values( );

    // =========================================================================
    // XDMFAttributeConfig struct
    // =========================================================================
    py::class_< tio::XDMFAttributeConfig >( m, "XDMFAttributeConfig",
            R"doc(

Configuration for an attribute on an XDMF grid.

Defines how a dataset column or set of columns maps to a ParaView attribute.

)doc" )
            .def_readonly( "name", &tio::XDMFAttributeConfig::name,
                    R"doc(Attribute name displayed in ParaView.)doc" )
            .def_readonly( "type", &tio::XDMFAttributeConfig::type,
                    R"doc(Attribute type (Scalar, Vector, Tensor, Matrix).)doc" )
            .def_readonly( "h5_file_path", &tio::XDMFAttributeConfig::h5FilePath,
                    R"doc(Path to the HDF5 file containing the data.)doc" )
            .def_readonly( "dataset_path", &tio::XDMFAttributeConfig::datasetPath,
                    R"doc(Path to the dataset within the HDF5 file.)doc" )
            .def_readonly( "column_indices", &tio::XDMFAttributeConfig::columnIndices,
                    R"doc(Column indices to extract from 2D dataset.)doc" )
            .def_readonly( "num_elements", &tio::XDMFAttributeConfig::numElements,
                    R"doc(Number of elements (rows) in the dataset.)doc" );

    // =========================================================================
    // TrajectoryConfig struct
    // =========================================================================
    py::class_< tio::TrajectoryConfig >( m, "TrajectoryConfig",
            R"doc(

Configuration for a trajectory in XDMF output.

Contains all information needed to generate XDMF visualization for a trajectory.

)doc" )
            .def_readonly( "body_name", &tio::TrajectoryConfig::bodyName,
                    R"doc(Name of the body (used in grid naming).)doc" )
            .def_readonly( "h5_file_path", &tio::TrajectoryConfig::h5FilePath,
                    R"doc(Path to the HDF5 file containing trajectory data.)doc" )
            .def_readonly( "states_dataset", &tio::TrajectoryConfig::statesDataset,
                    R"doc(Path to the states dataset in HDF5.)doc" )
            .def_readonly( "times_dataset", &tio::TrajectoryConfig::timesDataset,
                    R"doc(Path to the times dataset in HDF5.)doc" )
            .def_readonly( "num_time_steps", &tio::TrajectoryConfig::numTimeSteps,
                    R"doc(Number of time steps in the trajectory.)doc" )
            .def_readonly( "state_size", &tio::TrajectoryConfig::stateSize,
                    R"doc(Size of state vector (typically 6 for pos+vel).)doc" )
            .def_readonly( "dependent_variables_dataset", &tio::TrajectoryConfig::dependentVariablesDataset,
                    R"doc(Path to dependent variables dataset (optional).)doc" )
            .def_readonly( "dependent_variables_size", &tio::TrajectoryConfig::dependentVariablesSize,
                    R"doc(Size of dependent variables vector.)doc" )
            .def_readonly( "dependent_variable_attributes", &tio::TrajectoryConfig::dependentVariableAttributes,
                    R"doc(XDMF attribute configs for dependent variables.)doc" );

    // =========================================================================
    // ObservationXDMFConfig struct
    // =========================================================================
    py::class_< tio::ObservationXDMFConfig >( m, "ObservationXDMFConfig",
            R"doc(

Configuration for an observation set in XDMF output.

Contains all information needed to generate XDMF visualization for observations.

)doc" )
            .def_readonly( "set_name", &tio::ObservationXDMFConfig::setName,
                    R"doc(Name of the observation set (used in grid naming).)doc" )
            .def_readonly( "observable_type_name", &tio::ObservationXDMFConfig::observableTypeName,
                    R"doc(Name of the observable type.)doc" )
            .def_readonly( "h5_file_path", &tio::ObservationXDMFConfig::h5FilePath,
                    R"doc(Path to the HDF5 file.)doc" )
            .def_readonly( "h5_group_path", &tio::ObservationXDMFConfig::h5GroupPath,
                    R"doc(Path to the observation set group in HDF5.)doc" )
            .def_readonly( "num_observations", &tio::ObservationXDMFConfig::numObservations,
                    R"doc(Number of observations.)doc" )
            .def_readonly( "observable_size", &tio::ObservationXDMFConfig::observableSize,
                    R"doc(Size of each observation.)doc" )
            .def_readonly( "has_weights", &tio::ObservationXDMFConfig::hasWeights,
                    R"doc(Whether weights are available.)doc" )
            .def_readonly( "has_residuals", &tio::ObservationXDMFConfig::hasResiduals,
                    R"doc(Whether residuals are available.)doc" )
            .def_readonly( "has_dependent_variables", &tio::ObservationXDMFConfig::hasDependentVariables,
                    R"doc(Whether dependent variables are available.)doc" )
            .def_readonly( "dependent_variable_attributes", &tio::ObservationXDMFConfig::dependentVariableAttributes,
                    R"doc(XDMF attribute configs for dependent variables.)doc" );

    // =========================================================================
    // HDF5OutputFile class
    // =========================================================================
    py::class_< tio::HDF5OutputFile >( m, "HDF5OutputFile",
            R"doc(

Class for writing Tudat simulation results to HDF5 files.

Provides a hierarchical structure for storing propagation and observation results
with XDMF3 compatibility for ParaView visualization.

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

)doc" )
            .def( py::init< const std::string&, bool >( ),
                  py::arg( "file_path" ),
                  py::arg( "overwrite" ) = true,
                  R"doc(

Construct a new HDF5OutputFile.

Parameters
----------
file_path : str
    Path to the HDF5 file to create.
overwrite : bool, default=True
    If True, overwrite existing file; otherwise append.

)doc" )
            .def( "add_single_arc_results",
                  &tio::HDF5OutputFile::addSingleArcResults< double, TIME_TYPE >,
                  py::arg( "results" ),
                  py::arg( "body_name" ),
                  py::arg( "group_path" ) = "/Trajectories/SingleArcSimulationResults",
                  R"doc(

Add SingleArcSimulationResults to the file.

Parameters
----------
results : SingleArcSimulationResults
    The simulation results to store.
body_name : str
    Name of the body (used as group name).
group_path : str, default="/Trajectories/SingleArcSimulationResults"
    Base path in HDF5 file.

)doc" )
            .def( "add_variational_results",
                  &tio::HDF5OutputFile::addVariationalResults< double, TIME_TYPE >,
                  py::arg( "results" ),
                  py::arg( "body_name" ),
                  py::arg( "group_path" ) = "/Trajectories/VariationalEquationsSimulationResults",
                  R"doc(

Add SingleArcVariationalSimulationResults to the file.

Parameters
----------
results : SingleArcVariationalSimulationResults
    The variational simulation results to store.
body_name : str
    Name of the body (used as group name).
group_path : str, default="/Trajectories/VariationalEquationsSimulationResults"
    Base path in HDF5 file.

)doc" )
            .def( "add_single_observation_set",
                  &tio::HDF5OutputFile::addSingleObservationSet< double, double >,
                  py::arg( "observation_set" ),
                  py::arg( "set_name" ),
                  py::arg( "group_path" ) = "/Observations/SingleObservationSets",
                  py::arg( "include_filtered_observations" ) = false,
                  R"doc(

Add a SingleObservationSet to the file.

Parameters
----------
observation_set : SingleObservationSet
    The observation set to store.
set_name : str
    Name for this observation set (used as group name).
group_path : str, default="/Observations/SingleObservationSets"
    Base path in HDF5 file.
include_filtered_observations : bool, default=False
    If True, also store filtered observations in a subgroup.

)doc" )
            // Overload for <double, Time> (Python default)
            .def( "add_single_observation_set",
                  &tio::HDF5OutputFile::addSingleObservationSet< double, tudat::Time >,
                  py::arg( "observation_set" ),
                  py::arg( "set_name" ),
                  py::arg( "group_path" ) = "/Observations/SingleObservationSets",
                  py::arg( "include_filtered_observations" ) = false )
            .def( "add_observation_collection",
                  &tio::HDF5OutputFile::addObservationCollection< double, double >,
                  py::arg( "observation_collection" ),
                  py::arg( "collection_name" ) = "default",
                  py::arg( "group_path" ) = "/Observations/ObservationCollections",
                  py::arg( "include_filtered_observations" ) = false,
                  R"doc(

Add an ObservationCollection to the file.

Parameters
----------
observation_collection : ObservationCollection
    The observation collection to store.
collection_name : str, default="default"
    Name for this collection (used as group name).
group_path : str, default="/Observations/ObservationCollections"
    Base path in HDF5 file.
include_filtered_observations : bool, default=False
    If True, also store filtered observations.

)doc" )
            // Overload for <double, Time> (Python default)
            .def( "add_observation_collection",
                  &tio::HDF5OutputFile::addObservationCollection< double, tudat::Time >,
                  py::arg( "observation_collection" ),
                  py::arg( "collection_name" ) = "default",
                  py::arg( "group_path" ) = "/Observations/ObservationCollections",
                  py::arg( "include_filtered_observations" ) = false )
            .def( "generate_xdmf",
                  py::overload_cast< const std::string& >( &tio::HDF5OutputFile::generateXDMF ),
                  py::arg( "xdmf_file_path" ),
                  R"doc(

Generate XDMF descriptor file for trajectory visualization.

Creates an XDMF3 file that references the HDF5 data and can be
opened directly in ParaView.

Parameters
----------
xdmf_file_path : str
    Path to the XDMF file to create.

)doc" )
            .def( "generate_xdmf",
                  py::overload_cast<>( &tio::HDF5OutputFile::generateXDMF ),
                  R"doc(

Generate XDMF descriptor file with default name.

Creates XDMF file with same base name as HDF5 file but .xdmf extension.

)doc" )
            .def( "generate_observation_xdmf",
                  py::overload_cast< const std::string& >( &tio::HDF5OutputFile::generateObservationXDMF ),
                  py::arg( "xdmf_file_path" ),
                  R"doc(

Generate XDMF descriptor file for observation data visualization.

Creates an XDMF3 file for visualizing observation data in ParaView.
Each observation set becomes a Polyvertex grid.

Parameters
----------
xdmf_file_path : str
    Path to the XDMF file to create.

)doc" )
            .def( "generate_observation_xdmf",
                  py::overload_cast<>( &tio::HDF5OutputFile::generateObservationXDMF ),
                  R"doc(

Generate observation XDMF with default name.

Creates XDMF file with same base name as HDF5 file but _observations.xdmf extension.

)doc" )
            .def( "close", &tio::HDF5OutputFile::close,
                  R"doc(

Close the HDF5 file explicitly.

The file is also closed automatically when the object is destroyed.

)doc" )
            .def_property_readonly( "file_path", &tio::HDF5OutputFile::getFilePath,
                  R"doc(Path to the HDF5 file.)doc" )
            .def_property_readonly( "trajectory_configs", &tio::HDF5OutputFile::getTrajectoryConfigs,
                  R"doc(Trajectory configurations for all stored trajectories.)doc" )
            .def_property_readonly( "observation_configs", &tio::HDF5OutputFile::getObservationConfigs,
                  R"doc(Observation configurations for all stored observation sets.)doc" );
}

}  // namespace hdf5_manager
}  // namespace data
}  // namespace tudatpy

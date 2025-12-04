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
#include "expose_unified_data_library_reader.h"

#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include <pybind11/pybind11.h>

#include <tudat/io/unifiedDataLibraryReader.h>

namespace py = pybind11;
namespace tio = tudat::io;
namespace tom = tudat::observation_models;

namespace tudatpy
{
namespace data
{
namespace unified_data_library
{

void expose_unified_data_library_reader( py::module& m )
{
    // =========================================================================
    // GeodeticPosition struct
    // =========================================================================
    py::class_< tio::GeodeticPosition >( m,
                                         "GeodeticPosition",
                                         R"doc(

Structure representing a geodetic position with longitude, latitude, and altitude.

The units depend on the data source (typically degrees for angles and kilometers for altitude
when reading from UTAS format).

)doc" )
            .def( py::init<>( ),
                  R"doc(

Default constructor initializing position to (0, 0, 0).

)doc" )
            .def( py::init< double, double, double >( ),
                  py::arg( "longitude" ),
                  py::arg( "latitude" ),
                  py::arg( "altitude" ),
                  R"doc(

Constructor with explicit coordinates.

Parameters
----------
longitude : float
    Longitude coordinate
latitude : float
    Latitude coordinate
altitude : float
    Altitude coordinate

)doc" )
            .def_readwrite( "longitude", &tio::GeodeticPosition::longitude,
                            R"doc(Longitude coordinate (units depend on data source).)doc" )
            .def_readwrite( "latitude", &tio::GeodeticPosition::latitude,
                            R"doc(Latitude coordinate (units depend on data source).)doc" )
            .def_readwrite( "altitude", &tio::GeodeticPosition::altitude,
                            R"doc(Altitude coordinate (units depend on data source).)doc" )
            .def( "is_zero", &tio::GeodeticPosition::isZero,
                  R"doc(

Check if all coordinates are zero.

Returns
-------
bool
    True if all coordinates are zero.

)doc" )
            .def( "to_eigen_vector", &tio::GeodeticPosition::toEigenVector,
                  R"doc(

Convert to Eigen vector [longitude, latitude, altitude].

Returns
-------
numpy.ndarray
    3-element array with [longitude, latitude, altitude].

)doc" );

    // =========================================================================
    // UDLObservationMetadata struct
    // =========================================================================
    py::class_< tio::UDLObservationMetadata >( m,
                                               "UDLObservationMetadata",
                                               R"doc(

Metadata structure for UDL observation sets.

Contains information about the stations, target, frequency, and data provenance
that is constant across all observations in a set.

)doc" )
            .def( py::init<>( ) )
            .def_readwrite( "station1_id", &tio::UDLObservationMetadata::station1Id,
                            R"doc(Identifier for the first ground station.)doc" )
            .def_readwrite( "station2_id", &tio::UDLObservationMetadata::station2Id,
                            R"doc(Identifier for the second ground station.)doc" )
            .def_readwrite( "station1_position", &tio::UDLObservationMetadata::station1Position,
                            R"doc(Geodetic position of the first ground station.)doc" )
            .def_readwrite( "station2_position", &tio::UDLObservationMetadata::station2Position,
                            R"doc(Geodetic position of the second ground station.)doc" )
            .def_readwrite( "target_id", &tio::UDLObservationMetadata::targetId,
                            R"doc(Identifier for the observed target (e.g., satellite number).)doc" )
            .def_readwrite( "frequency", &tio::UDLObservationMetadata::frequency,
                            R"doc(Observation frequency in Hz.)doc" )
            .def_readwrite( "data_mode", &tio::UDLObservationMetadata::dataMode,
                            R"doc(Data mode identifier.)doc" )
            .def_readwrite( "origin", &tio::UDLObservationMetadata::origin,
                            R"doc(Data origin identifier.)doc" )
            .def_readwrite( "source", &tio::UDLObservationMetadata::source,
                            R"doc(Data source identifier.)doc" );

    // =========================================================================
    // UDLTimeSeries struct
    // =========================================================================
    py::class_< tio::UDLTimeSeries >( m,
                                      "UDLTimeSeries",
                                      R"doc(

Time series data for UDL observations.

Contains vectors of epochs, TDOA/FDOA measurements and their uncertainties.
All vectors have the same length (one entry per observation).

)doc" )
            .def( py::init<>( ) )
            .def_readwrite( "epochs", &tio::UDLTimeSeries::epochs,
                            R"doc(Time epochs in seconds since J2000 TDB.)doc" )
            .def_readwrite( "tdoa", &tio::UDLTimeSeries::tdoa,
                            R"doc(Time Difference of Arrival measurements in seconds.)doc" )
            .def_readwrite( "tdoa_unc", &tio::UDLTimeSeries::tdoaUnc,
                            R"doc(TDOA uncertainties in seconds.)doc" )
            .def_readwrite( "fdoa", &tio::UDLTimeSeries::fdoa,
                            R"doc(Frequency Difference of Arrival measurements in Hz.)doc" )
            .def_readwrite( "fdoa_unc", &tio::UDLTimeSeries::fdoaUnc,
                            R"doc(FDOA uncertainties in Hz.)doc" )
            .def( "__len__", &tio::UDLTimeSeries::size,
                  R"doc(

Get the number of observations.

Returns
-------
int
    Number of observations in the time series.

)doc" )
            .def( "is_consistent", &tio::UDLTimeSeries::isConsistent,
                  R"doc(

Check if all vectors have the same length.

Returns
-------
bool
    True if all data vectors are consistent.

)doc" );

    // =========================================================================
    // UTASObservationSet class
    // =========================================================================
    py::class_< tio::UTASObservationSet, std::shared_ptr< tio::UTASObservationSet > >( m,
                                                                                        "UTASObservationSet",
                                                                                        R"doc(

UTAS-specific observation set parser.

Parses JSON data from UTAS format with strict type checking.
Contains metadata (constant fields) and time series data (time-varying fields).

)doc" )
            .def( "get_metadata", &tio::UTASObservationSet::getMetadata,
                  py::return_value_policy::reference_internal,
                  R"doc(

Get the observation set metadata.

Returns
-------
UDLObservationMetadata
    Metadata containing station info, target, frequency, etc.

)doc" )
            .def( "get_time_series", &tio::UTASObservationSet::getTimeSeries,
                  py::return_value_policy::reference_internal,
                  R"doc(

Get the time series data.

Returns
-------
UDLTimeSeries
    Time series containing epochs, TDOA, FDOA and uncertainties.

)doc" )
            .def( "num_observations", &tio::UTASObservationSet::numObservations,
                  R"doc(

Get the number of observations.

Returns
-------
int
    Number of observations in this set.

)doc" )
            .def( "get_sensor1_delay", &tio::UTASObservationSet::getSensor1Delay,
                  R"doc(

Get the delay for sensor 1.

Returns
-------
float
    Sensor 1 delay in seconds.

)doc" )
            .def( "get_sensor2_delay", &tio::UTASObservationSet::getSensor2Delay,
                  R"doc(

Get the delay for sensor 2.

Returns
-------
float
    Sensor 2 delay in seconds.

)doc" )
            .def( "get_bandwidth", &tio::UTASObservationSet::getBandwidth,
                  R"doc(

Get the observation bandwidth.

Returns
-------
float
    Bandwidth in Hz.

)doc" );

    // =========================================================================
    // UTASObservationCollection class
    // =========================================================================
    py::class_< tio::UTASObservationCollection >( m,
                                                   "UTASObservationCollection",
                                                   R"doc(

Collection of UTAS observation sets organized by target and station pairs.

Provides methods to load observation data from JSON files and access
observatory information.

)doc" )
            .def( py::init<>( ),
                  R"doc(

Default constructor creating an empty collection.

)doc" )
            .def( py::init< const std::vector< std::string >& >( ),
                  py::arg( "file_paths" ),
                  R"doc(

Construct from a list of JSON file paths.

Parameters
----------
file_paths : list[str]
    List of paths to JSON files containing UTAS observation data.

)doc" )
            .def( "add_from_file", &tio::UTASObservationCollection::addFromFile,
                  py::arg( "file_path" ),
                  R"doc(

Add observations from a JSON file.

Parameters
----------
file_path : str
    Path to the JSON file.

)doc" )
            .def( "get_observatory_names", &tio::UTASObservationCollection::getObservatoryNames,
                  R"doc(

Get unique observatory/station names.

Returns
-------
set[str]
    Set of all unique observatory names in the collection.

)doc" )
            .def( "get_observatory_positions", &tio::UTASObservationCollection::getObservatoryPositions,
                  R"doc(

Get observatory positions.

Returns
-------
dict[str, GeodeticPosition]
    Dictionary mapping observatory names to their geodetic positions.

)doc" )
            .def( "get_observed_targets", &tio::UTASObservationCollection::getObservedTargets,
                  R"doc(

Get unique observed target IDs.

Returns
-------
set[str]
    Set of all unique target identifiers in the collection.

)doc" );

    // =========================================================================
    // BatchVLBI class (convenience class)
    // =========================================================================
    py::class_< tio::BatchVLBI >( m,
                                   "BatchVLBI",
                                   R"doc(

Convenience class for loading and converting batch VLBI observations.

Combines UTASObservationCollection and conversion to Tudat format for easy use.
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

)doc" )
            .def( py::init< const std::vector< std::string >& >( ),
                  py::arg( "file_paths" ),
                  R"doc(

Construct from a list of JSON file paths.

Parameters
----------
file_paths : list[str]
    List of paths to JSON files containing VLBI observation data.

)doc" )
            .def( "to_tudat",
                  &tio::BatchVLBI::toTudat,
                  py::arg( "bodies" ),
                  py::arg( "included_targets" ) = std::vector< std::string >( ),
                  py::arg( "station_body" ) = "Earth",
                  R"doc(

Convert to Tudat observation collection.

Creates ground stations on the specified body and returns observations
in Tudat format for use with estimation.

Parameters
----------
bodies : SystemOfBodies
    System of bodies (will be modified to add ground stations).
included_targets : list[str], optional
    List of target IDs to include. If empty, all targets are included.
station_body : str, default="Earth"
    Name of the body on which to place ground stations.

Returns
-------
ObservationCollection
    Tudat observation collection ready for use with estimation.

)doc" )
            .def( "get_collection", &tio::BatchVLBI::getCollection,
                  py::return_value_policy::reference_internal,
                  R"doc(

Get the underlying observation collection.

Returns
-------
UTASObservationCollection
    The raw observation collection.

)doc" );
}

}  // namespace unified_data_library
}  // namespace data
}  // namespace tudatpy

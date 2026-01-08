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
    // GeodeticPositionNew struct
    // =========================================================================
    py::class_< tio::GeodeticPositionNew >( m,
                                            "GeodeticPosition",
                                            R"doc(

Structure representing a geodetic position with altitude, latitude, and longitude.

The units depend on the data source (typically degrees for angles and kilometers for altitude
when reading from UTAS format).

)doc" )
            .def( py::init<>( ),
                  R"doc(

Default constructor initializing position to (0, 0, 0).

)doc" )
            .def( py::init< double, double, double >( ),
                  py::arg( "altitude" ),
                  py::arg( "latitude" ),
                  py::arg( "longitude" ),
                  R"doc(

Constructor with explicit coordinates.

Parameters
----------
altitude : float
    Altitude coordinate
latitude : float
    Latitude coordinate
longitude : float
    Longitude coordinate

)doc" )
            .def_readwrite( "altitude", &tio::GeodeticPositionNew::altitude,
                            R"doc(Altitude coordinate (units depend on data source).)doc" )
            .def_readwrite( "latitude", &tio::GeodeticPositionNew::latitude,
                            R"doc(Latitude coordinate (units depend on data source).)doc" )
            .def_readwrite( "longitude", &tio::GeodeticPositionNew::longitude,
                            R"doc(Longitude coordinate (units depend on data source).)doc" )
            .def( "is_zero", &tio::GeodeticPositionNew::isZero,
                  R"doc(

Check if all coordinates are zero.

Returns
-------
bool
    True if all coordinates are zero.

)doc" )
            .def( "to_eigen_vector", &tio::GeodeticPositionNew::toEigenVector,
                  R"doc(

Convert to Eigen vector [altitude, latitude, longitude].

Returns
-------
numpy.ndarray
    3-element array with [altitude, latitude, longitude].

)doc" );

    // =========================================================================
    // UTASMetadata struct
    // =========================================================================
    py::class_< tio::UTASMetadata >( m,
                                     "UTASMetadata",
                                     R"doc(

Strongly-typed metadata structure for UTAS observations.

Contains UTAS-specific fields including target ID, signal parameters, and
data provenance. Station information is stored separately as there may be
multiple station pairs across files.

)doc" )
            .def( py::init<>( ) )
            .def_readonly( "target_id", &tio::UTASMetadata::targetId,
                           R"doc(Identifier for the observed target (e.g., satellite catalog number).)doc" )
            .def_readonly( "frequency", &tio::UTASMetadata::frequency,
                           R"doc(Center frequency of the signal in Hz.)doc" )
            .def_readonly( "bandwidth", &tio::UTASMetadata::bandwidth,
                           R"doc(Signal bandwidth in Hz.)doc" )
            .def_readonly( "sensor1_delay", &tio::UTASMetadata::sensor1Delay,
                           R"doc(Signal arrival delay for sensor 1 in seconds.)doc" )
            .def_readonly( "sensor2_delay", &tio::UTASMetadata::sensor2Delay,
                           R"doc(Signal arrival delay for sensor 2 in seconds.)doc" )
            .def_readonly( "data_mode", &tio::UTASMetadata::dataMode,
                           R"doc(Data classification: EXERCISE, REAL, SIMULATED, or TEST.)doc" )
            .def_readonly( "origin", &tio::UTASMetadata::origin,
                           R"doc(Originating system identifier.)doc" )
            .def_readonly( "source", &tio::UTASMetadata::source,
                           R"doc(Data source name.)doc" )
            .def_readonly( "ucts", &tio::UTASMetadata::ucts,
                           R"doc(Uncorrelated track status flag.)doc" );

    // =========================================================================
    // StationPairObservations struct (Time variant)
    // =========================================================================
    py::class_< tio::StationPairObservations< double, tudat::Time > >( m,
                                                                        "StationPairObservations",
                                                                        R"doc(

Time series data for a single station pair.

Contains observation epochs, TDOA/FDOA measurements, and their uncertainties
for observations made by a specific pair of ground stations.

Attributes
----------
epochs : list[Time]
    Observation epochs in TDB seconds since J2000.
tdoa : list[float]
    Time Difference of Arrival observations in seconds.
tdoa_uncertainties : list[float]
    TDOA measurement uncertainties in seconds.
fdoa : list[float]
    Frequency Difference of Arrival observations in Hz.
fdoa_uncertainties : list[float]
    FDOA measurement uncertainties in Hz.

)doc" )
            .def_readonly( "epochs", &tio::StationPairObservations< double, tudat::Time >::epochs,
                           R"doc(Observation epochs in TDB seconds since J2000.)doc" )
            .def_readonly( "tdoa", &tio::StationPairObservations< double, tudat::Time >::tdoa,
                           R"doc(TDOA observations in seconds.)doc" )
            .def_readonly( "tdoa_uncertainties", &tio::StationPairObservations< double, tudat::Time >::tdoaUnc,
                           R"doc(TDOA uncertainties in seconds.)doc" )
            .def_readonly( "fdoa", &tio::StationPairObservations< double, tudat::Time >::fdoa,
                           R"doc(FDOA observations in Hz.)doc" )
            .def_readonly( "fdoa_uncertainties", &tio::StationPairObservations< double, tudat::Time >::fdoaUnc,
                           R"doc(FDOA uncertainties in Hz.)doc" )
            .def( "__len__", &tio::StationPairObservations< double, tudat::Time >::size,
                  R"doc(Return the number of observations.)doc" );

    // =========================================================================
    // StationPairObservations struct (double variant)
    // =========================================================================
    py::class_< tio::StationPairObservations< double, double > >( m,
                                                                   "StationPairObservations_double",
                                                                   R"doc(

Time series data for a single station pair (double precision time).

Contains observation epochs, TDOA/FDOA measurements, and their uncertainties
for observations made by a specific pair of ground stations.

)doc" )
            .def_readonly( "epochs", &tio::StationPairObservations< double, double >::epochs,
                           R"doc(Observation epochs in TDB seconds since J2000.)doc" )
            .def_readonly( "tdoa", &tio::StationPairObservations< double, double >::tdoa,
                           R"doc(TDOA observations in seconds.)doc" )
            .def_readonly( "tdoa_uncertainties", &tio::StationPairObservations< double, double >::tdoaUnc,
                           R"doc(TDOA uncertainties in seconds.)doc" )
            .def_readonly( "fdoa", &tio::StationPairObservations< double, double >::fdoa,
                           R"doc(FDOA observations in Hz.)doc" )
            .def_readonly( "fdoa_uncertainties", &tio::StationPairObservations< double, double >::fdoaUnc,
                           R"doc(FDOA uncertainties in Hz.)doc" )
            .def( "__len__", &tio::StationPairObservations< double, double >::size,
                  R"doc(Return the number of observations.)doc" );

    // =========================================================================
    // BatchUTAS class (primary user-facing class)
    // =========================================================================
    py::class_< tio::BatchUTAS< double, tudat::Time > >( m,
                                                          "BatchUTAS",
                                                          R"doc(

Batch loader for UTAS format TDOA/FDOA observations.

This is the main user-facing class for loading UTAS observations from JSON files
and converting them to Tudat format for use in orbit determination.

**Important:** This class only supports single-target data. If your input files
contain observations of multiple targets, you must filter them beforehand and
create separate BatchUTAS instances for each target. Multiple station pairs
across files are supported (as long as all files observe the same target).

Example
-------
>>> batch = BatchUTAS(["observations_day1.json", "observations_day2.json"])
>>> print(f"Target: {batch.target_id}")
>>> print(f"Station pairs: {batch.station_pairs}")
>>> print(f"Station names: {batch.station_names}")
>>> print(f"Number of observations: {batch.num_observations}")
>>>
>>> # Convert to Tudat format (creates ground stations automatically)
>>> observation_collection = batch.to_tudat(bodies)

)doc" )
            .def( py::init< const std::vector< std::string >& >( ),
                  py::arg( "file_paths" ),
                  R"doc(

Construct from a list of JSON file paths.

Parameters
----------
file_paths : list[str]
    List of paths to UTAS JSON files. All files must contain observations
    of the same target. Different station pairs across files are supported.

Raises
------
RuntimeError
    If files contain multiple targets (lists all found targets in error message).

)doc" )
            // Main conversion method
            .def( "to_tudat",
                  &tio::BatchUTAS< double, tudat::Time >::toTudat,
                  py::arg( "bodies" ),
                  py::arg( "station_body" ) = "Earth",
                  py::arg( "target_name_override" ) = "",
                  R"doc(

Convert to Tudat observation collection.

This method performs all necessary setup:
1. Ensures the station body has a compatible shape model
2. Creates ground stations on the body
3. Builds and returns the observation collection

Parameters
----------
bodies : SystemOfBodies
    System of bodies (will be modified to add ground stations).
station_body : str, default="Earth"
    Name of the body on which to place ground stations.
target_name_override : str, default=""
    Custom name for the target in link definitions. If empty, uses the
    target ID from the data (typically NORAD ID). Use this to match
    the body name in your simulation.

Returns
-------
ObservationCollection
    Tudat observation collection containing TDOA and FDOA observation sets.

Raises
------
RuntimeError
    If station body has an incompatible shape model (must be OblateSpheroidBodyShapeModel).

Example
-------
>>> # Use custom target name instead of NORAD ID
>>> observation_collection = batch.to_tudat(bodies, target_name_override="MySatellite")

)doc" )
            // Individual pipeline steps (for advanced users)
            .def( "ensure_shape_model",
                  &tio::BatchUTAS< double, tudat::Time >::ensureShapeModel,
                  py::arg( "bodies" ),
                  py::arg( "station_body" ) = "Earth",
                  R"doc(

Ensure the station body has a compatible shape model.

Creates an oblate spheroid shape model from SPICE if none exists.
Called automatically by to_tudat().

Parameters
----------
bodies : SystemOfBodies
    System of bodies.
station_body : str, default="Earth"
    Name of the body to check/modify.

Raises
------
RuntimeError
    If body has an incompatible (non-oblate-spheroid) shape model.

)doc" )
            .def( "create_ground_stations",
                  &tio::BatchUTAS< double, tudat::Time >::createGroundStations,
                  py::arg( "bodies" ),
                  py::arg( "station_body" ) = "Earth",
                  R"doc(

Create ground stations on the specified body.

Called automatically by to_tudat().

Parameters
----------
bodies : SystemOfBodies
    System of bodies (modified in place).
station_body : str, default="Earth"
    Body on which to create stations.

Returns
-------
list[str]
    Names of the created stations.

)doc" )
            .def( "get_link_definitions",
                  &tio::BatchUTAS< double, tudat::Time >::getLinkDefinitions,
                  py::arg( "station_body" ) = "Earth",
                  py::arg( "target_name_override" ) = "",
                  R"doc(

Get the link definitions for all station pairs in this batch.

Parameters
----------
station_body : str, default="Earth"
    Name of body hosting ground stations.
target_name_override : str, default=""
    Custom name for the target in link definitions. If empty, uses the
    target ID from the data (typically NORAD ID).

Returns
-------
list[LinkDefinition]
    Link definitions with receiver, receiver2, and transmitter link ends,
    one per station pair.

)doc" )
            .def( "get_observation_collection",
                  &tio::BatchUTAS< double, tudat::Time >::getObservationCollection,
                  py::arg( "station_body" ) = "Earth",
                  py::arg( "target_name_override" ) = "",
                  R"doc(

Get observation collection without modifying bodies.

Use this if you've already created ground stations manually.

Parameters
----------
station_body : str, default="Earth"
    Name of body hosting ground stations.
target_name_override : str, default=""
    Custom name for the target in link definitions. If empty, uses the
    target ID from the data (typically NORAD ID).

Returns
-------
ObservationCollection
    Observation collection with TDOA and FDOA sets.

)doc" )
            // Identification properties
            .def_property_readonly( "target_id",
                                    &tio::BatchUTAS< double, tudat::Time >::getTargetId,
                                    R"doc(Target identifier (e.g., satellite catalog number).)doc" )
            .def_property_readonly( "num_observations",
                                    &tio::BatchUTAS< double, tudat::Time >::getNumObservations,
                                    R"doc(Total number of observations across all station pairs.)doc" )
            .def_property_readonly( "station_pairs",
                                    &tio::BatchUTAS< double, tudat::Time >::getStationPairs,
                                    R"doc(List of station pairs as (station1_id, station2_id) tuples.)doc" )
            .def_property_readonly( "station_names",
                                    &tio::BatchUTAS< double, tudat::Time >::getStationNames,
                                    R"doc(Set of unique station names across all station pairs.)doc" )
            .def_property_readonly( "num_station_pairs",
                                    &tio::BatchUTAS< double, tudat::Time >::getNumStationPairs,
                                    R"doc(Number of unique station pairs.)doc" )
            // Full metadata access
            .def( "get_metadata",
                  &tio::BatchUTAS< double, tudat::Time >::getMetadata,
                  py::return_value_policy::reference_internal,
                  R"doc(

Get full UTAS metadata.

Returns
-------
UTASMetadata
    Metadata containing all UTAS-specific fields.

)doc" )
            // Station-pair based observation access
            .def( "get_observations_for_station_pair",
                  &tio::BatchUTAS< double, tudat::Time >::getObservationsForStationPair,
                  py::arg( "station_pair" ),
                  py::return_value_policy::reference_internal,
                  R"doc(

Get observations for a specific station pair.

Parameters
----------
station_pair : tuple[str, str]
    The station pair as (station1_id, station2_id).

Returns
-------
StationPairObservations
    Observations containing epochs, TDOA, FDOA and their uncertainties.

Raises
------
RuntimeError
    If the station pair is not found.

Example
-------
>>> obs = batch.get_observations_for_station_pair(("STATION_A", "STATION_B"))
>>> print(f"Number of observations: {len(obs)}")
>>> print(f"TDOA values: {obs.tdoa}")

)doc" )
            .def( "get_all_observations_by_station_pair",
                  &tio::BatchUTAS< double, tudat::Time >::getAllObservationsByStationPair,
                  py::return_value_policy::reference_internal,
                  R"doc(

Get all observations organized by station pair.

Returns
-------
dict[tuple[str, str], StationPairObservations]
    Dictionary mapping station pairs to their observation data.

Example
-------
>>> all_obs = batch.get_all_observations_by_station_pair()
>>> for station_pair, obs in all_obs.items():
...     print(f"{station_pair}: {len(obs)} observations")

)doc" );

    // =========================================================================
    // BatchUTAS with double TimeType (for compatibility)
    // =========================================================================
    py::class_< tio::BatchUTAS< double, double > >( m,
                                                     "BatchUTAS_double",
                                                     R"doc(

Batch loader for UTAS format observations using double precision time.

This is an alternative version of BatchUTAS that uses double instead of Time
for the time type. Use the standard BatchUTAS class unless you specifically
need double precision time representation.

)doc" )
            .def( py::init< const std::vector< std::string >& >( ),
                  py::arg( "file_paths" ) )
            .def( "to_tudat",
                  &tio::BatchUTAS< double, double >::toTudat,
                  py::arg( "bodies" ),
                  py::arg( "station_body" ) = "Earth",
                  py::arg( "target_name_override" ) = "" )
            .def( "ensure_shape_model",
                  &tio::BatchUTAS< double, double >::ensureShapeModel,
                  py::arg( "bodies" ),
                  py::arg( "station_body" ) = "Earth" )
            .def( "create_ground_stations",
                  &tio::BatchUTAS< double, double >::createGroundStations,
                  py::arg( "bodies" ),
                  py::arg( "station_body" ) = "Earth" )
            .def( "get_link_definitions",
                  &tio::BatchUTAS< double, double >::getLinkDefinitions,
                  py::arg( "station_body" ) = "Earth",
                  py::arg( "target_name_override" ) = "" )
            .def( "get_observation_collection",
                  &tio::BatchUTAS< double, double >::getObservationCollection,
                  py::arg( "station_body" ) = "Earth",
                  py::arg( "target_name_override" ) = "" )
            .def_property_readonly( "target_id", &tio::BatchUTAS< double, double >::getTargetId )
            .def_property_readonly( "num_observations", &tio::BatchUTAS< double, double >::getNumObservations )
            .def_property_readonly( "station_pairs", &tio::BatchUTAS< double, double >::getStationPairs )
            .def_property_readonly( "station_names", &tio::BatchUTAS< double, double >::getStationNames )
            .def_property_readonly( "num_station_pairs", &tio::BatchUTAS< double, double >::getNumStationPairs )
            .def( "get_metadata", &tio::BatchUTAS< double, double >::getMetadata,
                  py::return_value_policy::reference_internal )
            .def( "get_observations_for_station_pair",
                  &tio::BatchUTAS< double, double >::getObservationsForStationPair,
                  py::arg( "station_pair" ),
                  py::return_value_policy::reference_internal )
            .def( "get_all_observations_by_station_pair",
                  &tio::BatchUTAS< double, double >::getAllObservationsByStationPair,
                  py::return_value_policy::reference_internal );
}

}  // namespace unified_data_library
}  // namespace data
}  // namespace tudatpy

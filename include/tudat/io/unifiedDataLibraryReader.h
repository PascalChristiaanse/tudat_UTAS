/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_UNIFIED_DATA_LIBRARY_READER_H
#define TUDAT_UNIFIED_DATA_LIBRARY_READER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <Eigen/Dense>
#include <map>
#include <set>
#include <stdexcept>
#include <cmath>

#include <nlohmann/json.hpp>

#include "tudat/basics/timeType.h"
#include "tudat/astro/observation_models/linkTypeDefs.h"
#include "tudat/astro/observation_models/observationModel.h"
#include "tudat/astro/observation_models/observableTypes.h"
#include "tudat/astro/basic_astro/dateTime.h"
#include "tudat/astro/earth_orientation/terrestrialTimeScaleConverter.h"
#include "tudat/astro/basic_astro/timeConversions.h"
#include "tudat/simulation/simulation.h"
#include "tudat/simulation/estimation_setup/singleObservationSet.h"
#include "tudat/simulation/estimation_setup/observationCollection.h"
#include "tudat/simulation/environment_setup/createGroundStations.h"
#include "tudat/math/basic/mathematicalConstants.h"

namespace tba = tudat::basic_astrodynamics;
namespace teo = tudat::earth_orientation;
namespace tom = tudat::observation_models;

namespace tudat
{
namespace io
{

// ============================================================================
// GENERIC UDL BASE STRUCTURES
// ============================================================================

/**
 * @brief Geodetic position structure (generic, unit-agnostic)
 */
struct GeodeticPosition {
    double longitude;  // Angular unit depends on supplier format
    double latitude;   // Angular unit depends on supplier format
    double altitude;   // Length unit depends on supplier format

    GeodeticPosition( ): longitude( 0.0 ), latitude( 0.0 ), altitude( 0.0 ) {}
    GeodeticPosition( double lon, double lat, double alt ): longitude( lon ), latitude( lat ), altitude( alt ) {}

    bool isZero( ) const
    {
        return longitude == 0.0 && latitude == 0.0 && altitude == 0.0;
    }

    Eigen::Vector3d toEigenVector( ) const
    {
        return Eigen::Vector3d( longitude, latitude, altitude );
    }
};

/**
 * @brief Station metadata for a single ground station
 */
struct UDLStationInfo {
    std::string stationId;
    GeodeticPosition position;
};

/**
 * @brief Base metadata structure for UDL observation sets
 *
 * Contains fields common to all UDL-compliant formats.
 * Supplier-specific parsers should extend this.
 */
struct UDLObservationMetadata {
    // Station identifiers
    std::string station1Id;
    std::string station2Id;

    // Station positions (in supplier-specific units)
    GeodeticPosition station1Position;
    GeodeticPosition station2Position;

    // Target identifier
    std::string targetId;

    // Observation frequency (Hz)
    double frequency = 0.0;

    // Data provenance
    std::string dataMode;
    std::string origin;
    std::string source;
};

/**
 * @brief Time series data for UDL observations
 *
 * All vectors must have the same length (one entry per observation).
 */
struct UDLTimeSeries {
    std::vector< double > epochs;   // Time in seconds since J2000 TDB
    std::vector< double > tdoa;     // Time Difference of Arrival (seconds)
    std::vector< double > tdoaUnc;  // TDOA uncertainty (seconds)
    std::vector< double > fdoa;     // Frequency Difference of Arrival (Hz)
    std::vector< double > fdoaUnc;  // FDOA uncertainty (Hz)

    size_t size( ) const
    {
        return epochs.size( );
    }

    bool isConsistent( ) const
    {
        size_t n = epochs.size( );
        return tdoa.size( ) == n && tdoaUnc.size( ) == n && fdoa.size( ) == n && fdoaUnc.size( ) == n;
    }
};

/**
 * @brief Base class for UDL observation sets
 */
class UDLObservationSet
{
public:
    virtual ~UDLObservationSet( ) = default;

    const UDLObservationMetadata& getMetadata( ) const
    {
        return metadata_;
    }
    const UDLTimeSeries& getTimeSeries( ) const
    {
        return timeSeries_;
    }

    size_t numObservations( ) const
    {
        return timeSeries_.size( );
    }

protected:
    UDLObservationMetadata metadata_;
    UDLTimeSeries timeSeries_;
};

// ============================================================================
// UTAS-SPECIFIC PARSER
// ============================================================================

/**
 * @brief UTAS-specific observation set parser
 *
 * Parses JSON data from UTAS format with strict type checking.
 * Throws std::runtime_error on any parsing failure.
 *
 * Expected JSON format: Array of observation objects, each containing:
 * - Constant fields (same for all observations): origSensorId1, origSensorId2,
 *   senlat, senlon, senalt, sen2lat, sen2lon, sen2alt, satNo, frequency, dataMode, etc.
 * - Time-varying fields: obTime, tdoa, tdoaUnc, fdoa, fdoaUnc
 *
 * Position units: degrees for lat/lon, km for altitude
 */
class UTASObservationSet : public UDLObservationSet
{
public:
    /**
     * @brief Parse UTAS JSON data
     * @param j JSON data (must be array of observation objects)
     * @throws std::runtime_error on parsing failure or validation error
     */
    explicit UTASObservationSet( const nlohmann::json& j );

    /**
     * @brief Get station delays (constant for all observations)
     */
    double getSensor1Delay( ) const
    {
        return sensor1Delay_;
    }
    double getSensor2Delay( ) const
    {
        return sensor2Delay_;
    }
    double getBandwidth( ) const
    {
        return bandwidth_;
    }
    int getUcts( ) const
    {
        return ucts_;
    }

private:
    // Parse methods
    void parseMetadata( const nlohmann::json& firstObs );
    void parseTimeSeries( const nlohmann::json& observations );
    void validateMetadataConsistency( const nlohmann::json& observations );
    double convertIsoStringToEpoch( const std::string& isoTime );

    // Helper to get required field with type checking
    template< typename T >
    static T getRequired( const nlohmann::json& obj, const std::string& key );

    template< typename T >
    static T getOptional( const nlohmann::json& obj, const std::string& key, const T& defaultValue );

    // UTAS-specific constant fields
    double sensor1Delay_ = 0.0;
    double sensor2Delay_ = 0.0;
    double bandwidth_ = 0.0;
    int ucts_ = 0;

    // Time converter
    std::shared_ptr< teo::TerrestrialTimeScaleConverter > timeConverter_;
};

// ============================================================================
// OBSERVATION COLLECTION
// ============================================================================

/**
 * @brief Collection of UTAS observation sets organized by target and station pairs
 */
class UTASObservationCollection
{
public:
    UTASObservationCollection( ) = default;

    /**
     * @brief Construct from list of JSON file paths
     */
    explicit UTASObservationCollection( const std::vector< std::string >& filePaths );

    /**
     * @brief Add observation set from file
     */
    void addFromFile( const std::string& filePath );

    /**
     * @brief Add observation set directly
     */
    void addSet( std::shared_ptr< UTASObservationSet > observationSet );

    /**
     * @brief Get unique observatory names
     */
    std::set< std::string > getObservatoryNames( ) const;

    /**
     * @brief Get observatory positions (lon, lat in degrees; alt in km)
     */
    std::map< std::string, GeodeticPosition > getObservatoryPositions( ) const;

    /**
     * @brief Get unique observed target IDs
     */
    std::set< std::string > getObservedTargets( ) const;

    /**
     * @brief Get all observations organized by target and station pair
     */
    const std::map< std::string, std::map< std::pair< std::string, std::string >, std::vector< std::shared_ptr< UTASObservationSet > > > >&
    getAllObservations( ) const
    {
        return observationsByTarget_;
    }

private:
    // Observations indexed by: target -> (station1, station2) -> observation sets
    std::map< std::string, std::map< std::pair< std::string, std::string >, std::vector< std::shared_ptr< UTASObservationSet > > > >
            observationsByTarget_;

    // Station positions (accumulated from all sets)
    std::map< std::string, GeodeticPosition > stationPositions_;
};

// ============================================================================
// TUDAT CONVERTER
// ============================================================================

/**
 * @brief Formatter/converter for UTAS data to Tudat format
 *
 * Handles unit conversions and creates Tudat-compatible observation collections.
 */
class UTASTudatFormatter
{
public:
    /**
     * @brief Angular unit for geodetic coordinates
     */
    enum class AngleUnit { Degrees, Radians };

    /**
     * @brief Length unit for altitude
     */
    enum class LengthUnit { Meters, Kilometers };

    /**
     * @brief Construct formatter with unit specifications
     * @param inputAngleUnit Unit of input angular coordinates (from UTAS: Degrees)
     * @param inputLengthUnit Unit of input altitude (from UTAS: Kilometers)
     */
    UTASTudatFormatter( AngleUnit inputAngleUnit = AngleUnit::Degrees, LengthUnit inputLengthUnit = LengthUnit::Kilometers ):
        inputAngleUnit_( inputAngleUnit ), inputLengthUnit_( inputLengthUnit )
    {}

    /**
     * @brief Convert UTAS observation collection to Tudat format
     *
     * Creates ground stations and observation sets in Tudat format.
     *
     * @param collection UTAS observation collection
     * @param bodies System of bodies (will be modified to add stations)
     * @param includedTargets List of target IDs to include (empty = all)
     * @param stationBody Body on which to place ground stations (default: "Earth")
     * @return Tudat observation collection
     */
    std::shared_ptr< tom::ObservationCollection< double, Time > > toTudat( const UTASObservationCollection& collection,
                                                                           simulation_setup::SystemOfBodies& bodies,
                                                                           const std::vector< std::string >& includedTargets = { },
                                                                           const std::string& stationBody = "Earth" );

private:
    /**
     * @brief Convert geodetic position to Tudat format (radians, meters)
     */
    Eigen::Vector3d convertToTudatGeodetic( const GeodeticPosition& pos ) const;

    AngleUnit inputAngleUnit_;
    LengthUnit inputLengthUnit_;
};

// ============================================================================
// BATCH VLBI (Convenience class)
// ============================================================================

/**
 * @brief Convenience class for loading batch VLBI observations
 *
 * Combines UTASObservationCollection and UTASTudatFormatter for easy use.
 */
// class BatchVLBI
// {
// public:
//     /**
//      * @brief Construct from list of JSON file paths
//      */
//     explicit BatchVLBI( const std::vector< std::string >& filePaths );

//     /**
//      * @brief Convert to Tudat observation collection
//      *
//      * Creates ground stations and returns observations in Tudat format.
//      */
//     std::shared_ptr< tom::ObservationCollection< double, Time > > toTudat( simulation_setup::SystemOfBodies& bodies,
//                                                                            const std::vector< std::string >& includedTargets = { },
//                                                                            const std::string& stationBody = "Earth" );

//     /**
//      * @brief Get underlying observation collection
//      */
//     const UTASObservationCollection& getCollection( ) const
//     {
//         return collection_;
//     }

// private:
//     UTASObservationCollection collection_;
//     UTASTudatFormatter formatter_;
// };

}  // namespace io
}  // namespace tudat

#endif  // TUDAT_UNIFIED_DATA_LIBRARY_READER_H

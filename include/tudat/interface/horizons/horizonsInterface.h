/*    Copyright (c) 2010-2024, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_HORIZONS_INTERFACE_H
#define TUDAT_HORIZONS_INTERFACE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

#include <Eigen/Core>

namespace tudat
{
namespace horizons_interface
{

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================

//! State vector type (position + velocity)
using Vector6d = Eigen::Matrix< double, 6, 1 >;

//! State history type (time -> state)
using StateHistory = std::map< double, Vector6d >;

// ============================================================================
// CONSTANTS
// ============================================================================

//! JPL Horizons API base URL
const std::string HORIZONS_API_URL = "https://ssd.jpl.nasa.gov/api/horizons.api";

//! Julian Date of J2000 epoch
constexpr double JULIAN_DAY_ON_J2000 = 2451545.0;

//! Seconds per Julian day
constexpr double JULIAN_DAY = 86400.0;

//! Astronomical Unit in meters
constexpr double ASTRONOMICAL_UNIT = 149597870700.0;

// ============================================================================
// EXCEPTIONS
// ============================================================================

/**
 * @brief Exception class for Horizons-related errors
 */
class HorizonsException : public std::runtime_error
{
public:
    explicit HorizonsException( const std::string& message ): std::runtime_error( "Horizons Error: " + message ) {}
};

/**
 * @brief Exception for HTTP/network errors
 */
class HorizonsNetworkException : public HorizonsException
{
public:
    explicit HorizonsNetworkException( const std::string& message ): HorizonsException( "Network error: " + message ) {}
};

/**
 * @brief Exception for API response parsing errors
 */
class HorizonsParseException : public HorizonsException
{
public:
    explicit HorizonsParseException( const std::string& message ): HorizonsException( "Parse error: " + message ) {}
};

// ============================================================================
// ENUMERATIONS AND TYPE DEFINITIONS
// ============================================================================

/**
 * @brief Reference frame orientation type (string-based)
 */
using FrameOrientation = std::string;

//! Earth mean equator and equinox of J2000 (ICRF)
inline const FrameOrientation FrameOrientation_J2000 = "J2000";
//! Ecliptic and mean equinox of J2000
inline const FrameOrientation FrameOrientation_ECLIPJ2000 = "ECLIPJ2000";

/**
 * @brief Aberration correction options
 */
enum class AberrationCorrection {
    Geometric,        //!< No corrections (geometric positions)
    LightTime,        //!< Light-time correction only
    LightTimeStellar  //!< Light-time and stellar aberration
};

// ============================================================================
// HORIZONS QUERY CLASS
// ============================================================================

/**
 * @brief Pure C++ interface to JPL Horizons System via REST API
 *
 * This class provides access to JPL Horizons ephemeris data using HTTP requests.
 * It supports querying Cartesian state vectors for planets, moons, asteroids,
 * comets, and spacecraft.
 *
 * Example usage:
 * @code
 * // Query Mars ephemeris from Solar System Barycenter
 * HorizonsQuery query( "499", "@0", 0.0, 86400.0 * 30, "1d" );
 * auto states = query.getCartesianStates();
 * @endcode
 */
class HorizonsQuery
{
public:
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================

    /**
     * @brief Construct a query with time range
     *
     * @param targetId JPL Horizons target identifier (e.g., "499" for Mars,
     *                 "-28" for JUICE, "Ceres" for asteroid Ceres)
     * @param location Coordinate center (e.g., "@0" for SSB, "500@399" for geocenter)
     * @param startEpoch Start time in seconds since J2000 TDB
     * @param endEpoch End time in seconds since J2000 TDB
     * @param stepSize Time step (e.g., "1d", "1h", "30m", or "10" for 10 steps)
     */
    HorizonsQuery( const std::string& targetId,
                   const std::string& location,
                   double startEpoch,
                   double endEpoch,
                   const std::string& stepSize );

    /**
     * @brief Construct a query with specific epoch list
     *
     * @param targetId JPL Horizons target identifier
     * @param location Coordinate center
     * @param epochs Vector of times in seconds since J2000 TDB
     */
    HorizonsQuery( const std::string& targetId, const std::string& location, const std::vector< double >& epochs );

    //! Destructor
    ~HorizonsQuery( );

    // Disable copy (pimpl idiom)
    HorizonsQuery( const HorizonsQuery& ) = delete;
    HorizonsQuery& operator=( const HorizonsQuery& ) = delete;

    // Enable move
    HorizonsQuery( HorizonsQuery&& ) noexcept;
    HorizonsQuery& operator=( HorizonsQuery&& ) noexcept;

    // ========================================================================
    // QUERY METHODS
    // ========================================================================

    /**
     * @brief Get Cartesian states from Horizons Vectors API
     *
     * Returns state vectors in SI units (meters, meters/second).
     *
     * @param frameOrientation Reference frame orientation
     * @param aberrationCorrection Type of aberration correction
     * @return StateHistory Map of epoch (seconds since J2000) to 6D state vector
     * @throws HorizonsNetworkException on HTTP errors
     * @throws HorizonsParseException on response parsing errors
     * @throws HorizonsException on API errors (e.g., unknown target)
     */
    StateHistory getCartesianStates( FrameOrientation frameOrientation = FrameOrientation_ECLIPJ2000,
                                     AberrationCorrection aberrationCorrection = AberrationCorrection::Geometric ) const;

    /**
     * @brief Get Cartesian state history (alias for getCartesianStates)
     *
     * Convenience method matching Tudat naming conventions.
     */
    StateHistory getCartesianStateHistory( FrameOrientation frameOrientation = FrameOrientation_ECLIPJ2000,
                                           AberrationCorrection aberrationCorrection = AberrationCorrection::Geometric ) const
    {
        return getCartesianStates( frameOrientation, aberrationCorrection );
    }

    // ========================================================================
    // PROPERTY ACCESSORS
    // ========================================================================

    //! Get the target identifier
    std::string getTargetId( ) const;

    //! Get the coordinate center
    std::string getLocation( ) const;

    //! Get the target's full name (available after query)
    std::string getTargetFullName( ) const;

private:
    class Impl;
    std::unique_ptr< Impl > pImpl_;
};

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

/**
 * @brief Get Cartesian state history directly without creating HorizonsQuery object
 *
 * @param targetId JPL Horizons target identifier
 * @param location Coordinate center
 * @param startEpoch Start time in seconds since J2000 TDB
 * @param endEpoch End time in seconds since J2000 TDB
 * @param stepSize Time step string
 * @param frameOrientation Reference frame orientation
 * @return StateHistory Map of epochs to state vectors
 */
StateHistory getHorizonsCartesianStateHistory( const std::string& targetId,
                                               const std::string& location,
                                               double startEpoch,
                                               double endEpoch,
                                               const std::string& stepSize,
                                               FrameOrientation frameOrientation = FrameOrientation_ECLIPJ2000 );

/**
 * @brief Get Cartesian state history for specific epochs
 *
 * @param targetId JPL Horizons target identifier
 * @param location Coordinate center
 * @param epochs Vector of times in seconds since J2000 TDB
 * @param frameOrientation Reference frame orientation
 * @return StateHistory Map of epochs to state vectors
 */
StateHistory getHorizonsCartesianStateHistory( const std::string& targetId,
                                               const std::string& location,
                                               const std::vector< double >& epochs,
                                               FrameOrientation frameOrientation = FrameOrientation_ECLIPJ2000 );

/**
 * @brief Convert aberration correction enum to string
 */
std::string aberrationCorrectionToString( AberrationCorrection correction );

// ============================================================================
// TIME CONVERSION UTILITIES
// ============================================================================

/**
 * @brief Convert seconds since J2000 TDB to Julian Date
 * @param secondsSinceJ2000 Time in seconds since J2000 TDB
 * @return Julian Date (TDB)
 */
inline double secondsToJulianDate( double secondsSinceJ2000 )
{
    return secondsSinceJ2000 / JULIAN_DAY + JULIAN_DAY_ON_J2000;
}

/**
 * @brief Convert Julian Date to seconds since J2000 TDB
 * @param julianDate Julian Date (TDB)
 * @return Seconds since J2000 TDB
 */
inline double julianDateToSeconds( double julianDate )
{
    return ( julianDate - JULIAN_DAY_ON_J2000 ) * JULIAN_DAY;
}

/**
 * @brief Convert seconds since J2000 TDB to ISO date string
 * @param secondsSinceJ2000 Time in seconds since J2000 TDB
 * @return ISO format date string (YYYY-MM-DD HH:MM:SS)
 */
std::string secondsToIsoDate( double secondsSinceJ2000 );

/**
 * @brief Convert ISO date string to seconds since J2000 TDB
 * @param isoDate ISO format date string
 * @return Seconds since J2000 TDB
 */
double isoDateToSeconds( const std::string& isoDate );

}  // namespace horizons_interface
}  // namespace tudat

#endif  // TUDAT_HORIZONS_INTERFACE_H

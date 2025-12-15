/*    Copyright (c) 2010-2024, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#include "tudat/interface/horizons/horizonsInterface.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <regex>
#include <algorithm>

namespace tudat
{
namespace horizons_interface
{

// ============================================================================
// CURL CALLBACK AND HELPERS
// ============================================================================

/**
 * @brief Callback function for libcurl to write received data
 */
static size_t writeCallback( void* contents, size_t size, size_t nmemb, std::string* output )
{
    size_t totalSize = size * nmemb;
    output->append( static_cast< char* >( contents ), totalSize );
    return totalSize;
}

/**
 * @brief URL-encode a string for use in query parameters
 */
static std::string urlEncode( CURL* curl, const std::string& value )
{
    char* encoded = curl_easy_escape( curl, value.c_str( ), static_cast< int >( value.length( ) ) );
    std::string result( encoded );
    curl_free( encoded );
    return result;
}

/**
 * @brief RAII wrapper for CURL handle
 */
class CurlHandle
{
public:
    CurlHandle( )
    {
        handle_ = curl_easy_init( );
        if( !handle_ )
        {
            throw HorizonsNetworkException( "Failed to initialize CURL" );
        }
    }

    ~CurlHandle( )
    {
        if( handle_ )
        {
            curl_easy_cleanup( handle_ );
        }
    }

    CURL* get( ) const
    {
        return handle_;
    }

    // Disable copy
    CurlHandle( const CurlHandle& ) = delete;
    CurlHandle& operator=( const CurlHandle& ) = delete;

private:
    CURL* handle_;
};

/**
 * @brief Global CURL initialization (thread-safe singleton)
 */
class CurlGlobalInit
{
public:
    static CurlGlobalInit& getInstance( )
    {
        static CurlGlobalInit instance;
        return instance;
    }

private:
    CurlGlobalInit( )
    {
        CURLcode result = curl_global_init( CURL_GLOBAL_DEFAULT );
        if( result != CURLE_OK )
        {
            throw HorizonsNetworkException( "Failed to initialize CURL globally" );
        }
    }

    ~CurlGlobalInit( )
    {
        curl_global_cleanup( );
    }

    CurlGlobalInit( const CurlGlobalInit& ) = delete;
    CurlGlobalInit& operator=( const CurlGlobalInit& ) = delete;
};

// ============================================================================
// HORIZONS QUERY IMPLEMENTATION
// ============================================================================

class HorizonsQuery::Impl
{
public:
    std::string targetId_;
    std::string location_;

    // Time range parameters
    bool useTimeRange_;
    double startEpoch_;
    double endEpoch_;
    std::string stepSize_;

    // Epoch list parameters
    std::vector< double > epochs_;

    // Cached results
    mutable std::string targetFullName_;

    /**
     * @brief Build the URL for a Horizons API request
     */
    std::string buildUrl( FrameOrientation frameOrientation, AberrationCorrection aberrationCorrection ) const
    {
        // Ensure CURL is initialized globally
        CurlGlobalInit::getInstance( );

        CurlHandle curl;

        std::ostringstream url;
        url << HORIZONS_API_URL << "?format=json";

        // Target ID - wrap in quotes
        url << "&COMMAND=" << urlEncode( curl.get( ), "'" + targetId_ + "'" );

        // Ephemeris type
        url << "&EPHEM_TYPE=" << urlEncode( curl.get( ), "'VECTORS'" );

        // Coordinate center
        url << "&CENTER=" << urlEncode( curl.get( ), "'" + location_ + "'" );

        // Enable ephemeris generation
        url << "&MAKE_EPHEM=" << urlEncode( curl.get( ), "'YES'" );

        // Object data (we want the name)
        url << "&OBJ_DATA=" << urlEncode( curl.get( ), "'YES'" );

        // Time parameters
        if( useTimeRange_ )
        {
            url << "&START_TIME=" << urlEncode( curl.get( ), "'" + secondsToIsoDate( startEpoch_ ) + "'" );
            url << "&STOP_TIME=" << urlEncode( curl.get( ), "'" + secondsToIsoDate( endEpoch_ ) + "'" );
            url << "&STEP_SIZE=" << urlEncode( curl.get( ), "'" + stepSize_ + "'" );
        }
        else
        {
            // Build TLIST from epochs (Julian Dates)
            std::ostringstream tlist;
            for( size_t i = 0; i < epochs_.size( ); ++i )
            {
                if( i > 0 ) tlist << ",";
                tlist << std::fixed << std::setprecision( 10 ) << secondsToJulianDate( epochs_[ i ] );
            }
            url << "&TLIST=" << urlEncode( curl.get( ), "'" + tlist.str( ) + "'" );
            url << "&TLIST_TYPE=" << urlEncode( curl.get( ), "'JD'" );
        }

        // Reference plane
        if( frameOrientation == FrameOrientation_ECLIPJ2000 )
        {
            url << "&REF_PLANE=" << urlEncode( curl.get( ), "'ECLIPTIC'" );
        }
        else
        {
            url << "&REF_PLANE=" << urlEncode( curl.get( ), "'FRAME'" );
        }

        // Output units: km and seconds (we'll convert to meters)
        url << "&OUT_UNITS=" << urlEncode( curl.get( ), "'KM-S'" );

        // Vector table type: state vectors only
        url << "&VEC_TABLE=" << urlEncode( curl.get( ), "'2'" );

        // Aberration correction
        if( aberrationCorrection == AberrationCorrection::Geometric )
        {
            url << "&VEC_CORR=" << urlEncode( curl.get( ), "'NONE'" );
        }
        else if( aberrationCorrection == AberrationCorrection::LightTime )
        {
            url << "&VEC_CORR=" << urlEncode( curl.get( ), "'LT'" );
        }
        else
        {
            url << "&VEC_CORR=" << urlEncode( curl.get( ), "'LT+S'" );
        }

        // CSV format for easier parsing
        url << "&CSV_FORMAT=" << urlEncode( curl.get( ), "'YES'" );

        // No labels in data
        url << "&VEC_LABELS=" << urlEncode( curl.get( ), "'NO'" );

        // TDB time type
        url << "&TIME_TYPE=" << urlEncode( curl.get( ), "'TDB'" );

        return url.str( );
    }

    /**
     * @brief Perform HTTP GET request
     */
    std::string performRequest( const std::string& url ) const
    {
        CurlGlobalInit::getInstance( );
        CurlHandle curl;

        std::string response;

        curl_easy_setopt( curl.get( ), CURLOPT_URL, url.c_str( ) );
        curl_easy_setopt( curl.get( ), CURLOPT_WRITEFUNCTION, writeCallback );
        curl_easy_setopt( curl.get( ), CURLOPT_WRITEDATA, &response );
        curl_easy_setopt( curl.get( ), CURLOPT_FOLLOWLOCATION, 1L );
        curl_easy_setopt( curl.get( ), CURLOPT_TIMEOUT, 60L );
        curl_easy_setopt( curl.get( ), CURLOPT_USERAGENT, "Tudat/1.0" );

        // SSL options
        curl_easy_setopt( curl.get( ), CURLOPT_SSL_VERIFYPEER, 1L );
        curl_easy_setopt( curl.get( ), CURLOPT_SSL_VERIFYHOST, 2L );

        CURLcode result = curl_easy_perform( curl.get( ) );

        if( result != CURLE_OK )
        {
            throw HorizonsNetworkException( std::string( "HTTP request failed: " ) + curl_easy_strerror( result ) );
        }

        long httpCode = 0;
        curl_easy_getinfo( curl.get( ), CURLINFO_RESPONSE_CODE, &httpCode );

        if( httpCode != 200 )
        {
            throw HorizonsNetworkException( "HTTP error code: " + std::to_string( httpCode ) );
        }

        return response;
    }

    /**
     * @brief Parse the Horizons API JSON response
     */
    StateHistory parseResponse( const std::string& jsonResponse ) const
    {
        using json = nlohmann::json;

        json response;
        try
        {
            response = json::parse( jsonResponse );
        }
        catch( const json::parse_error& e )
        {
            throw HorizonsParseException( std::string( "JSON parse error: " ) + e.what( ) );
        }

        // Check for API errors
        if( response.contains( "error" ) )
        {
            throw HorizonsException( response[ "error" ].get< std::string >( ) );
        }

        // Get the result text
        if( !response.contains( "result" ) )
        {
            throw HorizonsParseException( "Response missing 'result' field" );
        }

        std::string resultText = response[ "result" ].get< std::string >( );

        // Extract target name from result
        extractTargetName( resultText );

        // Find ephemeris data between $$SOE and $$EOE markers
        size_t soePos = resultText.find( "$$SOE" );
        size_t eoePos = resultText.find( "$$EOE" );

        if( soePos == std::string::npos || eoePos == std::string::npos )
        {
            // Check for common errors in the response
            if( resultText.find( "No matches found" ) != std::string::npos )
            {
                throw HorizonsException( "Target not found: " + targetId_ );
            }
            if( resultText.find( "Multiple major-bodies match" ) != std::string::npos ||
                resultText.find( "Multiple small-bodies match" ) != std::string::npos )
            {
                throw HorizonsException( "Ambiguous target ID. Please use a more specific identifier." );
            }
            throw HorizonsParseException( "Could not find ephemeris data markers ($$SOE/$$EOE)" );
        }

        // Extract data section
        std::string dataSection = resultText.substr( soePos + 5, eoePos - soePos - 5 );

        return parseEphemerisData( dataSection );
    }

    /**
     * @brief Extract target name from response text
     */
    void extractTargetName( const std::string& resultText ) const
    {
        // Look for "Target body name:" line
        std::regex nameRegex( R"(Target body name:\s*([^\n\(]+))" );
        std::smatch match;

        if( std::regex_search( resultText, match, nameRegex ) )
        {
            targetFullName_ = match[ 1 ].str( );
            // Trim whitespace
            targetFullName_.erase( 0, targetFullName_.find_first_not_of( " \t" ) );
            targetFullName_.erase( targetFullName_.find_last_not_of( " \t" ) + 1 );
        }
    }

    /**
     * @brief Parse CSV ephemeris data
     *
     * Expected format (CSV with VEC_TABLE=2):
     * JDTDB, Calendar Date, X, Y, Z, VX, VY, VZ, LT, RG, RR
     *
     * With VEC_LABELS=NO and CSV_FORMAT=YES:
     * 2451545.000000000, 2000-Jan-01 12:00:00.0000, 1.0, 2.0, 3.0, 0.1, 0.2, 0.3, ...
     */
    StateHistory parseEphemerisData( const std::string& dataSection ) const
    {
        StateHistory states;

        std::istringstream stream( dataSection );
        std::string line;

        while( std::getline( stream, line ) )
        {
            // Skip empty lines
            if( line.empty( ) || line.find_first_not_of( " \t\r\n" ) == std::string::npos )
            {
                continue;
            }

            // Parse CSV line
            std::vector< std::string > fields;
            std::istringstream lineStream( line );
            std::string field;

            while( std::getline( lineStream, field, ',' ) )
            {
                // Trim whitespace
                field.erase( 0, field.find_first_not_of( " \t" ) );
                field.erase( field.find_last_not_of( " \t\r\n" ) + 1 );
                fields.push_back( field );
            }

            // We need at least: JD, date string, X, Y, Z, VX, VY, VZ (8 fields)
            if( fields.size( ) < 8 )
            {
                continue;  // Skip malformed lines
            }

            try
            {
                // Parse Julian Date (first field)
                double jd = std::stod( fields[ 0 ] );
                double epoch = julianDateToSeconds( jd );

                // Parse state vector (fields 2-7, index 2-7 are X,Y,Z,VX,VY,VZ)
                // Note: field[1] is the calendar date string
                Vector6d state;
                state( 0 ) = std::stod( fields[ 2 ] ) * 1000.0;  // X: km -> m
                state( 1 ) = std::stod( fields[ 3 ] ) * 1000.0;  // Y: km -> m
                state( 2 ) = std::stod( fields[ 4 ] ) * 1000.0;  // Z: km -> m
                state( 3 ) = std::stod( fields[ 5 ] ) * 1000.0;  // VX: km/s -> m/s
                state( 4 ) = std::stod( fields[ 6 ] ) * 1000.0;  // VY: km/s -> m/s
                state( 5 ) = std::stod( fields[ 7 ] ) * 1000.0;  // VZ: km/s -> m/s

                states[ epoch ] = state;
            }
            catch( const std::exception& e )
            {
                // Skip lines that can't be parsed (might be header or footer)
                continue;
            }
        }

        if( states.empty( ) )
        {
            throw HorizonsParseException( "No valid ephemeris data found in response" );
        }

        return states;
    }
};

// ============================================================================
// HORIZONS QUERY PUBLIC METHODS
// ============================================================================

HorizonsQuery::HorizonsQuery( const std::string& targetId,
                              const std::string& location,
                              double startEpoch,
                              double endEpoch,
                              const std::string& stepSize ): pImpl_( std::make_unique< Impl >( ) )
{
    pImpl_->targetId_ = targetId;
    pImpl_->location_ = location;
    pImpl_->useTimeRange_ = true;
    pImpl_->startEpoch_ = startEpoch;
    pImpl_->endEpoch_ = endEpoch;
    pImpl_->stepSize_ = stepSize;
}

HorizonsQuery::HorizonsQuery( const std::string& targetId, const std::string& location, const std::vector< double >& epochs ):
    pImpl_( std::make_unique< Impl >( ) )
{
    pImpl_->targetId_ = targetId;
    pImpl_->location_ = location;
    pImpl_->useTimeRange_ = false;
    pImpl_->epochs_ = epochs;

    if( epochs.empty( ) )
    {
        throw HorizonsException( "Epoch list cannot be empty" );
    }
}

HorizonsQuery::~HorizonsQuery( ) = default;

HorizonsQuery::HorizonsQuery( HorizonsQuery&& ) noexcept = default;
HorizonsQuery& HorizonsQuery::operator=( HorizonsQuery&& ) noexcept = default;

StateHistory HorizonsQuery::getCartesianStates( FrameOrientation frameOrientation, AberrationCorrection aberrationCorrection ) const
{
    std::string url = pImpl_->buildUrl( frameOrientation, aberrationCorrection );
    std::string response = pImpl_->performRequest( url );
    return pImpl_->parseResponse( response );
}

std::string HorizonsQuery::getTargetId( ) const
{
    return pImpl_->targetId_;
}

std::string HorizonsQuery::getLocation( ) const
{
    return pImpl_->location_;
}

std::string HorizonsQuery::getTargetFullName( ) const
{
    return pImpl_->targetFullName_;
}

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

StateHistory getHorizonsCartesianStateHistory( const std::string& targetId,
                                               const std::string& location,
                                               double startEpoch,
                                               double endEpoch,
                                               const std::string& stepSize,
                                               FrameOrientation frameOrientation )
{
    HorizonsQuery query( targetId, location, startEpoch, endEpoch, stepSize );
    return query.getCartesianStates( frameOrientation );
}

StateHistory getHorizonsCartesianStateHistory( const std::string& targetId,
                                               const std::string& location,
                                               const std::vector< double >& epochs,
                                               FrameOrientation frameOrientation )
{
    HorizonsQuery query( targetId, location, epochs );
    return query.getCartesianStates( frameOrientation );
}

std::string aberrationCorrectionToString( AberrationCorrection correction )
{
    switch( correction )
    {
        case AberrationCorrection::Geometric:
            return "Geometric";
        case AberrationCorrection::LightTime:
            return "LightTime";
        case AberrationCorrection::LightTimeStellar:
            return "LightTimeStellar";
        default:
            return "Unknown";
    }
}

// ============================================================================
// TIME CONVERSION UTILITIES
// ============================================================================

std::string secondsToIsoDate( double secondsSinceJ2000 )
{
    // J2000 epoch: 2000-01-01 12:00:00 TDB
    // Convert to Unix time (seconds since 1970-01-01 00:00:00 UTC)
    // J2000 in Unix time: 946728000 (approximately, ignoring leap seconds)

    // More accurate: J2000 = 2000-01-01T11:58:55.816 UTC
    // For simplicity, we use 2000-01-01 12:00:00
    const double unixTimeAtJ2000 = 946728000.0;

    double unixTime = unixTimeAtJ2000 + secondsSinceJ2000;

    std::time_t timeT = static_cast< std::time_t >( unixTime );
    double fractionalSeconds = unixTime - static_cast< double >( timeT );

    std::tm* tm = std::gmtime( &timeT );
    if( !tm )
    {
        throw HorizonsException( "Failed to convert time" );
    }

    std::ostringstream ss;
    ss << std::put_time( tm, "%Y-%m-%d %H:%M:" );
    ss << std::fixed << std::setprecision( 4 ) << ( tm->tm_sec + fractionalSeconds );

    return ss.str( );
}

double isoDateToSeconds( const std::string& isoDate )
{
    std::tm tm = { };
    double seconds = 0.0;

    // Parse format: "YYYY-MM-DD HH:MM:SS.ssss"
    std::istringstream ss( isoDate );
    ss >> std::get_time( &tm, "%Y-%m-%d %H:%M:" );
    ss >> seconds;

    if( ss.fail( ) )
    {
        throw HorizonsException( "Failed to parse ISO date: " + isoDate );
    }

    // Convert to Unix time
    std::time_t timeT = timegm( &tm );
    double unixTime = static_cast< double >( timeT ) + ( seconds - static_cast< int >( seconds ) );

    // Convert to seconds since J2000
    const double unixTimeAtJ2000 = 946728000.0;
    return unixTime - unixTimeAtJ2000;
}

}  // namespace horizons_interface
}  // namespace tudat

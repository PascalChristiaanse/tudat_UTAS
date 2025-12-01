/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#include "tudat/io/unifiedDataLibraryReader.h"

#include <iostream>
#include <fstream>
#include <sstream>

namespace tudat
{
namespace io
{

using json = nlohmann::json;

// ============================================================================
// UTASObservationSet Implementation
// ============================================================================

template< typename T >
T UTASObservationSet::getRequired( const json& obj, const std::string& key )
{
    if( !obj.contains( key ) )
    {
        throw std::runtime_error( "UTASObservationSet: Required field '" + key + "' not found" );
    }
    
    try
    {
        return obj[ key ].get< T >( );
    }
    catch( const json::type_error& e )
    {
        throw std::runtime_error( "UTASObservationSet: Field '" + key + "' has wrong type: " + e.what( ) );
    }
}

template< typename T >
T UTASObservationSet::getOptional( const json& obj, const std::string& key, const T& defaultValue )
{
    if( !obj.contains( key ) )
    {
        return defaultValue;
    }
    
    try
    {
        return obj[ key ].get< T >( );
    }
    catch( const json::type_error& )
    {
        return defaultValue;
    }
}

/**
 * @brief Get a string field that may be stored as number in JSON
 * 
 * Some fields like "satNo" are semantically identifiers (strings) but stored as numbers.
 */
static std::string getStringOrNumber( const json& obj, const std::string& key )
{
    if( !obj.contains( key ) )
    {
        throw std::runtime_error( "UTASObservationSet: Required field '" + key + "' not found" );
    }
    
    const auto& val = obj[ key ];
    if( val.is_string( ) )
    {
        return val.get< std::string >( );
    }
    else if( val.is_number_integer( ) )
    {
        return std::to_string( val.get< int64_t >( ) );
    }
    else if( val.is_number( ) )
    {
        // For floating point, convert but this is unusual for IDs
        std::ostringstream oss;
        oss << val.get< double >( );
        return oss.str( );
    }
    else
    {
        throw std::runtime_error( "UTASObservationSet: Field '" + key + "' must be string or number" );
    }
}

UTASObservationSet::UTASObservationSet( const json& j )
{
    // Initialize time converter
    timeConverter_ = std::make_shared< teo::TerrestrialTimeScaleConverter >( );
    
    // Validate input is array
    if( !j.is_array( ) )
    {
        throw std::runtime_error( "UTASObservationSet: JSON input must be an array of observations" );
    }
    
    if( j.empty( ) )
    {
        throw std::runtime_error( "UTASObservationSet: Observation array is empty" );
    }
    
    // Parse metadata from first observation
    parseMetadata( j[ 0 ] );
    
    // Validate constant fields are consistent across all observations
    validateMetadataConsistency( j );
    
    // Parse time-varying data
    parseTimeSeries( j );
    
    // Final validation
    if( !timeSeries_.isConsistent( ) )
    {
        throw std::runtime_error( "UTASObservationSet: Time series data is inconsistent" );
    }
}

void UTASObservationSet::parseMetadata( const json& firstObs )
{
    // Station 1 identification and position
    metadata_.station1Id = getRequired< std::string >( firstObs, "origSensorId1" );
    metadata_.station1Position.latitude = getRequired< double >( firstObs, "senlat" );
    metadata_.station1Position.longitude = getRequired< double >( firstObs, "senlon" );
    metadata_.station1Position.altitude = getRequired< double >( firstObs, "senalt" );
    
    // Station 2 identification and position
    metadata_.station2Id = getRequired< std::string >( firstObs, "origSensorId2" );
    metadata_.station2Position.latitude = getRequired< double >( firstObs, "sen2lat" );
    metadata_.station2Position.longitude = getRequired< double >( firstObs, "sen2lon" );
    metadata_.station2Position.altitude = getRequired< double >( firstObs, "sen2alt" );
    
    // Target identification (may be number in JSON)
    metadata_.targetId = getStringOrNumber( firstObs, "satNo" );
    
    // Observation parameters
    metadata_.frequency = getRequired< double >( firstObs, "frequency" );
    metadata_.dataMode = getOptional< std::string >( firstObs, "dataMode", "" );
    metadata_.origin = getOptional< std::string >( firstObs, "origin", "" );
    metadata_.source = getOptional< std::string >( firstObs, "source", "" );
    
    // UTAS-specific fields
    sensor1Delay_ = getOptional< double >( firstObs, "sensor1Delay", 0.0 );
    sensor2Delay_ = getOptional< double >( firstObs, "sensor2Delay", 0.0 );
    bandwidth_ = getOptional< double >( firstObs, "bandwidth", 0.0 );
    ucts_ = getOptional< int >( firstObs, "ucts", 0 );
    
    // Validate positions are not zero (common parsing error indicator)
    if( metadata_.station1Position.isZero( ) )
    {
        std::cerr << "WARNING: Station 1 position is (0,0,0) - this may indicate a parsing error" << std::endl;
    }
    if( metadata_.station2Position.isZero( ) )
    {
        std::cerr << "WARNING: Station 2 position is (0,0,0) - this may indicate a parsing error" << std::endl;
    }
}

void UTASObservationSet::validateMetadataConsistency( const json& observations )
{
    const json& first = observations[ 0 ];
    
    // List of fields that must be constant across all observations
    std::vector< std::string > constantStringFields = {
        "origSensorId1", "origSensorId2", "dataMode", "origin", "source"
    };
    std::vector< std::string > constantStringOrNumberFields = {
        "satNo"  // May be stored as number in JSON
    };
    std::vector< std::string > constantDoubleFields = {
        "senlat", "senlon", "senalt", "sen2lat", "sen2lon", "sen2alt", "frequency"
    };
    
    for( size_t i = 1; i < observations.size( ); ++i )
    {
        const json& obs = observations[ i ];
        
        // Check string fields
        for( const auto& field : constantStringFields )
        {
            if( obs.contains( field ) && first.contains( field ) )
            {
                if( obs[ field ].get< std::string >( ) != first[ field ].get< std::string >( ) )
                {
                    throw std::runtime_error( "UTASObservationSet: Metadata field '" + field + 
                                              "' varies between observations (expected constant). " +
                                              "First value: " + first[ field ].get< std::string >( ) +
                                              ", observation " + std::to_string( i ) + " value: " + 
                                              obs[ field ].get< std::string >( ) );
                }
            }
        }
        
        // Check fields that may be string or number (like satNo)
        for( const auto& field : constantStringOrNumberFields )
        {
            if( obs.contains( field ) && first.contains( field ) )
            {
                std::string firstVal = getStringOrNumber( first, field );
                std::string obsVal = getStringOrNumber( obs, field );
                if( firstVal != obsVal )
                {
                    throw std::runtime_error( "UTASObservationSet: Metadata field '" + field + 
                                              "' varies between observations (expected constant). " +
                                              "First value: " + firstVal +
                                              ", observation " + std::to_string( i ) + " value: " + 
                                              obsVal );
                }
            }
        }
        
        // Check double fields with tolerance
        const double tolerance = 1e-9;
        for( const auto& field : constantDoubleFields )
        {
            if( obs.contains( field ) && first.contains( field ) )
            {
                double firstVal = first[ field ].get< double >( );
                double obsVal = obs[ field ].get< double >( );
                if( std::abs( firstVal - obsVal ) > tolerance )
                {
                    throw std::runtime_error( "UTASObservationSet: Metadata field '" + field + 
                                              "' varies between observations (expected constant). " +
                                              "First value: " + std::to_string( firstVal ) +
                                              ", observation " + std::to_string( i ) + " value: " + 
                                              std::to_string( obsVal ) );
                }
            }
        }
    }
}

void UTASObservationSet::parseTimeSeries( const json& observations )
{
    size_t numObs = observations.size( );
    
    // Reserve space
    timeSeries_.epochs.reserve( numObs );
    timeSeries_.tdoa.reserve( numObs );
    timeSeries_.tdoaUnc.reserve( numObs );
    timeSeries_.fdoa.reserve( numObs );
    timeSeries_.fdoaUnc.reserve( numObs );
    
    for( const auto& obs : observations )
    {
        // Time (required)
        std::string obTime = getRequired< std::string >( obs, "obTime" );
        double epoch = convertIsoStringToEpoch( obTime );
        timeSeries_.epochs.push_back( epoch );
        
        // TDOA (required)
        timeSeries_.tdoa.push_back( getRequired< double >( obs, "tdoa" ) );
        timeSeries_.tdoaUnc.push_back( getOptional< double >( obs, "tdoaUnc", 0.0 ) );
        
        // FDOA (required)
        timeSeries_.fdoa.push_back( getRequired< double >( obs, "fdoa" ) );
        timeSeries_.fdoaUnc.push_back( getOptional< double >( obs, "fdoaUnc", 0.0 ) );
    }
}

double UTASObservationSet::convertIsoStringToEpoch( const std::string& isoTime )
{
    // Strip trailing 'Z' if present
    std::string timeStr = isoTime;
    if( !timeStr.empty( ) && ( timeStr.back( ) == 'Z' || timeStr.back( ) == 'z' ) )
    {
        timeStr.pop_back( );
    }
    
    // Parse ISO 8601 format: YYYY-MM-DDTHH:MM:SS.sss
    if( timeStr.length( ) < 19 )
    {
        throw std::runtime_error( "UTASObservationSet: Invalid time format: " + isoTime );
    }
    
    try
    {
        int year = std::stoi( timeStr.substr( 0, 4 ) );
        int month = std::stoi( timeStr.substr( 5, 2 ) );
        int day = std::stoi( timeStr.substr( 8, 2 ) );
        int hour = std::stoi( timeStr.substr( 11, 2 ) );
        int minute = std::stoi( timeStr.substr( 14, 2 ) );
        double second = std::stod( timeStr.substr( 17 ) );
        
        // Convert to Julian day then to seconds since J2000 (UTC)
        double timeInUTC = tba::timeFromDecomposedDateTime< double >( year, month, day, hour, minute, second );
        
        // Convert UTC to TDB using time scale converter
        // Use a dummy position @ TODO improve with actual station position
        Eigen::Vector3d dummyPosition( 6378.0e3, 0.0, 0.0 );
        double timeInTDB = timeConverter_->getCurrentTime< double >(
            tba::TimeScales::utc_scale, tba::TimeScales::tdb_scale, timeInUTC, dummyPosition );
        
        return timeInTDB;
    }
    catch( const std::exception& e )
    {
        throw std::runtime_error( "UTASObservationSet: Failed to parse time '" + isoTime + "': " + e.what( ) );
    }
}


// ============================================================================
// UTASObservationCollection Implementation
// ============================================================================

UTASObservationCollection::UTASObservationCollection( const std::vector< std::string >& filePaths )
{
    for( const auto& path : filePaths )
    {
        addFromFile( path );
    }
}

void UTASObservationCollection::addFromFile( const std::string& filePath )
{
    std::ifstream file( filePath );
    if( !file.is_open( ) )
    {
        throw std::runtime_error( "UTASObservationCollection: Cannot open file: " + filePath );
    }
    
    json j;
    try
    {
        file >> j;
    }
    catch( const json::parse_error& e )
    {
        throw std::runtime_error( "UTASObservationCollection: JSON parse error in " + filePath + ": " + e.what( ) );
    }
    
    // Handle different JSON structures
    json observations;
    if( j.is_array( ) )
    {
        observations = j;
    }
    else if( j.is_object( ) && j.contains( "observations" ) && j[ "observations" ].is_array( ) )
    {
        observations = j[ "observations" ];
    }
    else
    {
        throw std::runtime_error( "UTASObservationCollection: Unexpected JSON structure in " + filePath );
    }
    
    auto observationSet = std::make_shared< UTASObservationSet >( observations );
    addSet( observationSet );
}

void UTASObservationCollection::addSet( std::shared_ptr< UTASObservationSet > observationSet )
{
    const auto& meta = observationSet->getMetadata( );
    
    // Index by target and station pair
    std::string target = meta.targetId;
    auto stationPair = std::make_pair( meta.station1Id, meta.station2Id );
    
    observationsByTarget_[ target ][ stationPair ].push_back( observationSet );
    
    // Accumulate station positions
    if( stationPositions_.find( meta.station1Id ) == stationPositions_.end( ) )
    {
        stationPositions_[ meta.station1Id ] = meta.station1Position;
    }
    if( stationPositions_.find( meta.station2Id ) == stationPositions_.end( ) )
    {
        stationPositions_[ meta.station2Id ] = meta.station2Position;
    }
}

std::set< std::string > UTASObservationCollection::getObservatoryNames( ) const
{
    std::set< std::string > names;
    for( const auto& entry : stationPositions_ )
    {
        names.insert( entry.first );
    }
    return names;
}

std::map< std::string, GeodeticPosition > UTASObservationCollection::getObservatoryPositions( ) const
{
    return stationPositions_;
}

std::set< std::string > UTASObservationCollection::getObservedTargets( ) const
{
    std::set< std::string > targets;
    for( const auto& entry : observationsByTarget_ )
    {
        targets.insert( entry.first );
    }
    return targets;
}


// ============================================================================
// UTASTudatFormatter Implementation
// ============================================================================

Eigen::Vector3d UTASTudatFormatter::convertToTudatGeodetic( const GeodeticPosition& pos ) const
{
    double longitude = pos.longitude;
    double latitude = pos.latitude;
    double altitude = pos.altitude;
    
    // Convert angles to radians if needed
    if( inputAngleUnit_ == AngleUnit::Degrees )
    {
        longitude *= mathematical_constants::PI / 180.0;
        latitude *= mathematical_constants::PI / 180.0;
    }
    
    // Convert altitude to meters if needed
    if( inputLengthUnit_ == LengthUnit::Kilometers )
    {
        altitude *= 1000.0;
    }
    
    return Eigen::Vector3d( altitude, latitude, longitude );
}

std::shared_ptr< tom::ObservationCollection< double, double > > 
UTASTudatFormatter::toTudat( const UTASObservationCollection& collection,
                             simulation_setup::SystemOfBodies& bodies,
                             const std::vector< std::string >& includedTargets,
                             const std::string& stationBody )
{
    // Ensure station body exists
    try
    {
        bodies.getBody( stationBody );
    }
    catch( const std::runtime_error& )
    {
        bodies.addBody( std::make_shared< simulation_setup::Body >( ), stationBody );
    }
    
    // Create ground stations
    auto observatoryPositions = collection.getObservatoryPositions( );
    for( const auto& entry : observatoryPositions )
    {
        const std::string& stationName = entry.first;
        const GeodeticPosition& pos = entry.second;
        
        std::cout << "Adding ground station " << stationName << " to body " << stationBody << std::endl;
        std::cout << "    Input position (deg, deg, km): " 
                  << pos.latitude << ", " << pos.longitude << ", " << pos.altitude << std::endl;
        
        Eigen::Vector3d tudatPos = convertToTudatGeodetic( pos );
        
        std::cout << "    Tudat position (rad, rad, m): " 
                  << tudatPos( 1 ) << ", " << tudatPos( 2 ) << ", " << tudatPos( 0 ) << std::endl;

        simulation_setup::createGroundStation(
            bodies.getBody( stationBody ), 
            stationName, 
            tudatPos,
            coordinate_conversions::geodetic_position );
    }
    
    // Determine which targets to include
    std::set< std::string > targetSet;
    if( includedTargets.empty( ) )
    {
        targetSet = collection.getObservedTargets( );
    }
    else
    {
        for( const auto& t : includedTargets )
        {
            targetSet.insert( t );
        }
    }
    
    // Debug: print available and requested targets
    std::cout << "Available targets in data: ";
    for( const auto& t : collection.getObservedTargets( ) )
    {
        std::cout << "'" << t << "' ";
    }
    std::cout << std::endl;
    std::cout << "Requested targets: ";
    for( const auto& t : targetSet )
    {
        std::cout << "'" << t << "' ";
    }
    std::cout << std::endl;
    
    // Create empty bodies for targets
    for( const auto& target : targetSet )
    {
        try
        {
            bodies.getBody( target );
            std::cout << "Target body " << target << " already exists" << std::endl;
        }
        catch( const std::runtime_error& )
        {
            bodies.addBody( std::make_shared< simulation_setup::Body >( ), target );
        }
    }
    
    // Build observation sets
    std::vector< std::shared_ptr< tom::SingleObservationSet< double, double > > > observationSetList;
    
    const auto& allObs = collection.getAllObservations( );
    for( const auto& targetEntry : allObs )
    {
        const std::string& target = targetEntry.first;
        if( targetSet.find( target ) == targetSet.end( ) )
        {
            continue;  // Skip targets not in inclusion list
        }
        
        for( const auto& stationPairEntry : targetEntry.second )
        {
            const auto& stationPair = stationPairEntry.first;
            const auto& observationSets = stationPairEntry.second;
            
            const std::string& station1 = stationPair.first;
            const std::string& station2 = stationPair.second;
            
            // Create link definition
            tom::LinkEnds linkEnds;
            linkEnds[ tom::receiver ] = std::make_pair( stationBody, station1 );
            linkEnds[ tom::receiver2 ] = std::make_pair( stationBody, station2 );
            linkEnds[ tom::transmitter ] = std::make_pair( target, std::string( "" ) );
            tom::LinkDefinition linkDefinition( linkEnds );
            
            // Accumulate observations from all sets for this link
            std::vector< double > observationTimes;
            std::vector< Eigen::VectorXd > tdoaObservations;
            std::vector< Eigen::VectorXd > fdoaObservations;
            
            for( const auto& obsSet : observationSets )
            {
                const auto& timeSeries = obsSet->getTimeSeries( );
                
                for( size_t i = 0; i < timeSeries.size( ); ++i )
                {
                    observationTimes.push_back( timeSeries.epochs[ i ] );
                    
                    Eigen::VectorXd tdoaEntry( 1 );
                    tdoaEntry( 0 ) = timeSeries.tdoa[ i ];
                    tdoaObservations.push_back( tdoaEntry );
                    
                    Eigen::VectorXd fdoaEntry( 1 );
                    fdoaEntry( 0 ) = timeSeries.fdoa[ i ];
                    fdoaObservations.push_back( fdoaEntry );
                }
            }
            
            // Create TDOA observation set
            auto tdoaSet = std::make_shared< tom::SingleObservationSet< double, double > >(
                tom::differenced_time_of_arrival,
                linkDefinition,
                tdoaObservations,
                observationTimes,
                tom::receiver );
            observationSetList.push_back( tdoaSet );
            
            // Create FDOA observation set
            auto fdoaSet = std::make_shared< tom::SingleObservationSet< double, double > >(
                tom::differenced_frequency_of_arrival,
                linkDefinition,
                fdoaObservations,
                observationTimes,
                tom::receiver );
            observationSetList.push_back( fdoaSet );
        }
    }
    
    return std::make_shared< tom::ObservationCollection< double, double > >( observationSetList );
}


// ============================================================================
// BatchVLBI Implementation
// ============================================================================

BatchVLBI::BatchVLBI( const std::vector< std::string >& filePaths )
    : collection_( filePaths ), formatter_( )
{
}

std::shared_ptr< tom::ObservationCollection< double, double > > 
BatchVLBI::toTudat( simulation_setup::SystemOfBodies& bodies,
                    const std::vector< std::string >& includedTargets,
                    const std::string& stationBody )
{
    return formatter_.toTudat( collection_, bodies, includedTargets, stationBody );
}


}  // namespace io
}  // namespace tudat

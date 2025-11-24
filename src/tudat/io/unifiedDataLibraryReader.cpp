#include "tudat/io/unifiedDataLibraryReader.h"
#include <limits>

namespace tudat
{
namespace io
{

// ----------------------
// ObservationSet time indexing methods
// ----------------------

void ObservationSet::setTimeColumn( const std::string& columnName )
{
    m_timeColumnName = columnName;
    m_timeIndexBuilt = false;
    m_timeIndex.clear( );
}

const std::string& ObservationSet::getTimeColumnName( ) const
{
    return m_timeColumnName;
}

bool ObservationSet::hasTimeIndex( ) const
{
    return m_timeIndexBuilt;
}

void ObservationSet::buildTimeIndex( )
{
    if( m_timeColumnName.empty( ) )
    {
        throw std::runtime_error( "Cannot build time index: time column not set" );
    }

    auto col = getColumn( m_timeColumnName );
    if( !col )
    {
        throw std::runtime_error( "Cannot build time index: time column '" + m_timeColumnName + "' not found" );
    }

    const double* times = col->asDoublePtr( );
    if( !times )
    {
        throw std::runtime_error( "Cannot build time index: time column '" + m_timeColumnName + "' is not numeric" );
    }

    m_timeIndex.clear( );
    size_t N = col->size( );
    for( size_t i = 0; i < N; ++i )
    {
        m_timeIndex[ times[ i ] ].push_back( i );
    }

    m_timeIndexBuilt = true;
}

void ObservationSet::ensureTimeIndex( ) const
{
    if( !m_timeIndexBuilt )
    {
        const_cast< ObservationSet* >( this )->buildTimeIndex( );
    }
}

const std::vector< size_t >& ObservationSet::getRowsAtTime( double time ) const
{
    ensureTimeIndex( );

    auto it = m_timeIndex.find( time );
    if( it == m_timeIndex.end( ) )
    {
        static const std::vector< size_t > EMPTY;
        return EMPTY;
    }
    return it->second;
}

// ----------------------
// UTASObservationSet
// ----------------------

UTASObservationSet::UTASObservationSet( const json& j )
{
    // 1) find observations array in the JSON
    const json* observations = nullptr;
    json meta = json::object( );

    if( j.is_array( ) )
    {
        observations = &j;
    }
    else if( j.is_object( ) )
    {
        // common patterns: { "observations": [...], "meta": {...} } or top-level object with a single array value
        if( j.contains( "observations" ) && j[ "observations" ].is_array( ) )
        {
            observations = &j[ "observations" ];
        }
        else
        {
            // try to detect the first array property that looks like observations
            for( auto it = j.begin( ); it != j.end( ); ++it )
            {
                if( it.value( ).is_array( ) )
                {
                    observations = &it.value( );
                    break;
                }
            }
        }
        // metadata: we may have metadata in top-level fields; copy them for later safe lookup
        meta = j;
    }

    if( !observations || observations->empty( ) )
    {
        throw std::runtime_error( "UTASObservationSet: JSON does not contain an observations array or it is empty" );
    }

    // 2) Try to read ancillary fields from meta or the first observation
    const json& firstObs = ( *observations )[ 0 ];

    auto readStringSafe = [ & ]( const json& obj, const std::string& key ) -> std::string {
        if( obj.contains( key ) && obj[ key ].is_string( ) ) return obj[ key ].get< std::string >( );
        return std::string( );
    };
    
    satNo = readStringSafe( meta, "satNo" ).empty( ) ? readStringSafe( firstObs, "satNo" ) : readStringSafe( meta, "satNo" );
    dataMode = readStringSafe( meta, "dataMode" ).empty( ) ? readStringSafe( firstObs, "dataMode" ) : readStringSafe( meta, "dataMode" );
    origSensorId1 = readStringSafe( meta, "origSensorId1" ).empty( ) ? readStringSafe( firstObs, "origSensorId1" )
                                                                     : readStringSafe( meta, "origSensorId1" );
    origSensorId2 = readStringSafe( meta, "origSensorId2" ).empty( ) ? readStringSafe( firstObs, "origSensorId2" )
                                                                     : readStringSafe( meta, "origSensorId2" );
    origin = readStringSafe( meta, "origin" ).empty( ) ? readStringSafe( firstObs, "origin" ) : readStringSafe( meta, "origin" );
    source = readStringSafe( meta, "source" ).empty( ) ? readStringSafe( firstObs, "source" ) : readStringSafe( meta, "source" );
    // ucts may be integer
    if( meta.contains( "ucts" ) && meta[ "ucts" ].is_number_integer( ) )
    {
        ucts = meta[ "ucts" ].get< int >( );
    }
    else if( firstObs.contains( "ucts" ) && firstObs[ "ucts" ].is_number_integer( ) )
    {
        ucts = firstObs[ "ucts" ].get< int >( );
    }
    else
    {
        ucts = 0;
    }

    // 3) Determine columns from the keys of the first observation.
    //    We will create appropriate typed columns based on fieldTypeMap or by simple heuristic.
    for( auto& el : firstObs.items( ) )
    {
        const std::string fieldName = el.key( );

        // If field is declared as ancillary in fieldTypeMap, we still want it as a column if it's actually per-row data
        // (fieldTypeMap defines types; do not skip creating columns for those unless they are strictly metadata.)
        std::string declaredType;
        auto itFt = fieldTypeMap.find( fieldName );
        if( itFt != fieldTypeMap.end( ) ) declaredType = itFt->second;

        // Create column object depending on declaredType or inference
        if( declaredType == "string" )
        {
            auto col = std::make_shared< TypedColumn< std::string > >( fieldName );
            addColumn( col );
        }
        else if( declaredType == "double" )
        {
            auto col = std::make_shared< NumericColumn >( fieldName );
            addColumn( col );
        }
        else
        {
            // infer from the first row value
            const json& v = el.value( );
            if( v.is_number( ) )
            {
                auto col = std::make_shared< NumericColumn >( fieldName );
                addColumn( col );
            }
            else
            {
                auto col = std::make_shared< TypedColumn< std::string > >( fieldName );
                addColumn( col );
            }
        }
    }

    // 4) Pre-reserve sizes for numeric columns
    size_t numRows = observations->size( );
    for( const auto& name : columnNames( ) )
    {
        auto colBase = getColumn( name );
        if( !colBase ) continue;
        if( auto numc = std::dynamic_pointer_cast< NumericColumn >( colBase ) )
        {
            numc->reserve( numRows );
        }
        else if( auto strc = std::dynamic_pointer_cast< TypedColumn< std::string > >( colBase ) )
        {
            strc->reserve( numRows );
        }
    }

    // 5) Populate columns row-by-row
    for( const auto& row : *observations )
    {
        for( const auto& name : columnNames( ) )
        {
            auto colBase = getColumn( name );
            if( !colBase ) continue;

            // If column is numeric
            if( auto numc = std::dynamic_pointer_cast< NumericColumn >( colBase ) )
            {
                // prefer numeric types; if missing or non-numeric, push NaN
                double value = std::numeric_limits< double >::quiet_NaN( );
                if( row.contains( name ) && row[ name ].is_number( ) )
                {
                    value = row[ name ].get< double >( );
                }
                else if( row.contains( name ) && row[ name ].is_string( ) )
                {
                    // attempt parse string to double if client sometimes sends numbers as strings
                    try
                    {
                        value = std::stod( row[ name ].get< std::string >( ) );
                    }
                    catch( ... )
                    {
                        value = std::numeric_limits< double >::quiet_NaN( );
                    }
                }
                numc->add( value );
            }
            else if( auto strc = std::dynamic_pointer_cast< TypedColumn< std::string > >( colBase ) )
            {
                std::string value;
                if( row.contains( name ) && row[ name ].is_string( ) )
                {
                    value = row[ name ].get< std::string >( );
                }
                else if( row.contains( name ) && row[ name ].is_number( ) )
                {
                    // convert numbers to string
                    value = std::to_string( row[ name ].get< double >( ) );
                }
                else if( row.contains( name ) && row[ name ].is_null( ) )
                {
                    value = "";
                }
                else
                {
                    // missing -> empty string
                    value = "";
                }
                strc->add( value );
            }
            else
            {
                // unknown column type -> skip
            }
        }
    }
}

json UTASObservationSet::toJson( ) const
{
    json j;
    j[ "columns" ] = json::array( );

    for( const auto& name : columnNames( ) )
    {
        auto col = getColumn( name );
        json col_json;
        col_json[ "name" ] = col->name( );
        col_json[ "type" ] = col->type( );
        col_json[ "data" ] = json::array( );

        if( col->type( ) == typeid( double ).name( ) )
        {
            auto num_col = std::dynamic_pointer_cast< NumericColumn >( col );
            for( size_t i = 0; i < num_col->size( ); ++i )
            {
                col_json[ "data" ].push_back( ( *num_col )[ i ] );
            }
        }
        else
        {
            throw std::runtime_error( "Unsupported column type for serialization: " + col->type( ) );
        }

        j[ "columns" ].push_back( col_json );
    }

    return j;
}

NumericColumn::NumericColumn( const std::string& name ): TypedColumn< double >( name ) {}

Eigen::Map< Eigen::VectorXd > NumericColumn::asEigenMap( )
{
    return Eigen::Map< Eigen::VectorXd >( this->TypedColumn< double >::data( ), this->size( ) );
}

Eigen::Map< const Eigen::VectorXd > NumericColumn::asEigenMap( ) const
{
    return Eigen::Map< const Eigen::VectorXd >( this->TypedColumn< double >::data( ), this->size( ) );
}

void ObservationSet::addColumn( std::shared_ptr< BaseColumn > col )
{
    m_columns[ col->name( ) ] = col;
    m_order.push_back( col->name( ) );
}

std::shared_ptr< BaseColumn > ObservationSet::getColumn( const std::string& name ) const
{
    auto it = m_columns.find( name );
    if( it != m_columns.end( ) ) return it->second;
    return nullptr;
}

Eigen::Map< const Eigen::VectorXd > ObservationSet::asEigen( const std::string& name ) const
{
    auto col = getColumn( name );
    if( !col ) throw std::runtime_error( "Column not found: " + name );

    const double* ptr = col->asDoublePtr( );
    if( !ptr ) throw std::runtime_error( "Column is not numeric: " + name );

    return Eigen::Map< const Eigen::VectorXd >( ptr, col->size( ) );
}

size_t ObservationSet::numRows( ) const
{
    if( m_columns.empty( ) ) return 0;
    return m_columns.begin( )->second->size( );
}

const std::vector< std::string >& ObservationSet::columnNames( ) const
{
    return m_order;
}

// ----------------------
// UTASObservationSet constructor with time column
// ----------------------

UTASObservationSet::UTASObservationSet( const json& j, const std::string& timeColumnName ): UTASObservationSet( j )
{
    // 6) Convert obTime string column to epoch numeric column
    auto obTimeColumn = getColumn( timeColumnName );
    if( !obTimeColumn ) throw std::runtime_error( "UTASObservationSet: time column '" + timeColumnName + "' not found." );

    auto strCol = std::dynamic_pointer_cast< TypedColumn< std::string > >( obTimeColumn );
    if( strCol )
    {
        // Create a new numeric column for converted times
        auto epochCol = std::make_shared< NumericColumn >( timeColumnName + "Epoch" );
        epochCol->reserve( strCol->size( ) );

        for( size_t i = 0; i < strCol->size( ); ++i )
        {
            const std::string& isoTime = ( *strCol )[ i ];
            // try
            // {
            auto epoch = convertIsoStringToEpoch( isoTime );
            epochCol->add( epoch );
            // }
            // catch( ... )
            // {
            //     epochCol->add( std::numeric_limits< tudat::Time >::quiet_NaN( ) );
            // }
        }

        addColumn( epochCol );
        setTimeColumn( "obTimeEpoch" );
        buildTimeIndex( );
    }
}

double UTASObservationSet::convertIsoStringToEpoch( const std::string& t )
{
    // Simple ISO 8601 to epoch converter (assumes UTC, uses tudat to convert to TBD Time object)

    // Location placeholder
    Eigen::Vector3d dummyPosition( 6378.0e3, 0.0, 0.0 );

    // Strip z or Z suffix if present
    std::string timeStr = t;
    if( !timeStr.empty( ) && ( timeStr.back( ) == 'Z' || timeStr.back( ) == 'z' ) )
    {
        timeStr.pop_back( );
    }
    int year = std::stoi( timeStr.substr( 0, 4 ) );
    int month = std::stoi( timeStr.substr( 5, 2 ) );
    int day = std::stoi( timeStr.substr( 8, 2 ) );
    int hour = std::stoi( timeStr.substr( 11, 2 ) );
    int minute = std::stoi( timeStr.substr( 14, 2 ) );
    double second = std::stod( timeStr.substr( 17, timeStr.size( ) - 17 ) );
    // std::cout << "Parsed time: " << year << "-" << month << "-" << day << " " << hour << ":" << minute << ":" << second << std::endl;
    // auto reference = tudat::basic_astrodynamics::getJulianDayOnJ2000< long double >( );
    auto timeInUTC = tba::timeFromDecomposedDateTime< double >( year, month, day, hour, minute, second );
    auto timeInTBD = defaultTimeScaleConverter->getCurrentTime< double >(
            tba::TimeScales::utc_scale, tba::TimeScales::tdb_scale, timeInUTC, dummyPosition );

    // std::cout<< "Time difference UTC to TDB: " << timeInTBD - timeInUTC << " seconds." <<std::endl;
    return timeInTBD;
}

std::shared_ptr< tom::ObservationCollection< double, double > > BatchVLBI::toTudat( SystemOfBodies& bodies,
                                                                                    const std::vector< std::string >& included_satellites,
                                                                                    const std::string& station_body )
{
    /* Converts the observations in the batch into a Tudat compatible format and
          sets up the relevant Tudat infrastructure to support estimation.
        This method does the following:\\
            0. Ensures station body exists in the system of bodies.\\
            1. Creates an empty body for each observed entity with their satId id as name.\\
            2. Adds this body to the system of bodies inputted to the method.\\
            3. Retrieves the global position of the terrestrial observatories from the UDL and adds these stations to the Tudat
       environment.\\
            4. Creates link definitions between each unique terrestrial
            observatory/ entity combination in the batch.\\
            5. Creates a `SingleObservationSet` object for each unique link that
            includes all observations for that link.\\
            6. (By Default) Add the relevant weights to the `SingleObservationSet`
            per observation.\\
            7. Combine the SingleObservationSet objects into an ObservationCollection
            and return the observations
            */

    // Step 0: Ensure station body exists
    try
    {
        bodies.getBody( station_body );
        // std::cout << "Station body " << station_body << " already exists in system of bodies." << std::endl;
    }
    catch( std::runtime_error& e )
    {
        bodies.addBody( std::make_shared< Body >( ), station_body );
    }

    // Step 1: Create empty bodies for each observed entity
    auto entities = getObservedObjects( );
    for( const auto& entity : entities )
    {
        try
        {
            bodies.getBody( entity );
            std::cout << "Body " << entity << " already exists in system of bodies." << std::endl;
        }
        catch( std::runtime_error& e )
        {
            // Step 2: Add body to system of bodies
            bodies.addBody( std::make_shared< Body >( ), entity );
        }
    }

    // Step 3: Add terrestrial observatories to Tudat environment
    auto observatoryMap = getObservatoryPositions( );
    for( const auto& observatory : observatoryMap )
    {
        auto geodeticCoordinates = Eigen::Vector3d( observatory.second[ 0 ] / 180.0 * mathematical_constants::PI,
                                                    observatory.second[ 1 ] / 180.0 * mathematical_constants::PI,
                                                    observatory.second[ 2 ] * 1000.0 );  // Convert km to m
        createGroundStation(
                bodies.getBody( station_body ), observatory.first, geodeticCoordinates, coordinate_conversions::geodetic_position );
    }

    // Step 4: Create link ends and observation models for each unique entity/(observatory pair) pair

    // Map:
    //  - Key: observed object,
    //  - Value: map:
    //       -- Key: pair(Observatory1, Observatory2)
    //       -- Value: vector of observation set pointers
    auto objectObservatoryPairs = getAllObservations( );
    std::vector< std::shared_ptr< tom::SingleObservationSet< double, double > > > observationSetList;
    for( const auto& setup : objectObservatoryPairs )
    {
        for( const auto& observationSetPtr : setup.second )
        {
            std::string entity = setup.first;
            std::string observatory1 = observationSetPtr.first.first;
            std::string observatory2 = observationSetPtr.first.second;
            auto data = observationSetPtr.second;

            tom::LinkEnds linkEnds;
            linkEnds[ observation_models::receiver ] = std::make_pair( observatory1, station_body );
            linkEnds[ observation_models::receiver2 ] = std::make_pair( observatory2, station_body );
            linkEnds[ observation_models::transmitter2 ] = std::make_pair( entity, std::string( "" ) );
            auto linkDefinition = tom::LinkDefinition( linkEnds );

            // Step 5: Create SingleObservationSet for each unique link

            // Compile all observation times and observations for this link
            std::vector< double > observationTimes;
            std::vector< Eigen::VectorXd > tdoaObservations;
            for( const auto& set : data )
            {
                auto timeCol = std::dynamic_pointer_cast< NumericColumn >( set->getColumn( "obTimeEpoch" ) );
                auto tdoaCol = std::dynamic_pointer_cast< NumericColumn >( set->getColumn( "tdoa" ) );

                observationTimes.insert(
                        observationTimes.end( ), std::make_move_iterator( timeCol->begin( ) ), std::make_move_iterator( timeCol->end( ) ) );
                for( auto it = tdoaCol->begin( ); it != tdoaCol->end( ); ++it )
                {
                    Eigen::VectorXd entry( 1 );
                    entry( 0 ) = *it;
                    tdoaObservations.push_back( entry );
                }
            }

            auto observationSet =
                    std::make_shared< tom::SingleObservationSet< double, double > >( tom::differenced_time_of_arrival,
                                                                                     linkDefinition,
                                                                                     tdoaObservations,
                                                                                     observationTimes,
                                                                                     tom::receiver );  // TODO confirm reference LinkEndType
            observationSetList.push_back( observationSet );
        }
    }
    // Step 6: Add weights to each SingleObservationSet TODO Implement (by standard deviation?)

    // Step 7: Create ObservationCollection from SingleObservationSets
    auto observationCollection = std::make_shared< tom::ObservationCollection< double, double > >( observationSetList );
    return observationCollection;
}
}  // namespace io
}  // namespace tudat

/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 *
 */

#ifndef TUDAT_UNIFIED_DATA_LIBRARY_READER_H
#define TUDAT_UNIFIED_DATA_LIBRARY_READER_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <span>
#include <Eigen/Dense>
#include <unordered_map>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <type_traits>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "tudat/basics/timeType.h"
#include "tudat/astro/observation_models/linkTypeDefs.h"
#include "tudat/astro/observation_models/observationModel.h"
#include "tudat/astro/basic_astro/dateTime.h"
#include "tudat/astro/earth_orientation/terrestrialTimeScaleConverter.h"
#include "tudat/astro/basic_astro/timeConversions.h"
#include <tudat/simulation/simulation.h>
#include <tudat/simulation/estimation_setup/singleObservationSet.h>
#include <tudat/simulation/estimation_setup/observationCollection.h>
#include <tudat/simulation/environment_setup/createGroundStations.h>
#include "tudat/interface/spice/spiceEphemeris.h"
#include "tudat/math/basic/mathematicalConstants.h"

namespace tba = tudat::basic_astrodynamics;
namespace teo = tudat::earth_orientation;
namespace tom = tudat::observation_models;

using namespace tudat;
using namespace tudat::simulation_setup;
using namespace tudat::propagators;
using namespace tudat::numerical_integrators;
using namespace tudat::orbital_element_conversions;
using namespace tudat::unit_conversions;
using namespace tudat::basic_astrodynamics;
using namespace tudat::basic_mathematics;
using namespace tudat::physical_constants;
using namespace tudat::gravitation;
using namespace tudat::numerical_integrators;

namespace tudat
{
namespace io
{

// ----------------------
// Base column interface
// ----------------------
class BaseColumn
{
public:
    virtual ~BaseColumn( ) = default;

    virtual const std::string& name( ) const = 0;
    virtual size_t size( ) const = 0;

    // Return nullptr for non-numeric columns
    virtual const double* asDoublePtr( ) const
    {
        return nullptr;
    }

    virtual std::string type( ) const = 0;
};

// ----------------------
// Typed column template
// ----------------------
template< typename T >
class TypedColumn : public BaseColumn
{
public:
    // clang-format off
    TypedColumn(const std::string& name) : m_name(name) {}
        
    // Data manipulation
    void add(const T& value) { m_data.push_back(value); }
    void reserve(size_t n) { m_data.reserve(n); }
    const T* data() const { return m_data.data(); }
    T* data() { return m_data.data(); }


    // Metadata
    size_t size() const override { return m_data.size(); }
    const std::string& name() const override { return m_name; }
    std::string type() const override { return typeid(T).name(); }


    // Numeric support
    const double* asDoublePtr() const override {
        // if constexpr is not available in C++14; use a runtime branch with compile-time trait check
        if (std::is_same<T, double>::value) {
            return reinterpret_cast<const double*>(m_data.data());
        }
        return nullptr;
    }

    // Element access
    const T& operator[](size_t i) const { return m_data[i]; }
    T& operator[](size_t i) { return m_data[i]; }


    // Iterators
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    iterator begin() { return m_data.begin(); } 
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }
    // clang-format on
private:
    std::string m_name;
    std::vector< T > m_data;
};

// ----------------------
// Numeric column helper
// ----------------------
class NumericColumn : public TypedColumn< double >
{
public:
    NumericColumn( const std::string& name );
    Eigen::Map< Eigen::VectorXd > asEigenMap( );
    Eigen::Map< const Eigen::VectorXd > asEigenMap( ) const;
};

class ObservationSet
{
public:
    void addColumn( std::shared_ptr< BaseColumn > col );
    std::shared_ptr< BaseColumn > getColumn( const std::string& name ) const;
    Eigen::Map< const Eigen::VectorXd > asEigen( const std::string& name ) const;
    size_t numRows( ) const;
    const std::vector< std::string >& columnNames( ) const;

    // Time indexing interface
    void setTimeColumn( const std::string& columnName );
    const std::string& getTimeColumnName( ) const;
    bool hasTimeIndex( ) const;
    void buildTimeIndex( );
    const std::vector< size_t >& getRowsAtTime( double time ) const;

private:
    void ensureTimeIndex( ) const;

    std::unordered_map< std::string, std::shared_ptr< BaseColumn > > m_columns;
    std::vector< std::string > m_order;

    // Time indexing state
    std::string m_timeColumnName;
    mutable bool m_timeIndexBuilt = false;
    mutable std::map< double, std::vector< size_t > > m_timeIndex;
};

struct UTASObservationSetAncillaryFieldProtocol {
    std::string dataMode;
    std::string origSensorId1;
    std::string origSensorId2;
    std::string origin;
    std::string source;
    
    double senalt;
    double senlat;
    double senlon;
    double sen2alt;
    double sen2lat;
    double sen2lon;
    
    std::string satNo;
    int ucts;

    // NumericColumn bandwidth;
    // NumericColumn fdoa;
    // NumericColumn fdoaUnc;
    // NumericColumn tdoa;
    // NumericColumn tdoaUnc;
    // NumericColumn frequency;
    // NumericColumn sensor1Delay;
    // NumericColumn sensor2Delay;
    const std::unordered_map< std::string, std::string > fieldTypeMap = {
        // clang-format off
        {"bandwidth", "double"},
        {"fdoa", "double"},
        {"fdoaUnc", "double"},
        {"frequency", "double"},
        {"obTime", "string"},
        {"sensor1Delay", "double"},
        {"sensor2Delay", "double"},
        {"tdoa", "double"},
        {"tdoaUnc", "double"},
        // clang-format on
    };
};

class UTASObservationSet : public ObservationSet, public UTASObservationSetAncillaryFieldProtocol
{
public:
    json toJson( ) const;
    UTASObservationSet( const json& j );
    UTASObservationSet( const json& j, const std::string& timeColumnName );

    double convertIsoStringToEpoch( const std::string& t );

private:
    decltype( teo::createDefaultTimeConverter( ) ) defaultTimeScaleConverter = teo::createDefaultTimeConverter( );
};

template< typename ObservationSetType = ObservationSet >
class ObservationCollection
{
    // static_assert( std::is_base_of< ObservationSet, ObservationSetType >::value, "ObservationSetType must derive from ObservationSet"
    // );

public:
    ObservationCollection( ) = default;
};

class UTASObservationCollection : public ObservationCollection< UTASObservationSet >
{
public:
    UTASObservationCollection( const std::vector< std::string >& filePaths )
    {
        for( const auto& filePath : filePaths )
        {
            addSet( filePath );
        }
    }

    UTASObservationCollection( const std::vector< UTASObservationSet >& observationSets )
    {
        for( const auto& observationSet : observationSets )
        {
            addSet( observationSet );
        }
    }

    UTASObservationCollection( const std::vector< std::shared_ptr< UTASObservationSet > >& observationSets )
    {
        for( const auto& observationSet : observationSets )
        {
            addSet( observationSet );
        }
    }

    void addSet( const std::string& filePath )
    {
        std::ifstream udlFileStream( filePath );
        json udlJson = json::parse( udlFileStream );
        auto observationSet = UTASObservationSet( udlJson, "obTime" );

        auto setKey = std::make_pair( observationSet.origSensorId1, observationSet.origSensorId2 );
        m_observatoryPairs[ observationSet.satNo ][ setKey ].push_back(
                std::make_shared< UTASObservationSet >( observationSet ) );
        
        m_observatoryPositions[ observationSet.origSensorId1 ] =
                Eigen::Vector3d( observationSet.senlon, observationSet.senlat, observationSet.senalt );
        m_observatoryPositions[ observationSet.origSensorId2 ] =
                Eigen::Vector3d( observationSet.sen2lon, observationSet.sen2lat, observationSet.sen2alt );
    }

    
    void addSet( const UTASObservationSet& observationSet )
    {
        auto setKey = std::make_pair( observationSet.origSensorId1, observationSet.origSensorId2 );
        m_observatoryPairs[ observationSet.satNo ][ setKey ].push_back(
                std::make_shared< UTASObservationSet >( observationSet ) );
    }

    void addSet( std::shared_ptr< UTASObservationSet > observationSet )
    {
        auto setKey = std::make_pair( observationSet->origSensorId1, observationSet->origSensorId2 );
        m_observatoryPairs[ observationSet->satNo ][ setKey ].push_back( observationSet );
    }

    std::set< std::string > getObservatoryNames( )
    {
        // Create unique list of observatories
        auto observatories = std::set< std::string >( );
        for( const auto& entry : m_observatoryPairs )
        {
            for( const auto& obsPairEntry : entry.second )
            {
                observatories.insert( obsPairEntry.first.first );
                observatories.insert( obsPairEntry.first.second );
            }
        }
        return observatories;
    }

    std::map< std::string, Eigen::Vector3d > getObservatoryPositions( )
    {
        return m_observatoryPositions;
    }

    std::set< std::string > getObservedObjects( )
    {
        auto objects = std::set< std::string >( );
        for( const auto& entry : m_observatoryPairs )
        {
            objects.insert( entry.first );
        }
        return objects;
    }

    std::vector< std::shared_ptr< UTASObservationSet > > getObservationsByObservatoryPair(
            std::pair< std::string, std::string > observatoryPair )
    {
        auto observations = std::vector< std::shared_ptr< UTASObservationSet > >( );
        for( const auto& entry : m_observatoryPairs )
        {
            auto it = entry.second.find( observatoryPair );
            for( const auto& obsSetPtr : it->second )
            {
                observations.push_back( obsSetPtr );
            }
        }
        return observations;
    }

    std::map< std::pair< std::string, std::string >, std::shared_ptr< UTASObservationSet > > getObservationsByObject( std::string object )
    {
        auto observations = std::map< std::pair< std::string, std::string >, std::shared_ptr< UTASObservationSet > >( );
        for( const auto& entry : m_observatoryPairs )
        {
            if( entry.first == object )
            {
                for( const auto& obsSetEntry : entry.second )
                {
                    for( const auto& obsSetPtr : obsSetEntry.second )
                    {
                        observations[ obsSetEntry.first ] = obsSetPtr;
                    }
                }
            }
        }
        return observations;
    }

    std::map< std::string, std::map< std::pair< std::string, std::string >, std::vector< std::shared_ptr< UTASObservationSet > > > > getAllObservations( )
    {
        return m_observatoryPairs;
    }

private:
    // Map:
    //  - Key: observed object,
    //  - Value: map:
    //       -- Key: pair(Observatory1, Observatory2)
    //       -- Value: vector of observation set pointers
    std::map< std::string, std::map< std::pair< std::string, std::string >, std::vector< std::shared_ptr< UTASObservationSet > > > >
            m_observatoryPairs;
    
    // Map of observatory names to positions
    // - Key: observatory name
    // - Value: position vector (Eigen::Vector3d) (degrees lon, lat; km alt)
    std::map< std::string, Eigen::Vector3d > m_observatoryPositions;
};

class BatchVLBI : public UTASObservationCollection
{
public:
    using UTASObservationCollection::UTASObservationCollection;
    std::shared_ptr< tom::ObservationCollection< double, double > > toTudat( SystemOfBodies& bodies,
                                                                             const std::vector< std::string >& included_satellites,
                                                                             const std::string& station_body = "Earth" );
};

}  // namespace io
}  // namespace tudat
#endif  // TUDAT_UNIFIED_DATA_LIBRARY_READER_H
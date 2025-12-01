/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 *    Notes
 *      HDF5 output interface for Tudat simulation results.
 *      Provides hierarchical storage of propagation results with XDMF3 compatibility.
 *
 */

#ifndef TUDAT_IO_HDF5_MANAGER_H
#define TUDAT_IO_HDF5_MANAGER_H

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
#include <type_traits>

#include <highfive/highfive.hpp>

#include "tudat/basics/timeType.h"
#include "tudat/simulation/propagation_setup/propagationResults.h"
#include "tudat/simulation/propagation_setup/dependentVariablesInterface.h"
#include "tudat/simulation/estimation_setup/singleObservationSet.h"
#include "tudat/simulation/estimation_setup/observationCollection.h"
#include "tudat/simulation/estimation_setup/observationOutputSettings.h"
#include "tudat/astro/observation_models/observableTypes.h"
#include "tudat/io/xdmfGenerator.h"

namespace tudat
{
namespace io
{

// ================================================================================================
// Time conversion utilities
// ================================================================================================

//! Convert a time value to double (handles both double and Time types)
template< typename TimeType >
inline double timeToDouble( const TimeType& time )
{
    if constexpr ( std::is_same_v< TimeType, Time > )
    {
        return time.template getSeconds< double >( );
    }
    else
    {
        return static_cast< double >( time );
    }
}

//! Convert a state scalar to double
template< typename StateScalarType >
inline double stateScalarToDouble( const StateScalarType& scalar )
{
    return static_cast< double >( scalar );
}

// ================================================================================================
// Data conversion utilities
// ================================================================================================

//! Convert std::map<TimeType, Eigen::VectorXd> to separate time and data arrays
template< typename TimeType, typename StateScalarType >
void convertStateHistoryToArrays(
    const std::map< TimeType, Eigen::Matrix< StateScalarType, Eigen::Dynamic, 1 > >& stateHistory,
    std::vector< double >& times,
    std::vector< std::vector< double > >& states )
{
    times.clear( );
    states.clear( );
    times.reserve( stateHistory.size( ) );
    states.reserve( stateHistory.size( ) );
    
    for ( const auto& [time, state] : stateHistory )
    {
        times.push_back( timeToDouble( time ) );
        std::vector< double > stateVec( state.size( ) );
        for ( int i = 0; i < state.size( ); ++i )
        {
            stateVec[i] = stateScalarToDouble( state( i ) );
        }
        states.push_back( stateVec );
    }
}

//! Convert std::map<TimeType, Eigen::VectorXd> (dependent variables) to arrays
template< typename TimeType >
void convertDependentVariableHistoryToArrays(
    const std::map< TimeType, Eigen::VectorXd >& depVarHistory,
    std::vector< double >& times,
    std::vector< std::vector< double > >& depVars )
{
    times.clear( );
    depVars.clear( );
    times.reserve( depVarHistory.size( ) );
    depVars.reserve( depVarHistory.size( ) );
    
    for ( const auto& [time, depVar] : depVarHistory )
    {
        times.push_back( timeToDouble( time ) );
        std::vector< double > depVarVec( depVar.size( ) );
        for ( int i = 0; i < depVar.size( ); ++i )
        {
            depVarVec[i] = depVar( i );
        }
        depVars.push_back( depVarVec );
    }
}

//! Convert std::map<TimeType, Eigen::MatrixXd> to arrays (for variational results)
template< typename TimeType >
void convertMatrixHistoryToArrays(
    const std::map< TimeType, Eigen::MatrixXd >& matrixHistory,
    std::vector< double >& times,
    std::vector< std::vector< double > >& matrices )
{
    times.clear( );
    matrices.clear( );
    times.reserve( matrixHistory.size( ) );
    matrices.reserve( matrixHistory.size( ) );
    
    for ( const auto& [time, matrix] : matrixHistory )
    {
        times.push_back( timeToDouble( time ) );
        // Flatten matrix in row-major order
        std::vector< double > flatMatrix( matrix.rows( ) * matrix.cols( ) );
        for ( int i = 0; i < matrix.rows( ); ++i )
        {
            for ( int j = 0; j < matrix.cols( ); ++j )
            {
                flatMatrix[i * matrix.cols( ) + j] = matrix( i, j );
            }
        }
        matrices.push_back( flatMatrix );
    }
}

//! Convert std::map<std::pair<int,int>, std::string> to vectors for HDF5 storage
inline void convertIdMapToArrays(
    const std::map< std::pair< int, int >, std::string >& idMap,
    std::vector< int >& startIndices,
    std::vector< int >& sizes,
    std::vector< std::string >& names )
{
    startIndices.clear( );
    sizes.clear( );
    names.clear( );
    startIndices.reserve( idMap.size( ) );
    sizes.reserve( idMap.size( ) );
    names.reserve( idMap.size( ) );
    
    for ( const auto& [indexPair, name] : idMap )
    {
        startIndices.push_back( indexPair.first );
        sizes.push_back( indexPair.second );
        names.push_back( name );
    }
}

// ================================================================================================
// Observation data conversion utilities
// ================================================================================================

//! Convert a vector of observation vectors to a 2D array for HDF5 storage
template< typename ObservationScalarType >
void convertObservationsToArrays(
    const std::vector< Eigen::Matrix< ObservationScalarType, Eigen::Dynamic, 1 > >& observations,
    std::vector< std::vector< double > >& observationArrays )
{
    observationArrays.clear( );
    observationArrays.reserve( observations.size( ) );
    
    for ( const auto& obs : observations )
    {
        std::vector< double > obsVec( obs.size( ) );
        for ( int i = 0; i < obs.size( ); ++i )
        {
            obsVec[i] = static_cast< double >( obs( i ) );
        }
        observationArrays.push_back( obsVec );
    }
}

//! Convert observation times to double array
template< typename TimeType >
void convertObservationTimesToDoubleArray(
    const std::vector< TimeType >& times,
    std::vector< double >& doubleTimes )
{
    doubleTimes.clear( );
    doubleTimes.reserve( times.size( ) );
    
    for ( const auto& t : times )
    {
        doubleTimes.push_back( timeToDouble( t ) );
    }
}

//! Convert LinkEnds to serializable string arrays
inline void serializeLinkEnds(
    const observation_models::LinkEnds& linkEnds,
    std::vector< int >& linkEndTypes,
    std::vector< std::string >& bodyNames,
    std::vector< std::string >& stationNames )
{
    linkEndTypes.clear( );
    bodyNames.clear( );
    stationNames.clear( );
    
    for ( const auto& [linkEndType, linkEndId] : linkEnds )
    {
        linkEndTypes.push_back( static_cast< int >( linkEndType ) );
        bodyNames.push_back( linkEndId.bodyName_ );
        stationNames.push_back( linkEndId.stationName_ );
    }
}

//! Get string name for an observable type
inline std::string getObservableTypeName( const observation_models::ObservableType observableType )
{
    return observation_models::getObservableName( observableType );
}

//! Convert dependent variable bookkeeping to ID map format
inline std::map< std::pair< int, int >, std::string > convertDependentVariableBookkeepingToIdMap(
    const std::shared_ptr< simulation_setup::ObservationDependentVariableBookkeeping >& bookkeeping )
{
    std::map< std::pair< int, int >, std::string > idMap;
    
    if ( bookkeeping != nullptr )
    {
        auto settingsMap = bookkeeping->getSettingsIndicesAndSizes( );
        for ( const auto& [indexPair, settings] : settingsMap )
        {
            // Create a descriptive name from the variable type and identifier
            std::string name = simulation_setup::getObservationDependentVariableName( settings->variableType_ );
            name += settings->getIdentifier( );
            idMap[ indexPair ] = name;
        }
    }
    
    return idMap;
}

// ================================================================================================
// HDF5OutputFile class
// ================================================================================================

/**
 * @brief Class for writing Tudat simulation results to HDF5 files
 * 
 * This class provides a hierarchical structure for storing propagation results:
 * 
 * /Trajectories/
 *     /SingleArcSimulationResults/
 *         /<BodyName>/
 *             times              - 1D array of time values
 *             states             - 2D array (numTimeSteps x stateSize)
 *             dependent_variables - 2D array (numTimeSteps x depVarSize)
 *             state_ids/
 *                 start_indices  - 1D array
 *                 sizes          - 1D array
 *                 names          - 1D array of strings
 *             dependent_variable_ids/
 *                 start_indices  - 1D array
 *                 sizes          - 1D array
 *                 names          - 1D array of strings
 *             metadata (as attributes)
 *     /VariationalEquationsSimulationResults/
 *         /<BodyName>/
 *             ... (same as above, plus:)
 *             state_transition_matrix - 2D array (numTimeSteps x stmSize*stmSize)
 *             sensitivity_matrix      - 2D array (numTimeSteps x sensRows*sensCols)
 *             stm_size               - scalar attribute
 *             sensitivity_size       - scalar attribute
 */
class HDF5OutputFile
{
public:
    /**
     * @brief Construct a new HDF5OutputFile
     * @param filePath Path to the HDF5 file to create
     * @param overwrite If true, overwrite existing file; otherwise append
     */
    HDF5OutputFile( const std::string& filePath, bool overwrite = true );
    
    /**
     * @brief Destructor - ensures file is properly closed
     */
    ~HDF5OutputFile( );
    
    /**
     * @brief Add SingleArcSimulationResults to the file
     * @tparam StateScalarType Scalar type for state (default: double)
     * @tparam TimeType Time type (default: double, can be tudat::Time)
     * @param results The simulation results to store
     * @param bodyName Name of the body (used as group name)
     * @param groupPath Base path in HDF5 file (default: "/Trajectories/SingleArcSimulationResults")
     */
    template< typename StateScalarType = double, typename TimeType = double >
    void addSingleArcResults(
        const std::shared_ptr< propagators::SingleArcSimulationResults< StateScalarType, TimeType > >& results,
        const std::string& bodyName,
        const std::string& groupPath = "/Trajectories/SingleArcSimulationResults" );
    
    /**
     * @brief Add SingleArcVariationalSimulationResults to the file
     * @tparam StateScalarType Scalar type for state (default: double)
     * @tparam TimeType Time type (default: double, can be tudat::Time)
     * @param results The variational simulation results to store
     * @param bodyName Name of the body (used as group name)
     * @param groupPath Base path in HDF5 file (default: "/Trajectories/VariationalEquationsSimulationResults")
     */
    template< typename StateScalarType = double, typename TimeType = double >
    void addVariationalResults(
        const std::shared_ptr< propagators::SingleArcVariationalSimulationResults< StateScalarType, TimeType > >& results,
        const std::string& bodyName,
        const std::string& groupPath = "/Trajectories/VariationalEquationsSimulationResults" );
    
    /**
     * @brief Add a SingleObservationSet to the file
     * @tparam ObservationScalarType Scalar type for observations (default: double)
     * @tparam TimeType Time type (default: double, can be tudat::Time)
     * @param observationSet The observation set to store
     * @param setName Name for this observation set (used as group name)
     * @param groupPath Base path in HDF5 file (default: "/Observations/SingleObservationSets")
     * @param includeFilteredObservations If true, also store filtered observations in a subgroup
     * 
     * This creates a hierarchical structure:
     * /Observations/SingleObservationSets/<setName>/
     *     observations         - 2D array (numObs x observationSize)
     *     times               - 1D array of observation times
     *     weights             - 2D array (numObs x observationSize)
     *     residuals           - 2D array (numObs x observationSize)
     *     dependent_variables - 2D array (numObs x depVarSize) [if available]
     *     link_ends/
     *         link_end_types  - 1D array of LinkEndType values
     *         body_names      - 1D array of body name strings
     *         station_names   - 1D array of station name strings
     *     dependent_variable_ids/
     *         start_indices   - 1D array
     *         sizes           - 1D array
     *         names           - 1D array of strings
     *     metadata (as attributes):
     *         observable_type, observable_type_name, reference_link_end,
     *         num_observations, single_observation_size, time_bounds
     *     filtered_observations/ [if includeFilteredObservations and filtered data exists]
     *         (same structure as above)
     */
    template< typename ObservationScalarType = double, typename TimeType = double >
    void addSingleObservationSet(
        const std::shared_ptr< observation_models::SingleObservationSet< ObservationScalarType, TimeType > >& observationSet,
        const std::string& setName,
        const std::string& groupPath = "/Observations/SingleObservationSets",
        bool includeFilteredObservations = false );
    
    /**
     * @brief Add an ObservationCollection to the file
     * @tparam ObservationScalarType Scalar type for observations (default: double)
     * @tparam TimeType Time type (default: double, can be tudat::Time)
     * @param observationCollection The observation collection to store
     * @param collectionName Name for this collection (used as group name)
     * @param groupPath Base path in HDF5 file (default: "/Observations/ObservationCollections")
     * @param includeFilteredObservations If true, also store filtered observations
     * 
     * This creates a hierarchical structure organized by observable type and link ends:
     * /Observations/ObservationCollections/<collectionName>/
     *     metadata/
     *         observable_types      - 1D array of observable type integers
     *         observable_type_names - 1D array of observable type names
     *         total_observation_size - scalar
     *         time_bounds          - 2-element array [start, end]
     *     concatenated/
     *         observations         - 1D array (all observations concatenated)
     *         times               - 1D array (all times concatenated)
     *         weights             - 1D array (all weights concatenated)
     *         residuals           - 1D array (all residuals concatenated)
     *         link_end_ids        - 1D array (link end ID for each observation)
     *         observation_set_start_and_size - 2D array mapping observation sets
     *     by_observable_type/
     *         /<observable_type_name>/
     *             /<link_ends_id>/
     *                 /set_0/, /set_1/, ...  (same structure as SingleObservationSet)
     */
    template< typename ObservationScalarType = double, typename TimeType = double >
    void addObservationCollection(
        const std::shared_ptr< observation_models::ObservationCollection< ObservationScalarType, TimeType > >& observationCollection,
        const std::string& collectionName = "default",
        const std::string& groupPath = "/Observations/ObservationCollections",
        bool includeFilteredObservations = false );
    
    /**
     * @brief Get the underlying HighFive file object for custom operations
     * @return Reference to the HighFive::File object
     */
    HighFive::File& getFile( ) { return file_; }
    
    /**
     * @brief Close the file explicitly
     */
    void close( );
    
    /**
     * @brief Generate XDMF descriptor file for ParaView visualization
     * @param xdmfFilePath Path to the XDMF file to create
     * 
     * This creates an XDMF3 file that references the HDF5 data and can be
     * opened directly in ParaView. It includes:
     * - Static polyline trajectories
     * - Velocity vectors (for glyph visualization)
     * - Dependent variables as scalar/vector attributes
     */
    void generateXDMF( const std::string& xdmfFilePath );
    
    /**
     * @brief Generate XDMF descriptor file with default name (same as HDF5 but .xdmf)
     */
    void generateXDMF( );
    
    /**
     * @brief Generate XDMF descriptor file for observation data visualization
     * @param xdmfFilePath Path to the XDMF file to create
     * 
     * This creates an XDMF3 file for visualizing observation data in ParaView.
     * Each observation set becomes a Polyvertex grid where:
     * - Time is the X coordinate (1D geometry)
     * - Observation values are scalar or vector attributes
     * - Weights and residuals are additional attributes
     */
    void generateObservationXDMF( const std::string& xdmfFilePath );
    
    /**
     * @brief Generate observation XDMF with default name (same as HDF5 but _observations.xdmf)
     */
    void generateObservationXDMF( );
    
    /**
     * @brief Get the trajectory configurations for all stored trajectories
     * @return Vector of TrajectoryConfig objects
     */
    std::vector< TrajectoryConfig > getTrajectoryConfigs( ) const
    {
        return trajectoryConfigs_;
    }
    
    /**
     * @brief Get the observation configurations for all stored observation sets
     * @return Vector of ObservationXDMFConfig objects
     */
    std::vector< ObservationXDMFConfig > getObservationConfigs( ) const
    {
        return observationConfigs_;
    }
    
    /**
     * @brief Get the file path
     * @return The path to the HDF5 file
     */
    std::string getFilePath( ) const
    {
        return filePath_;
    }

private:
    /**
     * @brief Create a group, including all parent groups if they don't exist
     * @param path Full path to the group
     * @return The created or existing group
     */
    HighFive::Group createGroupRecursive( const std::string& path );
    
    /**
     * @brief Write state/dependent variable ID mapping to a subgroup
     * @param parentGroup Parent group to create subgroup in
     * @param subgroupName Name of the subgroup (e.g., "state_ids")
     * @param idMap The ID mapping to write
     */
    void writeIdMapping(
        HighFive::Group& parentGroup,
        const std::string& subgroupName,
        const std::map< std::pair< int, int >, std::string >& idMap );
    
    /**
     * @brief Write metadata attributes to a group
     * @tparam StateScalarType Scalar type for state
     * @tparam TimeType Time type
     * @param group Group to write attributes to
     * @param results Simulation results to extract metadata from
     */
    template< typename StateScalarType, typename TimeType >
    void writeMetadata(
        HighFive::Group& group,
        const std::shared_ptr< propagators::SingleArcSimulationResults< StateScalarType, TimeType > >& results );
    
    /**
     * @brief Write link ends information to a subgroup
     * @param parentGroup Parent group to create subgroup in
     * @param linkEnds The link ends to serialize
     */
    void writeLinkEnds(
        HighFive::Group& parentGroup,
        const observation_models::LinkEnds& linkEnds );
    
    /**
     * @brief Write observation set data to a group
     * @tparam ObservationScalarType Scalar type for observations
     * @tparam TimeType Time type
     * @param group Group to write to
     * @param observationSet The observation set data
     * @param includeFilteredObservations Whether to include filtered observations
     */
    template< typename ObservationScalarType, typename TimeType >
    void writeObservationSetData(
        HighFive::Group& group,
        const std::shared_ptr< observation_models::SingleObservationSet< ObservationScalarType, TimeType > >& observationSet,
        bool includeFilteredObservations );
    
    HighFive::File file_;
    std::string filePath_;
    bool isOpen_;
    std::vector< TrajectoryConfig > trajectoryConfigs_;
    std::vector< ObservationXDMFConfig > observationConfigs_;
};

// ================================================================================================
// Template implementations
// ================================================================================================

template< typename StateScalarType, typename TimeType >
void HDF5OutputFile::addSingleArcResults(
    const std::shared_ptr< propagators::SingleArcSimulationResults< StateScalarType, TimeType > >& results,
    const std::string& bodyName,
    const std::string& groupPath )
{
    if ( !isOpen_ )
    {
        throw std::runtime_error( "HDF5OutputFile: File is not open" );
    }
    
    // Create the body group
    std::string fullPath = groupPath + "/" + bodyName;
    HighFive::Group bodyGroup = createGroupRecursive( fullPath );
    
    // Get state history and convert to arrays
    auto stateHistory = results->getEquationsOfMotionNumericalSolution( );
    if ( !stateHistory.empty( ) )
    {
        std::vector< double > times;
        std::vector< std::vector< double > > states;
        convertStateHistoryToArrays( stateHistory, times, states );
        
        // Write times
        bodyGroup.createDataSet< double >( "times", HighFive::DataSpace( { times.size( ) } ) )
                 .write( times );
        
        // Write states (2D array)
        if ( !states.empty( ) )
        {
            size_t stateSize = states[0].size( );
            bodyGroup.createDataSet< double >( "states", HighFive::DataSpace( { states.size( ), stateSize } ) )
                     .write( states );
        }
    }
    
    // Get dependent variable history and convert to arrays
    auto depVarHistory = results->getDependentVariableHistory( );
    if ( !depVarHistory.empty( ) )
    {
        std::vector< double > depVarTimes;
        std::vector< std::vector< double > > depVars;
        convertDependentVariableHistoryToArrays( depVarHistory, depVarTimes, depVars );
        
        // Write dependent variables (2D array)
        if ( !depVars.empty( ) && !depVars[0].empty( ) )
        {
            size_t depVarSize = depVars[0].size( );
            bodyGroup.createDataSet< double >( "dependent_variables", 
                                               HighFive::DataSpace( { depVars.size( ), depVarSize } ) )
                     .write( depVars );
        }
    }
    
    // Write state IDs
    auto processedStateIds = results->getProcessedStateIds( );
    if ( !processedStateIds.empty( ) )
    {
        writeIdMapping( bodyGroup, "state_ids", processedStateIds );
    }
    
    // Write dependent variable IDs
    auto depVarIds = results->getDependentVariableId( );
    if ( !depVarIds.empty( ) )
    {
        writeIdMapping( bodyGroup, "dependent_variable_ids", depVarIds );
    }
    
    // Write metadata
    writeMetadata( bodyGroup, results );
    
    // Track trajectory configuration for XDMF generation
    TrajectoryConfig trajConfig;
    trajConfig.bodyName = bodyName;
    trajConfig.h5FilePath = filePath_;
    trajConfig.statesDataset = fullPath + "/states";
    trajConfig.timesDataset = fullPath + "/times";
    trajConfig.connectivityDataset = fullPath + "/connectivity";
    
    if ( !stateHistory.empty( ) )
    {
        trajConfig.numTimeSteps = stateHistory.size( );
        trajConfig.stateSize = stateHistory.begin( )->second.size( );
    }
    
    // Set up dependent variable attributes
    if ( !depVarHistory.empty( ) && !depVarIds.empty( ) )
    {
        trajConfig.dependentVariablesDataset = fullPath + "/dependent_variables";
        trajConfig.dependentVariablesSize = depVarHistory.begin( )->second.size( );
        
        // Create XDMF attribute config for each dependent variable
        for ( const auto& [indexPair, name] : depVarIds )
        {
            XDMFAttributeConfig attrConfig;
            attrConfig.name = name;
            attrConfig.h5FilePath = filePath_;
            attrConfig.datasetPath = fullPath + "/dependent_variables";
            attrConfig.numElements = trajConfig.numTimeSteps;
            
            int startIdx = indexPair.first;
            int size = indexPair.second;
            
            if ( size == 1 )
            {
                attrConfig.type = XDMFAttributeType::Scalar;
                attrConfig.columnIndices = { startIdx };
            }
            else if ( size == 3 )
            {
                attrConfig.type = XDMFAttributeType::Vector;
                attrConfig.columnIndices = { startIdx, startIdx + 1, startIdx + 2 };
            }
            else
            {
                // For other sizes, treat as scalar (first element only) or skip
                attrConfig.type = XDMFAttributeType::Scalar;
                attrConfig.columnIndices = { startIdx };
            }
            
            trajConfig.dependentVariableAttributes.push_back( attrConfig );
        }
    }
    
    trajectoryConfigs_.push_back( trajConfig );
}

template< typename StateScalarType, typename TimeType >
void HDF5OutputFile::addVariationalResults(
    const std::shared_ptr< propagators::SingleArcVariationalSimulationResults< StateScalarType, TimeType > >& results,
    const std::string& bodyName,
    const std::string& groupPath )
{
    if ( !isOpen_ )
    {
        throw std::runtime_error( "HDF5OutputFile: File is not open" );
    }
    
    // First, write the dynamics results (inherited from SingleArcSimulationResults)
    auto dynamicsResults = results->getDynamicsResults( );
    
    // Create the body group
    std::string fullPath = groupPath + "/" + bodyName;
    HighFive::Group bodyGroup = createGroupRecursive( fullPath );
    
    // Get state history and convert to arrays (from dynamics results)
    auto stateHistory = dynamicsResults->getEquationsOfMotionNumericalSolution( );
    if ( !stateHistory.empty( ) )
    {
        std::vector< double > times;
        std::vector< std::vector< double > > states;
        convertStateHistoryToArrays( stateHistory, times, states );
        
        // Write times
        bodyGroup.createDataSet< double >( "times", HighFive::DataSpace( { times.size( ) } ) )
                 .write( times );
        
        // Write states (2D array)
        if ( !states.empty( ) )
        {
            size_t stateSize = states[0].size( );
            bodyGroup.createDataSet< double >( "states", HighFive::DataSpace( { states.size( ), stateSize } ) )
                     .write( states );
        }
    }
    
    // Get dependent variable history
    auto depVarHistory = dynamicsResults->getDependentVariableHistory( );
    if ( !depVarHistory.empty( ) )
    {
        std::vector< double > depVarTimes;
        std::vector< std::vector< double > > depVars;
        convertDependentVariableHistoryToArrays( depVarHistory, depVarTimes, depVars );
        
        if ( !depVars.empty( ) && !depVars[0].empty( ) )
        {
            size_t depVarSize = depVars[0].size( );
            bodyGroup.createDataSet< double >( "dependent_variables", 
                                               HighFive::DataSpace( { depVars.size( ), depVarSize } ) )
                     .write( depVars );
        }
    }
    
    // Write state transition matrix history
    auto stmHistory = results->getStateTransitionSolution( );
    if ( !stmHistory.empty( ) )
    {
        std::vector< double > stmTimes;
        std::vector< std::vector< double > > stmMatrices;
        convertMatrixHistoryToArrays( stmHistory, stmTimes, stmMatrices );
        
        if ( !stmMatrices.empty( ) )
        {
            size_t flatSize = stmMatrices[0].size( );
            bodyGroup.createDataSet< double >( "state_transition_matrix", 
                                               HighFive::DataSpace( { stmMatrices.size( ), flatSize } ) )
                     .write( stmMatrices );
        }
    }
    
    // Write sensitivity matrix history
    auto sensHistory = results->getSensitivitySolution( );
    if ( !sensHistory.empty( ) )
    {
        std::vector< double > sensTimes;
        std::vector< std::vector< double > > sensMatrices;
        convertMatrixHistoryToArrays( sensHistory, sensTimes, sensMatrices );
        
        if ( !sensMatrices.empty( ) )
        {
            size_t flatSize = sensMatrices[0].size( );
            bodyGroup.createDataSet< double >( "sensitivity_matrix", 
                                               HighFive::DataSpace( { sensMatrices.size( ), flatSize } ) )
                     .write( sensMatrices );
        }
    }
    
    // Write state IDs
    auto processedStateIds = dynamicsResults->getProcessedStateIds( );
    if ( !processedStateIds.empty( ) )
    {
        writeIdMapping( bodyGroup, "state_ids", processedStateIds );
    }
    
    // Write dependent variable IDs
    auto depVarIds = dynamicsResults->getDependentVariableId( );
    if ( !depVarIds.empty( ) )
    {
        writeIdMapping( bodyGroup, "dependent_variable_ids", depVarIds );
    }
    
    // Write variational-specific metadata
    int stmSize = results->getStateTransitionMatrixSize( );
    int sensSize = results->getSensitivityMatrixSize( );
    bodyGroup.createAttribute< int >( "state_transition_matrix_size", stmSize );
    bodyGroup.createAttribute< int >( "sensitivity_matrix_size", sensSize );
    
    // Write standard metadata
    writeMetadata( bodyGroup, dynamicsResults );
}

template< typename StateScalarType, typename TimeType >
void HDF5OutputFile::writeMetadata(
    HighFive::Group& group,
    const std::shared_ptr< propagators::SingleArcSimulationResults< StateScalarType, TimeType > >& results )
{
    // Termination reason
    auto termDetails = results->getPropagationTerminationReason( );
    if ( termDetails )
    {
        group.createAttribute< std::string >( "termination_reason", 
                                              termDetails->getTerminationReasonString( ) );
        group.createAttribute< bool >( "terminated_on_exact_condition", 
                                       termDetails->getTerminationOnExactCondition( ) );
    }
    
    // Integration success
    group.createAttribute< bool >( "integration_successful", 
                                   results->integrationCompletedSuccessfully( ) );
    
    // Arc times
    auto arcTimes = results->getArcInitialAndFinalTime( );
    group.createAttribute< double >( "arc_start_time", timeToDouble( arcTimes.first ) );
    group.createAttribute< double >( "arc_end_time", timeToDouble( arcTimes.second ) );
    
    // Computation statistics
    double totalRuntime = results->getTotalComputationRuntime( );
    group.createAttribute< double >( "total_computation_runtime", totalRuntime );
    
    double totalFuncEvals = results->getTotalNumberOfFunctionEvaluations( );
    group.createAttribute< double >( "total_function_evaluations", totalFuncEvals );
}

// ================================================================================================
// Observation export template implementations
// ================================================================================================

template< typename ObservationScalarType, typename TimeType >
void HDF5OutputFile::writeObservationSetData(
    HighFive::Group& group,
    const std::shared_ptr< observation_models::SingleObservationSet< ObservationScalarType, TimeType > >& observationSet,
    bool includeFilteredObservations )
{
    // Get observation data
    auto observations = observationSet->getObservations( );
    auto times = observationSet->getObservationTimes( );
    auto weights = observationSet->getWeights( );
    auto residuals = observationSet->getResiduals( );
    auto depVars = observationSet->getObservationsDependentVariables( );
    
    unsigned int numObs = observationSet->getNumberOfObservables( );
    unsigned int obsSize = observationSet->getSingleObservableSize( );
    
    if ( numObs == 0 )
    {
        // Write empty marker attribute
        group.createAttribute< int >( "num_observations", 0 );
        return;
    }
    
    // Convert and write observations (2D array: numObs x obsSize)
    std::vector< std::vector< double > > obsArrays;
    convertObservationsToArrays( observations, obsArrays );
    group.createDataSet< double >( "observations", 
                                   HighFive::DataSpace( { numObs, obsSize } ) )
         .write( obsArrays );
    
    // Convert and write times (1D array)
    std::vector< double > doubleTimes;
    convertObservationTimesToDoubleArray( times, doubleTimes );
    group.createDataSet< double >( "times", 
                                   HighFive::DataSpace( { doubleTimes.size( ) } ) )
         .write( doubleTimes );
    
    // Convert and write weights (2D array: numObs x obsSize)
    std::vector< std::vector< double > > weightArrays;
    convertObservationsToArrays( weights, weightArrays );
    group.createDataSet< double >( "weights", 
                                   HighFive::DataSpace( { numObs, obsSize } ) )
         .write( weightArrays );
    
    // Convert and write residuals (2D array: numObs x obsSize)
    std::vector< std::vector< double > > residualArrays;
    convertObservationsToArrays( residuals, residualArrays );
    group.createDataSet< double >( "residuals", 
                                   HighFive::DataSpace( { numObs, obsSize } ) )
         .write( residualArrays );
    
    // Write dependent variables if available
    if ( !depVars.empty( ) )
    {
        size_t depVarSize = depVars[0].size( );
        std::vector< std::vector< double > > depVarArrays;
        for ( const auto& dv : depVars )
        {
            std::vector< double > dvVec( dv.size( ) );
            for ( int i = 0; i < dv.size( ); ++i )
            {
                dvVec[i] = dv( i );
            }
            depVarArrays.push_back( dvVec );
        }
        group.createDataSet< double >( "dependent_variables", 
                                       HighFive::DataSpace( { numObs, depVarSize } ) )
             .write( depVarArrays );
        
        // Write dependent variable ID mapping
        auto depVarBookkeeping = observationSet->getDependentVariableBookkeeping( );
        if ( depVarBookkeeping != nullptr )
        {
            auto depVarIdMap = convertDependentVariableBookkeepingToIdMap( depVarBookkeeping );
            if ( !depVarIdMap.empty( ) )
            {
                writeIdMapping( group, "dependent_variable_ids", depVarIdMap );
            }
        }
    }
    
    // Write link ends
    writeLinkEnds( group, observationSet->getLinkEnds( ).linkEnds_ );
    
    // Write metadata as attributes
    int observableTypeInt = static_cast< int >( observationSet->getObservableType( ) );
    group.createAttribute< int >( "observable_type", observableTypeInt );
    group.createAttribute< std::string >( "observable_type_name", 
                                          getObservableTypeName( observationSet->getObservableType( ) ) );
    group.createAttribute< int >( "reference_link_end", 
                                  static_cast< int >( observationSet->getReferenceLinkEnd( ) ) );
    group.createAttribute< unsigned int >( "num_observations", numObs );
    group.createAttribute< unsigned int >( "single_observation_size", obsSize );
    
    // Write time bounds
    auto timeBounds = observationSet->getTimeBounds( );
    std::vector< double > timeBoundsVec = { timeToDouble( timeBounds.first ), 
                                            timeToDouble( timeBounds.second ) };
    group.createDataSet< double >( "time_bounds", 
                                   HighFive::DataSpace( { 2 } ) )
         .write( timeBoundsVec );
    
    // Handle filtered observations if requested
    if ( includeFilteredObservations )
    {
        auto filteredSet = observationSet->getFilteredObservationSet( );
        if ( filteredSet != nullptr && filteredSet->getNumberOfObservables( ) > 0 )
        {
            HighFive::Group filteredGroup = group.createGroup( "filtered_observations" );
            writeObservationSetData( filteredGroup, filteredSet, false ); // Don't recurse further
        }
    }
}

template< typename ObservationScalarType, typename TimeType >
void HDF5OutputFile::addSingleObservationSet(
    const std::shared_ptr< observation_models::SingleObservationSet< ObservationScalarType, TimeType > >& observationSet,
    const std::string& setName,
    const std::string& groupPath,
    bool includeFilteredObservations )
{
    if ( !isOpen_ )
    {
        throw std::runtime_error( "HDF5OutputFile: File is not open" );
    }
    
    // Create the observation set group
    std::string fullPath = groupPath + "/" + setName;
    HighFive::Group setGroup = createGroupRecursive( fullPath );
    
    // Write the observation set data
    writeObservationSetData( setGroup, observationSet, includeFilteredObservations );
    
    // Track observation configuration for XDMF generation
    unsigned int numObs = observationSet->getNumberOfObservables( );
    if ( numObs > 0 )
    {
        ObservationXDMFConfig obsConfig;
        obsConfig.setName = setName;
        obsConfig.observableTypeName = getObservableTypeName( observationSet->getObservableType( ) );
        obsConfig.h5FilePath = filePath_;
        obsConfig.h5GroupPath = fullPath;
        obsConfig.numObservations = numObs;
        obsConfig.observableSize = observationSet->getSingleObservableSize( );
        obsConfig.hasWeights = true;
        obsConfig.hasResiduals = true;
        
        // Check for dependent variables
        auto depVars = observationSet->getObservationsDependentVariables( );
        if ( !depVars.empty( ) )
        {
            obsConfig.hasDependentVariables = true;
            obsConfig.dependentVariablesDataset = "dependent_variables";
            obsConfig.dependentVariablesSize = depVars[0].size( );
            
            // Create XDMF attribute configs for dependent variables
            auto depVarBookkeeping = observationSet->getDependentVariableBookkeeping( );
            if ( depVarBookkeeping != nullptr )
            {
                auto depVarIdMap = convertDependentVariableBookkeepingToIdMap( depVarBookkeeping );
                for ( const auto& [indexPair, name] : depVarIdMap )
                {
                    XDMFAttributeConfig attrConfig;
                    attrConfig.name = name;
                    attrConfig.h5FilePath = filePath_;
                    attrConfig.datasetPath = fullPath + "/dependent_variables";
                    attrConfig.numElements = numObs;
                    
                    int startIdx = indexPair.first;
                    int size = indexPair.second;
                    
                    if ( size == 1 )
                    {
                        attrConfig.type = XDMFAttributeType::Scalar;
                        attrConfig.columnIndices = { startIdx };
                    }
                    else if ( size == 3 )
                    {
                        attrConfig.type = XDMFAttributeType::Vector;
                        attrConfig.columnIndices = { startIdx, startIdx + 1, startIdx + 2 };
                    }
                    else
                    {
                        attrConfig.type = XDMFAttributeType::Scalar;
                        attrConfig.columnIndices = { startIdx };
                    }
                    
                    obsConfig.dependentVariableAttributes.push_back( attrConfig );
                }
            }
        }
        
        observationConfigs_.push_back( obsConfig );
    }
}

template< typename ObservationScalarType, typename TimeType >
void HDF5OutputFile::addObservationCollection(
    const std::shared_ptr< observation_models::ObservationCollection< ObservationScalarType, TimeType > >& observationCollection,
    const std::string& collectionName,
    const std::string& groupPath,
    bool includeFilteredObservations )
{
    if ( !isOpen_ )
    {
        throw std::runtime_error( "HDF5OutputFile: File is not open" );
    }
    
    // Create the collection group
    std::string fullPath = groupPath + "/" + collectionName;
    HighFive::Group collectionGroup = createGroupRecursive( fullPath );
    
    // Create metadata group
    HighFive::Group metadataGroup = collectionGroup.createGroup( "metadata" );
    
    // Get observable types and write them
    auto observableTypes = observationCollection->getObservableTypes( );
    std::vector< int > obsTypeInts;
    std::vector< std::string > obsTypeNames;
    for ( const auto& obsType : observableTypes )
    {
        obsTypeInts.push_back( static_cast< int >( obsType ) );
        obsTypeNames.push_back( getObservableTypeName( obsType ) );
    }
    metadataGroup.createDataSet< int >( "observable_types", 
                                        HighFive::DataSpace( { obsTypeInts.size( ) } ) )
                 .write( obsTypeInts );
    metadataGroup.createDataSet( "observable_type_names", obsTypeNames );
    
    // Write total observation size
    int totalObsSize = observationCollection->getTotalObservableSize( );
    metadataGroup.createAttribute< int >( "total_observation_size", totalObsSize );
    
    // Write time bounds
    auto timeBounds = observationCollection->getTimeBoundsDouble( );
    std::vector< double > timeBoundsVec = { timeBounds.first, timeBounds.second };
    metadataGroup.createDataSet< double >( "time_bounds", 
                                           HighFive::DataSpace( { 2 } ) )
                 .write( timeBoundsVec );
    
    // Write link end identifier map
    auto linkEndIdMap = observationCollection->getLinkEndIdentifierMap( );
    std::vector< int > linkEndIdValues;
    std::vector< std::string > linkEndIdStrings;
    for ( const auto& [linkEnds, id] : linkEndIdMap )
    {
        linkEndIdValues.push_back( id );
        // Create a string representation of the link ends
        std::string linkEndsStr;
        for ( const auto& [linkEndType, linkEndId] : linkEnds )
        {
            if ( !linkEndsStr.empty( ) ) linkEndsStr += ";";
            linkEndsStr += std::to_string( static_cast< int >( linkEndType ) ) + ":" +
                          linkEndId.bodyName_ + ":" + linkEndId.stationName_;
        }
        linkEndIdStrings.push_back( linkEndsStr );
    }
    if ( !linkEndIdValues.empty( ) )
    {
        metadataGroup.createDataSet< int >( "link_end_ids", 
                                            HighFive::DataSpace( { linkEndIdValues.size( ) } ) )
                     .write( linkEndIdValues );
        metadataGroup.createDataSet( "link_end_strings", linkEndIdStrings );
    }
    
    // Create concatenated data group
    HighFive::Group concatenatedGroup = collectionGroup.createGroup( "concatenated" );
    
    // Get concatenated data
    auto concatenatedObs = observationCollection->getObservationVector( );
    auto concatenatedTimes = observationCollection->getConcatenatedDoubleTimeVector( );
    auto concatenatedWeights = observationCollection->getUnparsedConcatenatedWeights( );
    auto concatenatedResiduals = observationCollection->getConcatenatedResiduals( );
    auto concatenatedLinkEndIds = observationCollection->getConcatenatedLinkEndIds( );
    
    // Write concatenated observations
    if ( concatenatedObs.size( ) > 0 )
    {
        std::vector< double > obsVec( concatenatedObs.size( ) );
        for ( int i = 0; i < concatenatedObs.size( ); ++i )
        {
            obsVec[i] = static_cast< double >( concatenatedObs( i ) );
        }
        concatenatedGroup.createDataSet< double >( "observations", 
                                                   HighFive::DataSpace( { obsVec.size( ) } ) )
                         .write( obsVec );
    }
    
    // Write concatenated times
    if ( !concatenatedTimes.empty( ) )
    {
        concatenatedGroup.createDataSet< double >( "times", 
                                                   HighFive::DataSpace( { concatenatedTimes.size( ) } ) )
                         .write( concatenatedTimes );
    }
    
    // Write concatenated weights
    if ( concatenatedWeights.size( ) > 0 )
    {
        std::vector< double > weightsVec( concatenatedWeights.size( ) );
        for ( int i = 0; i < concatenatedWeights.size( ); ++i )
        {
            weightsVec[i] = concatenatedWeights( i );
        }
        concatenatedGroup.createDataSet< double >( "weights", 
                                                   HighFive::DataSpace( { weightsVec.size( ) } ) )
                         .write( weightsVec );
    }
    
    // Write concatenated residuals
    if ( concatenatedResiduals.size( ) > 0 )
    {
        std::vector< double > residualsVec( concatenatedResiduals.size( ) );
        for ( int i = 0; i < concatenatedResiduals.size( ); ++i )
        {
            residualsVec[i] = static_cast< double >( concatenatedResiduals( i ) );
        }
        concatenatedGroup.createDataSet< double >( "residuals", 
                                                   HighFive::DataSpace( { residualsVec.size( ) } ) )
                         .write( residualsVec );
    }
    
    // Write concatenated link end IDs
    if ( !concatenatedLinkEndIds.empty( ) )
    {
        concatenatedGroup.createDataSet< int >( "link_end_ids", 
                                                HighFive::DataSpace( { concatenatedLinkEndIds.size( ) } ) )
                         .write( concatenatedLinkEndIds );
    }
    
    // Write observation set start and size mapping
    auto obsSetStartAndSize = observationCollection->getConcatenatedObservationSetStartAndSize( );
    if ( !obsSetStartAndSize.empty( ) )
    {
        std::vector< std::vector< int > > startAndSizeArray;
        for ( const auto& [start, size] : obsSetStartAndSize )
        {
            startAndSizeArray.push_back( { start, size } );
        }
        concatenatedGroup.createDataSet< int >( "observation_set_start_and_size", 
                                                HighFive::DataSpace( { startAndSizeArray.size( ), 2 } ) )
                         .write( startAndSizeArray );
    }
    
    // Create by_observable_type group with detailed per-set data
    HighFive::Group byTypeGroup = collectionGroup.createGroup( "by_observable_type" );
    
    auto observationSets = observationCollection->getObservationsSets( );
    for ( const auto& [obsType, linkEndsMap] : observationSets )
    {
        std::string obsTypeName = getObservableTypeName( obsType );
        // Replace invalid characters for HDF5 group names
        std::replace( obsTypeName.begin( ), obsTypeName.end( ), ' ', '_' );
        std::replace( obsTypeName.begin( ), obsTypeName.end( ), '-', '_' );
        
        HighFive::Group obsTypeGroup = byTypeGroup.createGroup( obsTypeName );
        
        int linkEndsCounter = 0;
        for ( const auto& [linkEnds, sets] : linkEndsMap )
        {
            // Create link ends group with a unique identifier
            std::string linkEndsGroupName = "link_ends_" + std::to_string( linkEndsCounter++ );
            HighFive::Group linkEndsGroup = obsTypeGroup.createGroup( linkEndsGroupName );
            
            // Write the link ends for reference
            writeLinkEnds( linkEndsGroup, linkEnds );
            
            // Write each observation set
            int setCounter = 0;
            for ( const auto& set : sets )
            {
                std::string setGroupName = "set_" + std::to_string( setCounter++ );
                HighFive::Group setGroup = linkEndsGroup.createGroup( setGroupName );
                writeObservationSetData( setGroup, set, includeFilteredObservations );
                
                // Track observation configuration for XDMF generation
                unsigned int numObs = set->getNumberOfObservables( );
                if ( numObs > 0 )
                {
                    std::string setFullPath = fullPath + "/by_observable_type/" + obsTypeName + 
                                              "/" + linkEndsGroupName + "/" + setGroupName;
                    
                    ObservationXDMFConfig obsConfig;
                    obsConfig.setName = collectionName + "_" + obsTypeName + "_" + 
                                        linkEndsGroupName + "_" + setGroupName;
                    obsConfig.observableTypeName = obsTypeName;
                    obsConfig.h5FilePath = filePath_;
                    obsConfig.h5GroupPath = setFullPath;
                    obsConfig.numObservations = numObs;
                    obsConfig.observableSize = set->getSingleObservableSize( );
                    obsConfig.hasWeights = true;
                    obsConfig.hasResiduals = true;
                    
                    // Check for dependent variables
                    auto depVars = set->getObservationsDependentVariables( );
                    if ( !depVars.empty( ) )
                    {
                        obsConfig.hasDependentVariables = true;
                        obsConfig.dependentVariablesDataset = "dependent_variables";
                        obsConfig.dependentVariablesSize = depVars[0].size( );
                        
                        auto depVarBookkeeping = set->getDependentVariableBookkeeping( );
                        if ( depVarBookkeeping != nullptr )
                        {
                            auto depVarIdMap = convertDependentVariableBookkeepingToIdMap( depVarBookkeeping );
                            for ( const auto& [indexPair, name] : depVarIdMap )
                            {
                                XDMFAttributeConfig attrConfig;
                                attrConfig.name = name;
                                attrConfig.h5FilePath = filePath_;
                                attrConfig.datasetPath = setFullPath + "/dependent_variables";
                                attrConfig.numElements = numObs;
                                
                                int startIdx = indexPair.first;
                                int size = indexPair.second;
                                
                                if ( size == 1 )
                                {
                                    attrConfig.type = XDMFAttributeType::Scalar;
                                    attrConfig.columnIndices = { startIdx };
                                }
                                else if ( size == 3 )
                                {
                                    attrConfig.type = XDMFAttributeType::Vector;
                                    attrConfig.columnIndices = { startIdx, startIdx + 1, startIdx + 2 };
                                }
                                else
                                {
                                    attrConfig.type = XDMFAttributeType::Scalar;
                                    attrConfig.columnIndices = { startIdx };
                                }
                                
                                obsConfig.dependentVariableAttributes.push_back( attrConfig );
                            }
                        }
                    }
                    
                    observationConfigs_.push_back( obsConfig );
                }
            }
        }
    }
}

}  // namespace io
}  // namespace tudat

#endif  // TUDAT_IO_HDF5_MANAGER_H
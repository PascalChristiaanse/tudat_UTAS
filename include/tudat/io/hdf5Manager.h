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
     * - Animated particle positions
     * - Velocity vectors (for glyph visualization)
     * - Dependent variables as scalar/vector attributes
     */
    void generateXDMF( const std::string& xdmfFilePath );
    
    /**
     * @brief Generate XDMF descriptor file with default name (same as HDF5 but .xdmf)
     */
    void generateXDMF( );
    
    /**
     * @brief Get the trajectory configurations for all stored trajectories
     * @return Vector of TrajectoryConfig objects
     */
    std::vector< TrajectoryConfig > getTrajectoryConfigs( ) const
    {
        return trajectoryConfigs_;
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
    
    HighFive::File file_;
    std::string filePath_;
    bool isOpen_;
    std::vector< TrajectoryConfig > trajectoryConfigs_;
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

}  // namespace io
}  // namespace tudat

#endif  // TUDAT_IO_HDF5_MANAGER_H
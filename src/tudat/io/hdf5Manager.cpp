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
 *      HDF5 output interface implementation for Tudat simulation results.
 *
 */

#include "tudat/io/hdf5Manager.h"

#include <sstream>
#include <algorithm>

namespace tudat
{
namespace io
{

// ================================================================================================
// HDF5OutputFile implementation
// ================================================================================================

HDF5OutputFile::HDF5OutputFile( const std::string& filePath, bool overwrite )
    : file_( filePath, overwrite ? HighFive::File::Overwrite : HighFive::File::OpenOrCreate ),
      filePath_( filePath ),
      isOpen_( true )
{
}

HDF5OutputFile::~HDF5OutputFile( )
{
    close( );
}

void HDF5OutputFile::close( )
{
    if ( isOpen_ )
    {
        // HighFive automatically flushes and closes on destruction
        // But we can explicitly mark as closed
        isOpen_ = false;
    }
}

HighFive::Group HDF5OutputFile::createGroupRecursive( const std::string& path )
{
    // Split path into components
    std::vector< std::string > components;
    std::stringstream ss( path );
    std::string component;
    
    while ( std::getline( ss, component, '/' ) )
    {
        if ( !component.empty( ) )
        {
            components.push_back( component );
        }
    }
    
    // Create groups recursively
    std::string currentPath;
    HighFive::Group currentGroup = file_.getGroup( "/" );
    
    for ( const auto& comp : components )
    {
        currentPath += "/" + comp;
        
        if ( !file_.exist( currentPath ) )
        {
            currentGroup = file_.createGroup( currentPath );
        }
        else
        {
            currentGroup = file_.getGroup( currentPath );
        }
    }
    
    return currentGroup;
}

void HDF5OutputFile::writeIdMapping(
    HighFive::Group& parentGroup,
    const std::string& subgroupName,
    const std::map< std::pair< int, int >, std::string >& idMap )
{
    if ( idMap.empty( ) )
    {
        return;
    }
    
    // Convert to arrays
    std::vector< int > startIndices;
    std::vector< int > sizes;
    std::vector< std::string > names;
    convertIdMapToArrays( idMap, startIndices, sizes, names );
    
    // Create subgroup
    HighFive::Group subgroup = parentGroup.createGroup( subgroupName );
    
    // Write arrays
    subgroup.createDataSet< int >( "start_indices", HighFive::DataSpace( { startIndices.size( ) } ) )
            .write( startIndices );
    
    subgroup.createDataSet< int >( "sizes", HighFive::DataSpace( { sizes.size( ) } ) )
            .write( sizes );
    
    // For string arrays, HighFive handles them directly
    subgroup.createDataSet< std::string >( "names", HighFive::DataSpace( { names.size( ) } ) )
            .write( names );
}

void HDF5OutputFile::writeLinkEnds(
    HighFive::Group& parentGroup,
    const observation_models::LinkEnds& linkEnds )
{
    // Create link_ends subgroup
    HighFive::Group linkEndsGroup = parentGroup.createGroup( "link_ends" );
    
    // Serialize link ends
    std::vector< int > linkEndTypes;
    std::vector< std::string > bodyNames;
    std::vector< std::string > stationNames;
    serializeLinkEnds( linkEnds, linkEndTypes, bodyNames, stationNames );
    
    // Write arrays
    if ( !linkEndTypes.empty( ) )
    {
        linkEndsGroup.createDataSet< int >( "link_end_types", 
                                            HighFive::DataSpace( { linkEndTypes.size( ) } ) )
                     .write( linkEndTypes );
        
        linkEndsGroup.createDataSet( "body_names", bodyNames );
        linkEndsGroup.createDataSet( "station_names", stationNames );
    }
}

void HDF5OutputFile::generateXDMF( const std::string& xdmfFilePath )
{
    if ( trajectoryConfigs_.empty( ) )
    {
        std::cerr << "Warning: No trajectories to export to XDMF" << std::endl;
        return;
    }
    
    // Create trajectory XDMF generator
    TrajectoryXDMFGenerator xdmfGenerator( trajectoryConfigs_ );
    
    // Write connectivity datasets to HDF5 (must be done before XDMF generation)
    xdmfGenerator.writeConnectivityToHDF5( );
    
    // Write the XDMF file
    xdmfGenerator.write( xdmfFilePath );
}

void HDF5OutputFile::generateXDMF( )
{
    // Generate default XDMF path (same as HDF5 but with .xdmf extension)
    std::string xdmfPath = filePath_;
    size_t dotPos = xdmfPath.find_last_of( '.' );
    if ( dotPos != std::string::npos )
    {
        xdmfPath = xdmfPath.substr( 0, dotPos ) + ".xdmf";
    }
    else
    {
        xdmfPath += ".xdmf";
    }
    
    generateXDMF( xdmfPath );
}

void HDF5OutputFile::generateObservationXDMF( const std::string& xdmfFilePath )
{
    if ( observationConfigs_.empty( ) )
    {
        std::cerr << "Warning: No observation sets to export to XDMF" << std::endl;
        return;
    }
    
    // Create observation XDMF generator
    ObservationXDMFGenerator xdmfGenerator( observationConfigs_ );
    
    // Write the XDMF file
    xdmfGenerator.write( xdmfFilePath );
}

void HDF5OutputFile::generateObservationXDMF( )
{
    // Generate default XDMF path (same as HDF5 but with _observations.xdmf extension)
    std::string xdmfPath = filePath_;
    size_t dotPos = xdmfPath.find_last_of( '.' );
    if ( dotPos != std::string::npos )
    {
        xdmfPath = xdmfPath.substr( 0, dotPos ) + "_observations.xdmf";
    }
    else
    {
        xdmfPath += "_observations.xdmf";
    }
    
    generateObservationXDMF( xdmfPath );
}

}  // namespace io
}  // namespace tudat
/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#include "tudat/io/xdmfGenerator.h"

namespace tudat
{
namespace io
{

// ================================================================================================
// XDMFGeneratorBase implementation
// ================================================================================================

void XDMFGeneratorBase::write( const std::string& xdmfFilePath )
{
    std::ofstream xdmfFile( xdmfFilePath );
    if ( !xdmfFile.is_open( ) )
    {
        throw std::runtime_error( "Failed to open XDMF file for writing: " + xdmfFilePath );
    }
    
    writeHeader( xdmfFile );
    generateGrids( xdmfFile );
    writeFooter( xdmfFile );
    
    xdmfFile.close( );
}

void XDMFGeneratorBase::writeHeader( std::ostream& os )
{
    os << R"(<?xml version="1.0" ?>
<!DOCTYPE Xdmf SYSTEM "Xdmf.dtd" []>
<Xdmf Version="3.0">
  <Domain>
)";
}

void XDMFGeneratorBase::writeFooter( std::ostream& os )
{
    os << R"(  </Domain>
</Xdmf>
)";
}

std::string XDMFGeneratorBase::getFilename( const std::string& path ) const
{
    size_t lastSlash = path.find_last_of( "/\\" );
    if ( lastSlash == std::string::npos )
    {
        return path;
    }
    return path.substr( lastSlash + 1 );
}

void XDMFGeneratorBase::writeHDF5DataItem( std::ostream& os,
                                            const std::string& h5FilePath,
                                            const std::string& datasetPath,
                                            const std::vector< size_t >& dimensions,
                                            const std::string& numberType,
                                            int precision,
                                            int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string filename = getFilename( h5FilePath );
    
    os << ind << "<DataItem Dimensions=\"";
    for ( size_t i = 0; i < dimensions.size( ); ++i )
    {
        if ( i > 0 ) os << " ";
        os << dimensions[ i ];
    }
    os << "\" NumberType=\"" << numberType << "\" Precision=\"" << precision 
       << "\" Format=\"HDF\">\n";
    os << ind << "  " << filename << ":" << datasetPath << "\n";
    os << ind << "</DataItem>\n";
}

void XDMFGeneratorBase::writeHDF5HyperSlabDataItem( std::ostream& os,
                                                     const std::string& h5FilePath,
                                                     const std::string& datasetPath,
                                                     const std::vector< size_t >& sourceDimensions,
                                                     size_t startRow,
                                                     size_t startCol,
                                                     size_t countRows,
                                                     size_t countCols,
                                                     const std::string& numberType,
                                                     int precision,
                                                     int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string filename = getFilename( h5FilePath );
    
    // Output dimensions
    os << ind << "<DataItem ItemType=\"HyperSlab\" Dimensions=\"" << countRows;
    if ( countCols > 1 )
    {
        os << " " << countCols;
    }
    os << "\" Type=\"HyperSlab\">\n";
    
    // HyperSlab selection: start, stride, count
    os << ind << "  <DataItem Dimensions=\"3 2\" Format=\"XML\">\n";
    os << ind << "    " << startRow << " " << startCol << "\n";  // start
    os << ind << "    1 1\n";                                     // stride
    os << ind << "    " << countRows << " " << countCols << "\n"; // count
    os << ind << "  </DataItem>\n";
    
    // Source dataset
    os << ind << "  <DataItem Dimensions=\"";
    for ( size_t i = 0; i < sourceDimensions.size( ); ++i )
    {
        if ( i > 0 ) os << " ";
        os << sourceDimensions[ i ];
    }
    os << "\" NumberType=\"" << numberType << "\" Precision=\"" << precision 
       << "\" Format=\"HDF\">\n";
    os << ind << "    " << filename << ":" << datasetPath << "\n";
    os << ind << "  </DataItem>\n";
    
    os << ind << "</DataItem>\n";
}

void XDMFGeneratorBase::writeJoinedColumnsDataItem( std::ostream& os,
                                                     const std::string& h5FilePath,
                                                     const std::string& datasetPath,
                                                     const std::vector< size_t >& sourceDimensions,
                                                     size_t numRows,
                                                     const std::vector< int >& columnIndices,
                                                     const std::string& numberType,
                                                     int precision,
                                                     int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string filename = getFilename( h5FilePath );
    size_t numCols = columnIndices.size( );
    
    // Use Function to JOIN columns: JOIN($0, $1, $2)
    os << ind << "<DataItem ItemType=\"Function\" Function=\"JOIN(";
    for ( size_t i = 0; i < numCols; ++i )
    {
        if ( i > 0 ) os << ", ";
        os << "$" << i;
    }
    os << ")\" Dimensions=\"" << numRows << " " << numCols << "\">\n";
    
    // Each column as a HyperSlab
    for ( size_t i = 0; i < numCols; ++i )
    {
        int col = columnIndices[ i ];
        
        os << ind << "  <DataItem ItemType=\"HyperSlab\" Dimensions=\"" << numRows << "\" Type=\"HyperSlab\">\n";
        os << ind << "    <DataItem Dimensions=\"3 2\" Format=\"XML\">\n";
        os << ind << "      0 " << col << "\n";   // start
        os << ind << "      1 1\n";                // stride
        os << ind << "      " << numRows << " 1\n"; // count
        os << ind << "    </DataItem>\n";
        os << ind << "    <DataItem Dimensions=\"";
        for ( size_t j = 0; j < sourceDimensions.size( ); ++j )
        {
            if ( j > 0 ) os << " ";
            os << sourceDimensions[ j ];
        }
        os << "\" NumberType=\"" << numberType << "\" Precision=\"" << precision 
           << "\" Format=\"HDF\">\n";
        os << ind << "      " << filename << ":" << datasetPath << "\n";
        os << ind << "    </DataItem>\n";
        os << ind << "  </DataItem>\n";
    }
    
    os << ind << "</DataItem>\n";
}

// ================================================================================================
// TrajectoryXDMFGenerator implementation
// ================================================================================================

void TrajectoryXDMFGenerator::writeConnectivityToHDF5( )
{
    for ( auto& config : trajectoryConfigs_ )
    {
        if ( config.numTimeSteps > 0 )
        {
            // Open HDF5 file for reading states and writing derived datasets
            HighFive::File h5File( config.h5FilePath, HighFive::File::ReadWrite );
            
            // Determine base path from states dataset
            size_t lastSlash = config.statesDataset.find_last_of( '/' );
            std::string basePath = ( lastSlash != std::string::npos ) 
                ? config.statesDataset.substr( 0, lastSlash + 1 )
                : "/";
            
            // Read full states once (we'll extract positions and velocities)
            std::vector< std::vector< double > > states;
            h5File.getDataSet( config.statesDataset ).read( states );
            
            // Write connectivity if needed for polyline
            if ( config.generateStaticPolyline )
            {
                if ( config.connectivityDataset.empty( ) )
                {
                    config.connectivityDataset = basePath + "connectivity";
                }
                
                if ( !h5File.exist( config.connectivityDataset ) )
                {
                    std::vector< int > connectivity( config.numTimeSteps );
                    for ( size_t i = 0; i < config.numTimeSteps; ++i )
                    {
                        connectivity[ i ] = static_cast< int >( i );
                    }
                    h5File.createDataSet< int >( 
                        config.connectivityDataset, 
                        HighFive::DataSpace( { config.numTimeSteps } ) 
                    ).write( connectivity );
                }
            }
            
            // Pre-extract positions (N x 3) for ParaView compatibility
            // ParaView doesn't handle JOIN() with HyperSlab well for Geometry
            config.positionsDataset = basePath + "positions_xyz";
            if ( !h5File.exist( config.positionsDataset ) )
            {
                std::vector< std::vector< double > > positions( config.numTimeSteps, std::vector< double >( 3 ) );
                for ( size_t i = 0; i < config.numTimeSteps; ++i )
                {
                    positions[i][0] = states[i][config.positionIndices[0]];
                    positions[i][1] = states[i][config.positionIndices[1]];
                    positions[i][2] = states[i][config.positionIndices[2]];
                }
                h5File.createDataSet< double >( 
                    config.positionsDataset, 
                    HighFive::DataSpace( { config.numTimeSteps, 3 } ) 
                ).write( positions );
            }
            
            // Pre-extract velocities (N x 3) if needed
            if ( config.includeVelocityVector )
            {
                config.velocitiesDataset = basePath + "velocities_xyz";
                if ( !h5File.exist( config.velocitiesDataset ) )
                {
                    std::vector< std::vector< double > > velocities( config.numTimeSteps, std::vector< double >( 3 ) );
                    for ( size_t i = 0; i < config.numTimeSteps; ++i )
                    {
                        velocities[i][0] = states[i][config.velocityIndices[0]];
                        velocities[i][1] = states[i][config.velocityIndices[1]];
                        velocities[i][2] = states[i][config.velocityIndices[2]];
                    }
                    h5File.createDataSet< double >( 
                        config.velocitiesDataset, 
                        HighFive::DataSpace( { config.numTimeSteps, 3 } ) 
                    ).write( velocities );
                }
            }
        }
    }
}

void TrajectoryXDMFGenerator::generateGrids( std::ostream& os )
{
    // Generate per-trajectory static polylines
    for ( const auto& config : trajectoryConfigs_ )
    {
        if ( config.generateStaticPolyline )
        {
            generateStaticTrajectory( os, config, 2 );
        }
    }
}

void TrajectoryXDMFGenerator::generateStaticTrajectory( std::ostream& os,
                                                         const TrajectoryConfig& config,
                                                         int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string filename = getFilename( config.h5FilePath );
    
    os << "\n" << ind << "<!-- Static trajectory polyline for " << config.bodyName << " -->\n";
    os << ind << "<Grid Name=\"" << config.bodyName << "_Trajectory\" GridType=\"Uniform\">\n";
    
    // Topology: Polyline with N nodes
    os << ind << "  <Topology TopologyType=\"Polyline\" NumberOfElements=\"1\" NodesPerElement=\"" 
       << config.numTimeSteps << "\">\n";
    writeHDF5DataItem( os, config.h5FilePath, config.connectivityDataset,
                       { config.numTimeSteps }, "Int", 4, indentLevel + 2 );
    os << ind << "  </Topology>\n";
    
    // Geometry: XYZ positions
    os << ind << "  <Geometry GeometryType=\"XYZ\">\n";
    writePositionGeometry( os, config, indentLevel + 2 );
    os << ind << "  </Geometry>\n";
    
    // Time attribute
    if ( config.includeTimeAttribute )
    {
        writeTimeAttribute( os, config, indentLevel + 1 );
    }
    
    // Velocity vector attribute
    if ( config.includeVelocityVector )
    {
        writeVelocityAttribute( os, config, indentLevel + 1 );
    }
    
    // Dependent variable attributes
    writeDependentVariableAttributes( os, config, indentLevel + 1 );
    
    os << ind << "</Grid>\n";
}

void TrajectoryXDMFGenerator::writePositionGeometry( std::ostream& os,
                                                      const TrajectoryConfig& config,
                                                      int indentLevel )
{
    // Use pre-extracted positions dataset (N x 3) - simple, no HyperSlab/JOIN needed
    writeHDF5DataItem( os, 
                       config.h5FilePath,
                       config.positionsDataset,
                       { config.numTimeSteps, 3 },
                       "Float", 8, indentLevel );
}

void TrajectoryXDMFGenerator::writeVelocityAttribute( std::ostream& os,
                                                       const TrajectoryConfig& config,
                                                       int indentLevel )
{
    std::string ind = indent( indentLevel );
    
    os << ind << "<Attribute Name=\"Velocity\" AttributeType=\"Vector\" Center=\"Node\">\n";
    // Use pre-extracted velocities dataset (N x 3)
    writeHDF5DataItem( os,
                       config.h5FilePath,
                       config.velocitiesDataset,
                       { config.numTimeSteps, 3 },
                       "Float", 8, indentLevel + 1 );
    os << ind << "</Attribute>\n";
}

void TrajectoryXDMFGenerator::writeTimeAttribute( std::ostream& os,
                                                   const TrajectoryConfig& config,
                                                   int indentLevel )
{
    std::string ind = indent( indentLevel );
    
    os << ind << "<Attribute Name=\"Time\" AttributeType=\"Scalar\" Center=\"Node\">\n";
    writeHDF5DataItem( os, config.h5FilePath, config.timesDataset,
                       { config.numTimeSteps }, "Float", 8, indentLevel + 1 );
    os << ind << "</Attribute>\n";
}

void TrajectoryXDMFGenerator::writeDependentVariableAttributes( std::ostream& os,
                                                                 const TrajectoryConfig& config,
                                                                 int indentLevel )
{
    std::string ind = indent( indentLevel );
    
    for ( const auto& attr : config.dependentVariableAttributes )
    {
        os << ind << "<Attribute Name=\"" << attr.name << "\" AttributeType=\"" 
           << attr.getTypeString( ) << "\" Center=\"Node\">\n";
        
        if ( attr.columnIndices.size( ) == 1 )
        {
            // Single column - use HyperSlab
            writeHDF5HyperSlabDataItem( os,
                                        attr.h5FilePath,
                                        attr.datasetPath,
                                        { attr.numElements, config.dependentVariablesSize },
                                        0, attr.columnIndices[ 0 ],
                                        attr.numElements, 1,
                                        "Float", 8, indentLevel + 1 );
        }
        else
        {
            // Multiple columns - use JOIN
            writeJoinedColumnsDataItem( os,
                                        attr.h5FilePath,
                                        attr.datasetPath,
                                        { attr.numElements, config.dependentVariablesSize },
                                        attr.numElements,
                                        attr.columnIndices,
                                        "Float", 8, indentLevel + 1 );
        }
        
        os << ind << "</Attribute>\n";
    }
}

// ================================================================================================
// PointCloudXDMFGenerator implementation
// ================================================================================================

void PointCloudXDMFGenerator::generateGrids( std::ostream& os )
{
    for ( const auto& config : pointCloudConfigs_ )
    {
        std::string ind = indent( 2 );
        
        os << "\n" << ind << "<!-- Point cloud: " << config.name << " -->\n";
        os << ind << "<Grid Name=\"" << config.name << "\" GridType=\"Uniform\">\n";
        
        // Topology: Polyvertex (collection of points)
        os << ind << "  <Topology TopologyType=\"Polyvertex\" NumberOfElements=\"" 
           << config.numPoints << "\"/>\n";
        
        // Geometry
        os << ind << "  <Geometry GeometryType=\"XYZ\">\n";
        writeHDF5DataItem( os, config.h5FilePath, config.positionsDataset,
                           { config.numPoints, 3 }, "Float", 8, 4 );
        os << ind << "  </Geometry>\n";
        
        // Attributes
        for ( const auto& attr : config.attributes )
        {
            os << ind << "  <Attribute Name=\"" << attr.name << "\" AttributeType=\"" 
               << attr.getTypeString( ) << "\" Center=\"Node\">\n";
            
            if ( attr.columnIndices.empty( ) || attr.columnIndices.size( ) == 1 )
            {
                // Simple dataset reference
                size_t dim = ( attr.type == XDMFAttributeType::Scalar ) ? 1 : 3;
                if ( attr.type == XDMFAttributeType::Scalar )
                {
                    writeHDF5DataItem( os, attr.h5FilePath, attr.datasetPath,
                                       { attr.numElements }, "Float", 8, 4 );
                }
                else
                {
                    writeHDF5DataItem( os, attr.h5FilePath, attr.datasetPath,
                                       { attr.numElements, 3 }, "Float", 8, 4 );
                }
            }
            
            os << ind << "  </Attribute>\n";
        }
        
        os << ind << "</Grid>\n";
    }
}

// ================================================================================================
// CompositeXDMFGenerator implementation
// ================================================================================================

void CompositeXDMFGenerator::generateGrids( std::ostream& os )
{
    // Generate trajectory grids
    trajectoryGenerator_.generateGrids( os );
    
    // Generate point cloud grids
    pointCloudGenerator_.generateGrids( os );
    
    // Generate observation grids
    observationGenerator_.generateGrids( os );
}

// ================================================================================================
// ObservationXDMFGenerator implementation
// ================================================================================================

void ObservationXDMFGenerator::generateGrids( std::ostream& os )
{
    // Generate per-observation-set grids
    for ( const auto& config : observationConfigs_ )
    {
        if ( config.numObservations > 0 )
        {
            generateObservationSetGrid( os, config, 2 );
        }
    }
}

void ObservationXDMFGenerator::generateObservationSetGrid( std::ostream& os,
                                                            const ObservationXDMFConfig& config,
                                                            int indentLevel )
{
    std::string ind = indent( indentLevel );
    
    os << "\n" << ind << "<!-- Observation set: " << config.setName 
       << " (" << config.observableTypeName << ") -->\n";
    os << ind << "<Grid Name=\"" << config.setName << "_" << config.observableTypeName 
       << "\" GridType=\"Uniform\">\n";
    
    // Topology: Polyvertex (collection of points)
    os << ind << "  <Topology TopologyType=\"Polyvertex\" NumberOfElements=\"" 
       << config.numObservations << "\"/>\n";
    
    // Geometry: XYZ with time as X, observation value as Y, and 0 as Z
    // This allows visualization as a scatter plot in ParaView
    os << ind << "  <Geometry GeometryType=\"XYZ\">\n";
    writeTimeGeometry( os, config, indentLevel + 2 );
    os << ind << "  </Geometry>\n";
    
    // Observation value attribute (for coloring/sizing glyphs)
    writeObservationAttribute( os, config, indentLevel + 1 );
    
    // Weights attribute
    if ( config.hasWeights )
    {
        writeWeightsAttribute( os, config, indentLevel + 1 );
    }
    
    // Residuals attribute
    if ( config.hasResiduals )
    {
        writeResidualsAttribute( os, config, indentLevel + 1 );
    }
    
    // Dependent variable attributes
    if ( config.hasDependentVariables )
    {
        writeDependentVariableAttributes( os, config, indentLevel + 1 );
    }
    
    os << ind << "</Grid>\n";
}

void ObservationXDMFGenerator::writeTimeGeometry( std::ostream& os,
                                                   const ObservationXDMFConfig& config,
                                                   int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string filename = getFilename( config.h5FilePath );
    std::string fullTimesPath = config.h5GroupPath + "/" + config.timesDataset;
    std::string fullObsPath = config.h5GroupPath + "/" + config.observationsDataset;
    
    // Create XYZ geometry using a Function that combines:
    // X = time, Y = observation value (first component), Z = 0
    // This creates a time-series scatter plot visualization
    
    os << ind << "<DataItem ItemType=\"Function\" Function=\"JOIN($0, $1, $2)\" "
       << "Dimensions=\"" << config.numObservations << " 3\">\n";
    
    // $0: Time values (X coordinate)
    os << ind << "  <DataItem Dimensions=\"" << config.numObservations 
       << "\" NumberType=\"Float\" Precision=\"15\" Format=\"HDF\">\n";
    os << ind << "    " << filename << ":" << fullTimesPath << "\n";
    os << ind << "  </DataItem>\n";
    
    // $1: First observation component (Y coordinate) - use HyperSlab for first column
    os << ind << "  <DataItem ItemType=\"HyperSlab\" Dimensions=\"" << config.numObservations 
       << "\" Type=\"HyperSlab\">\n";
    os << ind << "    <DataItem Dimensions=\"3 2\" Format=\"XML\">\n";
    os << ind << "      0 0\n";  // start: row 0, col 0
    os << ind << "      1 1\n";  // stride
    os << ind << "      " << config.numObservations << " 1\n";  // count
    os << ind << "    </DataItem>\n";
    os << ind << "    <DataItem Dimensions=\"" << config.numObservations << " " 
       << config.observableSize << "\" NumberType=\"Float\" Precision=\"15\" Format=\"HDF\">\n";
    os << ind << "      " << filename << ":" << fullObsPath << "\n";
    os << ind << "    </DataItem>\n";
    os << ind << "  </DataItem>\n";
    
    // $2: Zero values (Z coordinate) - create inline constant array
    os << ind << "  <DataItem ItemType=\"Function\" Function=\"0 * $0\" Dimensions=\"" 
       << config.numObservations << "\">\n";
    os << ind << "    <DataItem Dimensions=\"" << config.numObservations 
       << "\" NumberType=\"Float\" Precision=\"15\" Format=\"HDF\">\n";
    os << ind << "      " << filename << ":" << fullTimesPath << "\n";
    os << ind << "    </DataItem>\n";
    os << ind << "  </DataItem>\n";
    
    os << ind << "</DataItem>\n";
}

void ObservationXDMFGenerator::writeObservationAttribute( std::ostream& os,
                                                           const ObservationXDMFConfig& config,
                                                           int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string fullObsPath = config.h5GroupPath + "/" + config.observationsDataset;
    
    // Determine attribute type based on observable size
    std::string attrType;
    if ( config.observableSize == 1 )
    {
        attrType = "Scalar";
    }
    else if ( config.observableSize == 2 || config.observableSize == 3 )
    {
        attrType = "Vector";
    }
    else
    {
        // For larger sizes, treat as a tensor or just use first component as scalar
        attrType = "Scalar";
    }
    
    os << ind << "<Attribute Name=\"Observation\" AttributeType=\"" << attrType 
       << "\" Center=\"Node\">\n";
    
    if ( config.observableSize == 1 )
    {
        // Single column - use HyperSlab to extract column 0
        writeHDF5HyperSlabDataItem( os,
                                    config.h5FilePath,
                                    fullObsPath,
                                    { config.numObservations, config.observableSize },
                                    0, 0,
                                    config.numObservations, 1,
                                    "Float", 8, indentLevel + 1 );
    }
    else if ( config.observableSize == 2 || config.observableSize == 3 )
    {
        // Vector - use full dataset
        writeHDF5DataItem( os,
                          config.h5FilePath,
                          fullObsPath,
                          { config.numObservations, config.observableSize },
                          "Float", 8, indentLevel + 1 );
    }
    else
    {
        // Use first column only for unsupported sizes
        writeHDF5HyperSlabDataItem( os,
                                    config.h5FilePath,
                                    fullObsPath,
                                    { config.numObservations, config.observableSize },
                                    0, 0,
                                    config.numObservations, 1,
                                    "Float", 8, indentLevel + 1 );
    }
    
    os << ind << "</Attribute>\n";
}

void ObservationXDMFGenerator::writeWeightsAttribute( std::ostream& os,
                                                       const ObservationXDMFConfig& config,
                                                       int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string fullWeightsPath = config.h5GroupPath + "/" + config.weightsDataset;
    
    // Similar logic to observations
    std::string attrType = ( config.observableSize == 1 ) ? "Scalar" : 
                           ( ( config.observableSize == 2 || config.observableSize == 3 ) ? "Vector" : "Scalar" );
    
    os << ind << "<Attribute Name=\"Weight\" AttributeType=\"" << attrType 
       << "\" Center=\"Node\">\n";
    
    if ( config.observableSize == 1 )
    {
        writeHDF5HyperSlabDataItem( os,
                                    config.h5FilePath,
                                    fullWeightsPath,
                                    { config.numObservations, config.observableSize },
                                    0, 0,
                                    config.numObservations, 1,
                                    "Float", 8, indentLevel + 1 );
    }
    else if ( config.observableSize == 2 || config.observableSize == 3 )
    {
        writeHDF5DataItem( os,
                          config.h5FilePath,
                          fullWeightsPath,
                          { config.numObservations, config.observableSize },
                          "Float", 8, indentLevel + 1 );
    }
    else
    {
        writeHDF5HyperSlabDataItem( os,
                                    config.h5FilePath,
                                    fullWeightsPath,
                                    { config.numObservations, config.observableSize },
                                    0, 0,
                                    config.numObservations, 1,
                                    "Float", 8, indentLevel + 1 );
    }
    
    os << ind << "</Attribute>\n";
}

void ObservationXDMFGenerator::writeResidualsAttribute( std::ostream& os,
                                                         const ObservationXDMFConfig& config,
                                                         int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string fullResidualsPath = config.h5GroupPath + "/" + config.residualsDataset;
    
    std::string attrType = ( config.observableSize == 1 ) ? "Scalar" : 
                           ( ( config.observableSize == 2 || config.observableSize == 3 ) ? "Vector" : "Scalar" );
    
    os << ind << "<Attribute Name=\"Residual\" AttributeType=\"" << attrType 
       << "\" Center=\"Node\">\n";
    
    if ( config.observableSize == 1 )
    {
        writeHDF5HyperSlabDataItem( os,
                                    config.h5FilePath,
                                    fullResidualsPath,
                                    { config.numObservations, config.observableSize },
                                    0, 0,
                                    config.numObservations, 1,
                                    "Float", 8, indentLevel + 1 );
    }
    else if ( config.observableSize == 2 || config.observableSize == 3 )
    {
        writeHDF5DataItem( os,
                          config.h5FilePath,
                          fullResidualsPath,
                          { config.numObservations, config.observableSize },
                          "Float", 8, indentLevel + 1 );
    }
    else
    {
        writeHDF5HyperSlabDataItem( os,
                                    config.h5FilePath,
                                    fullResidualsPath,
                                    { config.numObservations, config.observableSize },
                                    0, 0,
                                    config.numObservations, 1,
                                    "Float", 8, indentLevel + 1 );
    }
    
    os << ind << "</Attribute>\n";
}

void ObservationXDMFGenerator::writeDependentVariableAttributes( std::ostream& os,
                                                                  const ObservationXDMFConfig& config,
                                                                  int indentLevel )
{
    std::string ind = indent( indentLevel );
    std::string fullDepVarPath = config.h5GroupPath + "/" + config.dependentVariablesDataset;
    
    for ( const auto& attr : config.dependentVariableAttributes )
    {
        os << ind << "<Attribute Name=\"" << attr.name << "\" AttributeType=\"" 
           << attr.getTypeString( ) << "\" Center=\"Node\">\n";
        
        if ( attr.columnIndices.size( ) == 1 )
        {
            // Single column - use HyperSlab
            writeHDF5HyperSlabDataItem( os,
                                        config.h5FilePath,
                                        fullDepVarPath,
                                        { config.numObservations, config.dependentVariablesSize },
                                        0, attr.columnIndices[ 0 ],
                                        config.numObservations, 1,
                                        "Float", 8, indentLevel + 1 );
        }
        else
        {
            // Multiple columns - use JOIN
            writeJoinedColumnsDataItem( os,
                                        config.h5FilePath,
                                        fullDepVarPath,
                                        { config.numObservations, config.dependentVariablesSize },
                                        config.numObservations,
                                        attr.columnIndices,
                                        "Float", 8, indentLevel + 1 );
        }
        
        os << ind << "</Attribute>\n";
    }
}

} // namespace io
} // namespace tudat

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
 *      XDMF3 generator for ParaView visualization of Tudat simulation results.
 *      Supports multiple HDF5 files, trajectory polylines, animated particles,
 *      velocity vectors for glyph visualization, and dependent variables.
 *
 */

#ifndef TUDAT_IO_XDMF_GENERATOR_H
#define TUDAT_IO_XDMF_GENERATOR_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

#include <highfive/highfive.hpp>

namespace tudat
{
namespace io
{

// ================================================================================================
// Forward declarations
// ================================================================================================
class XDMFGeneratorBase;
class TrajectoryXDMFGenerator;

// ================================================================================================
// Attribute type enumeration
// ================================================================================================

//! Type of attribute for XDMF
enum class XDMFAttributeType
{
    Scalar,
    Vector,
    Tensor,
    Matrix
};

// ================================================================================================
// Attribute configuration
// ================================================================================================

//! Configuration for an attribute on a grid
struct XDMFAttributeConfig
{
    std::string name;                     //!< Attribute name (displayed in ParaView)
    XDMFAttributeType type;               //!< Scalar, Vector, Tensor, Matrix
    std::string h5FilePath;               //!< Path to HDF5 file
    std::string datasetPath;              //!< Path to dataset within HDF5
    std::vector< int > columnIndices;     //!< Which columns to extract (for slicing from 2D dataset)
    size_t numElements = 0;               //!< Number of elements (rows)
    
    //! Get XDMF attribute type string
    std::string getTypeString( ) const
    {
        switch ( type )
        {
            case XDMFAttributeType::Scalar: return "Scalar";
            case XDMFAttributeType::Vector: return "Vector";
            case XDMFAttributeType::Tensor: return "Tensor";
            case XDMFAttributeType::Matrix: return "Matrix";
            default: return "Scalar";
        }
    }
};

// ================================================================================================
// Trajectory configuration
// ================================================================================================

//! Configuration for a single trajectory in XDMF output
struct TrajectoryConfig
{
    std::string bodyName;              //!< Name of the body (used in grid naming)
    std::string h5FilePath;            //!< Path to HDF5 file containing this trajectory
    std::string statesDataset;         //!< Path to states dataset (e.g., "/Trajectories/.../states")
    std::string timesDataset;          //!< Path to times dataset
    std::string connectivityDataset;   //!< Path to connectivity dataset (will be created if needed)
    size_t numTimeSteps = 0;           //!< Number of time steps
    size_t stateSize = 6;              //!< Size of state vector (typically 6 for pos+vel)
    
    // Position indices in state vector (default: x=0, y=1, z=2)
    std::vector< int > positionIndices = { 0, 1, 2 };
    
    // Velocity indices in state vector (default: vx=3, vy=4, vz=5)
    std::vector< int > velocityIndices = { 3, 4, 5 };
    
    // Output options
    bool generateStaticPolyline = true;      //!< Generate static full trajectory polyline
    bool generateAnimatedParticle = true;    //!< Generate temporal particle animation
    bool includeVelocityVector = true;       //!< Include velocity as vector attribute (for glyphs)
    bool includeTimeAttribute = true;        //!< Include time as scalar attribute
    
    // Dependent variables
    std::string dependentVariablesDataset;   //!< Path to dependent variables dataset (optional)
    size_t dependentVariablesSize = 0;       //!< Size of dependent variables vector
    
    // Dependent variable attributes (populated from ID mapping)
    std::vector< XDMFAttributeConfig > dependentVariableAttributes;
};

// ================================================================================================
// Point cloud configuration (for future use)
// ================================================================================================

//! Configuration for a point cloud in XDMF output
struct PointCloudConfig
{
    std::string name;                  //!< Name of the point cloud
    std::string h5FilePath;            //!< Path to HDF5 file
    std::string positionsDataset;      //!< Path to positions dataset
    size_t numPoints = 0;              //!< Number of points
    
    // Optional attributes
    std::vector< XDMFAttributeConfig > attributes;
};

// ================================================================================================
// Base XDMF Generator class
// ================================================================================================

//! Abstract base class for XDMF3 file generation
class XDMFGeneratorBase
{
public:
    
    //! Default constructor
    XDMFGeneratorBase( ) = default;
    
    //! Virtual destructor
    virtual ~XDMFGeneratorBase( ) = default;
    
    //! Write the XDMF file
    void write( const std::string& xdmfFilePath );
    
protected:
    
    //! Override to add grids to the domain
    virtual void generateGrids( std::ostream& os ) = 0;
    
    //! Write XDMF header
    void writeHeader( std::ostream& os );
    
    //! Write XDMF footer
    void writeFooter( std::ostream& os );
    
    //! Write an HDF5 DataItem reference (simple, full dataset)
    void writeHDF5DataItem( std::ostream& os,
                            const std::string& h5FilePath,
                            const std::string& datasetPath,
                            const std::vector< size_t >& dimensions,
                            const std::string& numberType = "Float",
                            int precision = 8,
                            int indentLevel = 4 );
    
    //! Write an HDF5 DataItem with HyperSlab (for extracting columns)
    void writeHDF5HyperSlabDataItem( std::ostream& os,
                                     const std::string& h5FilePath,
                                     const std::string& datasetPath,
                                     const std::vector< size_t >& sourceDimensions,
                                     size_t startRow,
                                     size_t startCol,
                                     size_t countRows,
                                     size_t countCols,
                                     const std::string& numberType = "Float",
                                     int precision = 8,
                                     int indentLevel = 4 );
    
    //! Write a Function DataItem that joins multiple HyperSlabs into a vector
    void writeJoinedColumnsDataItem( std::ostream& os,
                                     const std::string& h5FilePath,
                                     const std::string& datasetPath,
                                     const std::vector< size_t >& sourceDimensions,
                                     size_t numRows,
                                     const std::vector< int >& columnIndices,
                                     const std::string& numberType = "Float",
                                     int precision = 8,
                                     int indentLevel = 4 );
    
    //! Get indent string
    std::string indent( int level ) const
    {
        return std::string( level * 2, ' ' );
    }
    
    //! Extract just the filename from a path
    std::string getFilename( const std::string& path ) const;
};

// ================================================================================================
// Trajectory XDMF Generator
// ================================================================================================

// Forward declaration
class CompositeXDMFGenerator;

//! XDMF generator specialized for trajectory data
class TrajectoryXDMFGenerator : public XDMFGeneratorBase
{
    friend class CompositeXDMFGenerator;
    
public:
    
    //! Default constructor
    TrajectoryXDMFGenerator( ) = default;
    
    //! Constructor with single trajectory
    explicit TrajectoryXDMFGenerator( const TrajectoryConfig& config )
    {
        addTrajectory( config );
    }
    
    //! Constructor with multiple trajectories
    explicit TrajectoryXDMFGenerator( const std::vector< TrajectoryConfig >& configs )
        : trajectoryConfigs_( configs )
    { }
    
    //! Add a trajectory configuration
    void addTrajectory( const TrajectoryConfig& config )
    {
        trajectoryConfigs_.push_back( config );
    }
    
    //! Clear all trajectory configurations
    void clear( )
    {
        trajectoryConfigs_.clear( );
    }
    
    //! Get number of trajectories
    size_t getNumTrajectories( ) const
    {
        return trajectoryConfigs_.size( );
    }
    
    //! Write connectivity datasets to HDF5 files (call before write())
    void writeConnectivityToHDF5( );
    
protected:
    
    //! Generate all grids
    void generateGrids( std::ostream& os ) override;
    
private:
    
    //! Generate static polyline trajectory grid
    void generateStaticTrajectory( std::ostream& os,
                                   const TrajectoryConfig& config,
                                   int indentLevel );
    
    //! Generate animated particle temporal collection
    void generateAnimatedParticle( std::ostream& os,
                                   const TrajectoryConfig& config,
                                   int indentLevel );
    
    //! Write position geometry using joined columns
    void writePositionGeometry( std::ostream& os,
                                const TrajectoryConfig& config,
                                int indentLevel );
    
    //! Write velocity vector attribute
    void writeVelocityAttribute( std::ostream& os,
                                 const TrajectoryConfig& config,
                                 int indentLevel );
    
    //! Write time scalar attribute
    void writeTimeAttribute( std::ostream& os,
                             const TrajectoryConfig& config,
                             int indentLevel );
    
    //! Write dependent variable attributes
    void writeDependentVariableAttributes( std::ostream& os,
                                           const TrajectoryConfig& config,
                                           int indentLevel );
    
    //! Write single timestep position for animated particle
    void writeTimestepPositionGeometry( std::ostream& os,
                                        const TrajectoryConfig& config,
                                        size_t timeIndex,
                                        int indentLevel );
    
    //! Write single timestep velocity for animated particle
    void writeTimestepVelocityAttribute( std::ostream& os,
                                         const TrajectoryConfig& config,
                                         size_t timeIndex,
                                         int indentLevel );
    
    std::vector< TrajectoryConfig > trajectoryConfigs_;
};

// ================================================================================================
// Point Cloud XDMF Generator (placeholder for future)
// ================================================================================================

//! XDMF generator for point cloud data
class PointCloudXDMFGenerator : public XDMFGeneratorBase
{
    friend class CompositeXDMFGenerator;
    
public:
    
    //! Default constructor
    PointCloudXDMFGenerator( ) = default;
    
    //! Add a point cloud configuration
    void addPointCloud( const PointCloudConfig& config )
    {
        pointCloudConfigs_.push_back( config );
    }
    
protected:
    
    //! Generate all grids
    void generateGrids( std::ostream& os ) override;
    
private:
    
    std::vector< PointCloudConfig > pointCloudConfigs_;
};

// ================================================================================================
// Composite XDMF Generator (combines multiple generators)
// ================================================================================================

//! XDMF generator that combines trajectories and point clouds
class CompositeXDMFGenerator : public XDMFGeneratorBase
{
public:
    
    //! Default constructor
    CompositeXDMFGenerator( ) = default;
    
    //! Get trajectory generator for modification
    TrajectoryXDMFGenerator& getTrajectoryGenerator( )
    {
        return trajectoryGenerator_;
    }
    
    //! Get point cloud generator for modification
    PointCloudXDMFGenerator& getPointCloudGenerator( )
    {
        return pointCloudGenerator_;
    }
    
    //! Write connectivity to HDF5 (delegates to trajectory generator)
    void writeConnectivityToHDF5( )
    {
        trajectoryGenerator_.writeConnectivityToHDF5( );
    }
    
protected:
    
    //! Generate all grids from sub-generators
    void generateGrids( std::ostream& os ) override;
    
private:
    
    TrajectoryXDMFGenerator trajectoryGenerator_;
    PointCloudXDMFGenerator pointCloudGenerator_;
};

} // namespace io
} // namespace tudat

#endif // TUDAT_IO_XDMF_GENERATOR_H

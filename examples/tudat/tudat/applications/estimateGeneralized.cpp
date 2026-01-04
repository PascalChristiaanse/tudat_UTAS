// /*
//  *  Generalized Spacecraft State Estimation
//  *
//  *  Supports:
//  *  - TDOA and/or FDOA observations
//  *  - Simulated or actual (from UDL) observations
//  *  - JSON configuration file
//  *  - Command-line argument overrides
//  *  - HDF5 output for results
//  */

// #include <iostream>
// #include <iomanip>
// #include <cmath>
// #include <limits>
// #include <algorithm>
// #include <string>
// #include <cstdlib>
// #include <filesystem>
// #include <fstream>
// #include <getopt.h>

// #include <nlohmann/json.hpp>

// #include "tudat/interface/horizons.h"
// #include "tudat/astro/basic_astro.h"
// #include "tudat/simulation/simulation.h"
// #include "tudat/io.h"
// #include "tudat/io/hdf5Manager.h"
// #include "tudat/simulation/estimation_setup/orbitDeterminationManager.h"
// #include "tudat/simulation/estimation_setup/createObservationModel.h"
// #include "tudat/simulation/estimation_setup/observationSimulationSettings.h"
// #include "tudat/simulation/estimation_setup/simulateObservations.h"
// #include "tudat/simulation/estimation_setup/createEstimatableParameters.h"

// using namespace tudat;
// using namespace tudat::horizons_interface;
// using namespace tudat::basic_astrodynamics;
// using namespace tudat::simulation_setup;
// using namespace tudat::propagators;
// using namespace tudat::numerical_integrators;
// using namespace tudat::observation_models;
// using namespace tudat::earth_orientation;
// using namespace tudat::estimatable_parameters;
// using namespace tudat::ground_stations;
// using namespace tudat::io;

// using json = nlohmann::json;

// // =============================================================================
// // Configuration Structure
// // =============================================================================

// struct EstimationConfig {
//     // Observation mode
//     bool useTDOA = true;
//     bool useFDOA = false;
//     bool useRange = false;
//     bool useOWDMF = false;  // One-way Doppler measured frequency
//     bool simulateObservations = true;

//     // Frame configuration
//     std::string globalFrameOrigin = "SSB";
//     std::string globalFrameOrientation = "ECLIPJ2000";

//     // Target configuration
//     std::string targetName = "CAPSTONE";
//     std::string targetDisplayName = "CAPSTONE";
//     double targetMass = 25.0;
//     double transmitterFrequency = 8465000000.0;  // X-band Hz

//     // Time configuration
//     std::string startDateString = "2025-10-11T16:41:21.772479857";
//     std::string endDateString = "2025-10-11T17:00:10.772479857";
//     double observationTimestep = 1.0;
//     double ephemerisPadding = 1800.0;  // 30 minutes buffer
//     std::string horizonsStepSize = "1m";

//     // UDL data files
//     std::vector< std::string > udlFiles;

//     // Estimation configuration
//     Eigen::Vector3d positionPerturbation = Eigen::Vector3d( 1000.0, 1000.0, 1000.0 );
//     Eigen::Vector3d velocityPerturbation = Eigen::Vector3d( 1.0, 1.0, 1.0 );
//     int maxIterations = 100;
//     double convergenceThreshold = 1e-10;
//     int minIterations = 3;

//     // Integrator settings
//     double integratorStepSize = 10.0;

//     // SRP configuration
//     double srpSurfaceArea = 1.0;              // m^2
//     double srpReflectivityCoefficient = 1.5;  // typical for spacecraft

//     // Gravity model settings
//     int earthSphericalHarmonicsDegree = 50;
//     int earthSphericalHarmonicsOrder = 50;
//     int moonSphericalHarmonicsDegree = 50;
//     int moonSphericalHarmonicsOrder = 50;

//     // Output configuration
//     std::string outputPath = "./estimation_results.h5";
//     bool generateXDMF = true;

//     // Print configuration
//     void print( ) const
//     {
//         std::cout << std::boolalpha;
//         std::cout << "\n=== Estimation Configuration ===" << std::endl;
//         std::cout << "Observation types:" << std::endl;
//         std::cout << "  - TDOA: " << useTDOA << std::endl;
//         std::cout << "  - FDOA: " << useFDOA << std::endl;
//         std::cout << "  - Range: " << useRange << std::endl;
//         std::cout << "  - OWDMF: " << useOWDMF << std::endl;
//         std::cout << "  - Simulated: " << simulateObservations << std::endl;
//         std::cout << "Frame:" << std::endl;
//         std::cout << "  - Origin: " << globalFrameOrigin << std::endl;
//         std::cout << "  - Orientation: " << globalFrameOrientation << std::endl;
//         std::cout << "Target:" << std::endl;
//         std::cout << "  - Name: " << targetName << " (" << targetDisplayName << ")" << std::endl;
//         std::cout << "  - Mass: " << targetMass << " kg" << std::endl;
//         std::cout << "  - Transmitter frequency: " << transmitterFrequency / 1e9 << " GHz" << std::endl;
//         std::cout << "Time:" << std::endl;
//         std::cout << "  - Start: " << startDateString << std::endl;
//         std::cout << "  - End: " << endDateString << std::endl;
//         std::cout << "  - Timestep: " << observationTimestep << " s" << std::endl;
//         std::cout << "UDL files: " << udlFiles.size( ) << " file(s)" << std::endl;
//         for( const auto& f : udlFiles )
//         {
//             std::cout << "  - " << f << std::endl;
//         }
//         std::cout << "Estimation:" << std::endl;
//         std::cout << "  - Position perturbation: [" << positionPerturbation.transpose( ) << "] m" << std::endl;
//         std::cout << "  - Velocity perturbation: [" << velocityPerturbation.transpose( ) << "] m/s" << std::endl;
//         std::cout << "  - Max iterations: " << maxIterations << std::endl;
//         std::cout << "  - Convergence threshold: " << convergenceThreshold << std::endl;
//         std::cout << "Dynamics:" << std::endl;
//         std::cout << "  - Earth SH degree/order: " << earthSphericalHarmonicsDegree << "/" << earthSphericalHarmonicsOrder << std::endl;
//         std::cout << "  - Moon SH degree/order: " << moonSphericalHarmonicsDegree << "/" << moonSphericalHarmonicsOrder << std::endl;
//         std::cout << "  - SRP surface area: " << srpSurfaceArea << " m^2" << std::endl;
//         std::cout << "  - SRP reflectivity: " << srpReflectivityCoefficient << std::endl;
//         std::cout << "Output:" << std::endl;
//         std::cout << "  - Path: " << outputPath << std::endl;
//         std::cout << "  - Generate XDMF: " << generateXDMF << std::endl;
//         std::cout << std::noboolalpha;
//         std::cout << "================================\n" << std::endl;
//     }
// };

// // =============================================================================
// // Forward Declarations
// // =============================================================================

// void printUsage( const char* programName );
// EstimationConfig loadConfigFromJson( const std::string& configPath );
// EstimationConfig parseCommandLineArgs( int argc, char** argv );

// std::vector< Time > generateObservationTimes(
//         const EstimationConfig& config,
//         std::shared_ptr< TerrestrialTimeScaleConverter > timeConverter = std::make_shared< TerrestrialTimeScaleConverter >( ) );

// SystemOfBodies createSimulationEnvironment( const EstimationConfig& config );

// std::shared_ptr< ObservationCollection< double, Time > > setupGroundStations( SystemOfBodies& bodies, BatchVLBI& udl );

// void createSpacecraftBody( SystemOfBodies& bodies,
//                            const EstimationConfig& config,
//                            const std::map< double, Eigen::Vector6d >& ephemerisData );

// std::pair< std::vector< std::shared_ptr< ObservationModelSettings > >,
//            std::vector< std::shared_ptr< ObservationSimulationSettings< double > > > >
// createObservationSettings( const EstimationConfig& config,
//                            std::shared_ptr< ObservationCollection< double, Time > > udlObservations,
//                            const std::vector< double >& observationTimes );

// std::shared_ptr< ObservationCollection< double, double > > getObservations(
//         const EstimationConfig& config,
//         BatchVLBI& udl,
//         SystemOfBodies& bodies,
//         const std::vector< std::shared_ptr< ObservationModelSettings > >& modelSettings,
//         const std::vector< std::shared_ptr< ObservationSimulationSettings< double > > >& simSettings );

// // =============================================================================
// // Usage and Help
// // =============================================================================

// void printUsage( const char* programName )
// {
//     std::cout << "Usage: " << programName << " [options]\n"
//               << "\nOptions:\n"
//               << "  -h, --help                  Show this help message\n"
//               << "  -c, --config <file>         Path to JSON configuration file\n"
//               << "  -s, --simulate              Use simulated observations (default)\n"
//               << "  -a, --actual                Use actual observations from UDL\n"
//               << "  -t, --tdoa                  Enable TDOA observations\n"
//               << "  -f, --fdoa                  Enable FDOA observations\n"
//               << "  -r, --range                 Enable one-way range observations\n"
//               << "  -d, --owdmf                 Enable one-way Doppler measured frequency observations\n"
//               << "  -o, --output <path>         Output HDF5 file path\n"
//               << "  -p, --perturbation <x,y,z,vx,vy,vz>  Initial state perturbation\n"
//               << "  -S, --step-size <seconds>   Integrator step size (default: 10.0)\n"
//               << "\nNotes:\n"
//               << "  - Command-line arguments override config file settings\n"
//               << "  - If no observation type specified, config file settings are used\n"
//               << "  - Default output: ./estimation_results.h5\n"
//               << std::endl;
// }

// // =============================================================================
// // Configuration Loading
// // =============================================================================

// EstimationConfig loadConfigFromJson( const std::string& configPath )
// {
//     EstimationConfig config;

//     std::ifstream f( configPath );
//     if( !f.is_open( ) )
//     {
//         throw std::runtime_error( "Cannot open config file: " + configPath );
//     }

//     json j = json::parse( f );

//     // Observation mode
//     if( j.contains( "observation_types" ) )
//     {
//         auto types = j[ "observation_types" ].get< std::vector< std::string > >( );
//         config.useTDOA = std::find( types.begin( ), types.end( ), "TDOA" ) != types.end( );
//         config.useFDOA = std::find( types.begin( ), types.end( ), "FDOA" ) != types.end( );
//         config.useRange = std::find( types.begin( ), types.end( ), "Range" ) != types.end( );
//         config.useOWDMF = std::find( types.begin( ), types.end( ), "OWDMF" ) != types.end( );
//     }
//     if( j.contains( "simulate_observations" ) )
//     {
//         config.simulateObservations = j[ "simulate_observations" ];
//     }

//     // Frame configuration
//     if( j.contains( "frame" ) )
//     {
//         auto& frame = j[ "frame" ];
//         if( frame.contains( "origin" ) ) config.globalFrameOrigin = frame[ "origin" ];
//         if( frame.contains( "orientation" ) ) config.globalFrameOrientation = frame[ "orientation" ];
//     }

//     // Target configuration
//     if( j.contains( "target" ) )
//     {
//         auto& target = j[ "target" ];
//         if( target.contains( "name" ) ) config.targetName = target[ "name" ];
//         if( target.contains( "display_name" ) ) config.targetDisplayName = target[ "display_name" ];
//         if( target.contains( "mass" ) ) config.targetMass = target[ "mass" ];
//         if( target.contains( "transmitter_frequency" ) ) config.transmitterFrequency = target[ "transmitter_frequency" ];
//     }

//     // Time configuration
//     if( j.contains( "time" ) )
//     {
//         auto& time = j[ "time" ];
//         if( time.contains( "start_date" ) ) config.startDateString = time[ "start_date" ];
//         if( time.contains( "end_date" ) ) config.endDateString = time[ "end_date" ];
//         if( time.contains( "timestep" ) ) config.observationTimestep = time[ "timestep" ];
//         if( time.contains( "ephemeris_padding" ) ) config.ephemerisPadding = time[ "ephemeris_padding" ];
//         if( time.contains( "horizons_step_size" ) ) config.horizonsStepSize = time[ "horizons_step_size" ];
//     }

//     // UDL files
//     if( j.contains( "udl_files" ) )
//     {
//         config.udlFiles = j[ "udl_files" ].get< std::vector< std::string > >( );
//     }

//     // Estimation configuration
//     if( j.contains( "estimation" ) )
//     {
//         auto& est = j[ "estimation" ];
//         if( est.contains( "position_perturbation" ) )
//         {
//             auto p = est[ "position_perturbation" ].get< std::vector< double > >( );
//             if( p.size( ) == 3 ) config.positionPerturbation = Eigen::Vector3d( p[ 0 ], p[ 1 ], p[ 2 ] );
//         }
//         if( est.contains( "velocity_perturbation" ) )
//         {
//             auto v = est[ "velocity_perturbation" ].get< std::vector< double > >( );
//             if( v.size( ) == 3 ) config.velocityPerturbation = Eigen::Vector3d( v[ 0 ], v[ 1 ], v[ 2 ] );
//         }
//         if( est.contains( "max_iterations" ) ) config.maxIterations = est[ "max_iterations" ];
//         if( est.contains( "convergence_threshold" ) ) config.convergenceThreshold = est[ "convergence_threshold" ];
//         if( est.contains( "min_iterations" ) ) config.minIterations = est[ "min_iterations" ];
//         if( est.contains( "integrator_step_size" ) ) config.integratorStepSize = est[ "integrator_step_size" ];
//     }

//     // Dynamics configuration (SRP and gravity)
//     if( j.contains( "dynamics" ) )
//     {
//         auto& dyn = j[ "dynamics" ];
//         if( dyn.contains( "srp_surface_area" ) ) config.srpSurfaceArea = dyn[ "srp_surface_area" ];
//         if( dyn.contains( "srp_reflectivity" ) ) config.srpReflectivityCoefficient = dyn[ "srp_reflectivity" ];
//         if( dyn.contains( "earth_sh_degree" ) ) config.earthSphericalHarmonicsDegree = dyn[ "earth_sh_degree" ];
//         if( dyn.contains( "earth_sh_order" ) ) config.earthSphericalHarmonicsOrder = dyn[ "earth_sh_order" ];
//         if( dyn.contains( "moon_sh_degree" ) ) config.moonSphericalHarmonicsDegree = dyn[ "moon_sh_degree" ];
//         if( dyn.contains( "moon_sh_order" ) ) config.moonSphericalHarmonicsOrder = dyn[ "moon_sh_order" ];
//     }

//     // Output configuration
//     if( j.contains( "output" ) )
//     {
//         auto& out = j[ "output" ];
//         if( out.contains( "path" ) ) config.outputPath = out[ "path" ];
//         if( out.contains( "generate_xdmf" ) ) config.generateXDMF = out[ "generate_xdmf" ];
//     }

//     return config;
// }

// EstimationConfig parseCommandLineArgs( int argc, char** argv )
// {
//     EstimationConfig config;
//     std::string configFile;
//     bool tdoaSet = false, fdoaSet = false, rangeSet = false, owdmfSet = false;

//     static struct option longOptions[] = { { "help", no_argument, nullptr, 'h' },
//                                            { "config", required_argument, nullptr, 'c' },
//                                            { "simulate", no_argument, nullptr, 's' },
//                                            { "actual", no_argument, nullptr, 'a' },
//                                            { "tdoa", no_argument, nullptr, 't' },
//                                            { "fdoa", no_argument, nullptr, 'f' },
//                                            { "range", no_argument, nullptr, 'r' },
//                                            { "owdmf", no_argument, nullptr, 'd' },
//                                            { "output", required_argument, nullptr, 'o' },
//                                            { "perturbation", required_argument, nullptr, 'p' },
//                                            { "step-size", required_argument, nullptr, 'S' },
//                                            { nullptr, 0, nullptr, 0 } };

//     int opt;
//     while( ( opt = getopt_long( argc, argv, "hc:satfrdo:p:S:", longOptions, nullptr ) ) != -1 )
//     {
//         switch( opt )
//         {
//             case 'h':
//                 printUsage( argv[ 0 ] );
//                 std::exit( 0 );
//             case 'c':
//                 configFile = optarg;
//                 break;
//             case 's':
//                 config.simulateObservations = true;
//                 break;
//             case 'a':
//                 config.simulateObservations = false;
//                 break;
//             case 't':
//                 config.useTDOA = true;
//                 tdoaSet = true;
//                 break;
//             case 'f':
//                 config.useFDOA = true;
//                 fdoaSet = true;
//                 break;
//             case 'r':
//                 config.useRange = true;
//                 rangeSet = true;
//                 break;
//             case 'd':
//                 config.useOWDMF = true;
//                 owdmfSet = true;
//                 break;
//             case 'o':
//                 config.outputPath = optarg;
//                 break;
//             case 'p': {
//                 // Parse perturbation: x,y,z,vx,vy,vz
//                 std::string pertStr( optarg );
//                 std::vector< double > vals;
//                 std::stringstream ss( pertStr );
//                 std::string item;
//                 while( std::getline( ss, item, ',' ) )
//                 {
//                     vals.push_back( std::stod( item ) );
//                 }
//                 if( vals.size( ) >= 3 )
//                 {
//                     config.positionPerturbation = Eigen::Vector3d( vals[ 0 ], vals[ 1 ], vals[ 2 ] );
//                 }
//                 if( vals.size( ) >= 6 )
//                 {
//                     config.velocityPerturbation = Eigen::Vector3d( vals[ 3 ], vals[ 4 ], vals[ 5 ] );
//                 }
//                 break;
//             }
//             case 'S':
//                 config.integratorStepSize = std::stod( optarg );
//                 break;
//             default:
//                 printUsage( argv[ 0 ] );
//                 std::exit( 1 );
//         }
//     }

//     // Load config file if specified, then apply CLI overrides
//     if( !configFile.empty( ) )
//     {
//         EstimationConfig fileConfig = loadConfigFromJson( configFile );

//         // Preserve CLI-specified values
//         bool cliSimulate = config.simulateObservations;
//         std::string cliOutput = config.outputPath;
//         Eigen::Vector3d cliPosPert = config.positionPerturbation;
//         Eigen::Vector3d cliVelPert = config.velocityPerturbation;
//         bool cliTDOA = config.useTDOA;
//         bool cliFDOA = config.useFDOA;
//         bool cliRange = config.useRange;
//         bool cliOWDMF = config.useOWDMF;

//         // Start with file config
//         config = fileConfig;

//         // Apply CLI overrides
//         if( cliOutput != "./estimation_results.h5" )
//         {
//             config.outputPath = cliOutput;
//         }
//         if( cliPosPert != Eigen::Vector3d( 1000.0, 1000.0, 1000.0 ) )
//         {
//             config.positionPerturbation = cliPosPert;
//         }
//         if( cliVelPert != Eigen::Vector3d( 1.0, 1.0, 1.0 ) )
//         {
//             config.velocityPerturbation = cliVelPert;
//         }
//         if( tdoaSet )
//         {
//             config.useTDOA = cliTDOA;
//         }
//         if( fdoaSet )
//         {
//             config.useFDOA = cliFDOA;
//         }
//         if( rangeSet )
//         {
//             config.useRange = cliRange;
//         }
//         if( owdmfSet )
//         {
//             config.useOWDMF = cliOWDMF;
//         }
//     }

//     // Validate
//     if( config.udlFiles.empty( ) )
//     {
//         std::cerr << "Warning: No UDL files specified. Using defaults." << std::endl;
//         config.udlFiles = {
//             "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_ke_cd_2025-10-11T164121.v19.json",
//             "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_yg_cd_2025-10-11T164121.v19.json",
//             "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_yg_ke_2025-10-11T164121.v19.json",
//         };
//     }

//     if( !config.useTDOA && !config.useFDOA && !config.useRange && !config.useOWDMF )
//     {
//         std::cerr << "Error: At least one observation type (TDOA, FDOA, Range, or OWDMF) must be enabled." << std::endl;
//         std::exit( 1 );
//     }

//     return config;
// }

// // =============================================================================
// // Time Generation
// // =============================================================================

// std::vector< Time > generateObservationTimes( const EstimationConfig& config,
//                                               std::shared_ptr< TerrestrialTimeScaleConverter > timeConverter )
// {
//     // Convert start and end date strings UTC to seconds since J2000 TDB
//     Time startTimeUTC = tba::timeFromIsoString< tudat::Time >( config.startDateString );
//     Time endTimeUTC = tba::timeFromIsoString< tudat::Time >( config.endDateString );

//     // Convert UTC to TDB using time scale converter
//     Eigen::Vector3d dummyPosition( 6378.0e3, 0.0, 0.0 );
//     Time startTimeTDB =
//             timeConverter->getCurrentTime< Time >( tba::TimeScales::utc_scale, tba::TimeScales::tdb_scale, startTimeUTC, dummyPosition );
//     Time endTimeTDB =
//             timeConverter->getCurrentTime< Time >( tba::TimeScales::utc_scale, tba::TimeScales::tdb_scale, endTimeUTC, dummyPosition );

//     std::cout << std::setprecision( std::numeric_limits< double >::digits10 + 2 );
//     std::cout << "Start Julian Day: " << julianDayFromTime< double >( startTimeTDB ) << " TDB" << std::endl;
//     std::cout << "End   Julian Day: " << julianDayFromTime< double >( endTimeTDB ) << " TDB" << std::endl;
//     std::string isoStart = horizons_interface::secondsToIsoDate( static_cast< double >( startTimeTDB ) );
//     std::string isoEnd = horizons_interface::secondsToIsoDate( static_cast< double >( endTimeTDB ) );
//     std::cout << "Start ISO Date: " << isoStart << " TDB" << std::endl;
//     std::cout << "End   ISO Date: " << isoEnd << " TDB" << std::endl;

//     int size = static_cast< int >( std::ceil( ( static_cast< double >( endTimeTDB ) - static_cast< double >( startTimeTDB ) ) /
//                                               config.observationTimestep ) ) +
//             1;
//     std::vector< Time > observationTimes( size );
//     for( int i = 0; i < size; ++i )
//     {
//         observationTimes[ i ] = startTimeTDB + i * config.observationTimestep;
//     }
//     return observationTimes;
// }

// // =============================================================================
// // Environment Setup Functions
// // =============================================================================

// SystemOfBodies createSimulationEnvironment( const EstimationConfig& config )
// {
//     std::cout << "Setting up simulation environment..." << std::endl;

//     // Load Spice kernels
//     spice_interface::loadStandardSpiceKernels( );

//     // Create bodies - include Sun for SRP, inner planets (Mercury, Venus, Mars), and outer planets (Jupiter, Saturn)
//     std::vector< std::string > bodiesToCreate = { "Sun", "Mercury", "Venus", "Earth", "Moon", "Mars", "Jupiter", "Saturn" };
//     BodyListSettings bodySettings = getDefaultBodySettings( bodiesToCreate, config.globalFrameOrigin, config.globalFrameOrientation );

//     // Configure Earth rotation model for ground station positions
//     bodySettings.get( "Earth" )->rotationModelSettings = gcrsToItrsRotationModelSettings(
//             tudat::basic_astrodynamics::iau_2006, config.globalFrameOrientation, nullptr, nullptr, nullptr );
//     bodySettings.get( "Earth" )->shapeModelSettings = simulation_setup::fromSpiceOblateSphericalBodyShapeSettings( );

//     // Configure spherical harmonics gravity for Earth
//     bodySettings.get( "Earth" )->gravityFieldSettings =
//     std::make_shared< FromFileSphericalHarmonicsGravityFieldSettings >( goco05c, config.earthSphericalHarmonicsDegree );
    
//     // Configure spherical harmonics gravity for Moon
//     bodySettings.get( "Moon" )->gravityFieldSettings =
//     std::make_shared< FromFileSphericalHarmonicsGravityFieldSettings >( gggrx1200, config.moonSphericalHarmonicsDegree );
    

//     std::dynamic_pointer_cast< SphericalHarmonicsGravityFieldSettings >( bodySettings.at( "Earth" )->gravityFieldSettings )
//             ->resetAssociatedReferenceFrame( "ITRS" );
//     // std::dynamic_pointer_cast< SphericalHarmonicsGravityFieldSettings >( bodySettings.at( "Moon" )->gravityFieldSettings )
//     //         ->resetAssociatedReferenceFrame( "IAU_Moon" );

//     // Note: Sun radiation source is automatically configured by getDefaultBodySettings()

//     std::cout << "  Bodies: ";
//     for( const auto& body : bodiesToCreate ) std::cout << body << " ";
//     std::cout << std::endl;
//     std::cout << "  Earth gravity: spherical harmonics (" << config.earthSphericalHarmonicsDegree << "x"
//               << config.earthSphericalHarmonicsOrder << ")" << std::endl;
//     std::cout << "  Moon gravity: spherical harmonics (" << config.moonSphericalHarmonicsDegree << "x" << config.moonSphericalHarmonicsOrder
//               << ")" << std::endl;
//     std::cout << "  Sun: radiation source enabled" << std::endl;

//     return createSystemOfBodies< double, double >( bodySettings );
// }

// std::shared_ptr< ObservationCollection< double, Time > > setupGroundStations( SystemOfBodies& bodies, BatchVLBI& udl )
// {
//     std::cout << "Creating ground stations from UDL data..." << std::endl;

//     // Convert UDL observations to Tudat format (this creates ground stations)
//     auto udlObservations = udl.toTudat( bodies, { }, "Earth" );

//     auto groundStations = bodies.at( "Earth" )->getGroundStationMap( );
//     std::cout << "Created " << groundStations.size( ) << " ground stations:" << std::endl;
//     for( const auto& station : groundStations )
//     {
//         std::cout << "  - " << station.first << std::endl;
//     }

//     return udlObservations;
// }

// void createSpacecraftBody( SystemOfBodies& bodies,
//                            const EstimationConfig& config,
//                            const std::map< double, Eigen::Vector6d >& ephemerisData )
// {
//     std::cout << "Creating spacecraft body: " << config.targetDisplayName << std::endl;

//     bodies.createEmptyBody( config.targetName );
//     bodies.at( config.targetName )->setConstantBodyMass( config.targetMass );

//     // Create tabulated ephemeris from data
//     auto tabulatedEphemerisSettings =
//             std::make_shared< TabulatedEphemerisSettings >( ephemerisData, config.globalFrameOrigin, config.globalFrameOrientation );
//     auto tabulatedEphemeris = createBodyEphemeris< double, double >( tabulatedEphemerisSettings, config.targetName );
//     // bodies.at( config.targetName )->setEphemeris( tabulatedEphemeris );

//     // Set transmitter frequency for FDOA/OWDMF (required for frequency-based observables)
//     if( config.useFDOA || config.useOWDMF )
//     {
//         bodies.getBody( config.targetName )
//                 ->getVehicleSystems( )
//                 ->setTransmittedFrequencyCalculator(
//                         std::make_shared< ground_stations::ConstantFrequencyInterpolator >( config.transmitterFrequency ) );
//         std::cout << "  Set transmitter frequency: " << config.transmitterFrequency / 1e9 << " GHz" << std::endl;
//     }

//     // Set radiation pressure interface for SRP
//     std::vector< std::string > occultingBodies = { "Earth", "Moon" };
//     auto radiationPressureSettings =
//             cannonballRadiationPressureTargetModelSettings( config.srpSurfaceArea, config.srpReflectivityCoefficient, occultingBodies );
//     bodies.at( config.targetName )
//             ->setRadiationPressureTargetModels(
//                     createRadiationPressureTargetModel( radiationPressureSettings, config.targetName, bodies ) );
//     std::cout << "  Set SRP: area=" << config.srpSurfaceArea << " m^2, Cr=" << config.srpReflectivityCoefficient << std::endl;

//     std::cout << "  Ephemeris points: " << ephemerisData.size( ) << std::endl;
// }

// // =============================================================================
// // Observation Model Setup
// // =============================================================================

// std::pair< std::vector< std::shared_ptr< ObservationModelSettings > >,
//            std::vector< std::shared_ptr< ObservationSimulationSettings< double > > > >
// createObservationSettings( const EstimationConfig& config,
//                            std::shared_ptr< ObservationCollection< double, Time > > udlObservations,
//                            const std::vector< double >& observationTimes )
// {
//     std::cout << "Setting up observation models..." << std::endl;

//     std::vector< std::shared_ptr< ObservationModelSettings > > modelSettings;
//     std::vector< std::shared_ptr< ObservationSimulationSettings< double > > > simSettings;

//     // Get link definitions from the UDL observations (already created during ground station setup)
//     auto linkDefsPerObservable = udlObservations->getLinkDefinitionsPerObservable( );

//     std::cout << "Available observation types from UDL:" << std::endl;
//     for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
//     {
//         std::cout << "  " << getObservableName( obsType ) << ": " << linkDefs.size( ) << " link(s)" << std::endl;
//     }

//     // Create TDOA observation settings
//     if( config.useTDOA )
//     {
//         for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
//         {
//             if( obsType != differenced_time_of_arrival ) continue;

//             for( const auto& linkDef : linkDefs )
//             {
//                 LinkEnds linkEnds = linkDef.linkEnds_;
//                 std::string station1 = linkEnds.at( receiver ).stationName_;
//                 std::string station2 = linkEnds.at( receiver2 ).stationName_;
//                 std::string transmitterBody = linkEnds.at( transmitter ).bodyName_;
//                 std::cout << "  Adding TDOA link: " << station1 << " - " << station2 << " (transmitter: " << transmitterBody << ")"
//                           << std::endl;

//                 auto tdoaModelSettings = std::make_shared< DifferencedTimeOfArrivalObservationSettings >(
//                         linkEnds, std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ) );
//                 modelSettings.push_back( tdoaModelSettings );

//                 auto tdoaSimSettings = std::make_shared< TabulatedObservationSimulationSettings< double > >(
//                         differenced_time_of_arrival, linkEnds, observationTimes, receiver );
//                 simSettings.push_back( tdoaSimSettings );
//             }
//         }
//     }

//     // Create FDOA observation settings
//     if( config.useFDOA )
//     {
//         for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
//         {
//             if( obsType != differenced_frequency_of_arrival ) continue;

//             for( const auto& linkDef : linkDefs )
//             {
//                 LinkEnds linkEnds = linkDef.linkEnds_;
//                 std::string station1 = linkEnds.at( receiver ).stationName_;
//                 std::string station2 = linkEnds.at( receiver2 ).stationName_;
//                 std::string transmitterBody = linkEnds.at( transmitter ).bodyName_;
//                 std::cout << "  Adding FDOA link: " << station1 << " - " << station2 << " (transmitter: " << transmitterBody << ")"
//                           << std::endl;
                
                
//                 auto fdoaModelSettings = differencedFrequencyOfArrivalObservationSettings( linkEnds );
//                 modelSettings.push_back( fdoaModelSettings );

//                 auto fdoaSimSettings = std::make_shared< TabulatedObservationSimulationSettings< double > >(
//                         differenced_frequency_of_arrival, linkEnds, observationTimes, receiver );
//                 simSettings.push_back( fdoaSimSettings );
//             }
//         }
//     }

//     // Create one-way range observation settings
//     // For range, we create 2-way links (transmitter + receiver) from each ground station to the target
//     if( config.useRange )
//     {
//         // Get unique ground stations from any available link definitions
//         std::set< std::string > groundStations;
//         for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
//         {
//             for( const auto& linkDef : linkDefs )
//             {
//                 LinkEnds linkEnds = linkDef.linkEnds_;
//                 if( linkEnds.count( receiver ) )
//                 {
//                     groundStations.insert( linkEnds.at( receiver ).stationName_ );
//                 }
//                 if( linkEnds.count( receiver2 ) )
//                 {
//                     groundStations.insert( linkEnds.at( receiver2 ).stationName_ );
//                 }
//             }
//         }

//         std::cout << "  Creating range links from " << groundStations.size( ) << " ground stations" << std::endl;

//         for( const auto& stationName : groundStations )
//         {
//             // Create 2-way link: spacecraft transmits, ground station receives (downlink)
//             LinkEnds rangeLinkEnds;
//             rangeLinkEnds[ transmitter ] = LinkEndId( config.targetName, "" );
//             rangeLinkEnds[ receiver ] = LinkEndId( "Earth", stationName );

//             std::cout << "  Adding Range link: " << config.targetName << " -> " << stationName << std::endl;

//             auto rangeModelSettings = std::make_shared< ObservationModelSettings >(
//                     one_way_range, rangeLinkEnds, std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ) );
//             modelSettings.push_back( rangeModelSettings );

//             auto rangeSimSettings = std::make_shared< TabulatedObservationSimulationSettings< double > >(
//                     one_way_range, rangeLinkEnds, observationTimes, receiver );
//             simSettings.push_back( rangeSimSettings );
//         }
//     }

//     // Create one-way Doppler measured frequency observation settings
//     // This creates one-way links from spacecraft to each ground station
//     if( config.useOWDMF )
//     {
//         // Get unique ground stations from any available link definitions
//         std::set< std::string > groundStations;
//         for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
//         {
//             for( const auto& linkDef : linkDefs )
//             {
//                 LinkEnds linkEnds = linkDef.linkEnds_;
//                 if( linkEnds.count( receiver ) )
//                 {
//                     groundStations.insert( linkEnds.at( receiver ).stationName_ );
//                 }
//                 if( linkEnds.count( receiver2 ) )
//                 {
//                     groundStations.insert( linkEnds.at( receiver2 ).stationName_ );
//                 }
//             }
//         }

//         std::cout << "  Creating OWDMF links from " << groundStations.size( ) << " ground stations" << std::endl;

//         for( const auto& stationName : groundStations )
//         {
//             // Create one-way link: spacecraft transmits, ground station receives (downlink)
//             LinkEnds owdmfLinkEnds;
//             owdmfLinkEnds[ transmitter ] = LinkEndId( config.targetName, "" );
//             owdmfLinkEnds[ receiver ] = LinkEndId( "Earth", stationName );

//             std::cout << "  Adding OWDMF link: " << config.targetName << " -> " << stationName << std::endl;

//             // Use the one-way Doppler measured frequency observation model
//             auto owdmfModelSettings = oneWayDopplerMeasuredFrequencySettings(
//                     owdmfLinkEnds,
//                     std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ) );
//             modelSettings.push_back( owdmfModelSettings );

//             auto owdmfSimSettings = std::make_shared< TabulatedObservationSimulationSettings< double > >(
//                     one_way_doppler_measured_frequency, owdmfLinkEnds, observationTimes, receiver );
//             simSettings.push_back( owdmfSimSettings );
//         }
//     }

//     std::cout << "Created " << modelSettings.size( ) << " observation model settings" << std::endl;
//     return { modelSettings, simSettings };
// }

// // =============================================================================
// // Observation Retrieval
// // =============================================================================

// std::shared_ptr< ObservationCollection< double, double > > getObservations(
//         const EstimationConfig& config,
//         BatchVLBI& udl,
//         SystemOfBodies& bodies,
//         const std::vector< std::shared_ptr< ObservationModelSettings > >& modelSettings,
//         const std::vector< std::shared_ptr< ObservationSimulationSettings< double > > >& simSettings )
// {
//     if( config.simulateObservations )
//     {
//         std::cout << "\n=== Simulating observations ===" << std::endl;
//         auto observationSimulators = createObservationSimulators( modelSettings, bodies );
//         auto observations = simulateObservations( simSettings, observationSimulators, bodies );
//         std::cout << "Simulated " << observations->getTotalObservableSize( ) << " observations" << std::endl;
//         return observations;
//     }
//     else
//     {
//         std::cout << "\n=== Loading actual observations from UDL ===" << std::endl;
//         // UDL returns ObservationCollection<double, Time>, but we need <double, double>
//         // For now, only simulated observations are supported
//         throw std::runtime_error( "Actual observations from UDL not yet supported - use --simulate flag" );
//     }
// }

// int main( int argc, char** argv )
// {
//     std::cout << "=== Generalized Spacecraft State Estimation ===" << std::endl << std::endl;

//     // =========================================================================
//     // Parse configuration
//     // =========================================================================

//     EstimationConfig config = parseCommandLineArgs( argc, argv );
//     config.print( );

//     // =========================================================================
//     // Load UDL data
//     // =========================================================================

//     std::cout << "Loading UDL data..." << std::endl;
//     auto UDL = io::BatchVLBI( config.udlFiles );

//     // Update target name from UDL if available
//     auto targets = UDL.getCollection( ).getObservedTargets( );
//     if( !targets.empty( ) )
//     {
//         config.targetName = *targets.begin( );
//         std::cout << "Target from UDL: " << config.targetName << " (" << config.targetDisplayName << ")" << std::endl;
//     }

//     // =========================================================================
//     // Generate observation times
//     // =========================================================================

//     std::vector< Time > observationTimesTime = generateObservationTimes( config );
//     std::vector< double > observationTimesDouble( observationTimesTime.size( ) );
//     std::transform( observationTimesTime.begin( ), observationTimesTime.end( ), observationTimesDouble.begin( ), []( const Time& t ) {
//         return static_cast< double >( t );
//     } );

//     double startEpoch = *std::min_element( observationTimesDouble.begin( ), observationTimesDouble.end( ) );
//     double endEpoch = *std::max_element( observationTimesDouble.begin( ), observationTimesDouble.end( ) );
//     double startEpochPadded = startEpoch - config.ephemerisPadding;
//     double endEpochPadded = endEpoch + config.ephemerisPadding;

//     std::cout << "\nObservation time configuration:" << std::endl;
//     std::cout << "  Duration: " << ( endEpoch - startEpoch ) / 60.0 << " minutes" << std::endl;
//     std::cout << "  Epochs: " << observationTimesTime.size( ) << std::endl;

//     // =========================================================================
//     // Query Horizons for true ephemeris
//     // =========================================================================

//     std::cout << "\nQuerying JPL Horizons for " << config.targetDisplayName << " ephemeris..." << std::endl;

//     // Map global frame origin to Horizons center code
//     std::string horizonsCenter = "@SSB";  // default: Solar System Barycenter
//     if( config.globalFrameOrigin == "SSB" || config.globalFrameOrigin == "Solar System Barycenter" )
//     {
//         horizonsCenter = "@SSB";
//     }
//     else if( config.globalFrameOrigin == "Sun" )
//     {
//         horizonsCenter = "@sun";
//     }
//     else if( config.globalFrameOrigin == "Earth" )
//     {
//         horizonsCenter = "@399";  // Earth center
//     }
//     else if( config.globalFrameOrigin == "Moon" )
//     {
//         horizonsCenter = "@301";  // Moon center
//     }
//     else if( config.globalFrameOrigin == "Mars" )
//     {
//         horizonsCenter = "@499";  // Mars center
//     }
//     else
//     {
//         std::cout << "  Warning: Unknown frame origin '" << config.globalFrameOrigin << "', using SSB for Horizons query" << std::endl;
//     }
//     std::cout << "  Horizons center: " << horizonsCenter << " (frame origin: " << config.globalFrameOrigin << ")" << std::endl;

//     auto horizonsStateHistory =
//             HorizonsQuery( config.targetDisplayName, horizonsCenter, startEpochPadded - 300, endEpochPadded + 300, config.horizonsStepSize )
//                     .getCartesianStateHistory( config.globalFrameOrientation );
//     std::cout << "Retrieved " << horizonsStateHistory.size( ) << " ephemeris points" << std::endl;

//     // =========================================================================
//     // Set up simulation environment
//     // =========================================================================

//     SystemOfBodies simulationBodies = createSimulationEnvironment( config );
//     auto simulationUdlObservations = setupGroundStations( simulationBodies, UDL );
//     createSpacecraftBody( simulationBodies, config, horizonsStateHistory );

//     // =========================================================================
//     // Extract true initial state
//     // =========================================================================
//     // Create tabulated ephemeris from data
//     auto tabulatedEphemerisSettings =
//             std::make_shared< TabulatedEphemerisSettings >( horizonsStateHistory, config.globalFrameOrigin, config.globalFrameOrientation );
//     auto tabulatedEphemeris = createBodyEphemeris< double, double >( tabulatedEphemerisSettings, config.targetName );

//     // Get initial state at padded epoch for propagation (needed for light-time calculations)
//     Eigen::Vector6d trueInitialStatePadded = tabulatedEphemeris->getCartesianState( startEpochPadded );
//     // Also get initial state at observation start epoch for comparison
//     Eigen::Vector6d trueInitialState = tabulatedEphemeris->getCartesianState( startEpoch );

//     std::cout << "\nTRUE initial state from Horizons (at observation start):" << std::endl;
//     std::cout << "  Epoch: " << horizons_interface::secondsToIsoDate( startEpoch ) << " TDB" << std::endl;
//     std::cout << "  Position (km): [" << trueInitialState.head< 3 >( ).transpose( ) / 1e3 << "]" << std::endl;
//     std::cout << "  Velocity (km/s): [" << trueInitialState.tail< 3 >( ).transpose( ) / 1e3 << "]" << std::endl;

//     // =========================================================================
//     // Verify dynamical model against Horizons ephemeris
//     // =========================================================================

//     std::cout << "\n=== Verifying dynamical model against Horizons ephemeris ===" << std::endl;

//     // Set up acceleration model for verification (same as estimation)
//     SelectedAccelerationMap verifyAccelerationSettings;
//     // Spherical harmonics for Earth and Moon
//     verifyAccelerationSettings[ config.targetName ][ "Earth" ].push_back( std::make_shared< SphericalHarmonicAccelerationSettings >(
//             config.earthSphericalHarmonicsDegree, config.earthSphericalHarmonicsOrder ) );
//     verifyAccelerationSettings[ config.targetName ][ "Moon" ].push_back( std::make_shared< SphericalHarmonicAccelerationSettings >(
//             config.moonSphericalHarmonicsDegree, config.moonSphericalHarmonicsOrder ) );

//     verifyAccelerationSettings[ config.targetName ][ "Sun" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     verifyAccelerationSettings[ config.targetName ][ "Mercury" ].push_back(
//             std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     verifyAccelerationSettings[ config.targetName ][ "Venus" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     verifyAccelerationSettings[ config.targetName ][ "Mars" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     verifyAccelerationSettings[ config.targetName ][ "Jupiter" ].push_back(
//             std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     verifyAccelerationSettings[ config.targetName ][ "Saturn" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );

//     verifyAccelerationSettings[ config.targetName ][ "Sun" ].push_back( radiationPressureAcceleration( ) );

//     verifyAccelerationSettings[ config.targetName ][ "Sun" ].push_back(
//             std::make_shared< RelativisticAccelerationCorrectionSettings >( true, false, false ) );
//     verifyAccelerationSettings[ config.targetName ][ "Earth" ].push_back(
//             std::make_shared< RelativisticAccelerationCorrectionSettings >( true, false, false ) );
//     verifyAccelerationSettings[ config.targetName ][ "Moon" ].push_back(
//             std::make_shared< RelativisticAccelerationCorrectionSettings >( true, false, false ) );

//     AccelerationMap verifyAccelerationModelMap = createAccelerationModelsMap(
//             simulationBodies, verifyAccelerationSettings, { config.targetName }, { config.globalFrameOrigin } );

//     std::shared_ptr< IntegratorSettings< double > > verifyIntegratorSettings =
//             std::make_shared< RungeKuttaFixedStepSizeSettings< double > >( config.integratorStepSize,
//                                                                            CoefficientSets::rungeKuttaFehlberg78 );

//     // Propagate from padded start to padded end to cover light-time calculations
//     std::shared_ptr< TranslationalStatePropagatorSettings< double > > verifyPropagatorSettings =
//             std::make_shared< TranslationalStatePropagatorSettings< double > >(
//                     std::vector< std::string >{ config.globalFrameOrigin },
//                     verifyAccelerationModelMap,
//                     std::vector< std::string >{ config.targetName },
//                     trueInitialStatePadded,  // Use state at padded epoch
//                     startEpochPadded,
//                     verifyIntegratorSettings,
//                     std::make_shared< PropagationTimeTerminationSettings >( endEpochPadded, true ) );

//     // Run propagation
//     SingleArcDynamicsSimulator< double > verifySimulator( simulationBodies, verifyPropagatorSettings );

//     std::map< double, Eigen::VectorXd > propagatedStates = verifySimulator.getEquationsOfMotionNumericalSolution( );

//     // Compare propagated states to Horizons ephemeris
//     double sumSquaredPosError = 0.0;
//     double sumSquaredVelError = 0.0;
//     double maxPosError = 0.0;
//     double maxVelError = 0.0;
//     int comparisonCount = 0;

//     for( const auto& [ epoch, state ] : propagatedStates )
//     {
//         auto trueState = tabulatedEphemeris->getCartesianState( epoch );

//         Eigen::Vector3d posError = state.head< 3 >( ) - trueState.head< 3 >( );
//         Eigen::Vector3d velError = state.segment< 3 >( 3 ) - trueState.tail< 3 >( );

//         double posErrorNorm = posError.norm( );
//         double velErrorNorm = velError.norm( );

//         sumSquaredPosError += posErrorNorm * posErrorNorm;
//         sumSquaredVelError += velErrorNorm * velErrorNorm;
//         maxPosError = std::max( maxPosError, posErrorNorm );
//         maxVelError = std::max( maxVelError, velErrorNorm );
//         comparisonCount++;
//     }

//     double rmsPositionError = std::sqrt( sumSquaredPosError / comparisonCount );
//     double rmsVelocityError = std::sqrt( sumSquaredVelError / comparisonCount );

//     std::cout << "\nDynamical model verification results:" << std::endl;
//     std::cout << "  Comparison points: " << comparisonCount << std::endl;
//     std::cout << "  RMS position error: " << rmsPositionError << " m" << std::endl;
//     std::cout << "  RMS velocity error: " << rmsVelocityError << " m/s" << std::endl;
//     std::cout << "  Max position error: " << maxPosError << " m" << std::endl;
//     std::cout << "  Max velocity error: " << maxVelError << " m/s" << std::endl;

//     if( rmsPositionError > 1000.0 )
//     {
//         std::cout << "\n  WARNING: Large position error! Dynamical model may not match Horizons ephemeris." << std::endl;
//         std::cout << "           Consider checking: gravity model, SRP parameters, reference frame." << std::endl;
//     }
//     else if( rmsPositionError < 100.0 )
//     {
//         std::cout << "\n  Dynamical model matches Horizons ephemeris well." << std::endl;
//     }

//     // =========================================================================
//     // Create observation settings and get observations
//     // =========================================================================

//     auto [ modelSettings, simSettings ] = createObservationSettings( config, simulationUdlObservations, observationTimesDouble );

//     // Use propagated trajectory for observation simulation (not Horizons) to ensure consistency
//     // This way, zero perturbation should give zero residual
//     std::map< double, Eigen::Vector6d > propagatedStatesMap;
//     for( const auto& [ epoch, state ] : propagatedStates )
//     {
//         propagatedStatesMap[ epoch ] = state;
//     }

//     auto propagatedEphemerisSettings =
//             std::make_shared< TabulatedEphemerisSettings >( propagatedStatesMap, config.globalFrameOrigin, config.globalFrameOrientation );
//     auto propagatedEphemeris = createBodyEphemeris< double, double >( propagatedEphemerisSettings, config.targetName );
//     simulationBodies.getBody( config.targetName )->setEphemeris( propagatedEphemeris );

//     auto observations = getObservations( config, UDL, simulationBodies, modelSettings, simSettings );

//     // =========================================================================
//     // Set up parameter estimation
//     // =========================================================================

//     std::cout << "\n=== Setting up state estimation ===" << std::endl;

//     SystemOfBodies estimationBodies = createSimulationEnvironment( config );
//     auto udlObservations = setupGroundStations( estimationBodies, UDL );
//     createSpacecraftBody( estimationBodies, config, horizonsStateHistory );

//     Eigen::Vector6d statePerturbation;
//     statePerturbation << config.positionPerturbation, config.velocityPerturbation;
//     Eigen::Vector6d perturbedInitialState = trueInitialState + statePerturbation;

//     std::cout << "Perturbation applied:" << std::endl;
//     std::cout << "  Position (m): [" << config.positionPerturbation.transpose( ) << "]" << std::endl;
//     std::cout << "  Velocity (m/s): [" << config.velocityPerturbation.transpose( ) << "]" << std::endl;

//     // Define acceleration models
//     SelectedAccelerationMap accelerationSettings;

//     // Spherical harmonics for Earth and Moon
//     accelerationSettings[ config.targetName ][ "Earth" ].push_back( std::make_shared< SphericalHarmonicAccelerationSettings >(
//             config.earthSphericalHarmonicsDegree, config.earthSphericalHarmonicsOrder ) );
//     accelerationSettings[ config.targetName ][ "Moon" ].push_back( std::make_shared< SphericalHarmonicAccelerationSettings >(
//             config.moonSphericalHarmonicsDegree, config.moonSphericalHarmonicsOrder ) );

//     // Point mass gravity from Sun and planets
//     accelerationSettings[ config.targetName ][ "Sun" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     accelerationSettings[ config.targetName ][ "Mercury" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     accelerationSettings[ config.targetName ][ "Venus" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     accelerationSettings[ config.targetName ][ "Mars" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     accelerationSettings[ config.targetName ][ "Jupiter" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
//     accelerationSettings[ config.targetName ][ "Saturn" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );

//     // Solar radiation pressure from Sun
//     accelerationSettings[ config.targetName ][ "Sun" ].push_back( radiationPressureAcceleration( ) );

//     // Relativistic corrections (Schwarzschild) from Sun, Earth, and Moon
//     accelerationSettings[ config.targetName ][ "Sun" ].push_back(
//             std::make_shared< RelativisticAccelerationCorrectionSettings >( true, false, false ) );
//     accelerationSettings[ config.targetName ][ "Earth" ].push_back(
//             std::make_shared< RelativisticAccelerationCorrectionSettings >( true, false, false ) );
//     accelerationSettings[ config.targetName ][ "Moon" ].push_back(
//             std::make_shared< RelativisticAccelerationCorrectionSettings >( true, false, false ) );

//     std::cout << "Acceleration model:" << std::endl;
//     std::cout << "  Earth: Point mass + Schwarzschild" << std::endl;
//     std::cout << "  Moon: spherical harmonics (" << config.moonSphericalHarmonicsDegree << "x" << config.moonSphericalHarmonicsOrder
//               << ") + Schwarzschild" << std::endl;
//     std::cout << "  Point masses: Sun, Mercury, Venus, Mars, Jupiter, Saturn" << std::endl;
//     std::cout << "  SRP from Sun" << std::endl;
//     std::cout << "  Relativistic (Schwarzschild): Sun, Earth, Moon" << std::endl;

//     AccelerationMap accelerationModelMap =
//             createAccelerationModelsMap( estimationBodies, accelerationSettings, { config.targetName }, { config.globalFrameOrigin } );

//     std::shared_ptr< IntegratorSettings< double > > integratorSettings = std::make_shared< RungeKuttaFixedStepSizeSettings< double > >(
//             config.integratorStepSize, CoefficientSets::rungeKuttaFehlberg78 );

//     // Use padded epochs to ensure ephemeris covers light-time iterations
//     // (observation models need to look back in time from observation epoch)
//     Eigen::Vector6d perturbedInitialStatePadded = tabulatedEphemeris->getCartesianState( startEpochPadded ) + statePerturbation;

//     std::shared_ptr< TranslationalStatePropagatorSettings< double > > propagatorSettings =
//             std::make_shared< TranslationalStatePropagatorSettings< double > >(
//                     std::vector< std::string >{ config.globalFrameOrigin },
//                     accelerationModelMap,
//                     std::vector< std::string >{ config.targetName },
//                     perturbedInitialStatePadded,
//                     startEpochPadded,
//                     integratorSettings,
//                     std::make_shared< PropagationTimeTerminationSettings >( endEpochPadded, true ) );

//     std::vector< std::shared_ptr< EstimatableParameterSettings > > parameterSettings =
//             getInitialStateParameterSettings< double, double >( propagatorSettings, estimationBodies );

//     std::shared_ptr< EstimatableParameterSet< double > > parametersToEstimate =
//             createParametersToEstimate< double >( parameterSettings, estimationBodies );

//     std::cout << "Parameters: " << parametersToEstimate->getEstimatedParameterSetSize( ) << std::endl;

//     // =========================================================================
//     // Propagate perturbed trajectory (for comparison)
//     // =========================================================================

//     std::cout << "\n=== Propagating perturbed trajectory ===" << std::endl;

//     SingleArcDynamicsSimulator< double > perturbedSimulator( estimationBodies, propagatorSettings );
//     auto perturbedResults = perturbedSimulator.getSingleArcPropagationResults( );
//     std::cout << "Perturbed trajectory: " << perturbedResults->getEquationsOfMotionNumericalSolution( ).size( ) << " points" << std::endl;

//     // =========================================================================
//     // Run estimation
//     // =========================================================================

//     std::cout << "\n=== Running state estimation ===" << std::endl;

//     OrbitDeterminationManager< double, double > orbitDeterminationManager(
//             estimationBodies, parametersToEstimate, modelSettings, propagatorSettings );

//     std::shared_ptr< EstimationInput< double, double > > estimationInput = std::make_shared< EstimationInput< double, double > >(
//             observations,
//             Eigen::MatrixXd::Zero( 0, 0 ),
//             std::make_shared< EstimationConvergenceChecker >(
//                     config.maxIterations, config.convergenceThreshold, 0.0, config.minIterations ) );

//     estimationInput->defineEstimationSettings( true, true, true, true, true, false );

//     std::shared_ptr< EstimationOutput< double, double > > estimationOutput =
//             orbitDeterminationManager.estimateParameters( estimationInput );

//     // =========================================================================
//     // Print results
//     // =========================================================================

//     std::cout << "\n" << std::string( 60, '=' ) << std::endl;
//     std::cout << "=== ESTIMATION RESULTS ===" << std::endl;
//     std::cout << std::string( 60, '=' ) << std::endl;

//     Eigen::Vector6d estimatedState = estimationOutput->parameterEstimate_.head< 6 >( );
//     Eigen::VectorXd formalErrors = estimationOutput->getFormalErrorVector( );

//     // Compare to true state at the padded epoch (same epoch as estimated state)
//     Eigen::Vector3d positionError = estimatedState.head< 3 >( ) - trueInitialStatePadded.head< 3 >( );
//     Eigen::Vector3d velocityError = estimatedState.tail< 3 >( ) - trueInitialStatePadded.tail< 3 >( );

//     std::cout << std::setprecision( 6 );
//     std::cout << "\nTRUE state:      Pos (km): [" << trueInitialStatePadded.head< 3 >( ).transpose( ) / 1e3 << "]" << std::endl;
//     std::cout << "                 Vel (km/s): [" << trueInitialStatePadded.tail< 3 >( ).transpose( ) / 1e3 << "]" << std::endl;
//     std::cout << "\nEstimated state: Pos (km): [" << estimatedState.head< 3 >( ).transpose( ) / 1e3 << "]" << std::endl;
//     std::cout << "                 Vel (km/s): [" << estimatedState.tail< 3 >( ).transpose( ) / 1e3 << "]" << std::endl;
//     std::cout << "\nPosition error: " << positionError.norm( ) << " m" << std::endl;
//     std::cout << "Velocity error: " << velocityError.norm( ) << " m/s" << std::endl;

//     double positionRecovery = ( 1.0 - positionError.norm( ) / statePerturbation.head< 3 >( ).norm( ) ) * 100.0;
//     double velocityRecovery = ( 1.0 - velocityError.norm( ) / statePerturbation.tail< 3 >( ).norm( ) ) * 100.0;

//     std::cout << "\nRecovery: Position " << std::setprecision( 2 ) << positionRecovery << "%, Velocity " << velocityRecovery << "%"
//               << std::endl;

//     Eigen::VectorXd finalResiduals = estimationOutput->residuals_;
//     double rmsResidual = std::sqrt( finalResiduals.squaredNorm( ) / finalResiduals.size( ) );
//     std::cout << "RMS residual: " << std::setprecision( 12 ) << rmsResidual << " s" << std::endl;

//     // =========================================================================
//     // Propagate corrected trajectory (with estimated state)
//     // =========================================================================

//     std::cout << "\n=== Propagating corrected trajectory ===" << std::endl;

//     // Create new propagator settings with estimated initial state
//     // Note: estimated state is at startEpochPadded, propagate to endEpochPadded
//     std::shared_ptr< TranslationalStatePropagatorSettings< double > > correctedPropagatorSettings =
//             std::make_shared< TranslationalStatePropagatorSettings< double > >(
//                     std::vector< std::string >{ config.globalFrameOrigin },
//                     accelerationModelMap,
//                     std::vector< std::string >{ config.targetName },
//                     estimatedState,
//                     startEpochPadded,
//                     integratorSettings,
//                     std::make_shared< PropagationTimeTerminationSettings >( endEpochPadded ) );

//     SingleArcDynamicsSimulator< double > correctedSimulator( estimationBodies, correctedPropagatorSettings );
//     auto correctedResults = correctedSimulator.getSingleArcPropagationResults( );
//     std::cout << "Corrected trajectory: " << correctedResults->getEquationsOfMotionNumericalSolution( ).size( ) << " points" << std::endl;

//     // =========================================================================
//     // Save to HDF5
//     // =========================================================================

//     std::cout << "\n=== Saving results to HDF5 ===" << std::endl;

//     HDF5OutputFile hdf5File( config.outputPath, true );
//     HighFive::File& file = hdf5File.getFile( );

//     // Save observations
//     hdf5File.addObservationCollection( observations, "observations" );

//     // =========================================================================
//     // Save trajectories
//     // =========================================================================

//     std::cout << "Saving trajectories..." << std::endl;

//     // 1. Save Horizons reference trajectory (raw map, not from simulator)
//     {
//         HighFive::Group trajGroup = file.exist( "/Trajectories" ) ? file.getGroup( "/Trajectories" ) : file.createGroup( "/Trajectories" );

//         HighFive::Group horizonsGroup = trajGroup.createGroup( "Horizons" );

//         std::vector< double > times;
//         std::vector< std::vector< double > > states;
//         times.reserve( horizonsStateHistory.size( ) );
//         states.reserve( horizonsStateHistory.size( ) );

//         for( const auto& [ t, state ] : horizonsStateHistory )
//         {
//             times.push_back( t );
//             states.push_back( { state( 0 ), state( 1 ), state( 2 ), state( 3 ), state( 4 ), state( 5 ) } );
//         }

//         horizonsGroup.createDataSet< double >( "times", HighFive::DataSpace( { times.size( ) } ) ).write( times );
//         horizonsGroup.createDataSet< double >( "states", HighFive::DataSpace( { states.size( ), 6 } ) ).write( states );

//         // Add trajectory config for XDMF generation
//         TrajectoryConfig horizonsConfig;
//         horizonsConfig.bodyName = "Horizons";
//         horizonsConfig.h5FilePath = config.outputPath;
//         horizonsConfig.statesDataset = "/Trajectories/Horizons/states";
//         horizonsConfig.timesDataset = "/Trajectories/Horizons/times";
//         horizonsConfig.numTimeSteps = times.size( );
//         horizonsConfig.stateSize = 6;
//         hdf5File.addTrajectoryConfig( horizonsConfig );

//         std::cout << "  Horizons: " << times.size( ) << " points" << std::endl;
//     }

//     // 2. Save unperturbed trajectory (from verification propagation)
//     hdf5File.addSingleArcResults( verifySimulator.getSingleArcPropagationResults( ), "Unperturbed", "/Trajectories" );
//     std::cout << "  Unperturbed: " << verifySimulator.getSingleArcPropagationResults( )->getEquationsOfMotionNumericalSolution( ).size( )
//               << " points" << std::endl;

//     // 3. Save perturbed trajectory
//     hdf5File.addSingleArcResults( perturbedResults, "Perturbed", "/Trajectories" );
//     std::cout << "  Perturbed: " << perturbedResults->getEquationsOfMotionNumericalSolution( ).size( ) << " points" << std::endl;

//     // 4. Save corrected trajectory (with estimated state)
//     hdf5File.addSingleArcResults( correctedResults, "Corrected", "/Trajectories" );
//     std::cout << "  Corrected: " << correctedResults->getEquationsOfMotionNumericalSolution( ).size( ) << " points" << std::endl;

//     // Generate XDMF for trajectory visualization
//     if( config.generateXDMF )
//     {
//         hdf5File.generateXDMF( );
//         std::cout << "Generated trajectory XDMF file" << std::endl;
//     }

//     // =========================================================================
//     // Save estimation results as custom datasets
//     // =========================================================================

//     // Create estimation results group
//     if( !file.exist( "/EstimationResults" ) )
//     {
//         file.createGroup( "/EstimationResults" );
//     }
//     HighFive::Group estGroup = file.getGroup( "/EstimationResults" );

//     // Save vectors as datasets
//     std::vector< double > trueStateVec( trueInitialStatePadded.data( ), trueInitialStatePadded.data( ) + 6 );
//     std::vector< double > estStateVec( estimatedState.data( ), estimatedState.data( ) + 6 );
//     std::vector< double > perturbedStateVec( perturbedInitialStatePadded.data( ), perturbedInitialStatePadded.data( ) + 6 );
//     std::vector< double > formalErrorsVec( formalErrors.data( ), formalErrors.data( ) + formalErrors.size( ) );
//     std::vector< double > residualsVec( finalResiduals.data( ), finalResiduals.data( ) + finalResiduals.size( ) );

//     estGroup.createDataSet( "true_initial_state", trueStateVec );
//     estGroup.createDataSet( "estimated_state", estStateVec );
//     estGroup.createDataSet( "perturbed_initial_state", perturbedStateVec );
//     estGroup.createDataSet( "formal_errors", formalErrorsVec );
//     estGroup.createDataSet( "residuals", residualsVec );

//     // Save scalar results as attributes
//     estGroup.createAttribute( "true_initial_epoch", startEpochPadded );
//     estGroup.createAttribute( "position_error_m", positionError.norm( ) );
//     estGroup.createAttribute( "velocity_error_ms", velocityError.norm( ) );
//     estGroup.createAttribute( "position_recovery_percent", positionRecovery );
//     estGroup.createAttribute( "velocity_recovery_percent", velocityRecovery );
//     estGroup.createAttribute( "rms_residual", rmsResidual );
//     estGroup.createAttribute( "use_tdoa", config.useTDOA );
//     estGroup.createAttribute( "use_fdoa", config.useFDOA );
//     estGroup.createAttribute( "simulated_observations", config.simulateObservations );

//     if( config.generateXDMF )
//     {
//         hdf5File.generateObservationXDMF( );
//     }

//     std::cout << "Results saved to: " << config.outputPath << std::endl;

//     // =========================================================================
//     // Save estimation output to CSV
//     // =========================================================================

//     // Derive CSV path from HDF5 output path
//     std::string csvPath = config.outputPath;
//     size_t dotPos = csvPath.rfind( '.' );
//     if( dotPos != std::string::npos )
//     {
//         csvPath = csvPath.substr( 0, dotPos ) + "_estimation.csv";
//     }
//     else
//     {
//         csvPath += "_estimation.csv";
//     }

//     std::cout << "Saving estimation output to CSV: " << csvPath << std::endl;

//     std::ofstream csvFile( csvPath );
//     csvFile << std::setprecision( 16 );

//     // Write header info as comments
//     csvFile << "# Estimation Output\n";
//     csvFile << "# True initial epoch: " << startEpoch << "\n";
//     csvFile << "# Number of iterations: " << estimationOutput->residualHistory_.size( ) << "\n";
//     csvFile << "# Best iteration: " << estimationOutput->bestIteration_ << "\n";
//     csvFile << "# Final RMS residual: " << rmsResidual << "\n";
//     csvFile << "#\n";

//     // Write parameter history
//     csvFile << "# Parameter History (iteration, x, y, z, vx, vy, vz)\n";
//     csvFile << "iteration,x,y,z,vx,vy,vz\n";
//     for( size_t i = 0; i < estimationOutput->parameterHistory_.size( ); ++i )
//     {
//         const auto& params = estimationOutput->parameterHistory_[ i ];
//         csvFile << i;
//         for( int j = 0; j < std::min( static_cast< int >( params.size( ) ), 6 ); ++j )
//         {
//             csvFile << "," << params( j );
//         }
//         csvFile << "\n";
//     }
//     csvFile << "\n";

//     // Write residual history (RMS per iteration)
//     csvFile << "# Residual History (full residual vectors per iteration)\n";
//     csvFile << "# Each row: iteration, residual_0, residual_1, ..., residual_n\n";

//     // First write the number of residuals per iteration
//     if( !estimationOutput->residualHistory_.empty( ) )
//     {
//         csvFile << "# Residuals per iteration: " << estimationOutput->residualHistory_[ 0 ].size( ) << "\n";
//     }

//     // Write residual data
//     for( size_t i = 0; i < estimationOutput->residualHistory_.size( ); ++i )
//     {
//         const auto& residuals = estimationOutput->residualHistory_[ i ];
//         csvFile << i;
//         for( int j = 0; j < residuals.size( ); ++j )
//         {
//             csvFile << "," << residuals( j );
//         }
//         csvFile << "\n";
//     }
//     csvFile << "\n";

//     // Write RMS residual per iteration
//     csvFile << "# RMS Residual per Iteration\n";
//     csvFile << "iteration,rms_residual\n";
//     for( size_t i = 0; i < estimationOutput->residualHistory_.size( ); ++i )
//     {
//         const auto& residuals = estimationOutput->residualHistory_[ i ];
//         double rms = std::sqrt( residuals.squaredNorm( ) / residuals.size( ) );
//         csvFile << i << "," << rms << "\n";
//     }

//     csvFile.close( );
//     std::cout << "Estimation CSV saved to: " << csvPath << std::endl;

//     // =========================================================================
//     // Summary
//     // =========================================================================

//     std::cout << "\n=== Estimation Summary ===" << std::endl;
//     if( positionRecovery > 99.0 && velocityRecovery > 99.0 )
//         std::cout << "SUCCESS: State recovered with >99% accuracy!" << std::endl;
//     else if( positionRecovery > 90.0 && velocityRecovery > 90.0 )
//         std::cout << "GOOD: State recovered with >90% accuracy" << std::endl;
//     else
//         std::cout << "PARTIAL: State partially recovered" << std::endl;

//     std::cout << "\n=== Estimation Complete ===" << std::endl;

//     return 0;
// }

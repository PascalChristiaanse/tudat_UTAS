/*    Copyright (c) 2010-2024, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>

#include "tudat/interface/horizons.h"
#include "tudat/astro/basic_astro.h"
#include "tudat/simulation/simulation.h"
#include "tudat/simulation/estimation_setup/createObservationModel.h"
#include "tudat/simulation/estimation_setup/observationSimulationSettings.h"
#include "tudat/simulation/estimation_setup/simulateObservations.h"
#include "tudat/io.h"
#include "tudat/io/hdf5Manager.h"

using namespace tudat;
using namespace tudat::horizons_interface;
using namespace tudat::basic_astrodynamics;
using namespace tudat::simulation_setup;
using namespace tudat::propagators;
using namespace tudat::numerical_integrators;
using namespace tudat::observation_models;
using namespace tudat::io;

int main( )
{
    std::cout << "=== targetSpacecraft TDOA/FDOA Observation Simulation ===" << std::endl;

    // =========================================================================
    // Load UDL observations to get ground station definitions
    // =========================================================================
    std::vector< std::string > udlFiles = {
    "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_ke_cd_2025-10-11T164116.v6.json",
    "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_yg_cd_2025-10-11T164116.v6.json",
    "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_yg_ke_2025-10-11T164116.v6.json",
    };
    auto UDL = io::BatchVLBI( udlFiles );

    // =========================================================================
    // Grab targetSpacecraft spacecraft ephemeris from JPL Horizons
    // =========================================================================

    // Get time range from UDL observation data (find min/max across all observation sets)
    double minEpoch = std::numeric_limits< double >::max( );
    double maxEpoch = std::numeric_limits< double >::lowest( );

    const auto& allObs = UDL.getCollection( ).getAllObservations( );
    for( const auto& [ targetId, stationPairMap ] : allObs )
    {
        for( const auto& [ stationPair, obsSets ] : stationPairMap )
        {
            for( const auto& obsSet : obsSets )
            {
                const auto& timeSeries = obsSet->getTimeSeries( );
                if( !timeSeries.epochs.empty( ) )
                {
                    minEpoch = std::min( minEpoch, timeSeries.epochs.front( ) );
                    maxEpoch = std::max( maxEpoch, timeSeries.epochs.back( ) );
                }
            }
        }
    }

    double startEpoch = minEpoch - 600.0;  // Start time from UDL data - 10 min buffer
    double endEpoch = maxEpoch + 600.0;    // End time from UDL data + 10 min buffer
    std::string stepSize = "1m";           // 1 minute intervals for ephemeris

    std::cout << "Fetching targetSpacecraft ephemeris from JPL Horizons..." << std::endl;
    HorizonsQuery targetSpacecraftEph( "CAPSTONE", "500", startEpoch - 600, endEpoch + 600, stepSize );
    StateHistory targetSpacecraftStates = targetSpacecraftEph.getCartesianStates( FrameOrientation::J2000 );
    Vector6d targetSpacecraftInitialState = targetSpacecraftStates.begin( )->second;

    std::cout << "targetSpacecraft initial state (ECI):" << std::endl;
    std::cout << "  Position (km): [" << targetSpacecraftInitialState( 0 ) / 1e3 << ", " << targetSpacecraftInitialState( 1 ) / 1e3 << ", "
              << targetSpacecraftInitialState( 2 ) / 1e3 << "]" << std::endl;
    std::cout << "  Velocity (km/s): [" << targetSpacecraftInitialState( 3 ) / 1e3 << ", " << targetSpacecraftInitialState( 4 ) / 1e3
              << ", " << targetSpacecraftInitialState( 5 ) / 1e3 << "]" << std::endl;

    // =========================================================================
    // Set up simulation environment: Earth, Moon, targetSpacecraft
    // =========================================================================

    // Load Spice kernels
    spice_interface::loadStandardSpiceKernels( );

    std::vector< std::string > bodiesToCreate = { "Earth", "Moon" };
    BodyListSettings bodySettings = getDefaultBodySettings( bodiesToCreate, "Earth", "J2000" );
    SystemOfBodies bodies = createSystemOfBodies< double, double >( bodySettings );

    // Create ground stations from UDL data
    auto udlObservations = UDL.toTudat( bodies, { }, "Earth" );

    // Print ground station Cartesian positions
    std::cout << "\nGround station Cartesian positions (ECEF, meters):" << std::endl;
    auto groundStationMap = bodies.at( "Earth" )->getGroundStationMap( );
    for( const auto& [ stationName, groundStation ] : groundStationMap )
    {
        Eigen::Vector3d cartesianPosition = groundStation->getNominalStationState( )->getNominalCartesianPosition( );
        std::cout << "  " << stationName << ": [" << std::fixed << std::setprecision( 3 ) << cartesianPosition( 0 ) << ", "
                  << cartesianPosition( 1 ) << ", " << cartesianPosition( 2 ) << "]" << std::endl;
    }

    // Create targetSpacecraft body with tabulated ephemeris from Horizons
    auto capstoneName = *UDL.getCollection( ).getObservedTargets( ).begin( );
    std::cout << "Capstone is known as target ID: " << capstoneName << " in UDL data." << std::endl;
    bodies.createEmptyBody( capstoneName );
    bodies.at( capstoneName )->setConstantBodyMass( 25.0 );  // Approximate mass of Capstone
    auto ephSet = std::make_shared< TabulatedEphemerisSettings >( targetSpacecraftStates, "Earth", "J2000" );
    auto eph = createBodyEphemeris< double, double >( ephSet, capstoneName );
    bodies.at( capstoneName )->setEphemeris( eph );

    std::cout << "Created simulation environment with Earth, Moon, and Capstone." << std::endl;

    // =========================================================================
    // Get link ends from UDL observations
    // =========================================================================

    auto linkDefsPerObservable = udlObservations->getLinkDefinitionsPerObservable( );

    std::cout << "\nAvailable observation types and link ends from UDL:" << std::endl;
    for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
    {
        std::cout << "  " << getObservableName( obsType ) << ": " << linkDefs.size( ) << " link definition(s)" << std::endl;
    }

    // =========================================================================
    // Set up TDOA/FDOA observation models using link ends from UDL
    // =========================================================================

    std::vector< std::shared_ptr< ObservationModelSettings > > observationModelSettings;
    std::vector< std::shared_ptr< ObservationSimulationSettings< double > > > observationSimulationSettings;

    // Define observation times
    std::vector< double > observationTimes;
    double obsInterval = 1.0;  // 1 second interval
    for( double t = startEpoch + obsInterval; t <= endEpoch - obsInterval; t += obsInterval )
    {
        observationTimes.push_back( t );
    }
    std::cout << "\nSimulating " << observationTimes.size( ) << " observation epochs over " << ( endEpoch - startEpoch ) / 86400.0
              << " days." << std::endl;

    // X-band transmitter frequency for FDOA
    double transmitterFrequency = 2.2093339688e+09;  // S-band

    // Create observation model settings for each link definition
    for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
    {
        for( const auto& linkDef : linkDefs )
        {
            // Get link ends from UDL data
            LinkEnds linkEnds = linkDef.linkEnds_;

            if( obsType == differenced_time_of_arrival )
            {
                // TDOA observation model
                auto tdoaModelSettings = std::make_shared< DifferencedTimeOfArrivalObservationSettings >(
                        linkEnds, std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ) );
                observationModelSettings.push_back( tdoaModelSettings );

                // TDOA simulation settings
                auto tdoaSimSettings = std::make_shared< TabulatedObservationSimulationSettings< double > >(
                        differenced_time_of_arrival, linkEnds, observationTimes, receiver );
                observationSimulationSettings.push_back( tdoaSimSettings );
            }
            else if( obsType == differenced_frequency_of_arrival )
            {
                // FDOA observation model
                auto fdoaModelSettings = std::make_shared< DifferencedFrequencyOfArrivalObservationSettings >(
                        linkEnds, std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ) );
                observationModelSettings.push_back( fdoaModelSettings );

                // FDOA simulation settings with ancillary data
                auto fdoaSimSettings = std::make_shared< TabulatedObservationSimulationSettings< double > >(
                        differenced_frequency_of_arrival, linkEnds, observationTimes, receiver );
                auto fdoaAncillarySettings = getFdoaAncilliarySettings( transmitterFrequency );
                fdoaSimSettings->setAncilliarySettings( fdoaAncillarySettings );
                observationSimulationSettings.push_back( fdoaSimSettings );
            }
        }
    }

    std::cout << "Created " << observationModelSettings.size( ) << " observation model settings." << std::endl;

    // =========================================================================
    // Create observation simulators and simulate observations
    // =========================================================================

    std::cout << "Creating observation simulators..." << std::endl;
    auto observationSimulators = createObservationSimulators( observationModelSettings, bodies );

    std::cout << "Simulating observations..." << std::endl;
    auto observationCollection = simulateObservations( observationSimulationSettings, observationSimulators, bodies );

    std::cout << "\nObservations simulated:" << std::endl;
    std::cout << "  Total observation size: " << observationCollection->getTotalObservableSize( ) << std::endl;
    std::cout << "  Observable types: " << observationCollection->getObservableTypes( ).size( ) << std::endl;

    // =========================================================================
    // Print sample observations
    // =========================================================================

    for( const auto& [ obsType, linkDefs ] : linkDefsPerObservable )
    {
        for( const auto& linkDef : linkDefs )
        {
            auto obsSets = observationCollection->getSingleLinkAndTypeObservationSets( obsType, linkDef );

            if( !obsSets.empty( ) )
            {
                auto obsHistory = obsSets.at( 0 )->getObservationsHistory( );
                std::cout << "\n=== Sample " << getObservableName( obsType ) << " Observations ===" << std::endl;

                int count = 0;
                for( const auto& obs : obsHistory )
                {
                    if( obsType == differenced_time_of_arrival )
                    {
                        std::cout << "  Time: " << std::fixed << std::setprecision( 1 ) << obs.first
                                  << " s, TDOA: " << std::setprecision( 12 ) << obs.second( 0 ) * 1e9 << " ns" << std::endl;
                    }
                    else if( obsType == differenced_frequency_of_arrival )
                    {
                        std::cout << "  Time: " << std::fixed << std::setprecision( 1 ) << obs.first
                                  << " s, FDOA: " << std::setprecision( 6 ) << obs.second( 0 ) << " Hz" << std::endl;
                    }
                    if( ++count >= 3 ) break;
                }
            }
        }
    }

    // =========================================================================
    // Set observation weights
    // =========================================================================

    // TDOA noise: ~1 nanosecond precision
    observationCollection->setConstantWeight( 1.0 / ( 1.0e-9 * 1.0e-9 ), observationParser( differenced_time_of_arrival ) );

    // FDOA noise: ~1 mHz precision
    observationCollection->setConstantWeight( 1.0 / ( 1.0e-3 * 1.0e-3 ), observationParser( differenced_frequency_of_arrival ) );

    std::cout << "\nWeights set for observations." << std::endl;

    // =========================================================================
    // Export to HDF5
    // =========================================================================

    std::string outputDirectory = "/home/pascal/Documents/lunar-od/tudatpy/output/";
    std::string hdf5Filename = "capstone_no_corrections.h5";
    std::string xdmfFilename = "capstone_no_corrections.xdmf";

    std::string hdf5Path = outputDirectory + hdf5Filename;
    std::string xdmfPath = outputDirectory + xdmfFilename;

    std::cout << "\nExporting observations to HDF5: " << hdf5Path << std::endl;

    // Create HDF5 output file
    HDF5OutputFile hdf5File( hdf5Path, true );

    // Add the complete observation collection
    hdf5File.addObservationCollection( observationCollection, "capstone_vlbi_observations", "/Observations/ObservationCollections", false );

    // Also add individual observation sets for easy access
    auto singleObsSets = observationCollection->getSingleObservationSets( );
    int setCounter = 0;
    for( const auto& obsSet : singleObsSets )
    {
        std::string setName = "set_" + std::to_string( setCounter++ );
        hdf5File.addSingleObservationSet( obsSet, setName, "/Observations/IndividualSets" );
    }

    // Generate XDMF descriptor file for ParaView visualization
    std::cout << "Generating XDMF descriptor: " << xdmfPath << std::endl;
    hdf5File.generateObservationXDMF( xdmfPath );

    // Close file
    hdf5File.close( );

    // =========================================================================
    // Summary
    // =========================================================================

    std::cout << "\n=== Export complete! ===" << std::endl;
    std::cout << "Output files:" << std::endl;
    std::cout << "  HDF5: " << hdf5Path << std::endl;
    std::cout << "  XDMF: " << xdmfPath << std::endl;
    std::cout << "\nHDF5 file structure:" << std::endl;
    std::cout << "/Observations/" << std::endl;
    std::cout << "  /ObservationCollections/" << std::endl;
    std::cout << "    /danuri_vlbi_observations/" << std::endl;
    std::cout << "      /metadata/              - Observable types, time bounds, link end IDs" << std::endl;
    std::cout << "      /concatenated/          - All observations in single arrays" << std::endl;
    std::cout << "      /by_observable_type/    - Organized by type -> link ends -> sets" << std::endl;
    std::cout << "  /IndividualSets/" << std::endl;
    for( int i = 0; i < setCounter; i++ )
    {
        std::cout << "    /set_" << i << "/" << std::endl;
    }

    std::cout << "\nYou can inspect the files with:" << std::endl;
    std::cout << "  h5dump -H " << hdf5Path << std::endl;
    std::cout << "  h5ls -r " << hdf5Path << std::endl;
    std::cout << "\nOpen in ParaView:" << std::endl;
    std::cout << "  paraview " << xdmfPath << std::endl;

    return 0;
}

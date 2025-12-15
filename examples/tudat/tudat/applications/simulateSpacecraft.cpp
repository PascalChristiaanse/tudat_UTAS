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
#include <algorithm>

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
using namespace tudat::earth_orientation;
using namespace tudat::io;

std::vector< double > generateObservationTimesFromUDL( const io::UTASObservationCollection& udlCollection )
{
    std::set< double > observationTimesSet;

    const auto& allObs = udlCollection.getAllObservations( );
    for( const auto& [ targetId, stationPairMap ] : allObs )
    {
        for( const auto& [ stationPair, obsSets ] : stationPairMap )
        {
            for( const auto& obsSet : obsSets )
            {
                const auto& timeSeries = obsSet->getTimeSeries( );
                observationTimesSet.insert( timeSeries.epochs.begin( ), timeSeries.epochs.end( ) );
            }
        }
    }

    return std::vector< double >( observationTimesSet.begin( ), observationTimesSet.end( ) );
}

std::vector< Time > generateObservationTimesFromPresetDateStrings(
        double timestep,
        const std::string& startDateString,
        const std::string& endDateString,
        std::shared_ptr< TerrestrialTimeScaleConverter > timeConverter = std::make_shared< TerrestrialTimeScaleConverter >( ) )
{
    // Convert start and end date strings UTC to seconds since J2000 TDB
    Time startTimeUTC = tba::timeFromIsoString< tudat::Time >( startDateString );
    Time endTimeUTC = tba::timeFromIsoString< tudat::Time >( endDateString );

    // Convert UTC to TDB using time scale converter
    // Use a dummy position @ TODO improve with actual station position
    Eigen::Vector3d dummyPosition( 6378.0e3, 0.0, 0.0 );
    Time startTimeTDB =
            timeConverter->getCurrentTime< Time >( tba::TimeScales::utc_scale, tba::TimeScales::tdb_scale, startTimeUTC, dummyPosition );
    Time endTimeTDB =
            timeConverter->getCurrentTime< Time >( tba::TimeScales::utc_scale, tba::TimeScales::tdb_scale, endTimeUTC, dummyPosition );

    // set maximum precision for output
    std::cout << std::setprecision( std::numeric_limits< double >::digits10 + 2 );
    std::cout << "Start Julian Day: " << julianDayFromTime< double >( startTimeTDB ) << " TDB" << std::endl;
    std::cout << "End   Julian Day: " << julianDayFromTime< double >( endTimeTDB ) << " TDB" << std::endl;
    std::string isoStart = horizons_interface::secondsToIsoDate( static_cast< double >( startTimeTDB ) );
    std::string isoEnd = horizons_interface::secondsToIsoDate( static_cast< double >( endTimeTDB ) );
    std::cout << "Start ISO Date: " << isoStart << " TDB" << std::endl;
    std::cout << "End   ISO Date: " << isoEnd << " TDB" << std::endl;
    int size =
            static_cast< int >( std::ceil( ( static_cast< double >( endTimeTDB ) - static_cast< double >( startTimeTDB ) ) / timestep ) ) +
            1;
    std::vector< Time > observationTimes( size );
    for( int i = 0; i < size; ++i )
    {
        observationTimes[ i ] = startTimeTDB + i * timestep;
    }
    return observationTimes;
}

int main( )
{
    std::cout << "=== targetSpacecraft TDOA/FDOA Observation Simulation ===" << std::endl;

    std::string outputDirectory = "/home/pascal/Documents/lunar-od/results/final_dataset-10dec/";
    std::string hdf5Filename = "capstone_no_de-bias_GCRS.h5";
    std::string xdmfFilename = "capstone_no_de-bias_GCRS.xdmf";

    std::string globalFrameOrientation = std::string( "ECLIPJ2000" );
    std::string globalFrameOrigin = std::string( "SSB" );

    // =========================================================================
    // Load UDL observations to get ground station definitions
    // =========================================================================
    
    
    
    std::vector< std::string > udlFiles = {
        "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_ke_cd_2025-10-11T164121.v19.json",
        "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_yg_cd_2025-10-11T164121.v19.json",
        "/home/pascal/Documents/lunar-od/data/processed/Capstone/CAPSTONE_TDOA-FDOA_yg_ke_2025-10-11T164121.v19.json",
    };
    auto UDL = io::BatchVLBI( udlFiles );

    // =========================================================================
    // Grab targetSpacecraft spacecraft ephemeris from JPL Horizons
    // =========================================================================
    // std::vector< double > observationTimes = generateObservationTimesFromUDL( UDL.getCollection( ) );
    std::vector< Time > observationTimesTime =
            generateObservationTimesFromPresetDateStrings( 1.0, "2025-10-11T16:41:21.772479857", "2025-10-11T17:00:10.772479857" );

    // Convert Time vector to double vector for observation simulation settings
    std::vector< double > observationTimes( observationTimesTime.size( ) );
    std::transform( observationTimesTime.begin( ), observationTimesTime.end( ), observationTimes.begin( ),
                    []( const Time& t ) { return static_cast< double >( t ); } );

    Time startEpochPadded =
            *std::min_element( observationTimesTime.begin( ), observationTimesTime.end( ) ) - 600.0;  // Start time from UDL data - 10 min buffer
    Time endEpochPadded =
            *std::max_element( observationTimesTime.begin( ), observationTimesTime.end( ) ) + 600.0;  // End time from UDL data + 10 min buffer
    std::string stepSize = "1m";                                                              // 1 minute intervals for ephemeris

    auto targetSpacecraftStates = HorizonsQuery( "CAPSTONE", "@SSB", startEpochPadded, endEpochPadded, stepSize )
                                          .getCartesianStateHistory( globalFrameOrientation );

    // =========================================================================
    // Set up simulation environment: Earth, Moon, targetSpacecraft
    // =========================================================================

    // Load Spice kernels
    spice_interface::loadStandardSpiceKernels( );

    std::vector< std::string > bodiesToCreate = { "Earth", "Moon" };
    BodyListSettings bodySettings = getDefaultBodySettings( bodiesToCreate, globalFrameOrigin, globalFrameOrientation );

    bodySettings.get( "Earth" )->rotationModelSettings =
            gcrsToItrsRotationModelSettings( tudat::basic_astrodynamics::iau_2006, globalFrameOrientation, nullptr, nullptr, nullptr );
    bodySettings.get( "Earth" )->shapeModelSettings = simulation_setup::fromSpiceOblateSphericalBodyShapeSettings( );

    SystemOfBodies bodies = createSystemOfBodies< double, double >( bodySettings );

    // Create ground stations from UDL data
    auto udlObservations = UDL.toTudat( bodies, { }, "Earth" );

    // Create targetSpacecraft body with tabulated ephemeris from Horizons
    auto capstoneName = *UDL.getCollection( ).getObservedTargets( ).begin( );
    std::cout << "Capstone is known as target ID: " << capstoneName << " in UDL data." << std::endl;

    bodies.createEmptyBody( capstoneName );
    auto ephSet = std::make_shared< TabulatedEphemerisSettings >( targetSpacecraftStates, globalFrameOrigin, globalFrameOrientation );
    auto eph = createBodyEphemeris< double, double >( ephSet, capstoneName );
    bodies.at( capstoneName )->setEphemeris( eph );

    // X-band transmitter frequency for FDOA
    double transmitterFrequency = 8465000000;  // X-band
    bodies.getBody( capstoneName )
            ->getVehicleSystems( )
            ->setTransmittedFrequencyCalculator(
                    std::make_shared< ground_stations::ConstantFrequencyInterpolator >( transmitterFrequency ) );

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
            if (obsType == differenced_frequency_of_arrival )
            {
                // FDOA observation model
                auto fdoaModelSettings = std::make_shared< DifferencedFrequencyOfArrivalObservationSettings >(
                        linkEnds, std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ) );
                observationModelSettings.push_back( fdoaModelSettings );

                // FDOA simulation settings
                auto fdoaSimSettings = std::make_shared< TabulatedObservationSimulationSettings< double > >(
                        differenced_frequency_of_arrival, linkEnds, observationTimes, receiver );
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
            if( obsType == differenced_frequency_of_arrival )
            {
                // std::cout << "\nNote: FDOA observations are not yet implemented in this example." << std::endl;
                continue;
            }
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
                        auto timeJD = julianDayFromTime< double >( obs.first );
                        std::cout << "  Time: " << std::fixed << std::setprecision( 9 ) << timeJD << " TDB" << obs.first << " s,"
                                  << " s, TDOA: " << std::setprecision( 12 ) << obs.second( 0 ) * 1e9 << " ns" << std::endl;
                    }
                    else if( obsType == differenced_frequency_of_arrival )
                    {
                        auto timeJD = julianDayFromTime< double >( obs.first );
                        std::cout << "  Time: " << std::fixed << std::setprecision( 9 ) << timeJD << " TDB" << obs.first << " s,"
                                  << " s, FDOA: " << std::setprecision( 12 ) << obs.second( 0 ) * 1e9 << " ns" << std::endl;
                    }
                    if( ++count >= 3 ) break;
                }
            }
        }
    }

    // =========================================================================
    // Export to HDF5
    // =========================================================================

    std::string hdf5Path = outputDirectory + hdf5Filename;
    std::string xdmfPath = outputDirectory + xdmfFilename;

    std::cout << "\nExporting observations to HDF5: " << hdf5Path << std::endl;

    // Create HDF5 output file
    HDF5OutputFile hdf5File( hdf5Path, true );

    // Add the complete observation collection
    hdf5File.addObservationCollection( observationCollection, "capstone_vlbi_observations", "/Observations/ObservationCollections", false );

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
    std::cout << "    /$target$_vlbi_observations/" << std::endl;
    std::cout << "      /metadata/              - Observable types, time bounds, link end IDs" << std::endl;
    std::cout << "      /concatenated/          - All observations in single arrays" << std::endl;
    std::cout << "      /by_observable_type/    - Organized by type -> link ends -> sets" << std::endl;

    std::cout << "\nYou can inspect the files with:" << std::endl;
    std::cout << "  h5dump -H " << hdf5Path << std::endl;
    std::cout << "  h5ls -r " << hdf5Path << std::endl;
    std::cout << "\nOpen in ParaView:" << std::endl;
    std::cout << "  paraview " << xdmfPath << std::endl;

    return 0;
}

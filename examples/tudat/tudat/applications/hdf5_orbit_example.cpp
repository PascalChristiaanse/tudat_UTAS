/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 *    Example: Propagate a basic orbit and save trajectory to HDF5 with XDMF
 *
 */

#include <iostream>
#include <string>

// Tudat core includes
#include "tudat/astro/basic_astro/physicalConstants.h"
#include "tudat/astro/basic_astro/unitConversions.h"
#include "tudat/simulation/simulation.h"
#include "tudat/io/hdf5Manager.h"

int main()
{
    using namespace tudat;
    using namespace tudat::simulation_setup;
    using namespace tudat::propagators;
    using namespace tudat::numerical_integrators;
    using namespace tudat::basic_astrodynamics;
    using namespace tudat::io;

    std::cout << "=== HDF5 Orbit Propagation Example ===" << std::endl;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////     CREATE ENVIRONMENT       //////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Load Spice kernels
    spice_interface::loadStandardSpiceKernels();

    // Define simulation time settings
    const double simulationStartEpoch = 0.0;
    const double simulationEndEpoch = 3.0 * physical_constants::JULIAN_DAY; // 3 days

    // Define bodies in simulation
    std::vector< std::string > bodiesToCreate = { "Earth", "Sun", "Moon" };

    // Create body settings
    BodyListSettings bodySettings = getDefaultBodySettings( bodiesToCreate );

    // Create bodies
    SystemOfBodies bodies = createSystemOfBodies< double, double >( bodySettings );

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////     CREATE SPACECRAFT        //////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Add spacecraft
    bodies.createEmptyBody( "Satellite" );
    bodies.at( "Satellite" )->setConstantBodyMass( 1000.0 ); // 1000 kg

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////     CREATE ACCELERATIONS     //////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Define accelerations acting on satellite
    SelectedAccelerationMap accelerationMap;
    std::map< std::string, std::vector< std::shared_ptr< AccelerationSettings > > > accelerationsOfSatellite;

    // Point mass gravity from Earth, Sun, Moon
    accelerationsOfSatellite[ "Earth" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
    accelerationsOfSatellite[ "Sun" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );
    accelerationsOfSatellite[ "Moon" ].push_back( std::make_shared< AccelerationSettings >( point_mass_gravity ) );

    accelerationMap[ "Satellite" ] = accelerationsOfSatellite;

    // Define bodies to propagate and central bodies
    std::vector< std::string > bodiesToPropagate = { "Satellite" };
    std::vector< std::string > centralBodies = { "Earth" };

    // Create acceleration models
    AccelerationMap accelerationModelMap = createAccelerationModelsMap(
            bodies, accelerationMap, bodiesToPropagate, centralBodies );

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////     CREATE INITIAL STATE     //////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Define initial Keplerian elements (LEO orbit)
    Eigen::Vector6d initialKeplerianElements;
    initialKeplerianElements( orbital_element_conversions::semiMajorAxisIndex ) = 7000.0e3;  // 7000 km
    initialKeplerianElements( orbital_element_conversions::eccentricityIndex ) = 0.01;       // Nearly circular
    initialKeplerianElements( orbital_element_conversions::inclinationIndex ) = 
        unit_conversions::convertDegreesToRadians( 51.6 );  // ISS-like inclination
    initialKeplerianElements( orbital_element_conversions::argumentOfPeriapsisIndex ) = 
        unit_conversions::convertDegreesToRadians( 45.0 );
    initialKeplerianElements( orbital_element_conversions::longitudeOfAscendingNodeIndex ) = 
        unit_conversions::convertDegreesToRadians( 30.0 );
    initialKeplerianElements( orbital_element_conversions::trueAnomalyIndex ) = 
        unit_conversions::convertDegreesToRadians( 0.0 );

    // Convert to Cartesian state
    double earthGravitationalParameter = bodies.at( "Earth" )->getGravityFieldModel()->getGravitationalParameter();
    Eigen::Vector6d initialCartesianState = orbital_element_conversions::convertKeplerianToCartesianElements(
            initialKeplerianElements, earthGravitationalParameter );

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////     CREATE PROPAGATOR        //////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Define dependent variables to save
    std::vector< std::shared_ptr< SingleDependentVariableSaveSettings > > dependentVariables;
    dependentVariables.push_back( std::make_shared< SingleDependentVariableSaveSettings >(
            altitude_dependent_variable, "Satellite", "Earth" ) );
    dependentVariables.push_back( std::make_shared< SingleDependentVariableSaveSettings >(
            relative_speed_dependent_variable, "Satellite", "Earth" ) );
    dependentVariables.push_back( std::make_shared< SingleAccelerationDependentVariableSaveSettings >(
            point_mass_gravity, "Satellite", "Earth" ) );

    // Create propagator settings
    std::shared_ptr< TranslationalStatePropagatorSettings< double, double > > propagatorSettings =
            std::make_shared< TranslationalStatePropagatorSettings< double, double > >(
                    centralBodies,
                    accelerationModelMap,
                    bodiesToPropagate,
                    initialCartesianState,
                    simulationStartEpoch,
                    std::make_shared< IntegratorSettings< double > >(
                            rungeKutta4, simulationStartEpoch, 60.0 ),  // RK4, 60s step
                    std::make_shared< PropagationTimeTerminationSettings >( simulationEndEpoch ),
                    cowell,
                    dependentVariables );

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////     PROPAGATE ORBIT          //////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::cout << "Starting orbit propagation..." << std::endl;

    // Create dynamics simulator and propagate
    SingleArcDynamicsSimulator< double, double > dynamicsSimulator(
            bodies, propagatorSettings );

    // Get propagation results (cast from base SimulationResults to SingleArcSimulationResults)
    std::shared_ptr< SingleArcSimulationResults< double, double > > propagationResults = 
            std::dynamic_pointer_cast< SingleArcSimulationResults< double, double > >(
                dynamicsSimulator.getPropagationResults() );

    std::cout << "Propagation complete!" << std::endl;
    std::cout << "  Number of states saved: " << propagationResults->getEquationsOfMotionNumericalSolution().size() << std::endl;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////     SAVE TO HDF5             //////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::cout << "Saving results to HDF5..." << std::endl;

    // Create HDF5 output file
    std::string outputFilename = "orbit_propagation_results.h5";
    HDF5OutputFile hdf5File( outputFilename );

    // Add the simulation results (includes metadata automatically)
    hdf5File.addSingleArcResults( propagationResults, "Satellite" );

    // Close the file
    hdf5File.close();

    std::cout << "Results saved to: " << outputFilename << std::endl;
    std::cout << "XDMF descriptor: " << outputFilename.substr(0, outputFilename.find_last_of('.')) + ".xdmf" << std::endl;

    std::cout << "\n=== Example Complete ===" << std::endl;
    std::cout << "Open the .xdmf file in ParaView to visualize the trajectory." << std::endl;

    return 0;
}

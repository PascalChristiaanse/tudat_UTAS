/*    Copyright (c) 2010-2024, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 *
 *    Example: Using the Pure C++ Horizons Interface
 *    
 *    This example demonstrates how to query JPL Horizons directly from C++ 
 *    using HTTP requests (libcurl). No Python dependencies required!
 */

#include <iostream>
#include <iomanip>
#include <cmath>

#include "tudat/interface/horizons.h"
#include "tudat/astro/basic_astro/physicalConstants.h"

using namespace tudat;
using namespace tudat::horizons_interface;

int main( )
{
    std::cout << "=== JPL Horizons Pure C++ Interface Example ===" << std::endl;
    std::cout << std::endl;
    
    // =========================================================================
    // Example 1: Query Mars ephemeris from Solar System Barycenter
    // =========================================================================
    
    std::cout << "--- Example 1: Mars Ephemeris from SSB ---" << std::endl;
    
    try
    {
        // Define time range: 1 week starting from J2000
        double startEpoch = 0.0;  // J2000
        double endEpoch = 7.0 * 86400.0;  // 7 days in seconds
        std::string stepSize = "1d";  // 1 day intervals
        
        // Query Mars (ID: 499) from Solar System Barycenter (@0)
        std::cout << "Querying Mars ephemeris from JPL Horizons..." << std::endl;
        
        HorizonsQuery marsQuery( "499", "@0", startEpoch, endEpoch, stepSize );
        
        // Get Cartesian states as map of epoch -> state vector
        StateHistory marsStates = marsQuery.getCartesianStates( 
            FrameOrientation::J2000, 
            AberrationCorrection::Geometric );
        
        std::cout << "Target: " << marsQuery.getTargetFullName( ) << std::endl;
        std::cout << "Retrieved " << marsStates.size( ) << " state vectors." << std::endl;
        std::cout << std::endl;
        
        // Print first few states
        std::cout << "States (epoch [days since J2000], position [Gm], velocity [km/s]):" << std::endl;
        std::cout << std::fixed << std::setprecision( 3 );
        
        int count = 0;
        for( const auto& entry : marsStates )
        {
            if( count >= 3 ) break;
            
            double epoch = entry.first;
            const Vector6d& state = entry.second;
            
            std::cout << "  Day " << epoch / 86400.0 << ": "
                      << "pos=(" << state( 0 ) / 1e9 << ", " 
                      << state( 1 ) / 1e9 << ", " 
                      << state( 2 ) / 1e9 << ") Gm, "
                      << "vel=(" << state( 3 ) / 1e3 << ", " 
                      << state( 4 ) / 1e3 << ", " 
                      << state( 5 ) / 1e3 << ") km/s"
                      << std::endl;
            count++;
        }
    }
    catch( const HorizonsException& e )
    {
        std::cerr << "Horizons query failed: " << e.what( ) << std::endl;
    }
    
    std::cout << std::endl;
    
    // =========================================================================
    // Example 2: Query with specific epoch list
    // =========================================================================
    
    std::cout << "--- Example 2: Query with Specific Epochs ---" << std::endl;
    
    try
    {
        // Create a list of specific epochs
        std::vector< double > epochList = {
            0.0,           // J2000
            86400.0,       // J2000 + 1 day
            172800.0,      // J2000 + 2 days
            604800.0       // J2000 + 1 week
        };
        
        std::cout << "Querying Moon ephemeris at " << epochList.size( ) << " specific epochs..." << std::endl;
        
        // Query Moon (ID: 301) from Earth center (500@399)
        HorizonsQuery moonQuery( "301", "500@399", epochList );
        
        StateHistory moonStates = moonQuery.getCartesianStates( FrameOrientation::J2000 );
        
        std::cout << "Target: " << moonQuery.getTargetFullName( ) << std::endl;
        std::cout << "Retrieved " << moonStates.size( ) << " state vectors for Moon." << std::endl;
        
        // Print all states with distance
        for( const auto& entry : moonStates )
        {
            double epoch = entry.first;
            const Vector6d& state = entry.second;
            
            double distanceKm = state.head< 3 >( ).norm( ) / 1e3;
            
            std::cout << "  Day " << epoch / 86400.0 << ": "
                      << "distance = " << std::fixed << std::setprecision( 0 ) 
                      << distanceKm << " km" << std::endl;
        }
    }
    catch( const HorizonsException& e )
    {
        std::cerr << "Moon query failed: " << e.what( ) << std::endl;
    }
    
    std::cout << std::endl;
    
    // =========================================================================
    // Example 3: Using utility function for quick state history
    // =========================================================================
    
    std::cout << "--- Example 3: Quick State History Utility ---" << std::endl;
    
    try
    {
        std::cout << "Using getHorizonsCartesianStateHistory() utility..." << std::endl;
        
        // One-liner to get state history for Venus
        StateHistory venusHistory = getHorizonsCartesianStateHistory(
            "299",      // Venus
            "@0",       // From SSB
            0.0,        // Start: J2000
            30 * 86400, // End: J2000 + 30 days
            "5d",       // 5-day step
            FrameOrientation::ECLIPJ2000
        );
        
        std::cout << "Got " << venusHistory.size( ) << " states for Venus." << std::endl;
        
        // Print epochs and distances from SSB
        for( const auto& entry : venusHistory )
        {
            double epoch = entry.first;
            const Vector6d& state = entry.second;
            double distance = state.head< 3 >( ).norm( ) / physical_constants::ASTRONOMICAL_UNIT;
            
            std::cout << "  Day " << std::fixed << std::setprecision( 0 ) 
                      << epoch / 86400.0 << ": " 
                      << std::setprecision( 4 ) << distance << " AU from SSB" << std::endl;
        }
    }
    catch( const HorizonsException& e )
    {
        std::cerr << "Venus query failed: " << e.what( ) << std::endl;
    }
    
    std::cout << std::endl;
    
    // =========================================================================
    // Example 4: Query spacecraft
    // =========================================================================
    
    std::cout << "--- Example 4: JUICE Spacecraft ---" << std::endl;
    
    try
    {
        // JUICE spacecraft ID: -28
        // Query from Sun center (@10)
        // Around October 2024 (about 4.8 years after J2000)
        double juiceStartEpoch = 4.8 * 365.25 * 86400.0;  // ~Oct 2024
        double juiceEndEpoch = juiceStartEpoch + 7.0 * 86400.0;  // 7 days
        
        std::cout << "Querying JUICE spacecraft from JPL Horizons..." << std::endl;
        
        HorizonsQuery juiceQuery( "-28", "@10", juiceStartEpoch, juiceEndEpoch, "1d" );
        
        StateHistory juiceStates = juiceQuery.getCartesianStates( );
        
        std::cout << "Target: " << juiceQuery.getTargetFullName( ) << std::endl;
        std::cout << "Retrieved " << juiceStates.size( ) << " state vectors." << std::endl;
        
        if( !juiceStates.empty( ) )
        {
            auto it = juiceStates.begin( );
            const Vector6d& state = it->second;
            double distanceAU = state.head< 3 >( ).norm( ) / physical_constants::ASTRONOMICAL_UNIT;
            
            std::cout << "First state: distance from Sun = " 
                      << std::fixed << std::setprecision( 4 ) << distanceAU << " AU" << std::endl;
        }
    }
    catch( const HorizonsException& e )
    {
        std::cerr << "JUICE query failed (may not be available for this timespan): " 
                  << e.what( ) << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "=== Example Complete ===" << std::endl;
    
    return 0;
}

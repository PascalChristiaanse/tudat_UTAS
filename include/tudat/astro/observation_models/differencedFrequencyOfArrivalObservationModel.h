/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_DIFFERENCEDFREQUENCYOFARRIVALOBSERVATIONMODEL_H
#define TUDAT_DIFFERENCEDFREQUENCYOFARRIVALOBSERVATIONMODEL_H

#include <map>
#include <iostream>

#include <functional>

#include <Eigen/Core>

#include "tudat/astro/basic_astro/physicalConstants.h"
#include "tudat/astro/observation_models/observationModel.h"
#include "tudat/astro/observation_models/lightTimeSolution.h"

namespace tudat
{

namespace observation_models
{



template< typename ObservationScalarType = double, typename TimeType = double >
class OneWayDifferencedFrequencyOfArrivalObservationModel : public ObservationModel< 1, ObservationScalarType, TimeType >
{
public:
    typedef Eigen::Matrix< ObservationScalarType, 6, 1 > StateType;
    typedef Eigen::Matrix< ObservationScalarType, 3, 1 > PositionType;
    
    using ObservationModel< 1, ObservationScalarType, TimeType >::frequencyInterpolator_;
    
    OneWayDifferencedFrequencyOfArrivalObservationModel(
            const LinkEnds& linkEnds,
            const std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > firstReceiverLightTimeCalculator,
            const std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > secondReceiverLightTimeCalculator,
            const std::shared_ptr< ObservationBias< 1 > > observationBiasCalculator = nullptr,
            const std::map< LinkEndType, std::shared_ptr< ground_stations::GroundStationState > >& stationStates =
                    std::map< LinkEndType, std::shared_ptr< ground_stations::GroundStationState > >( ),
            const basic_astrodynamics::TimeScales observableTimeScale = basic_astrodynamics::tdb_scale ):
        ObservationModel< 1, ObservationScalarType, TimeType >( differenced_frequency_of_arrival, linkEnds, observationBiasCalculator ),
            firstReceiverLightTimeCalculator_( firstReceiverLightTimeCalculator ), secondReceiverLightTimeCalculator_( secondReceiverLightTimeCalculator ),
            stationStates_( stationStates ), observableTimeScale_( observableTimeScale )
    {
        if( observableTimeScale_ == basic_astrodynamics::utc_scale || observableTimeScale_ == basic_astrodynamics::ut1_scale )
        {
            if( stationStates.count( receiver ) == 0 )
            {
                throw std::runtime_error( "Error when making differenced frequency of arrival observation model, no state model found for receiver " +
                                          linkEnds.at( receiver ).bodyName_ + ", " + linkEnds.at( receiver ).stationName_ );
            }

            if( stationStates.count( receiver2 ) == 0 )
            {
                throw std::runtime_error( "Error when making differenced frequency of arrival observation model, no state model found for receiver2 " +
                                          linkEnds.at( receiver2 ).bodyName_ + ", " + linkEnds.at( receiver2 ).stationName_ );
            }
        }
    }

    //! Destructor
    ~OneWayDifferencedFrequencyOfArrivalObservationModel( ) {}
    
    Eigen::Matrix< ObservationScalarType, 1, 1 > computeIdealObservationsWithLinkEndData(
            const TimeType time,
            const LinkEndType linkEndAssociatedWithTime,
            std::vector< double >& linkEndTimes,
            std::vector< Eigen::Matrix< double, 6, 1 > >& linkEndStates,
            const std::shared_ptr< ObservationAncilliarySimulationSettings > ancilliarySetingsInput = nullptr )
    {
        ObservationScalarType lightTimeForFirstReceiver;
        ObservationScalarType lightTimeForSecondReceiver;
        
        // Resize link end data vectors to 3 (Transmitter, two Receivers)
        linkEndTimes.resize( 3 );
        linkEndStates.resize( 3 );
        
        StateType transmitterStateForFirstLink, receiverStateForFirstLink, transmitterStateForSecondLink, receiverStateForSecondLink;
        TimeType fullPrecisionTimeAtReceiver2;
        if( linkEndAssociatedWithTime == receiver )
        {
            // Calculate reception time at ground station at the start and end of the count interval at reception time.
            linkEndTimes[ 1 ] = static_cast< double >( time );

            std::shared_ptr< ObservationAncilliarySimulationSettings > ancilliarySetings;
            this->setFrequencyProperties( time, receiver, firstReceiverLightTimeCalculator_, ancilliarySetingsInput, ancilliarySetings );
            lightTimeForFirstReceiver = firstReceiverLightTimeCalculator_->calculateLightTimeWithLinkEndsStates(
                    receiverStateForFirstLink, transmitterStateForFirstLink, time, 1, ancilliarySetings );
            
            linkEndTimes[ 0 ] = linkEndTimes[ 1 ] - static_cast< double >( lightTimeForFirstReceiver );

            this->setFrequencyProperties( time, receiver, secondReceiverLightTimeCalculator_, ancilliarySetingsInput, ancilliarySetings );
            lightTimeForSecondReceiver = secondReceiverLightTimeCalculator_->calculateLightTimeWithLinkEndsStates( 
                    receiverStateForSecondLink, transmitterStateForSecondLink, time - lightTimeForFirstReceiver, 0, ancilliarySetings );
            fullPrecisionTimeAtReceiver2 = time - ( lightTimeForFirstReceiver - lightTimeForSecondReceiver );
            linkEndTimes[ 2 ] = static_cast< double >( fullPrecisionTimeAtReceiver2 );

        }
        else
        {
            throw std::runtime_error( "Error in differenced frequency of arrival, reference link end not recognized" );
        }

        linkEndStates[ 0 ] = transmitterStateForFirstLink.template cast< double >( );
        linkEndStates[ 1 ] = receiverStateForFirstLink.template cast< double >( );
        linkEndStates[ 2 ] = receiverStateForSecondLink.template cast< double >( );

        // ==================== FDOA COMPUTATION (FULL RELATIVISTIC) ====================
        
        // Retrieve transmitter frequency - prefer interpolator, fall back to ancillary settings
        ObservationScalarType transmitterFrequency;
        
        if( frequencyInterpolator_ != nullptr )
        {
            // Use frequency interpolator at transmitter time
            transmitterFrequency = frequencyInterpolator_->
                template getTemplatedCurrentFrequency< ObservationScalarType, double >( linkEndTimes[ 0 ] );
            
            // Store in ancillary settings for partials computation (if settings object exists)
            if( ancilliarySetingsInput != nullptr )
            {
                ancilliarySetingsInput->setAncilliaryDoubleData( 
                    transmitter_frequency, 
                    static_cast< double >( transmitterFrequency ) );
            }
        }
        else if( ancilliarySetingsInput != nullptr )
        {
            // Graceful degradation: warn and fall back to ancillary settings
            static bool warningIssued = false;
            if( !warningIssued )
            {
                std::cerr << "Warning: FDOA observation model has no frequency interpolator set. "
                          << "Falling back to ancillary settings for transmitter frequency. "
                          << "This warning is only shown once." << std::endl;
                warningIssued = true;
            }
            
            try
            {
                transmitterFrequency = static_cast< ObservationScalarType >(
                    ancilliarySetingsInput->getAncilliaryDoubleData( transmitter_frequency, true ) );
            }
            catch( const std::runtime_error& caughtException )
            {
                throw std::runtime_error(
                    "Error when retrieving transmitter frequency for FDOA observable from ancillary settings: " +
                    std::string( caughtException.what( ) ) );
            }
        }
        else
        {
            throw std::runtime_error( 
                "Error in FDOA observation: no frequency interpolator set and no ancillary settings provided." );
        }
        
        // Get speed of light
        ObservationScalarType c = physical_constants::getSpeedOfLight< ObservationScalarType >( );
        
        // Extract positions and velocities for all three link ends
        PositionType r_t = transmitterStateForFirstLink.template head< 3 >( );  // Transmitter position
        PositionType v_t = transmitterStateForFirstLink.template tail< 3 >( );  // Transmitter velocity
        
        PositionType r_1 = receiverStateForFirstLink.template head< 3 >( );    // Receiver 1 position
        PositionType v_1 = receiverStateForFirstLink.template tail< 3 >( );    // Receiver 1 velocity
        
        PositionType r_2 = receiverStateForSecondLink.template head< 3 >( );   // Receiver 2 position
        PositionType v_2 = receiverStateForSecondLink.template tail< 3 >( );   // Receiver 2 velocity
        
        // Calculate position vectors from receivers to transmitter
        PositionType rho_1 = r_t - r_1;  // Vector from receiver 1 to transmitter
        PositionType rho_2 = r_t - r_2;  // Vector from receiver 2 to transmitter
        
        // Calculate ranges and unit vectors
        ObservationScalarType norm_rho_1 = rho_1.norm( );
        ObservationScalarType norm_rho_2 = rho_2.norm( );
        
        PositionType rho_1_hat = rho_1 / norm_rho_1;  // Unit vector receiver 1 → transmitter
        PositionType rho_2_hat = rho_2 / norm_rho_2;  // Unit vector receiver 2 → transmitter
        
        // Calculate relative velocities
        PositionType v_rel_1 = v_t - v_1;  // Transmitter velocity relative to receiver 1
        PositionType v_rel_2 = v_t - v_2;  // Transmitter velocity relative to receiver 2
        
        // Calculate beta values (normalized line-of-sight velocity components)
        // beta = (v · n̂) / c where positive means closing (approaching)
        ObservationScalarType beta_1 = rho_1_hat.dot( v_rel_1 ) / c;
        ObservationScalarType beta_2 = rho_2_hat.dot( v_rel_2 ) / c;
        
        // Full relativistic Doppler formula (including time dilation)
        // f_received / f_transmitted = sqrt((1 + beta) / (1 - beta))
        // For FDOA, we compute the difference of these ratios
        
        ObservationScalarType one_plus_beta_1 = mathematical_constants::getFloatingInteger< ObservationScalarType >( 1 ) + beta_1;
        ObservationScalarType one_minus_beta_1 = mathematical_constants::getFloatingInteger< ObservationScalarType >( 1 ) - beta_1;
        ObservationScalarType one_plus_beta_2 = mathematical_constants::getFloatingInteger< ObservationScalarType >( 1 ) + beta_2;
        ObservationScalarType one_minus_beta_2 = mathematical_constants::getFloatingInteger< ObservationScalarType >( 1 ) - beta_2;
        
        // Calculate Doppler ratios with full relativistic formula
        ObservationScalarType doppler_ratio_1 = std::sqrt( one_plus_beta_1 / one_minus_beta_1 );
        ObservationScalarType doppler_ratio_2 = std::sqrt( one_plus_beta_2 / one_minus_beta_2 );
        
        // Compute fractional frequency difference (dimensionless)
        // FDOA_fractional = (f_2 / f_tx) - (f_1 / f_tx) = doppler_ratio_2 - doppler_ratio_1
        ObservationScalarType fdoa_fractional = doppler_ratio_2 - doppler_ratio_1;
        
        // Scale by transmitter frequency to get absolute frequency difference in Hz
        // FDOA [Hz] = f_tx [Hz] × FDOA_fractional
        ObservationScalarType fdoa_hz = fdoa_fractional * transmitterFrequency;
        
        // ==================== END FDOA COMPUTATION ====================

        // Return absolute frequency difference in Hz
        return ( Eigen::Matrix< ObservationScalarType, 1, 1 >( ) << fdoa_hz ).finished( );
 
    }

    //! Light time calculator to compute light time at the beginning of the integration time
    std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > getFirstReceiverLightTimeCalculator( )
    {
        return firstReceiverLightTimeCalculator_;
    }

    //! Light time calculator to compute light time at the end of the integration time
    std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > getSecondReceiverLightTimeCalculator( )
    {
        return secondReceiverLightTimeCalculator_;
    }

private:
    //! Light time calculator to compute light time at the beginning of the integration time
    std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > firstReceiverLightTimeCalculator_;

    //! Light time calculator to compute light time at the end of the integration time
    std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > secondReceiverLightTimeCalculator_;

    std::map< LinkEndType, std::shared_ptr< ground_stations::GroundStationState > > stationStates_;
    
    basic_astrodynamics::TimeScales observableTimeScale_;
};

}  // namespace observation_models

}  // namespace tudat

#endif  // TUDAT_DIFFERENCEDFREQUENCYOFARRIVALOBSERVATIONMODEL_H

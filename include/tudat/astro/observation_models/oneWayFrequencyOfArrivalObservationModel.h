/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_ONEWAYFREQUENCYOFARRIVALOBSERVATIONMODEL_H
#define TUDAT_ONEWAYFREQUENCYOFARRIVALOBSERVATIONMODEL_H

#include <map>
#include <iostream>

#include <functional>

#include <Eigen/Core>

#include "tudat/astro/basic_astro/physicalConstants.h"
#include "tudat/astro/observation_models/observationModel.h"
#include "tudat/astro/observation_models/lightTimeSolution.h"
#include "tudat/astro/earth_orientation/terrestrialTimeScaleConverter.h"
#include "tudat/astro/basic_astro/timeConversions.h"

namespace tudat
{

namespace observation_models
{

template< typename ObservationScalarType = double, typename TimeType = double >
class OneWayFrequencyOfArrivalObservationModel : public ObservationModel< 1, ObservationScalarType, TimeType >
{
public:
    typedef Eigen::Matrix< ObservationScalarType, 6, 1 > StateType;
    typedef Eigen::Matrix< ObservationScalarType, 3, 1 > PositionType;

    using ObservationModel< 1, ObservationScalarType, TimeType >::frequencyInterpolator_;
    using ObservationModel< 1, ObservationScalarType, TimeType >::timeScaleConverter_;

    OneWayFrequencyOfArrivalObservationModel(
            const LinkEnds& linkEnds,
            const std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > >
                    lightTimeCalculator,
            const std::shared_ptr< ObservationBias< 1 > > observationBiasCalculator = nullptr,
            const std::map< LinkEndType, std::shared_ptr< ground_stations::GroundStationState > >& stationStates =
                    std::map< LinkEndType, std::shared_ptr< ground_stations::GroundStationState > >( ),
            const basic_astrodynamics::TimeScales observableTimeScale = basic_astrodynamics::tdb_scale ):
        ObservationModel< 1, ObservationScalarType, TimeType >( one_way_frequency_of_arrival, linkEnds, observationBiasCalculator ),
        lightTimeCalculator_( lightTimeCalculator ), stationStates_( stationStates ),
        observableTimeScale_( observableTimeScale )
    {
        if( observableTimeScale_ == basic_astrodynamics::utc_scale || observableTimeScale_ == basic_astrodynamics::ut1_scale )
        {
            if( stationStates.count( receiver ) == 0 )
            {
                throw std::runtime_error(
                        "Error when making differenced frequency of arrival observation model, no state model found for receiver " +
                        linkEnds.at( receiver ).bodyName_ + ", " + linkEnds.at( receiver ).stationName_ );
            }

            if (stationStates.count( transmitter ) == 0)
            {
                throw std::runtime_error(
                        "Error when making one way frequency of arrival observation model, no state model found for transmitter " +
                        linkEnds.at( transmitter ).bodyName_ + ", " + linkEnds.at( transmitter ).stationName_ );
            }
        }
    }

    //! Destructor
    ~OneWayFrequencyOfArrivalObservationModel( ) {}

    Eigen::Matrix< ObservationScalarType, 1, 1 > computeIdealObservationsWithLinkEndData(
            const TimeType time,
            const LinkEndType linkEndAssociatedWithTime,
            std::vector< double >& linkEndTimes,
            std::vector< Eigen::Matrix< double, 6, 1 > >& linkEndStates,
            const std::shared_ptr< ObservationAncilliarySimulationSettings > ancilliarySetingsInput = nullptr )
    {
        ObservationScalarType lightTimeForFirstReceiver;
        ObservationScalarType lightTimeForSecondReceiver;

        // Resize link end data vectors to 2 (Transmitter, Receiver)
        linkEndTimes.resize( 2 );
        linkEndStates.resize( 2 );

        StateType transmitterState, receiverState;
        if( linkEndAssociatedWithTime == receiver )
        {
            // Calculate reception time at ground station at the start and end of the count interval at reception time.
            linkEndTimes[ 1 ] = static_cast< double >( time );

            std::shared_ptr< ObservationAncilliarySimulationSettings > ancilliarySetings;
            this->setFrequencyProperties( time, receiver, lightTimeCalculator_, ancilliarySetingsInput, ancilliarySetings );
            lightTimeForFirstReceiver = lightTimeCalculator_->calculateLightTimeWithLinkEndsStates(
                    receiverState, transmitterState, time, 1, ancilliarySetings );

            linkEndTimes[ 0 ] = linkEndTimes[ 1 ] - static_cast< double >( lightTimeForFirstReceiver );
        }
        else
        {
            throw std::runtime_error( "Error in differenced frequency of arrival, reference link end not recognized" );
        }

        linkEndStates[ 0 ] = transmitterState.template cast< double >( );
        linkEndStates[ 1 ] = receiverState.template cast< double >( );

        // ==================== FOA COMPUTATION (FULL RELATIVISTIC) ====================

        // Retrieve transmitter frequency - Require interpolator to be set
        ObservationScalarType transmitterFrequency;

        if( frequencyInterpolator_ == nullptr )
        {
            throw std::runtime_error( "Error in FDOA observation: no frequency interpolator has been set." );
        }

        // Convert time at transmitter to UTC for frequency interpolation as is required by the interpolator
        TimeType timeAtTransmitter;
        if( observableTimeScale_ != basic_astrodynamics::utc_scale )
        {
            timeAtTransmitter = timeScaleConverter_->template getCurrentTime< TimeType >( basic_astrodynamics::tdb_scale,
                                                                                     basic_astrodynamics::utc_scale,
                                                                                     linkEndTimes[ 0 ],
                                                                                     transmitterState.head( 3 ) );
        }
        else
        {
            timeAtTransmitter = linkEndTimes[ 0 ];
        }
        transmitterFrequency = frequencyInterpolator_->template getTemplatedCurrentFrequency< ObservationScalarType, TimeType >( timeAtTransmitter );

        // Get speed of light
        ObservationScalarType c = physical_constants::getSpeedOfLight< ObservationScalarType >( );

        // Extract positions and velocities for all three link ends
        PositionType r_t = transmitterState.template head< 3 >( );  // Transmitter position
        PositionType v_t = transmitterState.template tail< 3 >( );  // Transmitter velocity

        PositionType r_r = receiverState.template head< 3 >( );  // Receiver position
        PositionType v_r = receiverState.template tail< 3 >( );  // Receiver velocity

        // Calculate position vectors from receivers to transmitter
        PositionType rho = r_t - r_r;  // Vector from receiver to transmitter
        // Calculate ranges and unit vectors
        ObservationScalarType norm_rho = rho.norm( );

        PositionType rho_hat = rho / norm_rho;  // Unit vector receiver - transmitter

        // Calculate relative velocities
        PositionType v_rel = v_t - v_r;  // Transmitter velocity relative to receiver

        // Calculate beta values (normalized line-of-sight velocity components)
        // beta = (v · n̂) / c where positive means closing (approaching)
        ObservationScalarType beta = rho_hat.dot( v_rel ) / c;

        // Full relativistic Doppler formula (including time dilation)
        // f_received / f_transmitted = sqrt((1 + beta) / (1 - beta))

        ObservationScalarType one_plus_beta = mathematical_constants::getFloatingInteger< ObservationScalarType >( 1 ) + beta;
        ObservationScalarType one_minus_beta = mathematical_constants::getFloatingInteger< ObservationScalarType >( 1 ) - beta;

        // Calculate Doppler ratios with full relativistic formula
        ObservationScalarType doppler_ratio = std::sqrt( one_plus_beta / one_minus_beta );

        // Scale by transmitter frequency to get absolute frequency in Hz
        // FOA [Hz] = f_tx [Hz] × doppler_ratio
        ObservationScalarType foa_hz = (2 - doppler_ratio) * transmitterFrequency;

        // ==================== END FOA COMPUTATION ====================
        // Return absolute frequency in Hz
        return ( Eigen::Matrix< ObservationScalarType, 1, 1 >( ) << foa_hz ).finished( );
    }

    //! Light time calculator to compute light time at the beginning of the integration time
    std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > getLightTimeCalculator( )
    {
        return lightTimeCalculator_;
    }


private:
    //! Light time calculator to compute light time at the beginning of the integration time
    std::shared_ptr< observation_models::LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculator_;

    std::map< LinkEndType, std::shared_ptr< ground_stations::GroundStationState > > stationStates_;

    basic_astrodynamics::TimeScales observableTimeScale_;
};

}  // namespace observation_models

}  // namespace tudat

#endif  // TUDAT_DIFFERENCEDFREQUENCYOFARRIVALOBSERVATIONMODEL_H

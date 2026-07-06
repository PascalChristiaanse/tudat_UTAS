# Implementation Plan: Position Angle and Separation Observation Models

## Overview

Three new observation models for optical astrometry:
- **Position Angle** (size 1) — `position_angle = 21`
- **Angular Separation** (size 1) — `separation = 22`
- **Position Angle and Separation** (size 2) — `position_angle_and_separation = 23`

All three use 3 link ends: `transmitter`, `transmitter2`, `receiver` (same geometry as `relative_angular_position`).

---

## Mathematical Formulation

From two relative position vectors:

$$\begin{aligned}
\Delta\mathbf{r}_1 &= \mathbf{r}_{\text{receiver}} - \mathbf{r}_{\text{transmitter}} \\
\Delta\mathbf{r}_2 &= \mathbf{r}_{\text{receiver}} - \mathbf{r}_{\text{transmitter2}} \\
\alpha_i &= \operatorname{atan2}(\Delta r_{i,y}, \Delta r_{i,x}) \\
\delta_i &= \operatorname{atan2}(\Delta r_{i,z}, \sqrt{\Delta r_{i,x}^2 + \Delta r_{i,y}^2}) \\
\Delta\alpha &= \alpha_2 - \alpha_1
\end{aligned}$$

**Position Angle** (measured from north through east, i.e. from $\delta_1$ towards $\delta_2$):

$$\text{PA} = \operatorname{atan2}\!\big(\sin\Delta\alpha \cdot \cos\delta_2,\; \cos\delta_1 \cdot \sin\delta_2 - \sin\delta_1 \cdot \cos\delta_2 \cdot \cos\Delta\alpha\big)$$

**Angular Separation**:

$$\text{Sep} = \arccos\!\big(\sin\delta_1 \sin\delta_2 + \cos\delta_1 \cos\delta_2 \cos\Delta\alpha\big)$$

---

## Files Summary

| Action | File |
|---|---|
| **Edit** | `include/tudat/astro/observation_models/observableTypes.h` |
| **Edit** | `src/tudat/astro/observation_models/observableTypes.cpp` |
| **Create** | `include/tudat/astro/observation_models/positionAngleObservationModel.h` |
| **Create** | `include/tudat/astro/observation_models/separationObservationModel.h` |
| **Create** | `include/tudat/astro/observation_models/positionAngleAndSeparationObservationModel.h` |
| **Create** | `include/tudat/astro/orbit_determination/observation_partials/positionAngleAndSeparationPartial.h` |
| **Create** | `src/tudat/astro/orbit_determination/observation_partials/positionAngleAndSeparationPartial.cpp` |
| **Edit** | `include/tudat/simulation/estimation_setup/createObservationModelSettings.h` |
| **Edit** | `include/tudat/simulation/estimation_setup/createObservationModelFactory.h` |
| **Edit** | `include/tudat/simulation/estimation_setup/createObservationPartials.h` |
| **Edit** | `include/tudat/simulation/estimation_setup/createPositionPartialScaling.h` |
| **Edit** | `src/tudatpy/estimation/observable_models_setup/model_settings/expose_model_settings.cpp` |
| **Create** | `tests/test_tudat/src/astro/observation_models/unitTestPositionAngleAndSeparationObservationModel.cpp` |
| **Create** | `tests/test_tudat/src/astro/orbit_determination/observation_partials/unitTestPositionAngleAndSeparationPartials.cpp` |
| **Edit** | `tests/test_tudat/src/astro/observation_models/CMakeLists.txt` |
| **Edit** | `tests/test_tudat/src/astro/orbit_determination/observation_partials/CMakeLists.txt` |

---

## Part 1: Observable Type Enum

### File: `include/tudat/astro/observation_models/observableTypes.h`

**Edit:** Add three new values to the `ObservableType` enum after `pixel_coordinates = 20`:

```cpp
    pixel_coordinates = 20,
    position_angle = 21,
    separation = 22,
    position_angle_and_separation = 23
};
```

---

## Part 2: Observable Type Support Functions

### File: `src/tudat/astro/observation_models/observableTypes.cpp`

This file has ~20 switch statements that all need new cases. The key functions and their behavior:

#### `getObservableName` (line ~62 area)
```cpp
case position_angle:
    return "PositionAngle";
case separation:
    return "Separation";
case position_angle_and_separation:
    return "PositionAngleAndSeparation";
```

#### `getObservableType` (line ~682 area, string-to-enum parsing)
```cpp
if( observableName == "PositionAngle" ) { observableType = position_angle; }
else if( observableName == "Separation" ) { observableType = separation; }
else if( observableName == "PositionAngleAndSeparation" ) { observableType = position_angle_and_separation; }
```

#### `getObservableSize` (line ~104 area)
```cpp
case position_angle:
    return 1;
case separation:
    return 1;
case position_angle_and_separation:
    return 2;
```

#### `isObservableTypeMultiLink` (line ~142 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return true;
```

#### `isObservableOfIntegratedType` (line ~164 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return false;
```

#### `doesLinkEndTypeDefineId` (line ~222 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return false;
```

#### `requiresTransmittingStation` (line ~247 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return false;
```

#### `requiresFirstReceivingStation` (line ~283 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return false;
```

#### `requiresSecondReceivingStation` (line ~316 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return false;
```

#### `getUndifferencedObservableType` (line ~355 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return angular_position;
```

#### `getUnconcatenatedObservableType` (line ~394 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return angular_position;
```

#### `getBaseObservableType` (line ~408 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return angular_position;
```

#### `getUndifferencedTimeAndStateIndices` (line ~436 area)
For 3-link-end case (numberOfLinkEnds == 3):
```cpp
case position_angle:
case separation:
case position_angle_and_separation:
    return std::make_pair(
        std::vector< int >({ 0, 2 }),   // first: transmitter + receiver
        std::vector< int >({ 1, 2 }) ); // second: transmitter2 + receiver
```

#### `getUndifferencedLinkEnds` (line ~586 area)
```cpp
case position_angle:
case separation:
case position_angle_and_separation: {
    LinkEnds firstLinkEnds;
    firstLinkEnds[ transmitter ] = differencedLinkEnds.at( transmitter );
    firstLinkEnds[ receiver ] = differencedLinkEnds.at( receiver );
    LinkEnds secondLinkEnds;
    secondLinkEnds[ transmitter ] = differencedLinkEnds.at( transmitter2 );
    secondLinkEnds[ receiver ] = differencedLinkEnds.at( receiver );
    return std::make_pair( firstLinkEnds, secondLinkEnds );
}
```

#### Remaining switch statements to update:
- `getLinkEndIndicesForObservable` (line ~717 area)
- `getLinkEndTypeForIndex` (line ~773 area)
- `getLinkEndIndexForType` (line ~786 area)
- `getNumberOfLinkEndsForObservable` (line ~812 area)
- `getLinkEndTypeOrderForObservable` (line ~931 area)
- `getReferenceLinkEndForObservable` (line ~994 area)
- `getObservableMultiLinkEnds` (line ~1182 area)
- `getObservableLinkEndsFromMultiLinkEnds` (line ~1258 area)
- `getMultiLinkEndsFromObservableLinkEnds` (line ~1355 area)
- `getObservableLinkEndsFromMultiLinkEndsAndIndex` (line ~1662 area)
- `getMultiLinkEndsFromObservableLinkEndsAndIndex` (line ~1800 area)

For all of these, follow the same pattern as `relative_angular_position` (which also has 3 link ends: transmitter, transmitter2, receiver).

---

## Part 3: C++ Observation Model Headers

### File: `include/tudat/astro/observation_models/positionAngleObservationModel.h` (NEW)

```cpp
#ifndef TUDAT_POSITIONANGLEOBSERVATIONMODEL_H
#define TUDAT_POSITIONANGLEOBSERVATIONMODEL_H

#include <map>
#include <Eigen/Core>

#include "tudat/math/basic/coordinateConversions.h"
#include "tudat/astro/observation_models/lightTimeSolution.h"
#include "tudat/astro/observation_models/observationModel.h"

namespace tudat
{

namespace observation_models
{

inline double getPositionAngleScalingFactor(
    const observation_models::LinkEndType referenceLinkEnd,
    const std::vector< Eigen::Vector6d >& linkEndStates,
    const std::vector< double >& linkEndTimes,
    const std::shared_ptr< ObservationAncillarySimulationSettings > ancillarySettings,
    const bool isFirstPartial )
{
    return 1.0;
}

template< typename ObservationScalarType = double, typename TimeType = double >
class PositionAngleObservationModel : public ObservationModel< 1, ObservationScalarType, TimeType >
{
public:
    typedef Eigen::Matrix< ObservationScalarType, 6, 1 > StateType;

    static std::vector< std::shared_ptr< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > > >
    createFullLinkLightTimeCalculators(
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorFirstTransmitter,
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorSecondTransmitter )
    {
        return std::vector< std::shared_ptr< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > > >{
            std::make_shared< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > >(
                std::vector< std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > >{
                    lightTimeCalculatorFirstTransmitter },
                std::make_shared< LightTimeConvergenceCriteria >( ), false ),
            std::make_shared< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > >(
                std::vector< std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > >{
                    lightTimeCalculatorSecondTransmitter },
                std::make_shared< LightTimeConvergenceCriteria >( ), false )
        };
    }

    PositionAngleObservationModel(
        const LinkEnds linkEnds,
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorFirstTransmitter,
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorSecondTransmitter,
        const std::shared_ptr< ObservationBias< 1 > > observationBiasCalculator = nullptr ):
        ObservationModel< 1, ObservationScalarType, TimeType >(
            position_angle, linkEnds, observationBiasCalculator,
            createFullLinkLightTimeCalculators( lightTimeCalculatorFirstTransmitter, lightTimeCalculatorSecondTransmitter ) )
    {}

    ~PositionAngleObservationModel( ) {}

    Eigen::Matrix< ObservationScalarType, 1, 1 > computeIdealObservationsWithLinkEndData(
        const TimeType time,
        const LinkEndType linkEndAssociatedWithTime,
        std::vector< double >& linkEndTimes,
        std::vector< Eigen::Matrix< double, 6, 1 > >& linkEndStates,
        const std::shared_ptr< ObservationAncillarySimulationSettings > ancillarySettingsInput = nullptr ) override
    {
        if( linkEndAssociatedWithTime != receiver )
        {
            throw std::runtime_error(
                "Error when calculating position angle observation, link end associated with time is not receiver." );
        }

        // Compute light-times for both transmitter-receiver paths
        std::shared_ptr< ObservationAncillarySimulationSettings > ancillarySettings;
        std::vector< double > firstLinkEndTimes, secondLinkEndTimes;
        std::vector< Eigen::Matrix< double, 6, 1 > > firstLinkEndStates, secondLinkEndStates;

        this->setFrequencyProperties( time, linkEndAssociatedWithTime,
            getLightTimeCalculatorFirstTransmitter( ), ancillarySettingsInput, ancillarySettings );
        this->getFullLinkLightTimeCalculatorFromBase( 0 )->calculateLightTimeWithLinkEndsStates(
            time, linkEndAssociatedWithTime, firstLinkEndTimes, firstLinkEndStates, ancillarySettings );

        this->setFrequencyProperties( time, linkEndAssociatedWithTime,
            getLightTimeCalculatorSecondTransmitter( ), ancillarySettingsInput, ancillarySettings );
        this->getFullLinkLightTimeCalculatorFromBase( 1 )->calculateLightTimeWithLinkEndsStates(
            time, linkEndAssociatedWithTime, secondLinkEndTimes, secondLinkEndStates, ancillarySettings );

        Eigen::Matrix< ObservationScalarType, 6, 1 > receiverState =
            firstLinkEndStates.at( 1 ).template cast< ObservationScalarType >( );
        Eigen::Matrix< ObservationScalarType, 6, 1 > firstTransmitterState =
            firstLinkEndStates.at( 0 ).template cast< ObservationScalarType >( );
        Eigen::Matrix< ObservationScalarType, 6, 1 > secondTransmitterState =
            secondLinkEndStates.at( 0 ).template cast< ObservationScalarType >( );

        // Compute relative position vectors
        Eigen::Matrix< ObservationScalarType, 3, 1 > relativeState1 =
            firstTransmitterState.segment( 0, 3 ) - receiverState.segment( 0, 3 );
        Eigen::Matrix< ObservationScalarType, 3, 1 > relativeState2 =
            secondTransmitterState.segment( 0, 3 ) - receiverState.segment( 0, 3 );

        // Compute RA/Dec for both transmitters (robust atan formulation)
        double rightAscension1 = 2.0 * std::atan(
            relativeState1[ 1 ] / ( std::sqrt( relativeState1[ 0 ] * relativeState1[ 0 ] +
                relativeState1[ 1 ] * relativeState1[ 1 ] ) + relativeState1[ 0 ] ) );
        double declination1 = mathematical_constants::PI / 2.0 -
            std::acos( relativeState1[ 2 ] / relativeState1.norm( ) );

        double rightAscension2 = 2.0 * std::atan(
            relativeState2[ 1 ] / ( std::sqrt( relativeState2[ 0 ] * relativeState2[ 0 ] +
                relativeState2[ 1 ] * relativeState2[ 1 ] ) + relativeState2[ 0 ] ) );
        double declination2 = mathematical_constants::PI / 2.0 -
            std::acos( relativeState2[ 2 ] / relativeState2.norm( ) );

        double deltaRA = rightAscension2 - rightAscension1;

        // Compute position angle: atan2(sin(dRA)*cos(d2), cos(d1)*sin(d2) - sin(d1)*cos(d2)*cos(dRA))
        double positionAngle = std::atan2(
            std::sin( deltaRA ) * std::cos( declination2 ),
            std::cos( declination1 ) * std::sin( declination2 ) -
                std::sin( declination1 ) * std::cos( declination2 ) * std::cos( deltaRA ) );

        // Set link end times and states
        linkEndTimes.clear( );
        linkEndStates.clear( );
        linkEndStates.push_back( firstLinkEndStates.at( 0 ) );   // transmitter
        linkEndStates.push_back( secondLinkEndStates.at( 0 ) );  // transmitter2
        linkEndStates.push_back( firstLinkEndStates.at( 1 ) );   // receiver
        linkEndTimes.push_back( firstLinkEndTimes.at( 0 ) );
        linkEndTimes.push_back( secondLinkEndTimes.at( 0 ) );
        linkEndTimes.push_back( firstLinkEndTimes.at( 1 ) );

        return ( Eigen::Matrix< ObservationScalarType, 1, 1 >( ) << positionAngle ).finished( );
    }

    std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > getLightTimeCalculatorFirstTransmitter( )
    {
        return this->getSingleLegLightTimeCalculator( 0, 0 );
    }

    std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > getLightTimeCalculatorSecondTransmitter( )
    {
        return this->getSingleLegLightTimeCalculator( 1, 0 );
    }

    LinkEnds getFirstLinkEnds( )
    {
        LinkEnds firstLinkEnds;
        firstLinkEnds[ transmitter ] = this->linkEnds_[ transmitter ];
        firstLinkEnds[ receiver ] = this->linkEnds_[ receiver ];
        return firstLinkEnds;
    }

    LinkEnds getSecondLinkEnds( )
    {
        LinkEnds secondLinkEnds;
        secondLinkEnds[ transmitter ] = this->linkEnds_[ transmitter2 ];
        secondLinkEnds[ receiver ] = this->linkEnds_[ receiver ];
        return secondLinkEnds;
    }

    std::map< std::pair< LinkEndType, LinkEndType >, std::vector< std::shared_ptr< LightTimeCalculatorBase > > >
    getLegLightTimeCalculators( ) const override
    {
        return {
            { std::make_pair( transmitter, receiver ), { this->getSingleLegLightTimeCalculator( 0, 0 ) } },
            { std::make_pair( transmitter2, receiver ), { this->getSingleLegLightTimeCalculator( 1, 0 ) } }
        };
    }
};

}  // namespace observation_models

}  // namespace tudat

#endif  // TUDAT_POSITIONANGLEOBSERVATIONMODEL_H
```

### File: `include/tudat/astro/observation_models/separationObservationModel.h` (NEW)

Identical structure to `PositionAngleObservationModel` but:
- `ObservableType = separation`
- Scaling function named `getSeparationScalingFactor`
- Class named `SeparationObservationModel`
- Computes separation instead of position angle:

```cpp
// Compute angular separation: acos(sin(d1)*sin(d2) + cos(d1)*cos(d2)*cos(dRA))
double separation = std::acos(
    std::sin( declination1 ) * std::sin( declination2 ) +
    std::cos( declination1 ) * std::cos( declination2 ) * std::cos( deltaRA ) );
```

### File: `include/tudat/astro/observation_models/positionAngleAndSeparationObservationModel.h` (NEW)

Template class extending `ObservationModel< 2, ObservationScalarType, TimeType >`:

```cpp
#ifndef TUDAT_POSITIONANGLEANDSEPARATIONOBSERVATIONMODEL_H
#define TUDAT_POSITIONANGLEANDSEPARATIONOBSERVATIONMODEL_H

#include <map>
#include <Eigen/Core>

#include "tudat/math/basic/coordinateConversions.h"
#include "tudat/astro/observation_models/lightTimeSolution.h"
#include "tudat/astro/observation_models/observationModel.h"

namespace tudat
{

namespace observation_models
{

inline double getPositionAngleAndSeparationScalingFactor(
    const observation_models::LinkEndType referenceLinkEnd,
    const std::vector< Eigen::Vector6d >& linkEndStates,
    const std::vector< double >& linkEndTimes,
    const std::shared_ptr< ObservationAncillarySimulationSettings > ancillarySettings,
    const bool isFirstPartial )
{
    return 1.0;
}

template< typename ObservationScalarType = double, typename TimeType = double >
class PositionAngleAndSeparationObservationModel : public ObservationModel< 2, ObservationScalarType, TimeType >
{
public:
    typedef Eigen::Matrix< ObservationScalarType, 6, 1 > StateType;

    static std::vector< std::shared_ptr< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > > >
    createFullLinkLightTimeCalculators(
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorFirstTransmitter,
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorSecondTransmitter )
    {
        return std::vector< std::shared_ptr< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > > >{
            std::make_shared< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > >(
                std::vector< std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > >{
                    lightTimeCalculatorFirstTransmitter },
                std::make_shared< LightTimeConvergenceCriteria >( ), false ),
            std::make_shared< FullLinkLightTimeCalculator< ObservationScalarType, TimeType > >(
                std::vector< std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > >{
                    lightTimeCalculatorSecondTransmitter },
                std::make_shared< LightTimeConvergenceCriteria >( ), false )
        };
    }

    PositionAngleAndSeparationObservationModel(
        const LinkEnds linkEnds,
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorFirstTransmitter,
        const std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > lightTimeCalculatorSecondTransmitter,
        const std::shared_ptr< ObservationBias< 2 > > observationBiasCalculator = nullptr ):
        ObservationModel< 2, ObservationScalarType, TimeType >(
            position_angle_and_separation, linkEnds, observationBiasCalculator,
            createFullLinkLightTimeCalculators( lightTimeCalculatorFirstTransmitter, lightTimeCalculatorSecondTransmitter ) )
    {}

    ~PositionAngleAndSeparationObservationModel( ) {}

    Eigen::Matrix< ObservationScalarType, 2, 1 > computeIdealObservationsWithLinkEndData(
        const TimeType time,
        const LinkEndType linkEndAssociatedWithTime,
        std::vector< double >& linkEndTimes,
        std::vector< Eigen::Matrix< double, 6, 1 > >& linkEndStates,
        const std::shared_ptr< ObservationAncillarySimulationSettings > ancillarySettingsInput = nullptr ) override
    {
        if( linkEndAssociatedWithTime != receiver )
        {
            throw std::runtime_error(
                "Error when calculating position angle and separation observation, link end associated with time is not receiver." );
        }

        // Compute light-times for both transmitter-receiver paths
        std::shared_ptr< ObservationAncillarySimulationSettings > ancillarySettings;
        std::vector< double > firstLinkEndTimes, secondLinkEndTimes;
        std::vector< Eigen::Matrix< double, 6, 1 > > firstLinkEndStates, secondLinkEndStates;

        this->setFrequencyProperties( time, linkEndAssociatedWithTime,
            getLightTimeCalculatorFirstTransmitter( ), ancillarySettingsInput, ancillarySettings );
        this->getFullLinkLightTimeCalculatorFromBase( 0 )->calculateLightTimeWithLinkEndsStates(
            time, linkEndAssociatedWithTime, firstLinkEndTimes, firstLinkEndStates, ancillarySettings );

        this->setFrequencyProperties( time, linkEndAssociatedWithTime,
            getLightTimeCalculatorSecondTransmitter( ), ancillarySettingsInput, ancillarySettings );
        this->getFullLinkLightTimeCalculatorFromBase( 1 )->calculateLightTimeWithLinkEndsStates(
            time, linkEndAssociatedWithTime, secondLinkEndTimes, secondLinkEndStates, ancillarySettings );

        Eigen::Matrix< ObservationScalarType, 6, 1 > receiverState =
            firstLinkEndStates.at( 1 ).template cast< ObservationScalarType >( );
        Eigen::Matrix< ObservationScalarType, 6, 1 > firstTransmitterState =
            firstLinkEndStates.at( 0 ).template cast< ObservationScalarType >( );
        Eigen::Matrix< ObservationScalarType, 6, 1 > secondTransmitterState =
            secondLinkEndStates.at( 0 ).template cast< ObservationScalarType >( );

        // Compute relative position vectors
        Eigen::Matrix< ObservationScalarType, 3, 1 > relativeState1 =
            firstTransmitterState.segment( 0, 3 ) - receiverState.segment( 0, 3 );
        Eigen::Matrix< ObservationScalarType, 3, 1 > relativeState2 =
            secondTransmitterState.segment( 0, 3 ) - receiverState.segment( 0, 3 );

        // Compute RA/Dec for both transmitters
        double rightAscension1 = 2.0 * std::atan(
            relativeState1[ 1 ] / ( std::sqrt( relativeState1[ 0 ] * relativeState1[ 0 ] +
                relativeState1[ 1 ] * relativeState1[ 1 ] ) + relativeState1[ 0 ] ) );
        double declination1 = mathematical_constants::PI / 2.0 -
            std::acos( relativeState1[ 2 ] / relativeState1.norm( ) );

        double rightAscension2 = 2.0 * std::atan(
            relativeState2[ 1 ] / ( std::sqrt( relativeState2[ 0 ] * relativeState2[ 0 ] +
                relativeState2[ 1 ] * relativeState2[ 1 ] ) + relativeState2[ 0 ] ) );
        double declination2 = mathematical_constants::PI / 2.0 -
            std::acos( relativeState2[ 2 ] / relativeState2.norm( ) );

        double deltaRA = rightAscension2 - rightAscension1;

        // Compute position angle
        double positionAngle = std::atan2(
            std::sin( deltaRA ) * std::cos( declination2 ),
            std::cos( declination1 ) * std::sin( declination2 ) -
                std::sin( declination1 ) * std::cos( declination2 ) * std::cos( deltaRA ) );

        // Compute angular separation
        double separation = std::acos(
            std::sin( declination1 ) * std::sin( declination2 ) +
            std::cos( declination1 ) * std::cos( declination2 ) * std::cos( deltaRA ) );

        // Set link end times and states
        linkEndTimes.clear( );
        linkEndStates.clear( );
        linkEndStates.push_back( firstLinkEndStates.at( 0 ) );   // transmitter
        linkEndStates.push_back( secondLinkEndStates.at( 0 ) );  // transmitter2
        linkEndStates.push_back( firstLinkEndStates.at( 1 ) );   // receiver
        linkEndTimes.push_back( firstLinkEndTimes.at( 0 ) );
        linkEndTimes.push_back( secondLinkEndTimes.at( 0 ) );
        linkEndTimes.push_back( firstLinkEndTimes.at( 1 ) );

        return ( Eigen::Matrix< ObservationScalarType, 2, 1 >( ) << positionAngle, separation ).finished( );
    }

    std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > getLightTimeCalculatorFirstTransmitter( )
    {
        return this->getSingleLegLightTimeCalculator( 0, 0 );
    }

    std::shared_ptr< LightTimeCalculator< ObservationScalarType, TimeType > > getLightTimeCalculatorSecondTransmitter( )
    {
        return this->getSingleLegLightTimeCalculator( 1, 0 );
    }

    LinkEnds getFirstLinkEnds( )
    {
        LinkEnds firstLinkEnds;
        firstLinkEnds[ transmitter ] = this->linkEnds_[ transmitter ];
        firstLinkEnds[ receiver ] = this->linkEnds_[ receiver ];
        return firstLinkEnds;
    }

    LinkEnds getSecondLinkEnds( )
    {
        LinkEnds secondLinkEnds;
        secondLinkEnds[ transmitter ] = this->linkEnds_[ transmitter2 ];
        secondLinkEnds[ receiver ] = this->linkEnds_[ receiver ];
        return secondLinkEnds;
    }

    std::map< std::pair< LinkEndType, LinkEndType >, std::vector< std::shared_ptr< LightTimeCalculatorBase > > >
    getLegLightTimeCalculators( ) const override
    {
        return {
            { std::make_pair( transmitter, receiver ), { this->getSingleLegLightTimeCalculator( 0, 0 ) } },
            { std::make_pair( transmitter2, receiver ), { this->getSingleLegLightTimeCalculator( 1, 0 ) } }
        };
    }
};

}  // namespace observation_models

}  // namespace tudat

#endif  // TUDAT_POSITIONANGLEANDSEPARATIONOBSERVATIONMODEL_H
```

---

## Part 4: Observation Model Settings

### File: `include/tudat/simulation/estimation_setup/createObservationModelSettings.h`

Add three settings classes and three inline factory functions. Place them after the `DifferencedFrequencyOfArrivalObservationSettings` class (~line 1195).

#### Settings class for PositionAngle:

```cpp
class PositionAngleObservationModelSettings : public ObservationModelSettings
{
public:
    PositionAngleObservationModelSettings(
        const LinkDefinition linkEnds,
        const std::vector< std::shared_ptr< LightTimeCorrectionSettings > > lightTimeCorrectionsList =
            std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ),
        const std::shared_ptr< ObservationBiasSettings > biasSettings = nullptr,
        const std::shared_ptr< LightTimeConvergenceCriteria > lightTimeConvergenceCriteria =
            std::make_shared< LightTimeConvergenceCriteria >( ) ):
        ObservationModelSettings( position_angle, linkEnds, lightTimeCorrectionsList, biasSettings, lightTimeConvergenceCriteria )
    {}
};
```

#### Settings class for Separation:

```cpp
class SeparationObservationModelSettings : public ObservationModelSettings
{
public:
    SeparationObservationModelSettings(
        const LinkDefinition linkEnds,
        const std::vector< std::shared_ptr< LightTimeCorrectionSettings > > lightTimeCorrectionsList =
            std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ),
        const std::shared_ptr< ObservationBiasSettings > biasSettings = nullptr,
        const std::shared_ptr< LightTimeConvergenceCriteria > lightTimeConvergenceCriteria =
            std::make_shared< LightTimeConvergenceCriteria >( ) ):
        ObservationModelSettings( separation, linkEnds, lightTimeCorrectionsList, biasSettings, lightTimeConvergenceCriteria )
    {}
};
```

#### Settings class for PositionAngleAndSeparation:

```cpp
class PositionAngleAndSeparationObservationModelSettings : public ObservationModelSettings
{
public:
    PositionAngleAndSeparationObservationModelSettings(
        const LinkDefinition linkEnds,
        const std::vector< std::shared_ptr< LightTimeCorrectionSettings > > lightTimeCorrectionsList =
            std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ),
        const std::shared_ptr< ObservationBiasSettings > biasSettings = nullptr,
        const std::shared_ptr< LightTimeConvergenceCriteria > lightTimeConvergenceCriteria =
            std::make_shared< LightTimeConvergenceCriteria >( ) ):
        ObservationModelSettings( position_angle_and_separation, linkEnds, lightTimeCorrectionsList, biasSettings, lightTimeConvergenceCriteria )
    {}
};
```

#### Inline factory functions (place after `differencedFrequencyOfArrivalObservationSettings` ~line 1620):

```cpp
inline std::shared_ptr< ObservationModelSettings > positionAngleSettings(
    const LinkDefinition linkEnds,
    const std::vector< std::shared_ptr< LightTimeCorrectionSettings > > lightTimeCorrections =
        std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ),
    const std::shared_ptr< ObservationBiasSettings > biasSettings = nullptr,
    const std::shared_ptr< LightTimeConvergenceCriteria > lightTimeConvergenceCriteria =
        std::make_shared< LightTimeConvergenceCriteria >( ) )
{
    return std::make_shared< PositionAngleObservationModelSettings >(
        linkEnds, lightTimeCorrections, biasSettings, lightTimeConvergenceCriteria );
}

inline std::shared_ptr< ObservationModelSettings > separationSettings(
    const LinkDefinition linkEnds,
    const std::vector< std::shared_ptr< LightTimeCorrectionSettings > > lightTimeCorrections =
        std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ),
    const std::shared_ptr< ObservationBiasSettings > biasSettings = nullptr,
    const std::shared_ptr< LightTimeConvergenceCriteria > lightTimeConvergenceCriteria =
        std::make_shared< LightTimeConvergenceCriteria >( ) )
{
    return std::make_shared< SeparationObservationModelSettings >(
        linkEnds, lightTimeCorrections, biasSettings, lightTimeConvergenceCriteria );
}

inline std::shared_ptr< ObservationModelSettings > positionAngleAndSeparationSettings(
    const LinkDefinition linkEnds,
    const std::vector< std::shared_ptr< LightTimeCorrectionSettings > > lightTimeCorrections =
        std::vector< std::shared_ptr< LightTimeCorrectionSettings > >( ),
    const std::shared_ptr< ObservationBiasSettings > biasSettings = nullptr,
    const std::shared_ptr< LightTimeConvergenceCriteria > lightTimeConvergenceCriteria =
        std::make_shared< LightTimeConvergenceCriteria >( ) )
{
    return std::make_shared< PositionAngleAndSeparationObservationModelSettings >(
        linkEnds, lightTimeCorrections, biasSettings, lightTimeConvergenceCriteria );
}
```

---

## Part 5: Factory Registration

### File: `include/tudat/simulation/estimation_setup/createObservationModelFactory.h`

#### 5a. `ObservationModelCreator< 1 >` — add cases in the switch (~line 1589, before `default:`)

Add after the `differenced_frequency_of_arrival` case:

```cpp
case position_angle: {
    if( linkEnds.size( ) != 3 )
    {
        throw std::runtime_error( "Error when making position angle model, " +
            std::to_string( linkEnds.size( ) ) + " link ends found" );
    }
    if( linkEnds.count( receiver ) == 0 )
        throw std::runtime_error( "Error when making position angle model, no receiver found" );
    if( linkEnds.count( transmitter ) == 0 )
        throw std::runtime_error( "Error when making position angle model, no transmitter found" );
    if( linkEnds.count( transmitter2 ) == 0 )
        throw std::runtime_error( "Error when making position angle model, no second transmitter found" );

    std::shared_ptr< ObservationBias< 1 > > observationBias;
    if( observationSettings->biasSettings_ != nullptr )
    {
        observationBias = createObservationBiasCalculator< 1 >(
            linkEnds, observationSettings->observableType_, observationSettings->biasSettings_, bodies );
    }

    observationModel = std::make_shared< PositionAngleObservationModel< ObservationScalarType, TimeType > >(
        linkEnds,
        createLightTimeCalculator< ObservationScalarType, TimeType >( linkEnds, transmitter, receiver, bodies,
            topLevelObservableType, observationSettings->lightTimeCorrectionsList_,
            observationSettings->lightTimeConvergenceCriteria_ ),
        createLightTimeCalculator< ObservationScalarType, TimeType >( linkEnds, transmitter2, receiver, bodies,
            topLevelObservableType, observationSettings->lightTimeCorrectionsList_,
            observationSettings->lightTimeConvergenceCriteria_ ),
        observationBias );
    break;
}
case separation: {
    // Same as position_angle but with SeparationObservationModel
    // ...
    break;
}
```

#### 5b. `ObservationModelCreator< 2 >` — add case (~line 1826, after `relative_angular_position`)

```cpp
case position_angle_and_separation: {
    if( linkEnds.size( ) != 3 )
    {
        throw std::runtime_error( "Error when making position angle and separation model, " +
            std::to_string( linkEnds.size( ) ) + " link ends found" );
    }
    if( linkEnds.count( receiver ) == 0 )
        throw std::runtime_error( "Error when making position angle and separation model, no receiver found" );
    if( linkEnds.count( transmitter ) == 0 )
        throw std::runtime_error( "Error when making position angle and separation model, no transmitter found" );
    if( linkEnds.count( transmitter2 ) == 0 )
        throw std::runtime_error( "Error when making position angle and separation model, no second transmitter found" );

    std::shared_ptr< ObservationBias< 2 > > observationBias;
    if( observationSettings->biasSettings_ != nullptr )
    {
        observationBias = createObservationBiasCalculator< 2 >(
            linkEnds, observationSettings->observableType_, observationSettings->biasSettings_, bodies );
    }

    observationModel = std::make_shared< PositionAngleAndSeparationObservationModel< ObservationScalarType, TimeType > >(
        linkEnds,
        createLightTimeCalculator< ObservationScalarType, TimeType >( linkEnds, transmitter, receiver, bodies,
            topLevelObservableType, observationSettings->lightTimeCorrectionsList_,
            observationSettings->lightTimeConvergenceCriteria_ ),
        createLightTimeCalculator< ObservationScalarType, TimeType >( linkEnds, transmitter2, receiver, bodies,
            topLevelObservableType, observationSettings->lightTimeCorrectionsList_,
            observationSettings->lightTimeConvergenceCriteria_ ),
        observationBias );
    break;
}
```

#### 5c. `getLightTimeCorrections` — add cases (~line 2417)

```cpp
case observation_models::position_angle:
case observation_models::separation:
case observation_models::position_angle_and_separation: {
    // Same pattern as relative_angular_position
    std::shared_ptr< observation_models::PositionAngleObservationModel< ObservationScalarType, TimeType > >
        paModel = std::dynamic_pointer_cast<
            observation_models::PositionAngleObservationModel< ObservationScalarType, TimeType > >( observationModel );
    // (handle the three types with appropriate casts)
    currentLightTimeCorrections.push_back(
        model->getLightTimeCalculatorFirstTransmitter( )->getLightTimeCorrection( ) );
    currentLightTimeCorrections.push_back(
        model->getLightTimeCalculatorSecondTransmitter( )->getLightTimeCorrection( ) );
    break;
}
```

Note: For `getLightTimeCorrections`, you'll need to handle the three model types. The cleanest approach is to add a helper or use `getLegLightTimeCalculators()` which all three models implement. Alternatively, add three separate cases with appropriate `dynamic_pointer_cast`.

#### 5d. `UndifferencedObservationModelExtractor< 1 >` — add cases (~line 2577)

```cpp
case observation_models::position_angle: {
    std::shared_ptr< observation_models::PositionAngleObservationModel< ObservationScalarType, TimeType > >
        paModel = std::dynamic_pointer_cast<
            observation_models::PositionAngleObservationModel< ObservationScalarType, TimeType > >(
            differencedObservationModel );
    firstObservationModel = std::make_shared< observation_models::AngularPositionObservationModel< ObservationScalarType, TimeType > >(
        paModel->getFirstLinkEnds( ), paModel->getLightTimeCalculatorFirstTransmitter( ) );
    secondObservationModel = std::make_shared< observation_models::AngularPositionObservationModel< ObservationScalarType, TimeType > >(
        paModel->getSecondLinkEnds( ), paModel->getLightTimeCalculatorSecondTransmitter( ) );
    break;
}
case observation_models::separation: {
    // Same pattern with SeparationObservationModel
    break;
}
```

#### 5e. `UndifferencedObservationModelExtractor< 2 >` — add case (~line 2616)

```cpp
case observation_models::position_angle_and_separation: {
    std::shared_ptr< observation_models::PositionAngleAndSeparationObservationModel< ObservationScalarType, TimeType > >
        pasModel = std::dynamic_pointer_cast<
            observation_models::PositionAngleAndSeparationObservationModel< ObservationScalarType, TimeType > >(
            differencedObservationModel );
    firstObservationModel = std::make_shared< observation_models::AngularPositionObservationModel< ObservationScalarType, TimeType > >(
        pasModel->getFirstLinkEnds( ), pasModel->getLightTimeCalculatorFirstTransmitter( ) );
    secondObservationModel = std::make_shared< observation_models::AngularPositionObservationModel< ObservationScalarType, TimeType > >(
        pasModel->getSecondLinkEnds( ), pasModel->getLightTimeCalculatorSecondTransmitter( ) );
    break;
}
```

#### 5f. Add includes at top of file:

```cpp
#include "tudat/astro/observation_models/positionAngleObservationModel.h"
#include "tudat/astro/observation_models/separationObservationModel.h"
#include "tudat/astro/observation_models/positionAngleAndSeparationObservationModel.h"
```

---

## Part 6: Partial Derivative Implementation

### File: `include/tudat/astro/orbit_determination/observation_partials/positionAngleAndSeparationPartial.h` (NEW)

```cpp
#ifndef TUDAT_POSITIONANGLEANDSEPARATIONPARTIAL_H
#define TUDAT_POSITIONANGLEANDSEPARATIONPARTIAL_H

#include "tudat/astro/observation_models/linkTypeDefs.h"
#include "tudat/astro/orbit_determination/observation_partials/observationPartial.h"
#include "tudat/astro/orbit_determination/observation_partials/positionPartials.h"
#include "tudat/astro/orbit_determination/estimatable_parameters/estimatableParameter.h"
#include "tudat/astro/orbit_determination/observation_partials/lightTimeCorrectionPartial.h"

namespace tudat
{

namespace observation_partials
{

//! Derived class for scaling three-dimensional position partial to position angle observable partial (size 1)
class PositionAngleScaling : public DirectPositionPartialScaling< 1 >
{
public:
    PositionAngleScaling( ):
        DirectPositionPartialScaling< 1 >( observation_models::position_angle ) {}

    ~PositionAngleScaling( ) {}

    void update( const std::vector< Eigen::Vector6d >& linkEndStates,
                 const std::vector< double >& times,
                 const observation_models::LinkEndType fixedLinkEnd,
                 const Eigen::VectorXd currentObservation );

    Eigen::Matrix< double, 1, 3 > getPositionScalingFactor( const observation_models::LinkEndType linkEndType )
    {
        if( linkEndType == observation_models::transmitter )
            return referenceScalingFactorFirstTransmitter_;
        else if( linkEndType == observation_models::transmitter2 )
            return referenceScalingFactorSecondTransmitter_;
        else if( linkEndType == observation_models::receiver )
            return -( referenceScalingFactorFirstTransmitter_ + referenceScalingFactorSecondTransmitter_ );
        else
            throw std::runtime_error( "Error when getting position angle scaling factor, incorrect link end type." );
    }

    Eigen::Matrix< double, 1, 1 > getLightTimePartialScalingFactor( )
    {
        return referenceLightTimeCorrectionScaling_;
    }

    observation_models::LinkEndType getCurrentLinkEndType( )
    {
        return currentLinkEndType_;
    }

private:
    Eigen::Matrix< double, 1, 3 > referenceScalingFactorFirstTransmitter_;
    Eigen::Matrix< double, 1, 3 > referenceScalingFactorSecondTransmitter_;
    Eigen::Matrix< double, 1, 1 > referenceLightTimeCorrectionScaling_;
    observation_models::LinkEndType currentLinkEndType_;
};

//! Derived class for scaling three-dimensional position partial to separation observable partial (size 1)
class SeparationScaling : public DirectPositionPartialScaling< 1 >
{
public:
    SeparationScaling( ):
        DirectPositionPartialScaling< 1 >( observation_models::separation ) {}

    ~SeparationScaling( ) {}

    void update( const std::vector< Eigen::Vector6d >& linkEndStates,
                 const std::vector< double >& times,
                 const observation_models::LinkEndType fixedLinkEnd,
                 const Eigen::VectorXd currentObservation );

    Eigen::Matrix< double, 1, 3 > getPositionScalingFactor( const observation_models::LinkEndType linkEndType )
    {
        if( linkEndType == observation_models::transmitter )
            return referenceScalingFactorFirstTransmitter_;
        else if( linkEndType == observation_models::transmitter2 )
            return referenceScalingFactorSecondTransmitter_;
        else if( linkEndType == observation_models::receiver )
            return -( referenceScalingFactorFirstTransmitter_ + referenceScalingFactorSecondTransmitter_ );
        else
            throw std::runtime_error( "Error when getting separation scaling factor, incorrect link end type." );
    }

    Eigen::Matrix< double, 1, 1 > getLightTimePartialScalingFactor( )
    {
        return referenceLightTimeCorrectionScaling_;
    }

    observation_models::LinkEndType getCurrentLinkEndType( )
    {
        return currentLinkEndType_;
    }

private:
    Eigen::Matrix< double, 1, 3 > referenceScalingFactorFirstTransmitter_;
    Eigen::Matrix< double, 1, 3 > referenceScalingFactorSecondTransmitter_;
    Eigen::Matrix< double, 1, 1 > referenceLightTimeCorrectionScaling_;
    observation_models::LinkEndType currentLinkEndType_;
};

//! Derived class for scaling three-dimensional position partial to position angle and separation observable partial (size 2)
class PositionAngleAndSeparationScaling : public DirectPositionPartialScaling< 2 >
{
public:
    PositionAngleAndSeparationScaling( ):
        DirectPositionPartialScaling< 2 >( observation_models::position_angle_and_separation ) {}

    ~PositionAngleAndSeparationScaling( ) {}

    void update( const std::vector< Eigen::Vector6d >& linkEndStates,
                 const std::vector< double >& times,
                 const observation_models::LinkEndType fixedLinkEnd,
                 const Eigen::VectorXd currentObservation );

    Eigen::Matrix< double, 2, 3 > getPositionScalingFactor( const observation_models::LinkEndType linkEndType )
    {
        if( linkEndType == observation_models::transmitter )
            return referenceScalingFactorFirstTransmitter_;
        else if( linkEndType == observation_models::transmitter2 )
            return referenceScalingFactorSecondTransmitter_;
        else if( linkEndType == observation_models::receiver )
            return -( referenceScalingFactorFirstTransmitter_ + referenceScalingFactorSecondTransmitter_ );
        else
            throw std::runtime_error( "Error when getting position angle and separation scaling factor, incorrect link end type." );
    }

    Eigen::Vector2d getLightTimePartialScalingFactor( )
    {
        return referenceLightTimeCorrectionScaling_;
    }

    observation_models::LinkEndType getCurrentLinkEndType( )
    {
        return currentLinkEndType_;
    }

private:
    Eigen::Matrix< double, 2, 3 > referenceScalingFactorFirstTransmitter_;
    Eigen::Matrix< double, 2, 3 > referenceScalingFactorSecondTransmitter_;
    Eigen::Vector2d referenceLightTimeCorrectionScaling_;
    observation_models::LinkEndType currentLinkEndType_;
};

}  // namespace observation_partials

}  // namespace tudat

#endif  // TUDAT_POSITIONANGLEANDSEPARATIONPARTIAL_H
```

### File: `src/tudat/astro/orbit_determination/observation_partials/positionAngleAndSeparationPartial.cpp` (NEW)

This file implements the `update()` methods. The key mathematical derivation:

Given $\alpha_1, \delta_1$ (transmitter 1) and $\alpha_2, \delta_2$ (transmitter 2), $\Delta\alpha = \alpha_2 - \alpha_1$:

**Position Angle partials:**

Let $N = \sin\Delta\alpha \cdot \cos\delta_2$ and $D = \cos\delta_1 \cdot \sin\delta_2 - \sin\delta_1 \cdot \cos\delta_2 \cdot \cos\Delta\alpha$.

Then $\text{PA} = \operatorname{atan2}(N, D)$ and:

$$\frac{\partial\text{PA}}{\partial\alpha_1} = -\frac{D \cdot \cos\Delta\alpha \cdot \cos\delta_2 + N \cdot \sin\delta_1 \cdot \cos\delta_2 \cdot \sin\Delta\alpha}{N^2 + D^2}$$

$$\frac{\partial\text{PA}}{\partial\delta_1} = -\frac{N \cdot (-\sin\delta_1 \cdot \sin\delta_2 - \cos\delta_1 \cdot \cos\delta_2 \cdot \cos\Delta\alpha)}{N^2 + D^2}$$

$$\frac{\partial\text{PA}}{\partial\alpha_2} = \frac{D \cdot \cos\Delta\alpha \cdot \cos\delta_2 + N \cdot \sin\delta_1 \cdot \cos\delta_2 \cdot \sin\Delta\alpha}{N^2 + D^2}$$

$$\frac{\partial\text{PA}}{\partial\delta_2} = \frac{D \cdot (-\sin\Delta\alpha \cdot \sin\delta_2) + N \cdot (\cos\delta_1 \cdot \cos\delta_2 + \sin\delta_1 \cdot \sin\delta_2 \cdot \cos\Delta\alpha)}{N^2 + D^2}$$

Then chain-rule with $\partial\alpha/\partial\mathbf{r}$ and $\partial\delta/\partial\mathbf{r}$ from `angularPositionPartial.h`:

$$\frac{\partial\text{PA}}{\partial\mathbf{r}_1} = \frac{\partial\text{PA}}{\partial\alpha_1}\frac{\partial\alpha_1}{\partial\mathbf{r}_1} + \frac{\partial\text{PA}}{\partial\delta_1}\frac{\partial\delta_1}{\partial\mathbf{r}_1}$$

$$\frac{\partial\text{PA}}{\partial\mathbf{r}_2} = \frac{\partial\text{PA}}{\partial\alpha_2}\frac{\partial\alpha_2}{\partial\mathbf{r}_2} + \frac{\partial\text{PA}}{\partial\delta_2}\frac{\partial\delta_2}{\partial\mathbf{r}_2}$$

$$\frac{\partial\text{PA}}{\partial\mathbf{r}_R} = -\frac{\partial\text{PA}}{\partial\mathbf{r}_1} - \frac{\partial\text{PA}}{\partial\mathbf{r}_2}$$

**Separation partials:**

Let $C = \sin\delta_1 \sin\delta_2 + \cos\delta_1 \cos\delta_2 \cos\Delta\alpha$. Then $\text{Sep} = \arccos(C)$ and:

$$\frac{\partial\text{Sep}}{\partial\alpha_1} = \frac{1}{\sqrt{1-C^2}} \cdot \cos\delta_1 \cos\delta_2 \sin\Delta\alpha$$

$$\frac{\partial\text{Sep}}{\partial\delta_1} = \frac{1}{\sqrt{1-C^2}} \cdot (-\cos\delta_1 \sin\delta_2 + \sin\delta_1 \cos\delta_2 \cos\Delta\alpha)$$

$$\frac{\partial\text{Sep}}{\partial\alpha_2} = -\frac{1}{\sqrt{1-C^2}} \cdot \cos\delta_1 \cos\delta_2 \sin\Delta\alpha$$

$$\frac{\partial\text{Sep}}{\partial\delta_2} = \frac{1}{\sqrt{1-C^2}} \cdot (-\sin\delta_1 \cos\delta_2 + \cos\delta_1 \sin\delta_2 \cos\Delta\alpha)$$

Chain rule with $\partial\alpha/\partial\mathbf{r}$ and $\partial\delta/\partial\mathbf{r}$ as above.

```cpp
#include "tudat/astro/orbit_determination/observation_partials/positionAngleAndSeparationPartial.h"
#include "tudat/astro/orbit_determination/observation_partials/angularPositionPartial.h"

namespace tudat
{

namespace observation_partials
{

void PositionAngleScaling::update( const std::vector< Eigen::Vector6d >& linkEndStates,
                                    const std::vector< double >& times,
                                    const observation_models::LinkEndType fixedLinkEnd,
                                    const Eigen::VectorXd currentObservation )
{
    if( fixedLinkEnd != observation_models::receiver )
    {
        throw std::runtime_error(
            "Error when updating position angle scaling object, fixed link end must be receiver." );
    }

    // States: [0]=transmitter, [1]=transmitter2, [2]=receiver
    Eigen::Vector3d relativeRange1 = ( linkEndStates[ 2 ] - linkEndStates[ 0 ] ).segment( 0, 3 );
    Eigen::Vector3d relativeRange2 = ( linkEndStates[ 2 ] - linkEndStates[ 1 ] ).segment( 0, 3 );

    // Compute RA/Dec for both transmitters
    double ra1 = 2.0 * std::atan( relativeRange1[ 1 ] /
        ( std::sqrt( relativeRange1[ 0 ] * relativeRange1[ 0 ] + relativeRange1[ 1 ] * relativeRange1[ 1 ] ) + relativeRange1[ 0 ] ) );
    double dec1 = mathematical_constants::PI / 2.0 - std::acos( relativeRange1[ 2 ] / relativeRange1.norm( ) );

    double ra2 = 2.0 * std::atan( relativeRange2[ 1 ] /
        ( std::sqrt( relativeRange2[ 0 ] * relativeRange2[ 0 ] + relativeRange2[ 1 ] * relativeRange2[ 1 ] ) + relativeRange2[ 0 ] ) );
    double dec2 = mathematical_constants::PI / 2.0 - std::acos( relativeRange2[ 2 ] / relativeRange2.norm( ) );

    double dRA = ra2 - ra1;

    double sinDRA = std::sin( dRA );
    double cosDRA = std::cos( dRA );
    double sinD1 = std::sin( dec1 );
    double cosD1 = std::cos( dec1 );
    double sinD2 = std::sin( dec2 );
    double cosD2 = std::cos( dec2 );

    double N = sinDRA * cosD2;
    double D = cosD1 * sinD2 - sinD1 * cosD2 * cosDRA;
    double denom = N * N + D * D;

    // dPA/d(alpha1, delta1, alpha2, delta2)
    double dPA_dRA1 = -( D * cosDRA * cosD2 + N * sinD1 * cosD2 * sinDRA ) / denom;
    double dPA_dDec1 = -( N * ( -sinD1 * sinD2 - cosD1 * cosD2 * cosDRA ) ) / denom;
    double dPA_dRA2 = ( D * cosDRA * cosD2 + N * sinD1 * cosD2 * sinDRA ) / denom;
    double dPA_dDec2 = ( D * ( -sinDRA * sinD2 ) + N * ( cosD1 * cosD2 + sinD1 * sinD2 * cosDRA ) ) / denom;

    // Get d(RA,Dec)/d(position) for each link end
    Eigen::Matrix< double, 1, 3 > dRA1_dr1 = calculatePartialOfRightAscensionWrtLinkEndPosition( relativeRange1, true );
    Eigen::Matrix< double, 1, 3 > dDec1_dr1 = calculatePartialOfDeclinationWrtLinkEndPosition( relativeRange1, true );
    Eigen::Matrix< double, 1, 3 > dRA2_dr2 = calculatePartialOfRightAscensionWrtLinkEndPosition( relativeRange2, true );
    Eigen::Matrix< double, 1, 3 > dDec2_dr2 = calculatePartialOfDeclinationWrtLinkEndPosition( relativeRange2, true );

    // Chain rule: dPA/dr = dPA/dRA * dRA/dr + dPA/dDec * dDec/dr
    referenceScalingFactorFirstTransmitter_ = dPA_dRA1 * dRA1_dr1 + dPA_dDec1 * dDec1_dr1;
    referenceScalingFactorSecondTransmitter_ = dPA_dRA2 * dRA2_dr2 + dPA_dDec2 * dDec2_dr2;

    // Light-time correction scaling (simplified: use velocity projection)
    Eigen::Vector3d normRelRange1 = relativeRange1.normalized( );
    Eigen::Vector3d normRelRange2 = relativeRange2.normalized( );
    referenceLightTimeCorrectionScaling_( 0, 0 ) =
        referenceScalingFactorFirstTransmitter_ * linkEndStates[ 0 ].segment( 3, 3 ) /
        ( physical_constants::SPEED_OF_LIGHT - linkEndStates[ 0 ].segment( 3, 3 ).dot( normRelRange1 ) );

    currentLinkEndType_ = fixedLinkEnd;
}

void SeparationScaling::update( const std::vector< Eigen::Vector6d >& linkEndStates,
                                 const std::vector< double >& times,
                                 const observation_models::LinkEndType fixedLinkEnd,
                                 const Eigen::VectorXd currentObservation )
{
    if( fixedLinkEnd != observation_models::receiver )
    {
        throw std::runtime_error(
            "Error when updating separation scaling object, fixed link end must be receiver." );
    }

    Eigen::Vector3d relativeRange1 = ( linkEndStates[ 2 ] - linkEndStates[ 0 ] ).segment( 0, 3 );
    Eigen::Vector3d relativeRange2 = ( linkEndStates[ 2 ] - linkEndStates[ 1 ] ).segment( 0, 3 );

    double ra1 = 2.0 * std::atan( relativeRange1[ 1 ] /
        ( std::sqrt( relativeRange1[ 0 ] * relativeRange1[ 0 ] + relativeRange1[ 1 ] * relativeRange1[ 1 ] ) + relativeRange1[ 0 ] ) );
    double dec1 = mathematical_constants::PI / 2.0 - std::acos( relativeRange1[ 2 ] / relativeRange1.norm( ) );

    double ra2 = 2.0 * std::atan( relativeRange2[ 1 ] /
        ( std::sqrt( relativeRange2[ 0 ] * relativeRange2[ 0 ] + relativeRange2[ 1 ] * relativeRange2[ 1 ] ) + relativeRange2[ 0 ] ) );
    double dec2 = mathematical_constants::PI / 2.0 - std::acos( relativeRange2[ 2 ] / relativeRange2.norm( ) );

    double dRA = ra2 - ra1;
    double sinDRA = std::sin( dRA );
    double cosDRA = std::cos( dRA );
    double sinD1 = std::sin( dec1 );
    double cosD1 = std::cos( dec1 );
    double sinD2 = std::sin( dec2 );
    double cosD2 = std::cos( dec2 );

    double C = sinD1 * sinD2 + cosD1 * cosD2 * cosDRA;
    double invSqrt = 1.0 / std::sqrt( 1.0 - C * C );

    double dSep_dRA1 = invSqrt * cosD1 * cosD2 * sinDRA;
    double dSep_dDec1 = invSqrt * ( -cosD1 * sinD2 + sinD1 * cosD2 * cosDRA );
    double dSep_dRA2 = -invSqrt * cosD1 * cosD2 * sinDRA;
    double dSep_dDec2 = invSqrt * ( -sinD1 * cosD2 + cosD1 * sinD2 * cosDRA );

    Eigen::Matrix< double, 1, 3 > dRA1_dr1 = calculatePartialOfRightAscensionWrtLinkEndPosition( relativeRange1, true );
    Eigen::Matrix< double, 1, 3 > dDec1_dr1 = calculatePartialOfDeclinationWrtLinkEndPosition( relativeRange1, true );
    Eigen::Matrix< double, 1, 3 > dRA2_dr2 = calculatePartialOfRightAscensionWrtLinkEndPosition( relativeRange2, true );
    Eigen::Matrix< double, 1, 3 > dDec2_dr2 = calculatePartialOfDeclinationWrtLinkEndPosition( relativeRange2, true );

    referenceScalingFactorFirstTransmitter_ = dSep_dRA1 * dRA1_dr1 + dSep_dDec1 * dDec1_dr1;
    referenceScalingFactorSecondTransmitter_ = dSep_dRA2 * dRA2_dr2 + dSep_dDec2 * dDec2_dr2;

    Eigen::Vector3d normRelRange1 = relativeRange1.normalized( );
    referenceLightTimeCorrectionScaling_( 0, 0 ) =
        referenceScalingFactorFirstTransmitter_ * linkEndStates[ 0 ].segment( 3, 3 ) /
        ( physical_constants::SPEED_OF_LIGHT - linkEndStates[ 0 ].segment( 3, 3 ).dot( normRelRange1 ) );

    currentLinkEndType_ = fixedLinkEnd;
}

void PositionAngleAndSeparationScaling::update( const std::vector< Eigen::Vector6d >& linkEndStates,
                                                 const std::vector< double >& times,
                                                 const observation_models::LinkEndType fixedLinkEnd,
                                                 const Eigen::VectorXd currentObservation )
{
    if( fixedLinkEnd != observation_models::receiver )
    {
        throw std::runtime_error(
            "Error when updating position angle and separation scaling object, fixed link end must be receiver." );
    }

    Eigen::Vector3d relativeRange1 = ( linkEndStates[ 2 ] - linkEndStates[ 0 ] ).segment( 0, 3 );
    Eigen::Vector3d relativeRange2 = ( linkEndStates[ 2 ] - linkEndStates[ 1 ] ).segment( 0, 3 );

    double ra1 = 2.0 * std::atan( relativeRange1[ 1 ] /
        ( std::sqrt( relativeRange1[ 0 ] * relativeRange1[ 0 ] + relativeRange1[ 1 ] * relativeRange1[ 1 ] ) + relativeRange1[ 0 ] ) );
    double dec1 = mathematical_constants::PI / 2.0 - std::acos( relativeRange1[ 2 ] / relativeRange1.norm( ) );

    double ra2 = 2.0 * std::atan( relativeRange2[ 1 ] /
        ( std::sqrt( relativeRange2[ 0 ] * relativeRange2[ 0 ] + relativeRange2[ 1 ] * relativeRange2[ 1 ] ) + relativeRange2[ 0 ] ) );
    double dec2 = mathematical_constants::PI / 2.0 - std::acos( relativeRange2[ 2 ] / relativeRange2.norm( ) );

    double dRA = ra2 - ra1;
    double sinDRA = std::sin( dRA );
    double cosDRA = std::cos( dRA );
    double sinD1 = std::sin( dec1 );
    double cosD1 = std::cos( dec1 );
    double sinD2 = std::sin( dec2 );
    double cosD2 = std::cos( dec2 );

    // --- Position Angle partials ---
    double N = sinDRA * cosD2;
    double D = cosD1 * sinD2 - sinD1 * cosD2 * cosDRA;
    double denomPA = N * N + D * D;

    double dPA_dRA1 = -( D * cosDRA * cosD2 + N * sinD1 * cosD2 * sinDRA ) / denomPA;
    double dPA_dDec1 = -( N * ( -sinD1 * sinD2 - cosD1 * cosD2 * cosDRA ) ) / denomPA;
    double dPA_dRA2 = ( D * cosDRA * cosD2 + N * sinD1 * cosD2 * sinDRA ) / denomPA;
    double dPA_dDec2 = ( D * ( -sinDRA * sinD2 ) + N * ( cosD1 * cosD2 + sinD1 * sinD2 * cosDRA ) ) / denomPA;

    // --- Separation partials ---
    double C = sinD1 * sinD2 + cosD1 * cosD2 * cosDRA;
    double invSqrt = 1.0 / std::sqrt( 1.0 - C * C );

    double dSep_dRA1 = invSqrt * cosD1 * cosD2 * sinDRA;
    double dSep_dDec1 = invSqrt * ( -cosD1 * sinD2 + sinD1 * cosD2 * cosDRA );
    double dSep_dRA2 = -invSqrt * cosD1 * cosD2 * sinDRA;
    double dSep_dDec2 = invSqrt * ( -sinD1 * cosD2 + cosD1 * sinD2 * cosDRA );

    // Get d(RA,Dec)/d(position)
    Eigen::Matrix< double, 1, 3 > dRA1_dr1 = calculatePartialOfRightAscensionWrtLinkEndPosition( relativeRange1, true );
    Eigen::Matrix< double, 1, 3 > dDec1_dr1 = calculatePartialOfDeclinationWrtLinkEndPosition( relativeRange1, true );
    Eigen::Matrix< double, 1, 3 > dRA2_dr2 = calculatePartialOfRightAscensionWrtLinkEndPosition( relativeRange2, true );
    Eigen::Matrix< double, 1, 3 > dDec2_dr2 = calculatePartialOfDeclinationWrtLinkEndPosition( relativeRange2, true );

    // Chain rule for transmitter 1
    Eigen::Matrix< double, 1, 3 > dPA_dr1 = dPA_dRA1 * dRA1_dr1 + dPA_dDec1 * dDec1_dr1;
    Eigen::Matrix< double, 1, 3 > dSep_dr1 = dSep_dRA1 * dRA1_dr1 + dSep_dDec1 * dDec1_dr1;

    // Chain rule for transmitter 2
    Eigen::Matrix< double, 1, 3 > dPA_dr2 = dPA_dRA2 * dRA2_dr2 + dPA_dDec2 * dDec2_dr2;
    Eigen::Matrix< double, 1, 3 > dSep_dr2 = dSep_dRA2 * dRA2_dr2 + dSep_dDec2 * dDec2_dr2;

    // Assemble 2x3 matrices
    referenceScalingFactorFirstTransmitter_.row( 0 ) = dPA_dr1;
    referenceScalingFactorFirstTransmitter_.row( 1 ) = dSep_dr1;
    referenceScalingFactorSecondTransmitter_.row( 0 ) = dPA_dr2;
    referenceScalingFactorSecondTransmitter_.row( 1 ) = dSep_dr2;

    // Light-time correction scaling
    Eigen::Vector3d normRelRange1 = relativeRange1.normalized( );
    referenceLightTimeCorrectionScaling_ =
        referenceScalingFactorFirstTransmitter_ * linkEndStates[ 0 ].segment( 3, 3 ) /
        ( physical_constants::SPEED_OF_LIGHT - linkEndStates[ 0 ].segment( 3, 3 ).dot( normRelRange1 ) );

    currentLinkEndType_ = fixedLinkEnd;
}

}  // namespace observation_partials

}  // namespace tudat
```

---

## Part 7: Register Partials in createObservationPartials.h

### File: `include/tudat/simulation/estimation_setup/createObservationPartials.h`

#### 7a. Add include at top:
```cpp
#include "tudat/astro/orbit_determination/observation_partials/positionAngleAndSeparationPartial.h"
```

#### 7b. `ObservationPartialCreator< 1 >` — add cases (~line 210, before `default:`)

```cpp
case observation_models::position_angle:
case observation_models::separation:
    if( isPartialForConcatenatedObservable )
    {
        throw std::runtime_error(
            "Error when requesting partial creation for position angle/separation; concatenated partial not supported" );
    }
    observationPartials = createDifferencedObservablePartials< ObservationScalarType, TimeType, 1 >(
        observationModel, bodies, parametersToEstimate, isPartialForDifferencedObservable );
    break;
```

#### 7c. `ObservationPartialCreator< 2 >` — add case (~line 310, before `default:`)

```cpp
case observation_models::position_angle_and_separation:
    observationPartials = createDifferencedObservablePartials< ObservationScalarType, TimeType, 2 >(
        observationModel, bodies, parametersToEstimate, isPartialForDifferencedObservable );
    break;
```

#### 7d. `DifferencedObservationPartialCreator< 1 >` — add cases (~line 750, before `default:`)

```cpp
case position_angle:
case separation: {
    if( !isParameterObservationLinkTimeProperty(
            getDifferencedPartialParameterIdentifier( firstPartial, secondPartial ).first ) )
    {
        if( firstPartial != nullptr )
        {
            if( std::dynamic_pointer_cast< DirectObservationPartial< 1 > >( firstPartial ) == nullptr )
            {
                throw std::runtime_error(
                    "Error when creating position angle/separation partial; first input object type is incompatible" );
            }
            // Note: undifferenced partials are angular_position (size 2), but we're in size-1 context.
            // The DifferencedObservablePartial handles the size mismatch via the scaling factor.
        }
        if( secondPartial != nullptr )
        {
            if( std::dynamic_pointer_cast< DirectObservationPartial< 1 > >( secondPartial ) == nullptr )
            {
                throw std::runtime_error(
                    "Error when creating position angle/separation partial; second input object type is incompatible" );
            }
        }
    }

    differencedPartial = std::make_shared< DifferencedObservablePartial< 1 > >(
        firstPartial,
        secondPartial,
        ( differencedObservableType == position_angle ) ?
            &observation_models::getPositionAngleScalingFactor :
            &observation_models::getSeparationScalingFactor,
        getUndifferencedTimeAndStateIndices( differencedObservableType, linkEnds.size( ) ),
        &getDefaultDifferencedReferenceLinkEndTypes );
    break;
}
```

#### 7e. `DifferencedObservationPartialCreator< 2 >` — add case (~line 790, before `default:`)

```cpp
case position_angle_and_separation: {
    if( !isParameterObservationLinkTimeProperty(
            getDifferencedPartialParameterIdentifier( firstPartial, secondPartial ).first ) )
    {
        if( firstPartial != nullptr )
        {
            if( std::dynamic_pointer_cast< DirectObservationPartial< 2 > >( firstPartial ) == nullptr )
            {
                throw std::runtime_error(
                    "Error when creating position angle and separation partial; first input object type is incompatible" );
            }
            else if( std::dynamic_pointer_cast< DirectObservationPartial< 2 > >( firstPartial )->getObservableType( ) !=
                     angular_position )
            {
                throw std::runtime_error(
                    "Error when creating position angle and separation partial; first input observable type is incompatible" );
            }
        }
        if( secondPartial != nullptr )
        {
            if( std::dynamic_pointer_cast< DirectObservationPartial< 2 > >( secondPartial ) == nullptr )
            {
                throw std::runtime_error(
                    "Error when creating position angle and separation partial; second input object type is incompatible" );
            }
            else if( std::dynamic_pointer_cast< DirectObservationPartial< 2 > >( secondPartial )->getObservableType( ) !=
                     angular_position )
            {
                throw std::runtime_error(
                    "Error when creating position angle and separation partial; second input observable type is incompatible" );
            }
        }
    }

    differencedPartial = std::make_shared< DifferencedObservablePartial< 2 > >(
        firstPartial,
        secondPartial,
        &observation_models::getPositionAngleAndSeparationScalingFactor,
        getUndifferencedTimeAndStateIndices( position_angle_and_separation, linkEnds.size( ) ),
        &getDefaultDifferencedReferenceLinkEndTypes );
    break;
}
```

---

## Part 8: Register in createPositionPartialScaling.h

### File: `include/tudat/simulation/estimation_setup/createPositionPartialScaling.h`

#### 8a. Add include at top:
```cpp
#include "tudat/astro/orbit_determination/observation_partials/positionAngleAndSeparationPartial.h"
```

#### 8b. `ObservationPartialScalingCreator< 1 >::createPositionScalingObject` — add cases (~line 250, before `default:`)

```cpp
case observation_models::position_angle:
    positionPartialScaler = std::make_shared< PositionAngleScaling >( );
    break;
case observation_models::separation:
    positionPartialScaler = std::make_shared< SeparationScaling >( );
    break;
```

#### 8c. `ObservationPartialScalingCreator< 2 >::createPositionScalingObject` — add case (~line 420, before `default:`)

```cpp
case observation_models::position_angle_and_separation:
    positionPartialScaler = std::make_shared< PositionAngleAndSeparationScaling >( );
    break;
```

#### 8d. `ObservationPartialScalingCreator< 1 >::createDifferencedPositionPartialScalingObject` — add cases (~line 350, before `default:`)

```cpp
case observation_models::position_angle:
case observation_models::separation: {
    if( std::dynamic_pointer_cast< AngularPositionScaling >( firstPositionPartialScaling ) == nullptr )
    {
        throw std::runtime_error(
            "Error when creating position angle/separation partial scaling object, first partial is of incompatible type" );
    }
    if( std::dynamic_pointer_cast< AngularPositionScaling >( secondPositionPartialScaling ) == nullptr )
    {
        throw std::runtime_error(
            "Error when creating position angle/separation partial scaling object, second partial is of incompatible type" );
    }
    std::function< void( const observation_models::LinkEndType ) > customCheckFunction =
        []( const observation_models::LinkEndType fixedLinkEnd ) {
            if( fixedLinkEnd != observation_models::receiver )
            {
                throw std::runtime_error(
                    "Error when updating position angle/separation scaling object, fixed link end must be receiver." );
            }
        };
    positionPartialScaler = std::make_shared< DifferencedObservablePartialScaling >(
        firstPositionPartialScaling,
        secondPositionPartialScaling,
        observation_models::getUndifferencedTimeAndStateIndices( differencedObservableType, 3 ),
        &getDefaultDifferencedReferenceLinkEndTypes,
        customCheckFunction );
    break;
}
```

#### 8e. `ObservationPartialScalingCreator< 2 >::createDifferencedPositionPartialScalingObject` — add case (~line 490, before `default:`)

```cpp
case observation_models::position_angle_and_separation: {
    if( std::dynamic_pointer_cast< AngularPositionScaling >( firstPositionPartialScaling ) == nullptr )
    {
        throw std::runtime_error(
            "Error when creating position angle and separation partial scaling object, first partial is of incompatible type" );
    }
    if( std::dynamic_pointer_cast< AngularPositionScaling >( secondPositionPartialScaling ) == nullptr )
    {
        throw std::runtime_error(
            "Error when creating position angle and separation partial scaling object, second partial is of incompatible type" );
    }
    std::function< void( const observation_models::LinkEndType ) > customCheckFunction =
        []( const observation_models::LinkEndType fixedLinkEnd ) {
            if( fixedLinkEnd != observation_models::receiver )
            {
                throw std::runtime_error(
                    "Error when updating position angle and separation scaling object, fixed link end must be receiver." );
            }
        };
    positionPartialScaler = std::make_shared< DifferencedObservablePartialScaling >(
        firstPositionPartialScaling,
        secondPositionPartialScaling,
        observation_models::getUndifferencedTimeAndStateIndices( observation_models::position_angle_and_separation, 3 ),
        &getDefaultDifferencedReferenceLinkEndTypes,
        customCheckFunction );
    break;
}
```

---

## Part 9: Python Bindings

### File: `src/tudatpy/estimation/observable_models_setup/model_settings/expose_model_settings.cpp`

#### 9a. Add enum values (~line 82, after `pixel_coordinates_type`):

```cpp
.value( "position_angle_type", tom::ObservableType::position_angle )
.value( "separation_type", tom::ObservableType::separation )
.value( "position_angle_and_separation_type", tom::ObservableType::position_angle_and_separation )
```

#### 9b. Add Python factory functions (~line 690, after `relative_angular_position`):

```cpp
m.def( "position_angle",
    &tom::positionAngleSettings,
    py::arg( "link_ends" ),
    py::arg( "light_time_correction_settings" ) = std::vector< std::shared_ptr< tom::LightTimeCorrectionSettings > >( ),
    py::arg( "bias_settings" ) = nullptr,
    py::arg( "light_time_convergence_settings" ) = std::make_shared< tom::LightTimeConvergenceCriteria >( ),
    R"doc(

Function for creating settings for a position angle observable.

Function for creating observation model settings of position angle type observables.
It computes the position angle :math:`\theta` between two transmitters as seen from a receiver.
The position angle is measured from north through east, i.e. from the direction of the first transmitter
towards the second transmitter.

The observable :math:`h` of size 1 is computed as follows (in the unbiased case):

.. math::

    \Delta\alpha &= \alpha_2 - \alpha_1 \\
    h &= \operatorname{atan2}\!\big(\sin\Delta\alpha \cdot \cos\delta_2,\;
           \cos\delta_1 \cdot \sin\delta_2 - \sin\delta_1 \cdot \cos\delta_2 \cdot \cos\Delta\alpha\big)

where :math:`[\alpha_i;\delta_i]` are the right ascension and declination of transmitter :math:`i` as seen from the receiver.


Parameters
----------
link_ends : :class:`~tudatpy.estimation.observable_models_setup.links.LinkDefinition`
    Set of link ends that define the geometry of the observation. This observable requires the
    ``transmitter``, ``transmitter2`` and ``receiver`` :class:`~tudatpy.estimation.observable_models_setup.links.LinkEndType` entries to be defined.

light_time_correction_settings : List[ :class:`~tudatpy.estimation.observable_models_setup.light_time_corrections.LightTimeCorrectionSettings` ], default = list()
    List of corrections for the light-time that are to be used.

bias_settings : :class:`~tudatpy.estimation.observable_models_setup.biases.ObservationBiasSettings`, default = None
    Settings for the observation bias that is to be used for the observation.

light_time_convergence_settings : :class:`~tudatpy.estimation.observable_models_setup.light_time_corrections.LightTimeConvergenceCriteria`, default = :func:`~tudatpy.estimation.observable_models_setup.light_time_corrections.light_time_convergence_settings`
    Settings for convergence of the light-time

Returns
-------
:class:`~tudatpy.estimation.observable_models_setup.model_settings.ObservationModelSettings`
    Instance of the :class:`~tudatpy.estimation.observable_models_setup.model_settings.ObservationModelSettings` class defining the settings for the position angle observable.

)doc" );

m.def( "separation",
    &tom::separationSettings,
    py::arg( "link_ends" ),
    py::arg( "light_time_correction_settings" ) = std::vector< std::shared_ptr< tom::LightTimeCorrectionSettings > >( ),
    py::arg( "bias_settings" ) = nullptr,
    py::arg( "light_time_convergence_settings" ) = std::make_shared< tom::LightTimeConvergenceCriteria >( ),
    R"doc(

Function for creating settings for an angular separation observable.

Function for creating observation model settings of angular separation type observables.
It computes the angular separation :math:`\rho` between two transmitters as seen from a receiver.

The observable :math:`h` of size 1 is computed as follows (in the unbiased case):

.. math::

    \Delta\alpha &= \alpha_2 - \alpha_1 \\
    h &= \arccos\!\big(\sin\delta_1 \sin\delta_2 + \cos\delta_1 \cos\delta_2 \cos\Delta\alpha\big)

where :math:`[\alpha_i;\delta_i]` are the right ascension and declination of transmitter :math:`i` as seen from the receiver.


Parameters
----------
link_ends : :class:`~tudatpy.estimation.observable_models_setup.links.LinkDefinition`
    Set of link ends that define the geometry of the observation. This observable requires the
    ``transmitter``, ``transmitter2`` and ``receiver`` :class:`~tudatpy.estimation.observable_models_setup.links.LinkEndType` entries to be defined.

light_time_correction_settings : List[ :class:`~tudatpy.estimation.observable_models_setup.light_time_corrections.LightTimeCorrectionSettings` ], default = list()
    List of corrections for the light-time that are to be used.

bias_settings : :class:`~tudatpy.estimation.observable_models_setup.biases.ObservationBiasSettings`, default = None
    Settings for the observation bias that is to be used for the observation.

light_time_convergence_settings : :class:`~tudatpy.estimation.observable_models_setup.light_time_corrections.LightTimeConvergenceCriteria`, default = :func:`~tudatpy.estimation.observable_models_setup.light_time_corrections.light_time_convergence_settings`
    Settings for convergence of the light-time

Returns
-------
:class:`~tudatpy.estimation.observable_models_setup.model_settings.ObservationModelSettings`
    Instance of the :class:`~tudatpy.estimation.observable_models_setup.model_settings.ObservationModelSettings` class defining the settings for the angular separation observable.

)doc" );

m.def( "position_angle_and_separation",
    &tom::positionAngleAndSeparationSettings,
    py::arg( "link_ends" ),
    py::arg( "light_time_correction_settings" ) = std::vector< std::shared_ptr< tom::LightTimeCorrectionSettings > >( ),
    py::arg( "bias_settings" ) = nullptr,
    py::arg( "light_time_convergence_settings" ) = std::make_shared< tom::LightTimeConvergenceCriteria >( ),
    R"doc(

Function for creating settings for a position angle and separation observable.

Function for creating observation model settings of position angle and separation type observables.
It computes both the position angle :math:`\theta` and angular separation :math:`\rho` between two transmitters as seen from a receiver.

The observable :math:`\mathbf{h}` of size 2 is computed as follows (in the unbiased case):

.. math::

    \mathbf{h} = [\theta; \rho]

where :math:`\theta` is the position angle (see :func:`~tudatpy.estimation.observable_models_setup.model_settings.position_angle`)
and :math:`\rho` is the angular separation (see :func:`~tudatpy.estimation.observable_models_setup.model_settings.separation`).


Parameters
----------
link_ends : :class:`~tudatpy.estimation.observable_models_setup.links.LinkDefinition`
    Set of link ends that define the geometry of the observation. This observable requires the
    ``transmitter``, ``transmitter2`` and ``receiver`` :class:`~tudatpy.estimation.observable_models_setup.links.LinkEndType` entries to be defined.

light_time_correction_settings : List[ :class:`~tudatpy.estimation.observable_models_setup.light_time_corrections.LightTimeCorrectionSettings` ], default = list()
    List of corrections for the light-time that are to be used.

bias_settings : :class:`~tudatpy.estimation.observable_models_setup.biases.ObservationBiasSettings`, default = None
    Settings for the observation bias that is to be used for the observation.

light_time_convergence_settings : :class:`~tudatpy.estimation.observable_models_setup.light_time_corrections.LightTimeConvergenceCriteria`, default = :func:`~tudatpy.estimation.observable_models_setup.light_time_corrections.light_time_convergence_settings`
    Settings for convergence of the light-time

Returns
-------
:class:`~tudatpy.estimation.observable_models_setup.model_settings.ObservationModelSettings`
    Instance of the :class:`~tudatpy.estimation.observable_models_setup.model_settings.ObservationModelSettings` class defining the settings for the position angle and separation observable.

)doc" );
```

---

## Part 10: Unit Tests

### File: `tests/test_tudat/src/astro/observation_models/unitTestPositionAngleAndSeparationObservationModel.cpp` (NEW)

Pattern from `unitTestRelativeAngularPositionModel.cpp`:

```cpp
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <limits>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "tudat/basics/testMacros.h"
#include "tudat/io/basicInputOutput.h"
#include "tudat/interface/spice/spiceInterface.h"
#include "tudat/simulation/estimation_setup/createObservationModelFactory.h"
#include "tudat/simulation/environment_setup/createBodiesFactory.h"
#include "tudat/simulation/environment_setup/defaultBodies.h"
#include "tudat/simulation/environment_setup/createEphemeris.h"

namespace tudat
{
namespace unit_tests
{

using namespace tudat::gravitation;
using namespace tudat::ephemerides;
using namespace tudat::observation_models;
using namespace tudat::simulation_setup;
using namespace tudat::spice_interface;

BOOST_AUTO_TEST_SUITE( test_position_angle_and_separation_observation_model )

BOOST_AUTO_TEST_CASE( testPositionAngleObservationModel )
{
    spice_interface::loadStandardSpiceKernels( { paths::getSpiceKernelPath( ) + "/de430_mar097_small.bsp" } );

    // Define bodies
    std::vector< std::string > bodiesToCreate;
    bodiesToCreate.push_back( "Earth" );
    bodiesToCreate.push_back( "Sun" );
    bodiesToCreate.push_back( "Mars" );
    bodiesToCreate.push_back( "Phobos" );

    double initialEphemerisTime = 0.0;
    double finalEphemerisTime = initialEphemerisTime + 7.0 * 86400.0;
    double maximumTimeStep = 3600.0;
    double buffer = 10.0 * maximumTimeStep;

    BodyListSettings bodySettings = getDefaultBodySettings( bodiesToCreate, initialEphemerisTime - buffer, finalEphemerisTime + buffer );
    bodySettings.addSettings( "Phobos" );
    bodySettings.at( "Phobos" )->ephemerisSettings = getDefaultEphemerisSettings( "Phobos" );

    SystemOfBodies bodies = createSystemOfBodies( bodySettings );

    // Define link ends
    LinkDefinition linkEnds;
    linkEnds[ receiver ] = std::make_pair< std::string, std::string >( "Earth", "" );
    linkEnds[ transmitter ] = std::make_pair< std::string, std::string >( "Mars", "" );
    linkEnds[ transmitter2 ] = std::make_pair< std::string, std::string >( "Phobos", "" );

    // Create light-time correction settings
    std::vector< std::string > lightTimePerturbingBodies = { "Sun" };
    std::vector< std::shared_ptr< LightTimeCorrectionSettings > > lightTimeCorrectionSettings;
    lightTimeCorrectionSettings.push_back(
        std::make_shared< FirstOrderRelativisticLightTimeCorrectionSettings >( lightTimePerturbingBodies ) );

    // Create observation settings for all three models
    std::shared_ptr< ObservationModelSettings > paSettings =
        std::make_shared< PositionAngleObservationModelSettings >( linkEnds, lightTimeCorrectionSettings );
    std::shared_ptr< ObservationModelSettings > sepSettings =
        std::make_shared< SeparationObservationModelSettings >( linkEnds, lightTimeCorrectionSettings );
    std::shared_ptr< ObservationModelSettings > pasSettings =
        std::make_shared< PositionAngleAndSeparationObservationModelSettings >( linkEnds, lightTimeCorrectionSettings );

    // Create observation models
    std::shared_ptr< ObservationModel< 1 > > paModel =
        ObservationModelCreator< 1, double, double >::createObservationModel( paSettings, bodies );
    std::shared_ptr< ObservationModel< 1 > > sepModel =
        ObservationModelCreator< 1, double, double >::createObservationModel( sepSettings, bodies );
    std::shared_ptr< ObservationModel< 2 > > pasModel =
        ObservationModelCreator< 2, double, double >::createObservationModel( pasSettings, bodies );

    // Test at several epochs
    double receiverObservationTime = ( finalEphemerisTime + initialEphemerisTime ) / 2.0;

    std::vector< double > linkEndTimesPA, linkEndTimesSep, linkEndTimesPAS;
    std::vector< Eigen::Vector6d > linkEndStatesPA, linkEndStatesSep, linkEndStatesPAS;

    Eigen::VectorXd paObs = paModel->computeObservationsWithLinkEndData(
        receiverObservationTime, receiver, linkEndTimesPA, linkEndStatesPA );
    Eigen::VectorXd sepObs = sepModel->computeObservationsWithLinkEndData(
        receiverObservationTime, receiver, linkEndTimesSep, linkEndStatesSep );
    Eigen::VectorXd pasObs = pasModel->computeObservationsWithLinkEndData(
        receiverObservationTime, receiver, linkEndTimesPAS, linkEndStatesPAS );

    // Verify combined model matches individual models
    BOOST_CHECK_CLOSE_FRACTION( paObs( 0 ), pasObs( 0 ), std::numeric_limits< double >::epsilon( ) );
    BOOST_CHECK_CLOSE_FRACTION( sepObs( 0 ), pasObs( 1 ), std::numeric_limits< double >::epsilon( ) );

    // Verify link end states match
    for( int i = 0; i < 3; i++ )
    {
        TUDAT_CHECK_MATRIX_CLOSE_FRACTION( linkEndStatesPA[ i ], linkEndStatesPAS[ i ],
            std::numeric_limits< double >::epsilon( ) );
        TUDAT_CHECK_MATRIX_CLOSE_FRACTION( linkEndStatesSep[ i ], linkEndStatesPAS[ i ],
            std::numeric_limits< double >::epsilon( ) );
    }

    // Test error: wrong reference link end
    BOOST_CHECK_THROW(
        paModel->computeObservationsWithLinkEndData( receiverObservationTime, transmitter, linkEndTimesPA, linkEndStatesPA ),
        std::runtime_error );
}

BOOST_AUTO_TEST_SUITE_END( )

}  // namespace unit_tests
}  // namespace tudat
```

### File: `tests/test_tudat/src/astro/orbit_determination/observation_partials/unitTestPositionAngleAndSeparationPartials.cpp` (NEW)

Pattern from `unitTestRelativeAngularPositionPartials.cpp` and `unitTestDifferencedFrequencyOfArrivalPartials.cpp`:

```cpp
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <limits>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "tudat/basics/testMacros.h"
#include "tudat/io/basicInputOutput.h"
#include "tudat/interface/spice/spiceInterface.h"
#include "tudat/simulation/estimation_setup/createObservationModelFactory.h"
#include "tudat/astro/orbit_determination/estimatable_parameters/constantRotationRate.h"
#include "tudat/simulation/estimation_setup/createObservationPartials.h"
#include "tudat/support/numericalObservationPartial.h"
#include "tudat/simulation/environment_setup/createGroundStations.h"
#include "tudat/simulation/environment_setup/defaultBodies.h"
#include "tudat/simulation/environment_setup/createEphemeris.h"
#include "tudat/support/observationPartialTestFunctions.h"

namespace tudat
{
namespace unit_tests
{

using namespace tudat::gravitation;
using namespace tudat::ephemerides;
using namespace tudat::observation_models;
using namespace tudat::simulation_setup;
using namespace tudat::spice_interface;
using namespace tudat::observation_partials;
using namespace tudat::estimatable_parameters;

BOOST_AUTO_TEST_SUITE( test_position_angle_and_separation_partials )

//! Test partial derivatives of position angle and separation observables using general test suite.
BOOST_AUTO_TEST_CASE( testPositionAngleAndSeparationPartials )
{
    // Define and create ground stations.
    std::vector< std::pair< std::string, std::string > > groundStations;
    groundStations.resize( 3 );
    groundStations[ 0 ] = std::make_pair( "Earth", "Graz" );
    groundStations[ 1 ] = std::make_pair( "Mars", "MSL" );
    groundStations[ 2 ] = std::make_pair( "Moon", "" );

    Eigen::VectorXd parameterPerturbationMultipliers = Eigen::VectorXd::Constant( 4, 1.0 );
    parameterPerturbationMultipliers( 2 ) = 10.0;

    // Test partials with constant ephemerides (allows test of position partials)
    {
        SystemOfBodies bodies = setupEnvironment( groundStations, 1.0E7, 1.2E7, 1.1E7, true );

        LinkDefinition linkEnds;
        linkEnds[ receiver ] = groundStations[ 0 ];
        linkEnds[ transmitter ] = groundStations[ 1 ];
        linkEnds[ transmitter2 ] = groundStations[ 2 ];

        std::vector< std::string > perturbingBodies;
        perturbingBodies.push_back( "Earth" );

        // Test position angle partials
        {
            std::shared_ptr< ObservationModel< 1 > > paModel =
                ObservationModelCreator< 1, double, double >::createObservationModel(
                    std::make_shared< PositionAngleObservationModelSettings >(
                        linkEnds,
                        std::vector< std::shared_ptr< LightTimeCorrectionSettings > >{
                            std::make_shared< FirstOrderRelativisticLightTimeCorrectionSettings >( perturbingBodies ) } ),
                    bodies );

            std::shared_ptr< EstimatableParameterSet< double > > fullEstimatableParameterSet =
                createEstimatableParameters( bodies, 1.1E7 );

            testObservationPartials< 1 >( paModel, bodies, fullEstimatableParameterSet, linkEnds,
                position_angle, 1.0E-4, true, true, 1.0, parameterPerturbationMultipliers );
        }

        // Test separation partials
        {
            std::shared_ptr< ObservationModel< 1 > > sepModel =
                ObservationModelCreator< 1, double, double >::createObservationModel(
                    std::make_shared< SeparationObservationModelSettings >(
                        linkEnds,
                        std::vector< std::shared_ptr< LightTimeCorrectionSettings > >{
                            std::make_shared< FirstOrderRelativisticLightTimeCorrectionSettings >( perturbingBodies ) } ),
                    bodies );

            std::shared_ptr< EstimatableParameterSet< double > > fullEstimatableParameterSet =
                createEstimatableParameters( bodies, 1.1E7 );

            testObservationPartials< 1 >( sepModel, bodies, fullEstimatableParameterSet, linkEnds,
                separation, 1.0E-4, true, true, 1.0, parameterPerturbationMultipliers );
        }

        // Test position angle and separation combined partials
        {
            std::shared_ptr< ObservationModel< 2 > > pasModel =
                ObservationModelCreator< 2, double, double >::createObservationModel(
                    std::make_shared< PositionAngleAndSeparationObservationModelSettings >(
                        linkEnds,
                        std::vector< std::shared_ptr< LightTimeCorrectionSettings > >{
                            std::make_shared< FirstOrderRelativisticLightTimeCorrectionSettings >( perturbingBodies ) } ),
                    bodies );

            std::shared_ptr< EstimatableParameterSet< double > > fullEstimatableParameterSet =
                createEstimatableParameters( bodies, 1.1E7 );

            testObservationPartials< 2 >( pasModel, bodies, fullEstimatableParameterSet, linkEnds,
                position_angle_and_separation, 1.0E-4, true, true, 1.0, parameterPerturbationMultipliers );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END( )

}  // namespace unit_tests
}  // namespace tudat
```

---

## Part 11: CMakeLists Updates

### File: `tests/test_tudat/src/astro/observation_models/CMakeLists.txt`

Add after `RelativeAngularPositionModel`:
```cmake
TUDAT_ADD_TEST_CASE(PositionAngleAndSeparationObservationModel PRIVATE_LINKS ${Tudat_ESTIMATION_LIBRARIES})
```

### File: `tests/test_tudat/src/astro/orbit_determination/observation_partials/CMakeLists.txt`

Add after `RelativeAngularPositionPartials`:
```cmake
TUDAT_ADD_TEST_CASE(PositionAngleAndSeparationPartials
    PRIVATE_LINKS
    tudat_test_support
    ${Tudat_ESTIMATION_LIBRARIES}
    )
```

---

## Part 12: Important Design Note — Size Mismatch in DifferencedObservablePartial

The `DifferencedObservablePartial<1>` for `position_angle` and `separation` has a subtlety: the undifferenced observables are `angular_position` (size 2), but the differenced observable is size 1. The `DifferencedObservablePartial` template expects the undifferenced partials to also be size 1.

**Solution**: The `DifferencedObservablePartial<1>` will receive `DirectObservationPartial<1>` objects from the undifferenced extraction. But `AngularPositionScaling` produces size-2 partials. This means we need the `UndifferencedObservationModelExtractor<1>` to produce `AngularPositionObservationModel` objects (size 2), but the partial creation for those needs to produce size-1 partials.

**Alternative approach** (simpler): Instead of using `DifferencedObservablePartial`, implement the position_angle and separation partials as **direct** partials (not differenced). This means:

1. In `ObservationPartialCreator<1>`, for `position_angle` and `separation`, call `createSingleLinkObservationPartials` (not `createDifferencedObservablePartials`)
2. The `PositionAngleScaling` and `SeparationScaling` classes directly compute the 1×3 Jacobian from the 3 link end states
3. No need for `UndifferencedObservationModelExtractor<1>` entries for these types

This is actually cleaner because position angle and separation are not truly "differenced" observables in the same way as `relative_angular_position` (which is literally $\alpha_2 - \alpha_1$). They are nonlinear functions of both angular positions.

**Revised approach for Part 6-8:**

- `ObservationPartialCreator<1>`: `position_angle` and `separation` → `createSingleLinkObservationPartials`
- `ObservationPartialCreator<2>`: `position_angle_and_separation` → `createSingleLinkObservationPartials`
- `PositionAngleScaling::update()` takes all 3 link end states and computes the 1×3 Jacobian directly
- `SeparationScaling::update()` same
- `PositionAngleAndSeparationScaling::update()` same but 2×3
- No entries needed in `DifferencedObservationPartialCreator` for these types
- No entries needed in `UndifferencedObservationModelExtractor` for these types
- `createDifferencedPositionPartialScalingObject` not needed for these types

The `update()` methods already implemented above follow this direct approach (they take all 3 link end states and compute the Jacobian directly), so the code above is consistent with this revised design.
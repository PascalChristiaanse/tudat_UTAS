/*    Copyright (c) 2010-2019, Delft University of Technology
 *    All rigths reserved
 *
 *    This file is part of the Tudat. Redistribution and use in source and
 *    binary forms, with or without modification, are permitted exclusively
 *    under the terms of the Modified BSD license. You should have received
 *    a copy of the license with this file. If not, please or visit:
 *    http://tudat.tudelft.nl/LICENSE.
 */

#ifndef TUDAT_CONSTANTTRANSMITTERFREQUENCY_H
#define TUDAT_CONSTANTTRANSMITTERFREQUENCY_H

#include "tudat/astro/orbit_determination/estimatable_parameters/estimatableParameter.h"
#include "tudat/astro/ground_stations/transmittingFrequencies.h"

namespace tudat
{

namespace estimatable_parameters
{

//! Interface class for estimation of a body's constant transmitter frequency parameter.
/*!
 *  Interface class for estimation of a body's constant transmitter frequency parameter. Interfaces the estimation with the 
 *  frequency parameter of a ConstantFrequencyInterpolator object stored in VehicleSystems.
 */
class ConstantTransmitterFrequencyParameter : public EstimatableParameter< double >
{
public:
    //! Constructor
    /*!
     *  Constructor
     *  \param frequencyInterpolator ConstantFrequencyInterpolator object of which frequency is a property
     *  \param associatedBody Name of body of which parameter is a property.
     */
    ConstantTransmitterFrequencyParameter( 
            const std::shared_ptr< ground_stations::ConstantFrequencyInterpolator > frequencyInterpolator, 
            const std::string& associatedBody ):
        EstimatableParameter< double >( constant_transmitter_frequency, associatedBody ), 
        frequencyInterpolator_( frequencyInterpolator )
    { }

    //! Destructor
    ~ConstantTransmitterFrequencyParameter( ) { }

    //! Get value of transmitter frequency.
    /*!
     *  Get value of transmitter frequency.
     *  \return Value of transmitter frequency [Hz]
     */
    double getParameterValue( )
    {
        return frequencyInterpolator_->getFrequency( );
    }

    //! Reset value of transmitter frequency.
    /*!
     *  Reset value of transmitter frequency.
     *  \param parameterValue New value of transmitter frequency [Hz]
     */
    void setParameterValue( double parameterValue )
    {
        frequencyInterpolator_->setFrequency( parameterValue );
    }

    //! Function to retrieve the size of the parameter
    /*!
     *  Function to retrieve the size of the parameter
     *  \return Size of parameter value, 1 for this parameter
     */
    int getParameterSize( )
    {
        return 1;
    }

protected:

private:
    //! ConstantFrequencyInterpolator object of which frequency is a property
    std::shared_ptr< ground_stations::ConstantFrequencyInterpolator > frequencyInterpolator_;
};

}  // namespace estimatable_parameters

}  // namespace tudat

#endif  // TUDAT_CONSTANTTRANSMITTERFREQUENCY_H

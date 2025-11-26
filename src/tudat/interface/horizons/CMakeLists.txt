#    Copyright (c) 2010-2024, Delft University of Technology
#    All rigths reserved
#
#    This file is part of the Tudat. Redistribution and use in source and
#    binary forms, with or without modification, are permitted exclusively
#    under the terms of the Modified BSD license. You should have received
#    a copy of the license with this file. If not, please or visit:
#    http://tudat.tudelft.nl/LICENSE.
#

# Set the source files.
set(horizons_interface_SOURCES
        "horizonsInterface.cpp"
        )

# Set the header files.
set(horizons_interface_HEADERS
        "horizonsInterface.h"
        )

# Find required dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Add library  
TUDAT_ADD_LIBRARY("horizons_interface"
        "${horizons_interface_SOURCES}"
        "${horizons_interface_HEADERS}"
        PRIVATE_LINKS
            CURL::libcurl
            nlohmann_json::nlohmann_json
        )
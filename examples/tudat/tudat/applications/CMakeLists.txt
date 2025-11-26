#    Copyright (c) 2010-2019, Delft University of Technology
#    All rigths reserved
#
#    This file is part of the Tudat. Redistribution and use in source and
#    binary forms, with or without modification, are permitted exclusively
#    under the terms of the Modified BSD license. You should have received
#    a copy of the license with this file. If not, please or visit:
#    http://tudat.tudelft.nl/LICENSE.

# Build kplo_vlbi_estimation application

find_package(nlohmann_json 3.2.0 REQUIRED)

TUDAT_ADD_EXECUTABLE(kplo_vlbi_estimation
        "kplo_vlbi_estimation.cpp"
        ${Tudat_ESTIMATION_LIBRARIES}
        )
target_link_libraries(kplo_vlbi_estimation PUBLIC ${Tudat_ESTIMATION_LIBRARIES})

# Build horizons_example application (demonstrates pure C++ Horizons interface)
find_package(CURL REQUIRED)
TUDAT_ADD_EXECUTABLE(horizons_example
        "horizons_example.cpp"
        ${Tudat_ESTIMATION_LIBRARIES}
        )
target_link_libraries(horizons_example 
        PUBLIC ${Tudat_ESTIMATION_LIBRARIES}
        PRIVATE tudat_horizons_interface CURL::libcurl
        )

# Build bepicolombo application with liblro library
# add_library(lro STATIC liblro.cpp liblro.h)
# target_include_directories(lro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(lro PUBLIC ${Tudat_ESTIMATION_LIBRARIES})

# TUDAT_ADD_EXECUTABLE(bepicolombo
#         "bepicolombo.cpp"
#         ${Tudat_ESTIMATION_LIBRARIES}
#         )
# target_link_libraries(bepicolombo PRIVATE lro)

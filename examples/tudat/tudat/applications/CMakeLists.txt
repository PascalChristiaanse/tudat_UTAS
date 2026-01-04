#    Copyright (c) 2010-2019, Delft University of Technology
#    All rigths reserved
#
#    This file is part of the Tudat. Redistribution and use in source and
#    binary forms, with or without modification, are permitted exclusively
#    under the terms of the Modified BSD license. You should have received
#    a copy of the license with this file. If not, please or visit:
#    http://tudat.tudelft.nl/LICENSE.

# Build horizons_example application (demonstrates pure C++ Horizons interface)
# find_package(CURL REQUIRED)
# TUDAT_ADD_EXECUTABLE(horizons_example
#         "horizons_example.cpp"
#         ${Tudat_ESTIMATION_LIBRARIES}
#         )
# target_link_libraries(horizons_example 
#         PUBLIC ${Tudat_ESTIMATION_LIBRARIES}
#         PRIVATE tudat_horizons_interface CURL::libcurl
#         )

# Build hdf5SingleProcess example application
# find_package(HighFive CONFIG REQUIRED)
# find_package(HDF5 REQUIRED COMPONENTS CXX)

# # Build hdf5_orbit_example - demonstrates HDF5 output for orbit propagation
# TUDAT_ADD_EXECUTABLE(hdf5_orbit_example
#         "hdf5_orbit_example.cpp"
#         ${Tudat_ESTIMATION_LIBRARIES}
#         )
# target_link_libraries(hdf5_orbit_example 
#         PUBLIC ${Tudat_ESTIMATION_LIBRARIES}
#         PRIVATE HighFive HDF5::HDF5
#         )

# Build hdf5_observation_example - demonstrates HDF5 output for observation data
# TUDAT_ADD_EXECUTABLE(hdf5_observation_example
# "hdf5_observation_example.cpp"
# ${Tudat_ESTIMATION_LIBRARIES}
# )
# target_link_libraries(hdf5_observation_example 
# PUBLIC ${Tudat_ESTIMATION_LIBRARIES}
# PRIVATE HighFive HDF5::HDF5
# )

# Build simulateSpacecraft - spacecraft simulation with Horizons ephemeris
# TUDAT_ADD_EXECUTABLE(simulateSpacecraft
#         "simulateSpacecraft.cpp"
#         ${Tudat_ESTIMATION_LIBRARIES}
#         )
# target_link_libraries(simulateSpacecraft 
#         PUBLIC ${Tudat_ESTIMATION_LIBRARIES}
#         PRIVATE HighFive HDF5::HDF5 tudat_horizons_interface CURL::libcurl
#         )

# # Build estimateGeneralized - generalized state estimation with CLI args and JSON config
# TUDAT_ADD_EXECUTABLE(estimateGeneralized
#         "estimateGeneralized.cpp"
#         ${Tudat_ESTIMATION_LIBRARIES}
#         )
# target_link_libraries(estimateGeneralized 
#         PUBLIC ${Tudat_ESTIMATION_LIBRARIES}
#         PRIVATE HighFive HDF5::HDF5 tudat_horizons_interface CURL::libcurl
#         )

# Build bepicolombo application with liblro library
# add_library(lro STATIC liblro.cpp liblro.h)
# target_include_directories(lro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(lro PUBLIC ${Tudat_ESTIMATION_LIBRARIES})

# TUDAT_ADD_EXECUTABLE(bepicolombo
#         "bepicolombo.cpp"
#         ${Tudat_ESTIMATION_LIBRARIES}
#         )
# target_link_libraries(bepicolombo PRIVATE lro)

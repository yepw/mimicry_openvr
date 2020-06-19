cmake_minimum_required(VERSION 2.8.3)
project(mimicry_openvr)


find_package(PkgConfig)
PKG_CHECK_MODULES(OpenVRSDK REQUIRED openvr)

###########
## Build ##
###########

## Specify additional locations of header files
include_directories(
  include
  ${OpenVRSDK_INCLUDEDIR}
)

## Declare a C++ library
add_library(mimicry_app STATIC
  src/mimicry_app.cpp
)

## Declare a C++ executable
add_executable(param_writer 
  src/param_writer.cpp
)

add_executable(mimicry_control
  src/mimicry_control.cpp
)


## Specify libraries to link a library or executable target against
target_link_libraries(param_writer
  ${OpenVRSDK_LIBRARIES}
)

target_link_libraries(mimicry_app
  ${OpenVRSDK_LIBRARIES}
)

target_link_libraries(mimicry_control
  mimicry_app
)

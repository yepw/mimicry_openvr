cmake_minimum_required(VERSION 2.8.3)
project(mimicry_openvr)

find_package(catkin REQUIRED COMPONENTS 
  roscpp
)

find_package(PkgConfig)
PKG_CHECK_MODULES(OpenVRSDK REQUIRED openvr)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if your package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
   INCLUDE_DIRS include
#   LIBRARIES vectors
   CATKIN_DEPENDS roscpp
#  DEPENDS system_lib
)

###########
## Build ##
###########

## Specify additional locations of header files
include_directories(
  include
  include/glm
  ${OpenVRSDK_INCLUDEDIR}
  ${catkin_INCLUDE_DIRS}
)

## Declare a C++ library
add_library(mimicry_app STATIC
  src/mimicry_app.cpp
)

## Declare a C++ executable
add_executable(param_writer 
  src/param_writer.cpp
)

add_executable(updated_params
  src/updated_params.cpp
)
add_dependencies(updated_params ${catkin_EXPORTED_TARGETS})

add_executable(mimicry_control
  src/mimicry_control.cpp
)


## Specify libraries to link a library or executable target against
target_link_libraries(param_writer
  ${OpenVRSDK_LIBRARIES}
)

target_link_libraries(updated_params
  ${OpenVRSDK_LIBRARIES}
  ${catkin_LIBRARIES}
)

target_link_libraries(mimicry_app
  ${OpenVRSDK_LIBRARIES}
)

target_link_libraries(mimicry_control
  mimicry_app
  ${catkin_LIBRARIES}
)

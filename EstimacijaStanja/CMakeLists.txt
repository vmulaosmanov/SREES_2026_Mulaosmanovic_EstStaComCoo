cmake_minimum_required(VERSION 3.17)

set(SOLUTION_NAME EstimacijaStanja)	#Unijeti naziv solution-a

#BEGIN common part for all solutions
project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
if (WIN32)
	set(HOME_ROOT $ENV{USERPROFILE})
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
else()
	set(HOME_ROOT $ENV{HOME})
endif()
set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)
include(${NATID_SDK_ROOT}/DevEnv/Common.cmake)
#END common part for all solutions

include(${NATID_SDK_ROOT}/DevEnv/MatrixLib.cmake)
include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

set(SE3BUS_PLIST ${CMAKE_CURRENT_LIST_DIR}/src/Info.plist)

file(GLOB SE3BUS_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB SE3BUS_INCS    ${CMAKE_CURRENT_LIST_DIR}/src/*.h)

add_executable(${SOLUTION_NAME} ${SE3BUS_INCS} ${SE3BUS_SOURCES})

source_group("src" FILES ${SE3BUS_SOURCES} ${SE3BUS_INCS})

target_link_libraries(${SOLUTION_NAME} debug ${MU_LIB_DEBUG} optimized ${MU_LIB_RELEASE}
                    debug ${MATRIX_LIB_DEBUG} optimized ${MATRIX_LIB_RELEASE}
                    debug ${NATGUI_LIB_DEBUG} optimized ${NATGUI_LIB_RELEASE})

setTargetPropertiesForGUIApp(${SOLUTION_NAME} ${SE3BUS_PLIST})

setIDEPropertiesForGUIExecutable(${SOLUTION_NAME} ${CMAKE_CURRENT_LIST_DIR})

setPlatformDLLPath(${SOLUTION_NAME})

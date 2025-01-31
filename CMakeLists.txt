cmake_minimum_required(VERSION 3.12)

project(OpenSPH VERSION 0.4.0 LANGUAGES CXX)

option(WITH_TBB "Enable TBB parallelization" ON)
option(WITH_EIGEN "Enable additional algorithms using Eigen" OFF)
option(WITH_CHAISCRIPT "Enable scripting tools" OFF)
option(WITH_VDB "Enable conversion to OpenVDB files" OFF)
option(BUILD_UTILS "Build auxiliary utilities" OFF)
option(USE_SINGLE_PRECISION "Build OpenSPH with single precision" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

add_definitions(-DSPH_VERSION=${CMAKE_PROJECT_VERSION})

if (WIN32)
    add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE -D_CRT_SECURE_NO_WARNINGS)
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base adv propgrid aui)
include(${wxWidgets_USE_FILE})

if (WITH_EIGEN)
    find_package(Eigen REQUIRED)
    include_directories(${EIGEN_INCLUDE_DIRS})
    add_definitions(-DSPH_USE_EIGEN)
endif()

if (USE_SINGLE_PRECISION)
    message(STATUS "Building ${CMAKE_PROJECT_NAME} with single precision")
    add_definitions(-DSPH_SINGLE_PRECISION)
endif()

if (WITH_CHAISCRIPT)
    add_definitions(-DSPH_USE_CHAISCRIPT)
    if (WIN32)
        set(CHAISCRIPT_INCLUDE_DIR "Chaiscript_NOT_FOUND" CACHE PATH "Directory containing Chaiscript files")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
        include_directories(${CHAISCRIPT_INCLUDE_DIR})
    else()
        set(CHAISCRIPT_LIBRARIES -ldl)
    endif()
endif()

if (WITH_TBB)
    find_package(TBB REQUIRED tbb tbbmalloc)
    add_definitions(-DSPH_USE_TBB)
    set(TBB_LIBRARIES TBB::tbb TBB::tbbmalloc)
    if(${Tbb_VERSION_MAJOR} LESS 2021)
        message(STATUS "Using classic TBB")
        add_definitions(-DTBB_SUPPRESS_DEPRECATED_MESSAGES)
    else()
        message(STATUS "Using oneTBB")
    endif()
endif()

if (WITH_VDB)
    find_package(OpenVDB REQUIRED)
    add_definitions(-DSPH_USE_VDB)
    include_directories(${OpenVDB_INCLUDE_DIRS})

    # OpenVDB requires TBB
    if (NOT WITH_TBB)
        find_package(TBB REQUIRED tbb)
        set(TBB_LIBRARIES TBB::tbb)
    endif()

    # Workaround for OpenVDB before version 8.1
    if (OpenVDB_MAJOR_VERSION LESS "9" OR (OpenVDB_MAJOR_VERSION EQUAL "8" AND OpenVDB_MINOR_VERSION LESS "1"))
        message("OpenVDB ${OpenVDB_VERSION} version requires external IlmBase IMath Half implementation.")
        find_package(IlmBase COMPONENTS Half REQUIRED) # "Half" for "libHalf"
    endif()

    # if IlmBase was required (either directly or indirectly via OpenVDB) add it to all occurances of OpenVDB
    set(OpenVDB_ALL_LIBRARIES ${OpenVDB_LIBRARIES} ${IlmBase_LIBRARIES} ${TBB_LIBRARIES})
endif()

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -msse4.1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native -ffast-math")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DSPH_DEBUG)
endif()

add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(gui/launcherGui)
add_subdirectory(cli/launcher)
add_subdirectory(cli/info)

if (BUILD_UTILS)
    add_subdirectory(cli/ssftotxt)
endif()

TEMPLATE = app
CONFIG += c++14 thread silent
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -msse4.1 -Wall -Werror -std=c++14 -pthread
QMAKE_CXX = g++

INCLUDEPATH += ../lib ../../external/Catch/include
DEPENDPATH += . ../lib
LIBS += ../lib/libsph.a


CONFIG(release, debug|profile|release) {
  message( "SPH PROBLEMS --- Building for Release" )
}

CONFIG(profile, debug|profile|release) {
  message( "SPH PROBLEMS --- Building for Profile" )
  DEFINES += PROFILE
}

CONFIG(debug, debug|profile|release) {
  message( "SPH PROBLEMS --- Building for Debug" )
  DEFINES += DEBUG PROFILE
}

SOURCES += \
    main.cpp \
    sod/Sod.cpp

HEADERS += \  
    sod/solution.h

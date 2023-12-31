cmake_minimum_required (VERSION 3.13)

project(Soundwire)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES
source/CBitstreamDecoder.h
source/CBitstreamDecoder.cpp
source/CControlWordBuilder.h
source/CControlWordBuilder.cpp
source/CDynamicSyncGenerator.h
source/CDynamicSyncGenerator.cpp
source/CFrameReader.h
source/CFrameReader.cpp
source/CSyncFinder.h
source/CSyncFinder.cpp
source/SoundWireAnalyzer.cpp
source/SoundWireAnalyzerResults.h
source/SoundWireSimulationDataGenerator.cpp
source/SoundWireAnalyzer.h
source/SoundWireAnalyzerSettings.cpp
source/SoundWireProtocolDefs.h
source/SoundWireSimulationDataGenerator.h
source/SoundWireAnalyzerResults.cpp
source/SoundWireAnalyzerSettings.h
source/SoundWireProtocolDefs.h
source/SoundWireProtocolDefs.cpp
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})

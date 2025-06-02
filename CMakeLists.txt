cmake_minimum_required(VERSION 4.0.2)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

add_subdirectory(engine/src)

project(Caliope VERSION 1.0)

file(GLOB_RECURSE ENGINE_SRC_FILES engine/src/**.cpp)
add_library(engine SHARED ${ENGINE_SRC_FILES})
target_compile_definitions(engine PRIVATE CE_PLATFORM_WINDOWS=1 CE_EXPORT_DLL=1)
target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/engine/src)
target_precompile_headers(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/engine/src/cepch.h)
#find_package(spdlog)
#target_link_libraries(engine spdlog) 

file(GLOB_RECURSE SRC_FILES testbed/src/*.cpp)
add_executable(testbed ${SRC_FILES})
target_include_directories(testbed PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/testbed/src)
target_compile_definitions(testbed PRIVATE CE_PLATFORM_WINDOWS=1 CE_EXPORT_DLL=0)
target_link_libraries(testbed engine)

IF(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT testbed)
ENDIF()
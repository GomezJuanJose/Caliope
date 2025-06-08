Include(FetchContent)

cmake_minimum_required(VERSION 4.0.2)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)


find_package(Vulkan REQUIRED)



project(Caliope VERSION 1.0)

file(GLOB_RECURSE ENGINE_SRC_FILES engine/src/**.cpp)
file(GLOB_RECURSE ENGINE_HEADERS_FILES engine/src/*.h)
file(GLOB_RECURSE ENGINE_INL_FILES engine/src/*.inl)

add_library(engine SHARED ${ENGINE_SRC_FILES} ${ENGINE_HEADERS_FILES} ${ENGINE_INL_FILES})
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/src)
target_compile_definitions(engine PRIVATE CE_PLATFORM_WINDOWS=1 CE_EXPORT_DLL=1)
target_precompile_headers(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/engine/src/cepch.h)


FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.9.2
)
FetchContent_MakeAvailable(spdlog)
target_link_libraries(engine PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_MakeAvailable(glfw)
target_link_libraries(engine PRIVATE glfw Vulkan::Vulkan)


file(GLOB_RECURSE SRC_FILES testbed/src/*.cpp)
file(GLOB_RECURSE HEADERS_FILES testbed/src/*.h)
add_executable(testbed ${SRC_FILES} ${HEADERS_FILES})
target_include_directories(testbed PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/testbed/src)
target_compile_definitions(testbed PRIVATE CE_PLATFORM_WINDOWS=1 CE_EXPORT_DLL=0)
target_link_libraries(testbed engine)

IF(WIN32)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT testbed)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ENGINE_HEADERS_FILES} ${ENGINE_SRC_FILES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS_FILES} ${SRC_FILES})
ENDIF()


file(GLOB_RECURSE ENGINE_SRC_FILES src/**.cpp)
file(GLOB_RECURSE ENGINE_HEADERS_FILES src/*.h)
file(GLOB_RECURSE ENGINE_INL_FILES src/*.inl)

add_library(caliope_engine SHARED ${ENGINE_SRC_FILES} ${ENGINE_HEADERS_FILES} ${ENGINE_INL_FILES})

target_sources(caliope_engine PRIVATE ${ENGINE_SRC_FILES} ${ENGINE_HEADERS_FILES} ${ENGINE_INL_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${ENGINE_SRC_FILES} ${ENGINE_HEADERS_FILES} ${ENGINE_INL_FILES})

target_include_directories(caliope_engine PUBLIC src)

target_compile_definitions(caliope_engine PRIVATE CE_PLATFORM_WINDOWS=1 CE_EXPORT_DLL=1)

target_precompile_headers(caliope_engine PRIVATE src/cepch.h)

target_link_libraries(caliope_engine PRIVATE glfw Vulkan::Vulkan)
target_link_libraries(caliope_engine PRIVATE glm::glm)
target_link_libraries(caliope_engine PRIVATE miniaudio)
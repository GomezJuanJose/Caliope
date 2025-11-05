file(GLOB_RECURSE SRC_FILES src/*.cpp)
file(GLOB_RECURSE HEADERS_FILES src/*.h)

add_executable(testbed ${SRC_FILES} ${HEADERS_FILES})

target_sources(testbed PRIVATE ${SRC_FILES} ${HEADERS_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SRC_FILES} ${HEADERS_FILES})

target_include_directories(testbed PRIVATE src)

target_compile_definitions(testbed PRIVATE CE_PLATFORM_WINDOWS=1 CE_EXPORT_DLL=0)

target_link_libraries(testbed caliope_engine)
target_link_libraries(testbed glm::glm)

add_custom_command(TARGET testbed POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:testbed> 
        $<TARGET_RUNTIME_DLLS:testbed>
    COMMAND_EXPAND_LISTS
)


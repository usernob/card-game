cmake_minimum_required(VERSION 3.20)

project(card-game)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_CONFIGURATION_TYPES) # Multi-config check
    foreach(outputconfig ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${outputconfig} outputconfig_upper)

        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${outputconfig_upper}
            ${CMAKE_BINARY_DIR}/bin
        )
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${outputconfig_upper}
            ${CMAKE_BINARY_DIR}/bin/lib
        )
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${outputconfig_upper}
            ${CMAKE_BINARY_DIR}/lib
        )
    endforeach()
else() # Single config
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

file(GLOB_RECURSE SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(external)

find_package(SDL3 REQUIRED CONFIG)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE SDL3::SDL3
                            SDL3_image::SDL3_image
)

add_custom_target(
    copy-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/bin/assets
)

add_dependencies(${PROJECT_NAME} copy-files)

if(MSVC)
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
else()
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif()

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
        SDL_MAIN_USE_CALLBACKS
        ASSETS_PATH=$<IF:$<CONFIG:Debug>,"${CMAKE_CURRENT_SOURCE_DIR}/assets","./assets">
)

# add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND}
# -E copy_directory ${CMAKE_SOURCE_DIR}/assets
# ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets COMMENT "copying assets" )

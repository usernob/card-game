cmake_minimum_required(VERSION 3.20)

project(card-game)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# set output directory
function(set_output_directories base_dir)
    if(CMAKE_CONFIGURATION_TYPES) # Multi-config check
        foreach(outputconfig ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${outputconfig} outputconfig_upper)
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${outputconfig_upper}
                ${CMAKE_BINARY_DIR}/${base_dir}
                PARENT_SCOPE
            )
            set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${outputconfig_upper}
                ${CMAKE_BINARY_DIR}/${base_dir}/lib
                PARENT_SCOPE
            )
            set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${outputconfig_upper}
                ${CMAKE_BINARY_DIR}/lib
                PARENT_SCOPE
            )
        endforeach()
    else() # Single config
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
            ${CMAKE_BINARY_DIR}/${base_dir}
            PARENT_SCOPE
        )
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
            ${CMAKE_BINARY_DIR}/${base_dir}/lib
            PARENT_SCOPE
        )
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
            ${CMAKE_BINARY_DIR}/lib
            PARENT_SCOPE
        )
    endif()
endfunction()

if((APPLE AND NOT CMAKE_SYSTEM_NAME MATCHES "Darwin") OR EMSCRIPTEN)
    set(BUILD_SHARED_LIBS
        OFF
        CACHE INTERNAL ""
    ) # Disable shared builds on platforms where it does not make sense to use
      # them
    set(SDL_SHARED OFF)
else()
    set(SDL_SHARED ON)
endif()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp
     ${CMAKE_SOURCE_DIR}/src/*.h
)

set_output_directories("bin")
add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(external)

# copying assets to runtime directory
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Updating assets..."
)

# i don't know but msvc seems forced to use C++20
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX
        ".html"
        CACHE INTERNAL ""
    )
    set_target_properties(
        ${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file assets@/assets"
    )
    set(ASSETS_PATH_VALUE "/assets") # Virtual path di browser
else()
    set(ASSETS_PATH_VALUE
        $<IF:$<CONFIG:Debug>,"${CMAKE_CURRENT_SOURCE_DIR}/assets","./assets">
    )
endif()
# add defininition
target_compile_definitions(
    ${PROJECT_NAME} PRIVATE SDL_MAIN_USE_CALLBACKS
                            ASSETS_PATH=${ASSETS_PATH_VALUE}
)

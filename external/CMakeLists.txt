add_subdirectory(cram)

set(JSON_BuildTests
    OFF
    CACHE INTERNAL ""
)

add_subdirectory(json)

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

add_subdirectory(sdl)
add_subdirectory(sdl-image)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE SDL3::SDL3
                            SDL3_image::SDL3_image
)

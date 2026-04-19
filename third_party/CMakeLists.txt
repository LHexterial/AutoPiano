# 管控第三方库
add_subdirectory(midifile)

# imgui 库
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/imgui/core IMGUI_SRC_PATH)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends IMGUI_BACKENDS_PATH)

add_library(
    ImGuiLib
    STATIC
    ${IMGUI_SRC_PATH}
    ${IMGUI_BACKENDS_PATH}
)

target_include_directories(ImGuiLib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/core
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)
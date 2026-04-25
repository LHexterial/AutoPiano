project(
    Core 
    VERSION 1.0.0
    LANGUAGES CXX
)

# 指定源文件地址
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src CORE_SRC_PATH)

add_library(CoreLib
STATIC
${CORE_SRC_PATH}
)

target_include_directories(CoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(CoreLib PRIVATE winmm midifile)
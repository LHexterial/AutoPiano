cmake_minimum_required(VERSION 3.15)
project(AutoPiano VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置库文件的地址
set(OUT_PATH_LIB ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUT_PATH_LIB})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUT_PATH_LIB})
# 设置可执行文件的地址
set(OUT_PATH_EXE ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUT_PATH_EXE})

# 传递定义宏模块
add_definitions(
    -DPROJECT_NAME="${PROJECT_NAME}"
    -DPROJECT_VERSION="${PROJECT_VERSION}"
)

# 下分模块
add_subdirectory(third_party)
add_subdirectory(core)
add_subdirectory(app)
add_subdirectory(modules)
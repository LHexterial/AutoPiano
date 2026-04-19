
add_executable(${PROJECT_NAME}_App WIN32 main.cpp resource.rc)
set_target_properties(${PROJECT_NAME}_App PROPERTIES RUNTIME_OUTPUT_DIRECTORY
${CMAKE_CURRENT_SOURCE_DIR}/bin
)
# set_target_properties(${PROJECT_NAME}_App PROPERTIES WIN32_EXECUTABLE FALSE)
# 链接核心库

target_link_libraries(${PROJECT_NAME}_App 
PRIVATE CoreLib
GuiLib
)
if(MINGW)
    target_link_options(${PROJECT_NAME}_App PRIVATE "-Wl,--subsystem,console")
endif()

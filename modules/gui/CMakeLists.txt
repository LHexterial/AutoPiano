project(gui)
add_library(GuiLib
STATIC
src/ui_manager.cpp
)
target_include_directories(GuiLib PUBLIC include)
target_link_libraries(GuiLib PRIVATE
ImGuiLib
CoreLib
d3d11
d3dcompiler
dwmapi
comdlg32
)
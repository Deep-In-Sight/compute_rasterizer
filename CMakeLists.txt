cmake_minimum_required(VERSION 3.10)

# Set the project name
project(ComputeRasterizer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# includes the bundled libs
set(GLM_STATIC_LIBRARY_ENABLE ON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/glm/glm)

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Quick)

# opengl
if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW)
endif()
find_package(OpenGL REQUIRED)
find_package(PDAL REQUIRED)

# Add all cpp files under CMAKE_CURRENT_SOURCE_DIR/src to the SOURCES variable
set(COMPUTE_RASTERIZER_LIB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Framebuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GLTimerQueries.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GLBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OrbitControls.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PointCloudRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/unsuck_platform_specific.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CameraController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PointCloudItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PointCloudItem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PointCloudQuickRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PointManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/compute_loop_las/compute_loop_las.cpp)

set(COMPUTE_RASTERIZER_LIB_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/glm ${GLEW_INCLUDE_DIRS})

set(EXTRA_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/implot/implot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/implot/implot_items.cpp)

set(EXTRA_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/implot
    ${GLFW3_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS})

message("PDAL_INCLUDE_DIRS: ${PDAL_INCLUDE_DIRS}")
message("PDAL_LIBRARIES: ${PDAL_LIBRARIES}")

set(LIBRARIES GLEW::GLEW glfw OpenGL::GL pthread Qt5::Quick ${PDAL_LIBRARIES})

add_library(ComputeRasterizerLib STATIC ${COMPUTE_RASTERIZER_LIB_SRCS})
target_include_directories(ComputeRasterizerLib
                           PUBLIC ${COMPUTE_RASTERIZER_LIB_INCLUDES})
target_link_libraries(ComputeRasterizerLib PUBLIC ${LIBRARIES})
set_target_properties(ComputeRasterizerLib PROPERTIES AUTOMOC ON)

# Add the executable
add_executable(
  ComputeRasterizerMain ${EXTRA_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                        ${CMAKE_CURRENT_SOURCE_DIR}/src/ImGuiOverlay.cpp)
# Include directories
target_include_directories(ComputeRasterizerMain PUBLIC ${EXTRA_INCLUDES})
# Link the libraries
target_link_libraries(ComputeRasterizerMain ComputeRasterizerLib)

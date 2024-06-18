cmake_minimum_required(VERSION 3.10)
project(myVoxelEngine)

set(CMAKE_CXX_STANDARD 17)

# Force to use dynamic runtime library (MDd for Debug, MD for Release)
if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif()

# Include GLFW
set(GLFW_ROOT ${CMAKE_SOURCE_DIR}/external/glfw)
set(GLFW_INCLUDE_DIR ${GLFW_ROOT}/include)
set(GLFW_LIBRARY ${GLFW_ROOT}/lib-vc2019/glfw3.lib)  # Update path if needed

# Include GLEW
set(GLEW_ROOT ${CMAKE_SOURCE_DIR}/external/glew)
set(GLEW_INCLUDE_DIR ${GLEW_ROOT}/include)
set(GLEW_LIBRARY ${GLEW_ROOT}/lib/Release/x64/glew32.lib)  # Ensure correct path and 64-bit library

# Include GLM
set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/glm)

# Add include directories
include_directories(${GLFW_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${GLM_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/include)

# Add source files
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARY} ${GLFW_LIBRARY} opengl32)
else()
    target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARY} ${GLFW_LIBRARY} GL)
endif()

# Add ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui-master)

file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)

target_link_libraries(${PROJECT_NAME} imgui)

# Add stb_image
set(STB_IMAGE_SOURCE ${CMAKE_SOURCE_DIR}/src/stb_image.cpp)
add_library(stb_image ${STB_IMAGE_SOURCE})
target_include_directories(stb_image PUBLIC ${CMAKE_SOURCE_DIR}/include/)
target_link_libraries(${PROJECT_NAME} stb_image)

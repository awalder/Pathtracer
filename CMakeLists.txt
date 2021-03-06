cmake_minimum_required(VERSION 3.12)
set(NAME pathtracer)
project(${NAME}
        VERSION 0.0.1 DESCRIPTION
                "Pathtracer for course 'Advanced graphics course'"
        LANGUAGES C CXX)

find_package(Vulkan REQUIRED)
set(VULKAN_LIBRARY ${Vulkan_LIBRARY})
find_library(ASSIMP_LIBRARY NAMES assimp PATHS ${CMAKE_SOURCE_DIR}/lib/assimp)
find_package(OpenMP)

find_library(glfw_LIBRARY
             NAMES glfw3.lib
             PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib/glfw)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/spdlog)
#add_subdirectory(${CMAKE_SOURCE_DIR}/external/sobol)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/NVIDIA_RTX)

if(MSVC)
#  add_compile_options("/W4" "$<$<CONFIG:RELEASE>:/O2>" "/fp:fast" "/arch:AVX2")
else()
#  add_compile_options("-Wall" "-Wextra" "-Werror" "$<$<CONFIG:RELEASE>:-03>")
endif()

# These does not seem to make any difference
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_INSTALL_DIR ${CMAKE_SOURCE_DIR}/bin)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY VS_STARTUP_PROJECT ${NAME})

add_executable(${NAME}
               src/main.cpp
               src/vkContext.cpp
               src/vkContext.h
               src/vkWindow.cpp
               src/vkWindow.h
               src/vkTools.cpp
               src/vkTools.h
               src/vkDebugLayers.cpp
               src/vkDebugLayers.h
               src/Model.cpp
               src/Model.h
               src/CameraControls.cpp
               src/CameraControls.h
               src/AreaLight.cpp
               src/AreaLight.h
               src/rtutils.cpp
               src/rtutils.h
               src/vkRTX_setup.cpp
               src/vkRTX_setup.h
               src/implementations.cpp)

target_link_libraries(${NAME}
                      PRIVATE ${Vulkan_LIBRARY}
                              OpenMP::OpenMP_CXX
                              ${glfw_LIBRARY}
                              ${ASSIMP_LIBRARY}
                              spdlog
                              imgui
                              rtlib)

target_include_directories(${NAME}
                           PUBLIC ${CMAKE_SOURCE_DIR}/external
                                  ${CMAKE_SOURCE_DIR}/external/spdlog/include)

target_compile_features(${NAME} PRIVATE cxx_std_17)

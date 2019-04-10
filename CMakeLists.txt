cmake_minimum_required(VERSION 3.12)
set(NAME acgPathtracer)
project(${NAME}
        VERSION 0.0.1 DESCRIPTION
                "Pathtracer for course 'Advanced graphics course'"
        LANGUAGES CXX)

# file(GLOB_RECURSE SRCs ${CMAKE_SOURCE_DIR}/src/*.cpp) file(GLOB_RECURSE
# HEADER_FILES ${CMAKE_SOURCE_DIR}/src/*.h)

find_package(Vulkan REQUIRED)
set(VULKAN_LIBRARY ${Vulkan_LIBRARY})
find_library(ASSIMP_LIBRARY NAMES assimp PATHS ${CMAKE_SOURCE_DIR}/lib/assimp)

# Some GLFW specifics
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build basic example applications")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_DOCS OFF BOOL)
set(GLFW_BUILD_INSTALL OFF CACHE BOOL "GLFW lib only")

add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/spdlog)

if(MSVC)
	add_compile_options("/W4" "$<$<CONFIG:RELEASE>:/O2>" "/fp:fast" "/arch:AVX2")
else()
	add_compile_options("-Wall" "-Wextra" "-Werror" "$<$<CONFIG:RELEASE>:-03>")
endif()

#set(CMAKE_BUILD_PREFIX ${CMAKE_SOURCE_DIR}/bin)

# These does not seem to make any difference
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_INSTALL_DIR ${CMAKE_SOURCE_DIR}/bin)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${NAME})


add_executable(${NAME} ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${NAME}
                      PRIVATE ${Vulkan_LIBRARY}
                              ${ASSIMP_LIBRARY}
                              glfw
                              ${GLFW_LIBRARIES}
							  spdlog)

target_include_directories(${NAME}
                           PUBLIC ${CMAKE_SOURCE_DIR}/external
                                  ${CMAKE_SOURCE_DIR}/external/glfw/include
								  ${CMAKE_SOURCE_DIR}/external/spdlog/include)

target_compile_features(${NAME} PRIVATE cxx_std_17)

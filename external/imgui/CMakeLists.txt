project(imgui)

find_package(Vulkan REQUIRED)

add_library(${PROJECT_NAME}
            STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui_impl_glfw_vulkan.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp)

add_library(imgui::imgui ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARY})
message(${Vulkan_INCLUDE_DIRS})

# IF( NOT ${Vulkan_INCLUDE_DIRS}) set(Vulkan_INCLUDE_DIRS
# ${CMAKE_SOURCE_DIR}/external/vulkan) ENDIF()

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                  ${CMAKE_SOURCE_DIR}/external)

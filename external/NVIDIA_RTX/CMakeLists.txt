project(rtlib)

find_package(Vulkan REQUIRED)

add_library(${PROJECT_NAME} STATIC
            vkRT_BLAS.cpp
            vkRT_BLAS.h
            vkRT_DescriptorSets.cpp
            vkRT_DescriptorSets.h
            vkRT_Import.cpp
            vkRT_Pipeline.cpp
            vkRT_Pipeline.h
            vkRT_SBT.cpp
            vkRT_SBT.h
            vkRT_TLAS.cpp
            vkRT_TLAS.h)

add_library(rtlib::rtlib ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARY})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_SOURCE_DIR}/external)
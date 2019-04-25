project(RTX)

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

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
project(sobol)

add_library(${PROJECT_NAME} STATIC sobol.cpp)

add_library(sobol::sobol ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

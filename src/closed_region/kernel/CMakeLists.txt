set(TARGET_NAME closed_region_kernel)


set(SRCS
region2d.cpp
)
set(HEADERS
region2d.h
)


add_library(${TARGET_NAME} ${SRCS} ${HEADERS})
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${TARGET_NAME} ysclass ysbitmap)

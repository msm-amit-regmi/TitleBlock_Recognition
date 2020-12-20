set(TARGET_NAME region2dcmd)

add_executable(${TARGET_NAME} main.cpp)
target_link_libraries(${TARGET_NAME} closed_region_kernel)

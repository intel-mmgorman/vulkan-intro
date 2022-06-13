cmake_minimum_required(VERSION 3.10)
# set the project name
project(vulkan-intro)
# add SDL framework
add_subdirectory(external/SDL)
# add the executable
add_executable(vulkan-intro src/main.cpp)
target_link_libraries(vulkan-intro 
    SDL2-static
    )

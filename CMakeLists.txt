cmake_minimum_required(VERSION 3.20)
project(UrFuSFMLProject CXX)

# Export compile commands for VS Code IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Add local directory prefix for SFML 3.1.0
list(APPEND CMAKE_PREFIX_PATH "/home/lin/.local")

# 2. Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 3. Find SFML components
set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3.1 REQUIRED COMPONENTS Graphics Window System)

# 4. Declare our executable and its source files
add_executable(sfml_app 
    src/main.cpp 
    src/Game.cpp
)

# 5. Include the include directory for headers
target_include_directories(sfml_app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 6. Link SFML targets
target_link_libraries(sfml_app PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System
)

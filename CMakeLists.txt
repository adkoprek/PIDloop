# Project setup
cmake_minimum_required(VERSION 3.16)
project(pidloop VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Remove the lib prefix from libs, adding it manualy
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Profiles
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()

# Creates the compile_commands.json file for your LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create symlink for compile_commands.json in root dir for lsp
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
)

# Enable the Qt specific stuff
set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Output path
set(OutputDirectory "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OutputDirectory}/obj")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OutputDirectory}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OutputDirectory}")

# Get the Qt library
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Add cafe
include_directories(/opt/gfa/cafe/cpp/cafe-1.19.2-gcc-7.3.0/include)
link_directories(/opt/gfa/cafe/cpp/cafe-1.19.2-gcc-7.3.0/lib/RHEL8-x86_64)

# Add epics
include_directories(/usr/local/epics/base-7.0.7/include)
include_directories(/usr/local/epics/base-7.0.7/include/os/Linux)
include_directories(/usr/local/epics/base-7.0.7/include/compiler/gcc)
link_directories(/usr/local/epics/base-7.0.7/lib/RHEL8-x86_64)

# Add QWT
include_directories(/usr/include/qt5/qwt/)
link_directories(/usr/lib64)

# Add the include directory
include_directories(tests)

# Add the subdirectories
add_subdirectory(forms)
add_subdirectory(src/app)
add_subdirectory(src/logic)

# Build the QT app
add_executable(pidloop
    ${UI_FILES}
    ${APP_SRC_FILES}
    main.cpp
)

# Add the include files to avoid relative paths
target_include_directories(pidloop PRIVATE src/app)
target_include_directories(pidloop PRIVATE src/logic)
target_include_directories(pidloop PRIVATE forms)

# Link Epics Chanel Acess to custom lib
target_link_libraries(libpidloop PRIVATE ca)

# Link cafe with custom lib
target_link_libraries(libpidloop PRIVATE cafe)

# Link Qt5, QWT and custom lib to the main executable 
target_link_libraries(pidloop PRIVATE Qt5::Widgets)
target_link_libraries(pidloop PRIVATE qwt-qt5)
target_link_libraries(pidloop PRIVATE libpidloop)

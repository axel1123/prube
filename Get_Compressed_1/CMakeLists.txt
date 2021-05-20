cmake_minimum_required(VERSION 3.14)

project(Get_Compressed LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#adding the library mlog for the logging
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/mlog")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

#Load Qt5 Libraries
find_package(Qt5 COMPONENTS Core Quick SerialPort REQUIRED)

file(GLOB CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/include/*.h")
file(GLOB RC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qrc")

add_executable( ${PROJECT_NAME}
                ${CPP_FILES}
                ${HEADER_FILES}
                ${RC_FILES}
              )

#set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX _d)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::SerialPort mlog)

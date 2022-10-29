#sgn
cmake_minimum_required(VERSION 3.1)
project(SGNAttendance)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(nlohmann_json REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/SQLiteCpp/sqlite3")

add_executable(SGNAttendance
    src/Logger.cpp
    src/SGNMain.cpp
    src/DBInterface.cpp
    src/RESTful.cpp
    )

target_link_libraries(SGNAttendance
	sqlite3
    SQLiteCpp
    -lpthread
    -lpistache)
add_subdirectory(SQLiteCpp)

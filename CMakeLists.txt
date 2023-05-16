cmake_minimum_required(VERSION 3.25)
project(backtesting)

set(CMAKE_CXX_STANDARD 20)

include_directories(deps/fmt-9.1.0/include)
add_subdirectory(deps/fmt-9.1.0)
include_directories(deps/spdlog-1.11.0/include)
add_subdirectory(deps/spdlog-1.11.0)
include_directories(deps/cxxopts-3.1.1/include)
add_subdirectory(deps/cxxopts-3.1.1)

find_package(SQLite3 REQUIRED)
include_directories(deps/sqlite_orm-1.8.2/include)
add_subdirectory(deps/sqlite_orm-1.8.2)

aux_source_directory(. SOURCE)
add_executable(${PROJECT_NAME} ${SOURCE})
target_link_libraries(${PROJECT_NAME} PRIVATE fmt-header-only)
target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3 sqlite_orm::sqlite_orm)

# copy file
configure_file(hs300_60min.sqlite ${CMAKE_BINARY_DIR}/hs300_60min.sqlite COPYONLY)

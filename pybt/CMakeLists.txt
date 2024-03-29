set(PYTHON_EXECUTABLE /usr/local/Caskroom/miniconda/bin/python3)
cmake_minimum_required(VERSION 3.20)
project(pybt)

set(CMAKE_CXX_STANDARD 20)

include_directories(deps/fmt-9.1.0/include)
add_subdirectory(deps/fmt-9.1.0)
include_directories(deps/spdlog-1.11.0/include)
add_subdirectory(deps/spdlog-1.11.0)
include_directories(deps/pybind11-2.10.4/include)
add_subdirectory(deps/pybind11-2.10.4)

find_package(SQLite3 REQUIRED)
include_directories(deps/sqlite_orm-1.8.2/include)
add_subdirectory(deps/sqlite_orm-1.8.2)

aux_source_directory(. SOURCE)
pybind11_add_module(${PROJECT_NAME} ${SOURCE})
target_link_libraries(${PROJECT_NAME} PRIVATE fmt-header-only)
target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3 sqlite_orm::sqlite_orm)

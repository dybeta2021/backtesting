cmake_minimum_required(VERSION 3.25)
project(backtesting)

set(CMAKE_CXX_STANDARD 20)

include_directories(deps/fmt-10.0.0/include)
add_subdirectory(deps/fmt-10.0.0)
include_directories(deps/spdlog-1.11.0/include)
add_subdirectory(deps/spdlog-1.11.0)

find_package(SQLite3 REQUIRED)
include_directories(deps/sqlite_orm-1.8.2/include)
add_subdirectory(deps/sqlite_orm-1.8.2)


add_executable(ama_value cross_ama.cpp)
target_link_libraries(ama_value PRIVATE fmt-header-only)
target_link_libraries(ama_value PRIVATE SQLite::SQLite3 sqlite_orm::sqlite_orm)

add_executable(ama_params parallel_params_cross_ama.cpp)
target_link_libraries(ama_params PRIVATE fmt-header-only)
target_link_libraries(ama_params PRIVATE SQLite::SQLite3 sqlite_orm::sqlite_orm)


#aux_source_directory(. SOURCE)
#add_executable(${PROJECT_NAME} ${SOURCE})
#target_link_libraries(${PROJECT_NAME} PRIVATE fmt-header-only)
#target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3 sqlite_orm::sqlite_orm)

### copy file
##configure_file(ts.db ${CMAKE_BINARY_DIR}/ts.db COPYONLY)
#
#
#add_custom_command(
#        TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        $<TARGET_FILE:${PROJECT_NAME}>
#        ${CMAKE_SOURCE_DIR}
#)
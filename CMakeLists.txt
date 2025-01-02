cmake_minimum_required(VERSION 3.16)
project(protogen_app_test)

include(FetchContent)

set(PROTOGEN_APP_ID "protogenapptest")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROTOGEN_APP_SOURCES
    "${PROJECT_SOURCE_DIR}/src/ProtogenAppTest.cpp"
)

add_library(${PROJECT_NAME} MODULE ${PROTOGEN_APP_SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)

## cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG 924f214303b860b78350e1e2dfb0521a8724464f
)
FetchContent_Populate(httplib)
add_library(httplib INTERFACE IMPORTED)
set_target_properties(httplib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${httplib_SOURCE_DIR}"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${httplib_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
configure_file("cmake_vars.h.in" "cmake_vars.h")

install(TARGETS ${PROJECT_NAME} DESTINATION "share/protogen/apps/${PROTOGEN_APP_ID}")
install(DIRECTORY resources DESTINATION "share/protogen/apps/${PROTOGEN_APP_ID}")
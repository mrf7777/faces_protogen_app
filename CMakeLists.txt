cmake_minimum_required(VERSION 3.16)
project(faces_protogen_app)

include(FetchContent)

set(PROTOGEN_APP_ID "faces")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROTOGEN_APP_SOURCES
    "${PROJECT_SOURCE_DIR}/src/Faces.cpp"
    "${PROJECT_SOURCE_DIR}/src/images.cpp"
    "${PROJECT_SOURCE_DIR}/src/protogen_head_state.cpp"
    "${PROJECT_SOURCE_DIR}/src/renderer.cpp"
)

add_library(${PROJECT_NAME} MODULE ${PROTOGEN_APP_SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Create targets for external libraries.
## GraphicsMagick
add_library(graphics_magick INTERFACE)
execute_process(
    COMMAND GraphicsMagick++-config --cppflags
    OUTPUT_VARIABLE GraphicsMagick_CPP_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND GraphicsMagick++-config --ldflags --libs
    OUTPUT_VARIABLE GraphicsMagick_LIBS_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\n" " " GraphicsMagick_LIBS "${GraphicsMagick_LIBS_RAW}")
separate_arguments(GraphicsMagick_LIBS)
target_compile_options(graphics_magick INTERFACE ${GraphicsMagick_CPP_FLAGS})
target_link_libraries(graphics_magick INTERFACE ${GraphicsMagick_LIBS})

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
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE graphics_magick)
configure_file("cmake_vars.h.in" "cmake_vars.h")

install(TARGETS ${PROJECT_NAME} DESTINATION "share/protogen/apps/${PROTOGEN_APP_ID}")
install(DIRECTORY resources DESTINATION "share/protogen/apps/${PROTOGEN_APP_ID}")
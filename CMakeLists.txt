cmake_minimum_required(VERSION 3.22.1)

project(handmadehero VERSION 0.1 LANGUAGES C)

# For clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable("${PROJECT_NAME}" src/main.c)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Debug>)
endif()

# NOTE: target name is different from project name
#       target name is one created with add_executable, add_library or add_custom_target

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Link with X11
  find_package(X11 REQUIRED)
  link_libraries(${X11_LIBRARIES})
  include_directories(${X11_INCLUDE_DIR})
  # https://cmake.org/cmake/help/latest/command/target_link_libraries.html
  target_link_libraries(
    "${PROJECT_NAME}"
    PRIVATE
    ${X11_LIBRARIES}
    -lX11
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-fsanitize=address>
  )
endif()

message("Project source dir is ${PROJECT_SOURCE_DIR}")
message("CMAKE_SYSTEM_NAME is ${CMAKE_SYSTEM_NAME}")

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.c)
target_sources(${PROJECT_NAME} PRIVATE ${sources})

target_include_directories("${PROJECT_NAME}"
  PUBLIC
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/include"
  )
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
target_compile_options(${PROJECT_NAME}
    PRIVATE
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Wimplicit -fsanitize=address>
    $<$<C_COMPILER_ID:MSVC>:/W4>
    )
#target_link_libraries(${PROJECT_NAME} user32)

if(MSVC)
  message("Using MSVC")
  target_compile_options(${PROJECT_NAME} PUBLIC "/ZI")
  target_link_options(${PROJECT_NAME} PUBLIC /DEBUG)
endif()

#message("Generator is set to ${CMAKE_GENERATOR}")

#target_link_libraries(${PROJECT_NAME} user32.lib gdi32.lib)

# set variable for the C preprocessor to detect the operatong system
add_compile_definitions("DEBUG=$<CONFIG:Debug>")
#if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#  target_compile_definitions(ostest PUBLIC "LINUX_OS")
#endif()
#if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#  target_compile_definitions(ostest PUBLIC "MACOS_OS")
#endif()
#if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
#  target_compile_definitions(ostest PUBLIC "WINDOWS_OS")
#endif()

cmake_minimum_required(VERSION 3.0.0)
project(realtime-raytracing-demo VERSION 0.0.1)
set(CMAKE_C_STANDARD 11)

add_executable(realtime-raytracing-demo 
    main.c
    glad/src/gl.c
)
target_compile_options(realtime-raytracing-demo PRIVATE -Wall)

# Force regeneration when shader is changed
configure_file(${CMAKE_CURRENT_LIST_DIR}/rt.frag ${CMAKE_CURRENT_BINARY_DIR}/dummy.frag)

# Generate header to embed shader into binary
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/rt.h "const char* rt_frag_shader_source =")
file(READ ${CMAKE_CURRENT_LIST_DIR}/rt.frag SHADER_CONTENTS)

string(REGEX REPLACE ";" "\\\\\\\;" SHADER_CONTENTS "${SHADER_CONTENTS}")
string(REGEX REPLACE "\n" ";" SHADER_CONTENTS "${SHADER_CONTENTS}")
foreach(SHADER_LINE ${SHADER_CONTENTS})
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/rt.h "\n\"")
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/rt.h ${SHADER_LINE})
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/rt.h "\\n\"")
endforeach()

file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/rt.h "\n\"\\0\";\n")
include_directories(include ${CMAKE_CURRENT_BINARY_DIR})

# Include GLAD
include_directories(include glad/include)

# Setup GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(OpenGL_GL_PREFERENCE GLVND)
add_subdirectory(glfw)
target_link_libraries(realtime-raytracing-demo glfw)
find_package(OpenGL REQUIRED)
target_include_directories(realtime-raytracing-demo PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(realtime-raytracing-demo ${OPENGL_gl_LIBRARY})
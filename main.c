#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include <rt.h>

#define RESOLUTION_X 800
#define RESOLUTION_Y 600
#define CAMERA_MOVE_SPEED 0.02
#define CAMERA_ROTATE_SPEED 0.005

const char* vertex_shader_source = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main() {\n"
"gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

int u_resolution_location;

double mouse_x = RESOLUTION_X / 2.0;
double mouse_y = RESOLUTION_Y / 2.0;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

Vec3 vec_add(const Vec3 left, const Vec3 right) {
    Vec3 result = {left.x + right.x, left.y + right.y, left.z + right.z};
    return result;
}

Vec3 vec_sub(const Vec3 left, const Vec3 right) {
    Vec3 result = {left.x - right.x, left.y - right.y, left.z - right.z};
    return result;
}

Vec3 vec_mult_by_float(const Vec3 left, float right) {
    Vec3 result = {left.x * right, left.y * right, left.z * right};
    return result;
}

Vec3 vec_cross(const Vec3 left, const Vec3 right) {
    Vec3 result = {left.y * right.z - left.z * right.y, left.z * right.x - left.x * right.z, left.x * right.y - left.y * right.x};
    return result;
}

Vec3 vec_normalize(const Vec3 left) {
    float length = sqrtf(left.x * left.x + left.y * left.y + left.z * left.z);
    Vec3 result = {left.x / length, left.y / length, left.z / length};
    return result;
}

typedef struct {
    Vec3 position;
    Vec3 look_at;
    Vec3 vup;
} Camera;

Camera camera = {{0.0, 0.0, 0.0}, {0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}};

void on_resize(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glUniform2f(u_resolution_location, width, height);
}

void on_mouse_event(GLFWwindow* window, double x_position, double y_position) {
    double delta_x = x_position - mouse_x;
    double delta_y = mouse_y - y_position;
    mouse_x = x_position;
    mouse_y = y_position;
    camera.look_at = vec_normalize(vec_add(camera.look_at, vec_mult_by_float(vec_cross(camera.look_at, camera.vup), (delta_x * CAMERA_ROTATE_SPEED))));
    camera.look_at = vec_normalize(vec_add(camera.look_at, vec_mult_by_float(camera.vup, (delta_y * CAMERA_ROTATE_SPEED))));
}

int main(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(RESOLUTION_X, RESOLUTION_Y, "OpenGL", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window.\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        fprintf(stderr, "Failed to initialise OpenGL context.\n");
        glfwTerminate();
        return 1;
    }

    glViewport(0, 0, RESOLUTION_X, RESOLUTION_Y);
    glfwSetFramebufferSizeCallback(window, on_resize);
    glfwSetCursorPosCallback(window, on_mouse_event);

    int success;
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        fprintf(stderr, "Vertex shader compilation failed: %s", info_log);
        glfwTerminate();
        return 1;
    }

    unsigned int frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &rt_frag_shader_source, NULL);
    glCompileShader(frag_shader);
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
        fprintf(stderr, "Fragment shader compilation failed: %s", info_log);
        glfwTerminate();
        return 1;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, NULL, info_log);
        fprintf(stderr, "Program linking failed: %s", info_log);
        glfwTerminate();
        return 1;
    }
    // Pull uniforms into variables here.
    u_resolution_location = glGetUniformLocation(program, "u_resolution");
    int u_time_location = glGetUniformLocation(program, "u_time");
    int u_camera_location_location = glGetUniformLocation(program, "u_camera_location");
    int u_camera_lookat_location = glGetUniformLocation(program, "u_camera_lookat");
    int u_camera_vup_location = glGetUniformLocation(program, "u_camera_vup");
    glUseProgram(program);
    // Define uniforms here.
    glUniform2f(u_resolution_location, RESOLUTION_X, RESOLUTION_Y);
    glUniform1f(u_time_location, 0.0);
    glUniform3f(u_camera_location_location, camera.position.x, camera.position.y, camera.position.z);
    Vec3 worldspace_look_at = vec_add(camera.position, camera.look_at);
    glUniform3f(u_camera_lookat_location, worldspace_look_at.x, worldspace_look_at.y, worldspace_look_at.z);
    glUniform3f(u_camera_vup_location, camera.vup.x, camera.vup.y, camera.vup.z);
    glDeleteShader(vertex_shader);
    glDeleteShader(frag_shader);

    float rect_vertices[] = {
         1.0,  1.0,  0.0,
         1.0, -1.0,  0.0,
        -1.0, -1.0,  0.0,
        -1.0,  1.0,  0.0,
    };

    unsigned int triangle_draw_indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    unsigned int vbo, vao, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect_vertices), rect_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle_draw_indices), triangle_draw_indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.position = vec_add(camera.position, vec_mult_by_float(camera.look_at, CAMERA_MOVE_SPEED));
            glUniform3f(u_camera_location_location, camera.position.x, camera.position.y, camera.position.z);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.position = vec_sub(camera.position, vec_mult_by_float(camera.look_at, CAMERA_MOVE_SPEED));
            glUniform3f(u_camera_location_location, camera.position.x, camera.position.y, camera.position.z);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.position = vec_sub(camera.position, vec_mult_by_float(vec_normalize(vec_cross(camera.look_at, camera.vup)), CAMERA_MOVE_SPEED));
            glUniform3f(u_camera_location_location, camera.position.x, camera.position.y, camera.position.z);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.position = vec_add(camera.position, vec_mult_by_float(vec_normalize(vec_cross(camera.look_at, camera.vup)), CAMERA_MOVE_SPEED));
            glUniform3f(u_camera_location_location, camera.position.x, camera.position.y, camera.position.z);
        }
        Vec3 worldspace_look_at = vec_add(camera.position, camera.look_at);
        glUniform3f(u_camera_lookat_location, worldspace_look_at.x, worldspace_look_at.y, worldspace_look_at.z);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glUniform1f(u_time_location, glfwGetTime());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}
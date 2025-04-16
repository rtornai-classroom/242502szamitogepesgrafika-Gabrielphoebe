#include <array>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>
#include <GLM/gtc/type_ptr.hpp>
using namespace std;

static std::vector<glm::vec3> control_points = {
    glm::vec3(-0.5f, -0.5f, 0.0f),
    glm::vec3(-0.25f, 0.5f, 0.0f),
    glm::vec3(0.25f, 0.5f, 0.0f),
    glm::vec3(0.5f, -0.5f, 0.0f),
};

unsigned int factorial(unsigned int n) {
    return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

unsigned int binomial_coefficient(unsigned int n, unsigned int k) {
    return factorial(n) / (factorial(k) * factorial(n - k));
}

glm::vec3 bezier_point(const std::vector<glm::vec3>& points, float t) {
    int degree = points.size() - 1; // Degree of the curve
    glm::vec3 point(0.0f);

    for (int i = 0; i <= degree; ++i) {
        float coeff = binomial_coefficient(degree, i) * pow(1 - t, degree - i) * pow(t, i);
        point += points[i] * coeff;
    }

    return point;
}

#define num_vertex_buffers 1
#define num_vertex_arrays 1
GLuint VBO[num_vertex_buffers];
GLuint VAO[num_vertex_arrays];

int width = 600;
int height = 600;
char title[] = "Bezier Curve";

GLFWwindow* window = nullptr;
GLuint rendering_program;
GLint dragged_point = -1;

void print_shader_log(GLuint shader) {
    int length = 0;
    int chars_written = 0;
    char* log = nullptr;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 0) {
        log = (char*)malloc(length);

        glGetShaderInfoLog(shader, length, &chars_written, log);
        cout << "Shader Info Log: " << log << endl;
        free(log);
    }
}

void print_program_log(int prog) {
    int length = 0;
    int chars_written = 0;
    char* log = nullptr;

    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);

    if (length > 0) {
        log = (char*)malloc(length);

        glGetProgramInfoLog(prog, length, &chars_written, log);
        cout << "Program Info Log: " << log << endl;
        free(log);
    }
}

string read_shader_source(const char* file_path) {
    ifstream file_stream(file_path, ios::in);
    string content;
    string line;

    if (!file_stream.is_open()) {
        cerr << "Failed to open shader file: " << file_path << endl;
        return "";
    }

    while (getline(file_stream, line)) {
        content += line + "\n";
    }

    file_stream.close();

    return content;
}

GLuint create_shader_program() {
    string vert_shader_str = read_shader_source("vertexShader.glsl");
    string frag_shader_str = read_shader_source("fragmentShader.glsl");

    const char* vert_shader_src = vert_shader_str.c_str();
    const char* frag_shader_src = frag_shader_str.c_str();

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertex_shader, 1, &vert_shader_src, NULL);
    glShaderSource(fragment_shader, 1, &frag_shader_src, NULL);

    glCompileShader(vertex_shader);
    GLint vert_compiled;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vert_compiled);
    if (vert_compiled != 1) {
        cout << "Vertex compilation failed." << endl;
        print_shader_log(vertex_shader);
    }

    glCompileShader(fragment_shader);
    GLint frag_compiled;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &frag_compiled);
    if (frag_compiled != 1) {
        cout << "Fragment compilation failed." << endl;
        print_shader_log(fragment_shader);
    }

    GLuint vf_program = glCreateProgram();

    glAttachShader(vf_program, vertex_shader);
    glAttachShader(vf_program, fragment_shader);

    glLinkProgram(vf_program);
    GLint linked;
    glGetProgramiv(vf_program, GL_LINK_STATUS, &linked);
    if (linked != 1) {
        cout << "Shader linking failed." << endl;
        print_program_log(vf_program);
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return vf_program;
}

GLfloat distance_squared(glm::vec3 P1, glm::vec3 P2) {
    GLfloat dx = P1.x - P2.x;
    GLfloat dy = P1.y - P2.y;
    return dx * dx + dy * dy;
}

GLint get_active_point(vector<glm::vec3> p, GLfloat sensitivity, GLfloat x, GLfloat y) {
    GLfloat s = sensitivity * sensitivity; // Square the sensitivity threshold
    GLint size = p.size();
    GLint closest_point_index = -1;
    GLfloat min_distance = std::numeric_limits<GLfloat>::max();

    // Convert mouse position to normalized device coordinates
    GLfloat x_norm = x / static_cast<GLfloat>(width) * 2.0f - 1.0f;
    GLfloat y_norm = y / static_cast<GLfloat>(height) * 2.0f - 1.0f;

    glm::vec3 mouse_pos = glm::vec3(x_norm, y_norm, 0.0f);

    // Iterate through all points and find the closest one
    for (GLint i = 0; i < size; i++) {
        GLfloat distance = distance_squared(mouse_pos, p[i]);
        if (distance < min_distance && distance < s) {
            min_distance = distance;
            closest_point_index = i;
        }
    }

    return closest_point_index;
}

void render_scene() {
    glClear(GL_COLOR_BUFFER_BIT);
    glPointSize(10.0);
    glBindVertexArray(VAO[0]);
    glUseProgram(rendering_program);

    // Draw the control polygon sides in blue
    glUniform4f(glGetUniformLocation(rendering_program, "pointColor"), 0.0f, 0.0f, 1.0f, 1.0f);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, control_points.size() * sizeof(glm::vec3), &control_points[0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINE_STRIP, 0, control_points.size());

    // Draw the Bézier
    // Draw the Bézier curve points in red
    glUniform4f(glGetUniformLocation(rendering_program, "pointColor"), 1.0f, 0.0f, 0.0f, 1.0f);
    std::vector<glm::vec3> curve_points;
    for (int i = 0; i < control_points.size(); ++i) {
        curve_points.push_back(control_points[i]);
    }
    glBufferData(GL_ARRAY_BUFFER, curve_points.size() * sizeof(glm::vec3), curve_points.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, curve_points.size());

    // Draw the Bézier curve
    glUniform4f(glGetUniformLocation(rendering_program, "pointColor"), 0.0f, 1.0f, 0.0f, 1.0f);
    curve_points.clear();
    for (int i = 0; i <= 100; ++i) {
        float t = static_cast<float>(i) / 100.0f;
        curve_points.push_back(bezier_point(control_points, t));
    }
    glBufferData(GL_ARRAY_BUFFER, curve_points.size() * sizeof(glm::vec3), curve_points.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINE_STRIP, 0, curve_points.size());
}

void setup() {
    rendering_program = create_shader_program();
    glGenVertexArrays(num_vertex_arrays, VAO);
    glBindVertexArray(VAO[0]);
    glGenBuffers(num_vertex_buffers, VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, control_points.size() * sizeof(glm::vec3), &control_points[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void window_reshape_callback(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

bool left_mouse_button_pressed = false;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double x, y;

        glfwGetCursorPos(window, &x, &y);

        // Check if there is an active point being dragged
        dragged_point = get_active_point(control_points, 0.1f, x, height - y);

        if (dragged_point >= 0) {
            // If there is an active point being dragged, set the flag for dragging
            left_mouse_button_pressed = true;
        }
        else {
            // If not, add a new control point at the cursor position
            float x_norm = static_cast<float>(x) / static_cast<float>(width) * 2.0f - 1.0f;
            float y_norm = static_cast<float>(height - y) / static_cast<float>(height) * 2.0f - 1.0f;
            control_points.push_back(glm::vec3(x_norm, y_norm, 0.0f));

            // Update the buffer data
            glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
            glBufferData(GL_ARRAY_BUFFER, control_points.size() * sizeof(glm::vec3), &control_points[0], GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Redraw the scene
            render_scene();
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        double x, y;

        glfwGetCursorPos(window, &x, &y);

        // Check if there is any point near the cursor position
        int point_to_remove = get_active_point(control_points, 0.1f, x, height - y);

        if (point_to_remove >= 0) {
            // If a point is found near the cursor position, remove it
            control_points.erase(control_points.begin() + point_to_remove);

            // Update the buffer data
            glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
            glBufferData(GL_ARRAY_BUFFER, control_points.size() * sizeof(glm::vec3), &control_points[0], GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Redraw the scene
            render_scene();
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        dragged_point = -1;
        left_mouse_button_pressed = false; // Reset the flag for dragging
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (dragged_point >= 0) {
        // Invert y-axis
        ypos = height - ypos;

        // Convert mouse position to normalized device coordinates
        float x_norm = static_cast<float>(xpos) / static_cast<float>(width) * 2.0f - 1.0f;
        float y_norm = static_cast<float>(ypos) / static_cast<float>(height) * 2.0f - 1.0f;

        // Update the position of the dragged control point
        control_points[dragged_point] = glm::vec3(x_norm, y_norm, 0.0f);

        // Update the buffer data
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * control_points.size(), &control_points[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        render_scene();
    }
    else if (left_mouse_button_pressed) {
        ypos = height - ypos;

        float x_norm = static_cast<float>(xpos) / static_cast<float>(width) * 2.0f - 1.0f;
        float y_norm = static_cast<float>(ypos) / static_cast<float>(height) * 2.0f - 1.0f;

        control_points.push_back(glm::vec3(x_norm, y_norm, 0.0f));

        // Dynamically adjust the degree of the Bézier curve based on the number of control points
        // Degree = Number of Control Points - 1
        GLint degree = control_points.size() - 1;

        // Update the buffer data
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, control_points.size() * sizeof(glm::vec3), &control_points[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Redraw the scene
        render_scene();
    }
}

int main() {
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    window = glfwCreateWindow(width, height, title, NULL, NULL);

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        exit(EXIT_FAILURE);
    }

    glfwSetFramebufferSizeCallback(window, window_reshape_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    setup();

    while (!glfwWindowShouldClose(window)) {
        render_scene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

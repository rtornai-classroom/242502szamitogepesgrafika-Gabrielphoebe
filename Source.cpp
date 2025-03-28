#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#include <array>
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>

#define USE_STD_ARRAY
#ifdef USE_STD_ARRAY
static std::array<glm::vec3, 360> circleVertices;
#else
static GLfloat s_vertices[1080];
#endif

#define numVBOs 2
#define numVAOs 2
GLuint VBO[numVBOs];
GLuint VAO[numVAOs];

int window_width = 700;
int window_height = 700;
char window_title[] = "Moving circle";
GLFWwindow* window = nullptr;
GLuint circleRenderingProgram;
GLuint lineRenderingProgram;
GLuint circleColorLocation;
GLuint lineColorLocation;
GLuint colorSwapLocation;
GLfloat lineYAxis = 0.0f;
GLfloat circleXaxis = 0.0f;
GLfloat circleYaxis = 0.0f;

std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(10.0f, 35.0f);

const GLfloat circle_speed = 0.00007f;
const GLfloat line_speed = 0.002f;

GLfloat angle = 35.0f * (M_PI / 180.0f);

GLfloat circleSpeedXDir = 0.0f;
GLfloat circleSpeedYDir = 0.0f;
GLfloat lineSpeedXDir = 0.0f;
GLfloat lineSpeedYDir = 0.0f;
bool isBouncing = false;
bool isLineMoving = false;
GLfloat lineWidth = 0.5f;
bool isMoving = false;

bool checkOpenGLError() {
    bool foundError = false;
    int glErr = glGetError();

    while (glErr != GL_NO_ERROR) {
        std::cout << "glError: " << glErr << std::endl;
        foundError = true;
        glErr = glGetError();
    }

    return foundError;
}

std::string readShaderSource(const char* filePath) {
    std::ifstream fileStream(filePath, std::ios::in);
    std::string content;
    std::string line;

    while (std::getline(fileStream, line)) {
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}

GLuint createShaderProgram(const char* vertShaderPath, const char* fragShaderPath) {
    std::string vertShaderSrc = readShaderSource(vertShaderPath);
    std::string fragShaderSrc = readShaderSource(fragShaderPath);

    GLint vertCompiled;
    GLint fragCompiled;
    GLint linked;
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vertShaderCstr = vertShaderSrc.c_str();
    const char* fragShaderCstr = fragShaderSrc.c_str();

    glShaderSource(vShader, 1, &vertShaderCstr, NULL);
    glShaderSource(fShader, 1, &fragShaderCstr, NULL);

    glCompileShader(vShader);
    checkOpenGLError();
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &vertCompiled);
    if (vertCompiled != 1) {
        std::cout << "Vertex compilation failed." << std::endl;
    }

    glCompileShader(fShader);
    checkOpenGLError();
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &fragCompiled);
    if (fragCompiled != 1) {
        std::cout << "Fragment compilation failed." << std::endl;
    }

    GLuint vfProgram = glCreateProgram();
    glAttachShader(vfProgram, vShader);
    glAttachShader(vfProgram, fShader);

    glLinkProgram(vfProgram);
    checkOpenGLError();
    glGetProgramiv(vfProgram, GL_LINK_STATUS, &linked);
    if (linked != 1) {
        std::cout << "Shader linking failed." << std::endl;
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return vfProgram;
}

void init(GLFWwindow* window) {
    circleRenderingProgram = createShaderProgram("circleVshader.glsl", "circleFshader.glsl");
    lineRenderingProgram = createShaderProgram("lineVshader.glsl", "lineFshader.glsl");

    glGenBuffers(numVBOs, VBO);
    glGenVertexArrays(numVAOs, VAO);

    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
#ifdef USE_STD_ARRAY
    for (int i = 0; i < 360; ++i) {
        float angle = i * M_PI / 180.0f;
        circleVertices[i] = glm::vec3(0.1f * cos(angle), 0.1f * sin(angle), 0.0f);
    }
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(glm::vec3), circleVertices.data(), GL_STATIC_DRAW);
#else
    int index = 0;
    for (int i = 0; i < 360; ++i) {
        float angle = i * M_PI / 180.0f;
        s_vertices[index++] = 0.1f * cos(angle);
        s_vertices[index++] = 0.1f * sin(angle);
        s_vertices[index++] = 0.0f;
    }
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_vertices), s_vertices, GL_STATIC_DRAW);
#endif
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    colorSwapLocation = glGetUniformLocation(circleRenderingProgram, "colorSwap");

    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    GLfloat lineVertices[] = {
        -lineWidth / 2, lineYAxis, 0.0f,
        lineWidth / 2, lineYAxis, 0.0f };
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    lineColorLocation = glGetUniformLocation(lineRenderingProgram, "color");
}

void drawCircle() {
    glUseProgram(circleRenderingProgram);
    glBindVertexArray(VAO[0]);
    glUniform2f(glGetUniformLocation(circleRenderingProgram, "circleOffset"), circleXaxis, circleYaxis);
    glUniform1f(glGetUniformLocation(circleRenderingProgram, "circleRadius"), 0.1f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 360);
}

void drawLine() {
    glUseProgram(lineRenderingProgram);
    glBindVertexArray(VAO[1]);
    glUniform2f(glGetUniformLocation(lineRenderingProgram, "lineOffset"), 0.0f, lineYAxis);
    glDrawArrays(GL_LINES, 0, 2);
}

void display(GLFWwindow* window, double currentTime) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (isBouncing) {
        circleXaxis += circleSpeedXDir;
        circleYaxis += circleSpeedYDir;

        if ((circleXaxis + 0.1f >= 1.0f || circleXaxis - 0.1f <= -1.0f)) {
            circleSpeedXDir = -circleSpeedXDir;
        }
        if ((circleYaxis + 0.1f >= 1.0f || circleYaxis - 0.1f <= -1.0f)) {
            circleSpeedYDir = -circleSpeedYDir;
        }
    }

    if (isLineMoving) {
        lineYAxis += lineSpeedYDir;
        if ((lineYAxis + 0.05f >= 1.0f || lineYAxis - 0.05f <= -1.0f)) {
            lineSpeedYDir = -lineSpeedYDir;
        }
    }

    bool shouldSwapColors = isMoving &&
        (std::abs(circleYaxis - lineYAxis) < 0.1f) &&
        (std::abs(circleXaxis) < (lineWidth / 2.0f + 0.1f));

    glUseProgram(circleRenderingProgram);
    glUniform1i(colorSwapLocation, shouldSwapColors); // This controls the color swap

    bool tochLine = (std::abs(circleYaxis - lineYAxis) < 0.1f) &&
        (std::abs(circleXaxis) < (lineWidth / 2.0f + 0.1f));


    drawCircle();
    drawLine();

    glfwSwapBuffers(window);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    window_width = width;
    window_height = height;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((action == GLFW_PRESS) && (key == GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    else if (key == GLFW_KEY_UP && lineYAxis < 0.9f)
        lineYAxis += 0.1f;
    else if (key == GLFW_KEY_DOWN && lineYAxis > -0.9f)
        lineYAxis -= 0.1f;
    else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        if (!isBouncing) {
            isBouncing = true;
            isMoving = true;
            circleXaxis = 0.0f;
            circleYaxis = 0.0f;
            circleSpeedXDir = circle_speed * cos(angle);
            circleSpeedYDir = circle_speed * sin(angle);
        }
    }
    else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        isBouncing = false;
        isMoving = false;
        circleSpeedXDir = 0.0f;
        circleSpeedYDir = 0.0f;
    
    }
   
}

void printControls() {
    std::cout << "Controls:\n";
    std::cout << "S - Start circle movement\n";
    std::cout << "E - Stop circle movement\n";
    std::cout << "UP/DOWN - Move line vertically\n";
    std::cout << "ESC - Exit\n";
}

int main(void)
{
    printControls();

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    window = glfwCreateWindow(window_width, window_height, window_title, nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    if (glewInit() != GLEW_OK)
        exit(EXIT_FAILURE);

    glfwSetWindowSizeLimits(window, 400, 400, 800, 800);
    glfwSetWindowAspectRatio(window, 1, 1);
    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);

    init(window);

    while (!glfwWindowShouldClose(window))
    {
        display(window, glfwGetTime());
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    exit(EXIT_SUCCESS);
} 

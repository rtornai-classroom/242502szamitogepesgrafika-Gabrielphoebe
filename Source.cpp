#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

// Window dimensions
const GLuint WINDOW_WIDTH = 700, WINDOW_HEIGHT = 700;
float ballPosX = 0.0f, ballPosY = 0.0f;
float ballVelocityX, ballVelocityY;
float horizontalLineY = 0.0f;
bool isColorTransition = true;
bool isAnimationActive = false;

GLuint vertexArrayObject, vertexBufferObjects[2];

// Function prototypes
void handleKeyInput(GLFWwindow* window, int key, int scancode, int action, int mode);
void shiftLineUp();
void shiftLineDown();

// Shaders
const char* vertexShaderCode = R"(
    #version 330 core
    layout(location = 0) in vec2 vertexPosition;
    layout(location = 1) in vec3 vertexColor;
    out vec3 color;
    void main()
    {
        gl_Position = vec4(vertexPosition, 0.0, 1.0);
        color = vertexColor;
    }
)";

const char* fragmentShaderCode = R"(
    #version 330 core
    in vec3 color;
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(color, 1.0f);
    }
)";

void generateCircle(float* circleVertices, float* circleColors, int segments = 100) {
    // Center point of the circle
    circleVertices[0] = ballPosX;
    circleVertices[1] = ballPosY;
    circleColors[0] = isColorTransition ? 1.0f : 0.0f;
    circleColors[1] = isColorTransition ? 0.0f : 1.0f;
    circleColors[2] = 0.0f;

    // Circle's edge points
    float radius = 0.1f;
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14f * float(i) / float(segments);
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        int index = (i + 1) * 2;
        circleVertices[index] = x + ballPosX;
        circleVertices[index + 1] = y + ballPosY;

        int colorIndex = (i + 1) * 3;
        circleColors[colorIndex] = isColorTransition ? 0.0f : 1.0f;
        circleColors[colorIndex + 1] = isColorTransition ? 1.0f : 0.0f;
        circleColors[colorIndex + 2] = 0.0f;
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Bouncing Ball", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, handleKeyInput);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    // Set initial ball velocity at a 35° angle
    float angleInRadians = 35.0f * (3.14f / 180.0f);
    float ballSpeed = 0.00015f;
    ballVelocityX = ballSpeed * cos(angleInRadians);
    ballVelocityY = ballSpeed * sin(angleInRadians);

    // Create shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderCode, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glGenVertexArrays(1, &vertexArrayObject);
    glGenBuffers(2, vertexBufferObjects);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f); // Yellow background
        glClear(GL_COLOR_BUFFER_BIT);

        if (isAnimationActive) {
            // Update ball position
            ballPosX += ballVelocityX;
            ballPosY += ballVelocityY;

            // Ball collision with window boundaries
            if (ballPosX + 0.1f >= 1.0f || ballPosX - 0.1f <= -1.0f)
                ballVelocityX = -ballVelocityX;
            if (ballPosY + 0.1f >= 1.0f || ballPosY - 0.1f <= -1.0f)
                ballVelocityY = -ballVelocityY;

            // Change color when ball touches the line
            if (ballPosY <= horizontalLineY + 0.01f && ballPosY >= horizontalLineY - 0.01f) {
                isColorTransition = false;
            }
            else {
                isColorTransition = true;
            }
        }

        // Generate circle data
        float circleData[102 * 2];
        float circleColors[102 * 3];
        generateCircle(circleData, circleColors);

        // Render the circle
        glBindVertexArray(vertexArrayObject);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(circleData), circleData, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(circleColors), circleColors, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 102);

        // Render horizontal line
        float lineData[] = {
            -0.25f, horizontalLineY,
             0.25f, horizontalLineY
        };
        float lineColors[] = {
            0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f
        };

        GLuint lineVBOs[2];
        glGenBuffers(2, lineVBOs);

        glBindBuffer(GL_ARRAY_BUFFER, lineVBOs[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineData), lineData, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, lineVBOs[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineColors), lineColors, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_LINES, 0, 2);

        glDeleteBuffers(2, lineVBOs);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(2, vertexBufferObjects);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void handleKeyInput(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        isAnimationActive = true;
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        shiftLineUp();
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        shiftLineDown();
}

void shiftLineUp() {
    if (horizontalLineY < 0.9f)
        horizontalLineY += 0.1f;
}

void shiftLineDown() {
    if (horizontalLineY > -0.9f)
        horizontalLineY -= 0.1f;
}

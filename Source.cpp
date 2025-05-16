#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Shader sources
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform bool lightOn;
uniform bool magentaOn;
uniform sampler2D sunTexture;
uniform bool isLightSource;

void main()
{
    if (isLightSource) {
        FragColor = texture(sunTexture, TexCoord);
        if (FragColor.a < 0.1) discard; // Discard transparent pixels
        if (FragColor.r < 0.1 && FragColor.g < 0.1 && FragColor.b < 0.1) {
            FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Fallback yellow color
        }
        return;
    }

    vec3 objectColor = magentaOn ? vec3(1.0, 0.0, 1.0) : vec3(1.0, 1.0, 1.0);
    
    if (lightOn) {
        // Ambient
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;
        
        // Diffuse 
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        // Specular
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;
        
        vec3 result = (ambient + diffuse + specular) * objectColor;
        FragColor = vec4(result, 1.0);
    } else {
        FragColor = vec4(objectColor * 0.2, 1.0);
    }
}
)glsl";

// Window dimensions
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// Camera parameters

float cameraHeight = 0.0f;
float cameraRadius = 5.0f;
float cameraAngle = 0.0f;
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 6.0f);

// Light parameters
glm::vec3 lightPos = glm::vec3(2.0f, 1.0f, 0.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 0.0f);
float lightAngle = 0.0f;
float lightOrbitRadius = 3.50f;  // Increased orbit radius
float lightHeight = 2.0f;
bool lightOn = true;
bool lKeyPressed = false;

// Material parameters
bool magentaOn = false;
bool mKeyPressed = false;

// Mouse parameters
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Cube parameters
const float CUBE_SIZE = 0.5f;
const float CUBE_SPACING = CUBE_SIZE * 2.5f; // Slightly more spacing


unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Loaded texture successfully: " << path << std::endl;
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

void createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius = 1.0f, unsigned int sectorCount = 36, unsigned int stackCount = 18) {
    const float PI = 3.1415926f;
    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for (unsigned int i = 0; i <= stackCount; ++i) {
        stackAngle = PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (unsigned int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;

            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);

            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    unsigned int k1, k2;
    for (unsigned int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraHeight += cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraHeight -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraAngle -= cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraAngle += cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed) {
        lightOn = !lightOn;
        lKeyPressed = true;
        std::cout << "Light " << (lightOn ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        lKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed) {
        magentaOn = !magentaOn;
        mKeyPressed = true;
        std::cout << "Magenta material " << (magentaOn ? "ON" : "OFF") << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        mKeyPressed = false;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    cameraAngle -= xoffset;
    cameraHeight += yoffset;

    if (cameraHeight > 89.0f)
        cameraHeight = 89.0f;
    if (cameraHeight < -89.0f)
        cameraHeight = -89.0f;
}

unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Cubes with Lighting", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    unsigned int sunTexture = loadTexture("C:\\Users\\gabri\\OneDrive\\Documents\\sun.jpg");

    if (sunTexture == 0) {
        std::cerr << "Failed to load sun texture, using fallback color" << std::endl;
    }

    // Cube vertices with texture coordinates
    float vertices[] = {
        // Positions             // Normals           // Texture Coords
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,

        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f
    };

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Light source sphere (bigger sun)
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    createSphere(sphereVertices, sphereIndices, 0.3f); // Bigger sphere for light source

    unsigned int sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Update camera position on cylinder
        cameraPos.x = cameraRadius * cos(cameraAngle);
        cameraPos.z = cameraRadius * sin(cameraAngle);
        cameraPos.y = cameraHeight;

        // Update light position (circular path around origin)
        lightAngle += 0.5f * deltaTime;
        lightPos.x = lightOrbitRadius * cos(lightAngle);
        lightPos.z = lightOrbitRadius * sin(lightAngle);
        lightPos.y = 1.0f; // Keep it slightly above the cubes

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create view and projection matrices
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), cameraUp);

        // Activate shader
        glUseProgram(shaderProgram);

        // Set shader uniforms
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniform1i(glGetUniformLocation(shaderProgram, "lightOn"), lightOn);
        glUniform1i(glGetUniformLocation(shaderProgram, "magentaOn"), magentaOn);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        float spacing = CUBE_SIZE * 2.0f; // Spacing equal to cube size

        // Draw first cube (left)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-(CUBE_SIZE + spacing), 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(shaderProgram, "isLightSource"), 0);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw second cube (center)
        model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw third cube (right)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(CUBE_SIZE + spacing, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw light source (sun)
        if (sunTexture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sunTexture);
            glUniform1i(glGetUniformLocation(shaderProgram, "sunTexture"), 0);
        }

        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.5f)); // Bigger scale
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(shaderProgram, "isLightSource"), 1);
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    if (sunTexture != 0) {
        glDeleteTextures(1, &sunTexture);
    }
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
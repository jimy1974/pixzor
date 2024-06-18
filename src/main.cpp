#define GLM_ENABLE_EXPERIMENTAL

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>
#include "VoxelWorld.h"
#include "Camera.h"
#include "SelectionManager.h"
#include "ExtrusionManager.h"
#include <glm/gtx/string_cast.hpp>


// Initialize the camera
//Camera camera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), -90.0f, 0.0f, 45.0f);
// Initialize the camera with the logged values
Camera camera(glm::vec3(-20.323372f, 11.503551f, 17.729210f), glm::vec3(0.0f, 0.0f, 0.0f), -41.1f, -23.1f, 45.0f);


bool firstMouse = true;  // Variable to check if it's the first time the mouse is pressed
glm::ivec3 lastHoveredVoxel;
bool voxelHovered = false;
auto lastClickTime = std::chrono::steady_clock::now();
std::chrono::milliseconds debounceDelay(200);
GLuint lineShaderProgram;
GLuint lineVAO, lineVBO;
SelectionManager selectionManager;
ExtrusionManager extrusionManager;
bool isDragging = false;
glm::dvec2 dragStart, dragEnd;

// Global variables
glm::mat4 projection;
glm::mat4 view;
int windowWidth = 800; // Set your window width
int windowHeight = 600; // Set your window height
float distanceToPlane = 1.0f; // Set appropriate distance to plane
VoxelWorld voxelWorld(10); // Ensure voxelWorld is properly initialized
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame


void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, VoxelWorld& voxelWorld, glm::mat4 projection, glm::mat4 view);
std::string loadShaderSource(const char* filename);
GLuint compileShader(const char* shaderSource, GLenum shaderType);
GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);
void addGrid(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
void setupLineRendering();
void drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, const glm::mat4& MVP);

void checkShaderCompileErrors(GLuint shader, std::string type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "| ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "| ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

// Function to log camera state
void logCameraState(const Camera& camera) {
    std::cout << "Camera Position: " << glm::to_string(camera.position) << std::endl;
    std::cout << "Camera Yaw: " << camera.yaw << std::endl;
    std::cout << "Camera Pitch: " << camera.pitch << std::endl;
}

void drawVoxels(GLuint VAO, GLuint VBO, GLuint EBO, const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GLuint useTextureLoc, GLuint objectColorLoc, const glm::vec3& color, bool useTexture) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glUniform1i(useTextureLoc, useTexture ? 1 : 0);
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
}

void mainRenderLoop(GLFWwindow* window, VoxelWorld& voxelWorld, GLuint shaderProgram, GLuint VAO, GLuint VBO, GLuint EBO, GLuint selectedVAO, GLuint selectedVBO, GLuint selectedEBO, GLuint unselectedVAO, GLuint unselectedVBO, GLuint unselectedEBO, GLuint mvpLoc, GLuint modelLoc, GLuint viewLoc, GLuint projectionLoc, GLuint lightPosLoc, GLuint viewPosLoc, GLuint useTextureLoc, GLuint objectColorLoc, glm::mat4& model, glm::mat4& projection, glm::vec3& lightPos, GLuint texture1) {
    while (!glfwWindowShouldClose(window)) {
        // Calculate deltaTime
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        view = camera.getViewMatrix();
        projection = glm::perspective(glm::radians(camera.zoom), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

        processInput(window, voxelWorld, projection, view);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 MVP = projection * view * model;

        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.position));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        // Draw unselected voxels
        drawVoxels(unselectedVAO, unselectedVBO, unselectedEBO, voxelWorld.getUnselectedVertices(), voxelWorld.getUnselectedIndices(), useTextureLoc, objectColorLoc, glm::vec3(1.0f, 1.0f, 1.0f), true);

        // Draw selected voxels
        drawVoxels(selectedVAO, selectedVBO, selectedEBO, voxelWorld.getSelectedVertices(), voxelWorld.getSelectedIndices(), useTextureLoc, objectColorLoc, glm::vec3(1.0f, 0.0f, 0.0f), false);

        // Draw highlighted voxels during drag
        if (isDragging) {
            drawVoxels(selectedVAO, selectedVBO, selectedEBO, voxelWorld.getSelectedVertices(), voxelWorld.getSelectedIndices(), useTextureLoc, objectColorLoc, glm::vec3(1.0f, 0.7f, 0.0f), false);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}




int main() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Voxel Engine", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 600);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Make cursor visible

    std::string vertexShaderSource = loadShaderSource("vertex_shader.glsl");
    std::string fragmentShaderSource = loadShaderSource("fragment_shader.glsl");

    GLuint vertexShader = compileShader(vertexShaderSource.c_str(), GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource.c_str(), GL_FRAGMENT_SHADER);
    GLuint shaderProgram = linkProgram(vertexShader, fragmentShader);
    
    for (int x = -10; x <= 10; ++x) {
       for (int y = 0; y <= 0; ++y) {
          for (int z = -10; z <= 10; ++z) {
                voxelWorld.setVoxel(x, y, z, 1, "blue", "default");
          }
       }
    }

    voxelWorld.generateMeshData();

    GLuint VAO, selectedVAO, unselectedVAO;
    GLuint VBO, selectedVBO, unselectedVBO;
    GLuint EBO, selectedEBO, unselectedEBO;

    glGenVertexArrays(1, &VAO);
    glGenVertexArrays(1, &selectedVAO);
    glGenVertexArrays(1, &unselectedVAO);

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &selectedVBO);
    glGenBuffers(1, &unselectedVBO);

    glGenBuffers(1, &EBO);
    glGenBuffers(1, &selectedEBO);
    glGenBuffers(1, &unselectedEBO);

    // Setup unselected VAO
    glBindVertexArray(unselectedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, unselectedVBO);
    glBufferData(GL_ARRAY_BUFFER, voxelWorld.getUnselectedVertices().size() * sizeof(Vertex), voxelWorld.getUnselectedVertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, unselectedEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, voxelWorld.getUnselectedIndices().size() * sizeof(unsigned int), voxelWorld.getUnselectedIndices().data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(3);

    // Setup selected VAO
    glBindVertexArray(selectedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, selectedVBO);
    glBufferData(GL_ARRAY_BUFFER, voxelWorld.getSelectedVertices().size() * sizeof(Vertex), voxelWorld.getSelectedVertices().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, selectedEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, voxelWorld.getSelectedIndices().size() * sizeof(unsigned int), voxelWorld.getSelectedIndices().data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(3);

    glEnable(GL_DEPTH_TEST);

    GLuint mvpLoc = glGetUniformLocation(shaderProgram, "MVP");
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLuint useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
    GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    glm::mat4 model = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(camera.zoom), 800.0f / 600.0f, 0.1f, 100.0f);
    
    glm::vec3 lightPos = glm::vec3(2.0f, 15.0f, 5.0f); // Moved up to lighten up the scene

    GLuint texture1 = voxelWorld.loadTexture("./textures/wood.jpg");
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // Main render loop
    mainRenderLoop(window, voxelWorld, shaderProgram, VAO, VBO, EBO, selectedVAO, selectedVBO, selectedEBO, unselectedVAO, unselectedVBO, unselectedEBO, mvpLoc, modelLoc, viewLoc, projectionLoc, lightPosLoc, viewPosLoc, useTextureLoc, objectColorLoc, model, projection, lightPos, texture1);

    glfwTerminate();
    return 0;
}


glm::vec3 getRayFromMouse(const glm::mat4& projection, const glm::mat4& view, double mouseX, double mouseY, int screenWidth, int screenHeight) {
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    float z = 1.0f;
    glm::vec3 ray_nds = glm::vec3(x, y, z);
    glm::vec4 ray_clip = glm::vec4(ray_nds, 1.0f);
    glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
    ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
    glm::vec3 ray_wor = glm::vec3(glm::inverse(view) * ray_eye);
    ray_wor = glm::normalize(ray_wor);

    //std::cout << "Projection Matrix: " << glm::to_string(projection) << std::endl;
    //std::cout << "View Matrix: " << glm::to_string(view) << std::endl;
    std::cout << "Normalized Ray Direction: " << glm::to_string(ray_wor) << std::endl;

    return ray_wor;
}







void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static bool isLeftMousePressed = false;
    static glm::dvec2 initialMousePos;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (firstMouse) {
            camera.lastX = static_cast<float>(xpos);
            camera.lastY = static_cast<float>(ypos);
            firstMouse = false;
        }
        camera.processMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    } else {
        firstMouse = true;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!isLeftMousePressed) {
            glm::vec3 rayOrigin = camera.position;
            glm::vec3 rayDirection = getRayFromMouse(projection, view, xpos, ypos, windowWidth, windowHeight);
            glm::ivec3 hitVoxel;
            glm::vec3 hitNormal;
            FaceDirection hitFace;

            if (voxelWorld.raycast(rayOrigin, rayDirection, hitVoxel, hitNormal, hitFace)) {
                std::cout << "Normalized Ray Direction: " << glm::to_string(rayDirection) << std::endl;
                std::cout << "Hit voxel face: " << hitFace << std::endl;

                if (voxelWorld.isVoxelSelected(hitVoxel)) {
                    initialMousePos = glm::dvec2(xpos, ypos);
                    std::vector<glm::ivec3> selectedVoxels = selectionManager.getSelectedVoxels();
                    std::unordered_set<glm::ivec3> selectedVoxelSet(selectedVoxels.begin(), selectedVoxels.end());
                    extrusionManager.setSelectedVoxels(selectedVoxelSet);
                    extrusionManager.startExtrusion(hitVoxel, hitNormal, hitFace, initialMousePos);
                    isLeftMousePressed = true;
                }
            }
        } else {
            glm::dvec2 currentMousePos = glm::dvec2(xpos, ypos);
            extrusionManager.updateExtrusion(currentMousePos, voxelWorld);
        }
    } else {
        if (isLeftMousePressed) {
            extrusionManager.endExtrusion(voxelWorld);
            isLeftMousePressed = false;
        }
    }
}







    


















void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll(static_cast<float>(yoffset));
}



void processInput(GLFWwindow* window, VoxelWorld& voxelWorld, glm::mat4 projection, glm::mat4 view) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

     if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        logCameraState(camera); // Log camera state when 'L' key is pressed
    }
    
    // Check for extrusion keys
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        voxelWorld.extrudeVoxels(0, 1); // Extrude right
    } else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        voxelWorld.extrudeVoxels(1, 1); // Extrude left
    } else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        voxelWorld.extrudeVoxels(2, 1); // Extrude up
    } else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        voxelWorld.extrudeVoxels(3, 1); // Extrude down
    } else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        voxelWorld.extrudeVoxels(4, 1); // Extrude forward
    } else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        voxelWorld.extrudeVoxels(5, 1); // Extrude backward
    } else if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        voxelWorld.removeSelectedVoxels(); // Remove selected voxels
    }

    float cameraSpeed = 2.5f * deltaTime; // Adjust accordingly

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(CameraNamespace::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(CameraNamespace::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(CameraNamespace::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(CameraNamespace::RIGHT, deltaTime);

    // Rest of the processInput function...

    ///////////////////////////////

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float x = static_cast<float>((2.0 * xpos) / width - 1.0);
    float y = static_cast<float>(1.0 - (2.0 * ypos) / height);
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    glm::vec3 rayOrigin = camera.position;

    glm::ivec3 hitVoxel;
    glm::vec3 hitNormal; // Add this line to declare hitNormal
    FaceDirection hitFace; // Add this line to declare hitFace
    if (voxelWorld.raycast(rayOrigin, rayWorld, hitVoxel, hitNormal, hitFace)) { // Update this line to match the new signature
        if (!voxelHovered || hitVoxel != lastHoveredVoxel) {
            if (voxelHovered) {
                voxelWorld.resetHighlight(lastHoveredVoxel);
            }
            voxelWorld.highlightVoxel(hitVoxel);
            lastHoveredVoxel = hitVoxel;
            voxelHovered = true;
        }

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            auto now = std::chrono::steady_clock::now();
            if (now - lastClickTime > debounceDelay) {
                if (!isDragging) {
                    selectionManager.startSelection(hitVoxel);
                    isDragging = true;
                }
                selectionManager.updateSelection(hitVoxel, voxelWorld);
                lastClickTime = now;
            }
        } else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE && isDragging) {
            selectionManager.endSelection(voxelWorld);
            isDragging = false;
        }
    } else {
        if (voxelHovered) {
            voxelWorld.resetHighlight(lastHoveredVoxel);
            voxelHovered = false;
        }
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            selectionManager.clearSelections(); // Clear selection in SelectionManager
            voxelWorld.clearSelections(extrusionManager); // Clear selection in VoxelWorld and ExtrusionManager
        }
    }
    
    // Update extrusion based on mouse movement
    if (extrusionManager.isExtruding()) {
        glm::vec3 direction = camera.position - glm::vec3(extrusionManager.getExtrusionStart());
        extrusionManager.updateExtrusion(direction, voxelWorld);
    }
}





std::string loadShaderSource(const char* filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(const char* shaderSource, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    return program;
}

void addGrid(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    float gridSize = 10.0f;
    int gridLines = 20;

    for (int i = 0; i <= gridLines; ++i) {
        float t = (float)i / (float)gridLines;
        float x = t * gridSize;

        vertices.emplace_back(Vertex(x, 0, 0, 1.0f, 1.0f, 1.0f));
        vertices.emplace_back(Vertex(x, 0, gridSize, 1.0f, 1.0f, 1.0f));
        vertices.emplace_back(Vertex(0, 0, x, 1.0f, 1.0f, 1.0f));
        vertices.emplace_back(Vertex(gridSize, 0, x, 1.0f, 1.0f, 1.0f));

        indices.push_back(i * 4);
        indices.push_back(i * 4 + 1);
        indices.push_back(i * 4 + 2);
        indices.push_back(i * 4 + 3);
    }
}

void setupLineRendering() {
    std::string vertexShaderSource = loadShaderSource("line_vertex_shader.glsl");
    std::string fragmentShaderSource = loadShaderSource("line_fragment_shader.glsl");

    GLuint vertexShader = compileShader(vertexShaderSource.c_str(), GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource.c_str(), GL_FRAGMENT_SHADER);
    lineShaderProgram = linkProgram(vertexShader, fragmentShader);

    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, const glm::mat4& MVP) {
    Vertex vertices[] = {
        Vertex(start.x, start.y, start.z, color.r, color.g, color.b),
        Vertex(end.x, end.y, end.z, color.r, color.g, color.b)
    };

    glUseProgram(lineShaderProgram);
    GLuint mvpLoc = glGetUniformLocation(lineShaderProgram, "MVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}
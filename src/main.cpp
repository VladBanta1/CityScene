// ================================================================
// GPS Project – P1: Scene within a cube
// Skybox cu gradient + silhoueta munti + teren procedural + textura
// Controale: WASD = miscare, Mouse = rotire, E/Q = sus/jos, ESC = iesire
// ================================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "Shader.h"
#include "Camera.h"

// ── Constante fereastra ──────────────────────────────────────────
const unsigned int SCR_W = 1280;
const unsigned int SCR_H = 720;

// ── Camera globala ───────────────────────────────────────────────
Camera camera(glm::vec3(0.0f, 4.0f, 18.0f));
float lastX = SCR_W / 2.0f;
float lastY = SCR_H / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ── Callbacks ────────────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow*, double xIn, double yIn) {
    float x = (float)xIn, y = (float)yIn;
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    float xoff = (x - lastX);
    float yoff = (lastY - y);   // inversam Y
    lastX = x; lastY = y;
    camera.ProcessMouseMovement(xoff, yoff);
}

void scroll_callback(GLFWwindow*, double, double yo) {
    camera.ProcessMouseScroll((float)yo);
}

void processInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) camera.ProcessKeyboard(DOWN, deltaTime);
}

// ── Textura iarbă procedurala ─────────────────────────────────────
unsigned int createGrassTexture() {
    const int S = 256;
    std::vector<unsigned char> data(S * S * 3);
    srand(42);
    for (int y = 0; y < S; y++) {
        for (int x = 0; x < S; x++) {
            int i = (y * S + x) * 3;
            float fx = (float)x / S;
            float fy = (float)y / S;
            // Variatii cu functii sinus pentru a simula iarba
            float v = 0.5f
                + 0.25f * sinf(fx * 30.0f) * cosf(fy * 21.0f)
                + 0.15f * sinf(fx * 60.0f + fy * 40.0f)
                + 0.10f * ((float)(rand() % 100) / 100.0f);
            v = std::max(0.0f, std::min(1.0f, v));
            data[i + 0] = (unsigned char)(25.0f + v * 35.0f);   // R
            data[i + 1] = (unsigned char)(75.0f + v * 65.0f);   // G
            data[i + 2] = (unsigned char)(12.0f + v * 22.0f);   // B
        }
    }
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, S, S, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// ── Generare teren ────────────────────────────────────────────────
// Fiecare vertex: pozitie(3) + normala(3) + uv(2) = 8 floats
void generateTerrain(std::vector<float>& verts,
    std::vector<unsigned int>& idx,
    int N, float cell)
{
    // Functia de inaltime
    auto H = [](float x, float z) -> float {
        return 1.4f * sinf(x * 0.20f) * cosf(z * 0.20f)
            + 0.7f * sinf(x * 0.40f + 0.8f)
            + 0.4f * cosf(z * 0.33f + 1.2f)
            + 0.2f * sinf(x * 0.80f + z * 0.60f);
        };

    float origin = -(N * cell) / 2.0f;

    for (int iz = 0; iz <= N; iz++) {
        for (int ix = 0; ix <= N; ix++) {
            float x = origin + ix * cell;
            float z = origin + iz * cell;
            float y = H(x, z);

            // Normala prin diferente finite
            float eps = cell * 0.5f;
            float dydx = (H(x + eps, z) - H(x - eps, z)) / (2.0f * eps);
            float dydz = (H(x, z + eps) - H(x, z - eps)) / (2.0f * eps);
            glm::vec3 n = glm::normalize(glm::vec3(-dydx, 1.0f, -dydz));

            verts.push_back(x);    verts.push_back(y);    verts.push_back(z);
            verts.push_back(n.x);  verts.push_back(n.y);  verts.push_back(n.z);
            verts.push_back((float)ix / N * 10.0f);  // UV tiled
            verts.push_back((float)iz / N * 10.0f);
        }
    }

    for (int iz = 0; iz < N; iz++) {
        for (int ix = 0; ix < N; ix++) {
            unsigned int tl = iz * (N + 1) + ix;
            unsigned int tr = tl + 1;
            unsigned int bl = (iz + 1) * (N + 1) + ix;
            unsigned int br = bl + 1;
            idx.push_back(tl); idx.push_back(bl); idx.push_back(tr);
            idx.push_back(tr); idx.push_back(bl); idx.push_back(br);
        }
    }
}

// ── MAIN ─────────────────────────────────────────────────────────
int main() {
    // ── Init GLFW ──
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        SCR_W, SCR_H, "GPS Project – P1: City Scene Base", NULL, NULL);
    if (!window) {
        std::cerr << "GLFW window creation failed\n";
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ── Init GLAD ──
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ── Shaders ──
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
    Shader sceneShader("shaders/scene.vert", "shaders/scene.frag");

    // ── Skybox VAO ──
    // Un cub mare – vertex-urile sunt si directia de sampling
    float skyVerts[] = {
        -1, 1,-1,  -1,-1,-1,   1,-1,-1,   1,-1,-1,   1, 1,-1,  -1, 1,-1,
        -1,-1, 1,  -1,-1,-1,  -1, 1,-1,  -1, 1,-1,  -1, 1, 1,  -1,-1, 1,
         1,-1,-1,   1,-1, 1,   1, 1, 1,   1, 1, 1,   1, 1,-1,   1,-1,-1,
        -1,-1, 1,  -1, 1, 1,   1, 1, 1,   1, 1, 1,   1,-1, 1,  -1,-1, 1,
        -1, 1,-1,   1, 1,-1,   1, 1, 1,   1, 1, 1,  -1, 1, 1,  -1, 1,-1,
        -1,-1,-1,  -1,-1, 1,   1,-1,-1,   1,-1,-1,  -1,-1, 1,   1,-1, 1
    };
    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVerts), skyVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // ── Teren VAO ──
    std::vector<float>        terrVerts;
    std::vector<unsigned int> terrIdx;
    generateTerrain(terrVerts, terrIdx, 100, 0.7f);

    unsigned int terrVAO, terrVBO, terrEBO;
    glGenVertexArrays(1, &terrVAO);
    glGenBuffers(1, &terrVBO);
    glGenBuffers(1, &terrEBO);

    glBindVertexArray(terrVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrVBO);
    glBufferData(GL_ARRAY_BUFFER, terrVerts.size() * sizeof(float),
        terrVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrIdx.size() * sizeof(unsigned int),
        terrIdx.data(), GL_STATIC_DRAW);

    // pozitie
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // normala
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // ── Textura ──
    unsigned int grassTex = createGrassTexture();

    // ── Render loop ──────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;

        processInput(window);

        glClearColor(0.4f, 0.65f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_W / (float)SCR_H, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // ── Teren ──
        sceneShader.use();
        sceneShader.setMat4("model", glm::mat4(1.0f));
        sceneShader.setMat4("view", view);
        sceneShader.setMat4("projection", proj);
        sceneShader.setVec3("lightDir", glm::vec3(-0.4f, -1.0f, -0.6f));
        sceneShader.setVec3("lightColor", glm::vec3(1.0f, 0.97f, 0.88f));
        sceneShader.setInt("useTexture", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTex);
        sceneShader.setInt("texture1", 0);

        glBindVertexArray(terrVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)terrIdx.size(), GL_UNSIGNED_INT, 0);

        // ── Skybox (desenat ultimul, truc depth LEQUAL) ──
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        // Scoatem translatie din view matrix (skybox-ul e „la infinit")
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        skyboxShader.setMat4("view", skyView);
        skyboxShader.setMat4("projection", proj);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &skyVAO); glDeleteBuffers(1, &skyVBO);
    glDeleteVertexArrays(1, &terrVAO);
    glDeleteBuffers(1, &terrVBO); glDeleteBuffers(1, &terrEBO);
    glDeleteTextures(1, &grassTex);
    glfwTerminate();
    return 0;
}
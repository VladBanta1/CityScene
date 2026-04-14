// ================================================================
// GPS Project
// P1: Skybox gradient + silhoueta munti + teren (plat in oras)
// P2: Circuit dreptunghiular + strada centrala + 5 cladiri + 6 copaci
// P3: Camera completa, shadow mapping (soare), 8 stalpi point light
// Controale: WASD=miscare  E/Q=sus/jos  Mouse=rotire  ESC=iesire
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

// ── Fereastra ────────────────────────────────────────────────────
const unsigned int SCR_W = 1280, SCR_H = 720;
const unsigned int SHADOW_W = 2048, SHADOW_H = 2048;

// ── Camera ───────────────────────────────────────────────────────
Camera camera(glm::vec3(0.0f, 6.0f, 22.0f));
float lastX = SCR_W / 2.0f, lastY = SCR_H / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

// ── Callbacks ────────────────────────────────────────────────────
void framebuffer_size_callback(GLFWwindow*, int w, int h)
{
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow*, double xIn, double yIn) {
    float x = (float)xIn, y = (float)yIn;
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    camera.ProcessMouseMovement(x - lastX, lastY - y);
    lastX = x; lastY = y;
}
void scroll_callback(GLFWwindow*, double, double yo)
{
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

// ================================================================
// TEXTURI PROCEDURALE
// ================================================================
unsigned int createGrassTexture() {
    const int S = 256;
    std::vector<unsigned char> data(S * S * 3);
    srand(42);
    for (int y = 0; y < S; y++) for (int x = 0; x < S; x++) {
        int i = (y * S + x) * 3;
        float fx = (float)x / S, fy = (float)y / S;
        float v = 0.5f
            + 0.25f * sinf(fx * 30.f) * cosf(fy * 21.f)
            + 0.15f * sinf(fx * 60.f + fy * 40.f)
            + 0.10f * ((float)(rand() % 100) / 100.f);
        v = std::max(0.f, std::min(1.f, v));
        data[i + 0] = (unsigned char)(25.f + v * 35.f);
        data[i + 1] = (unsigned char)(75.f + v * 65.f);
        data[i + 2] = (unsigned char)(12.f + v * 22.f);
    }
    unsigned int tex;
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, S, S, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

unsigned int createRoadTexture() {
    const int W = 256, H = 256;
    std::vector<unsigned char> data(W * H * 3);
    srand(7);
    for (int y = 0;y < H;y++) for (int x = 0;x < W;x++) {
        int i = (y * W + x) * 3;
        unsigned char b = 52 + rand() % 12;
        data[i + 0] = b; data[i + 1] = b; data[i + 2] = b;
        // linie centrala alba punctata
        if (x >= W / 2 - 5 && x <= W / 2 + 5 && (y / 28) % 2 == 0)
        {
            data[i + 0] = 210; data[i + 1] = 210; data[i + 2] = 210;
        }
        // borduri galbene
        if (x<8 || x>W - 8)
        {
            data[i + 0] = 205; data[i + 1] = 170; data[i + 2] = 0;
        }
    }
    unsigned int tex;
    glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// ================================================================
// MESH HELPERS
// ================================================================
static inline float sStep(float e0, float e1, float x) {
    float t = std::max(0.f, std::min(1.f, (x - e0) / (e1 - e0)));
    return t * t * (3.f - 2.f * t);
}

// VAO cu layout: pos(3) norm(3) uv(2)
unsigned int makeVAO(const std::vector<float>& v,
    const std::vector<unsigned int>& idx,
    unsigned int& vbo, unsigned int& ebo)
{
    unsigned int vao;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    return vao;
}

// Teren: plat in centrul orasului, dealuri la margini
void generateTerrain(std::vector<float>& verts, std::vector<unsigned int>& idx,
    int N, float cell)
{
    auto H = [](float x, float z) -> float {
        float dist = sqrtf(x * x + z * z);
        float blend = sStep(22.f, 33.f, dist);
        float hills = 1.4f * sinf(x * .20f) * cosf(z * .20f)
            + 0.7f * sinf(x * .40f + .8f)
            + 0.4f * cosf(z * .33f + 1.2f)
            + 0.2f * sinf(x * .80f + z * .60f);
        return hills * blend;
        };
    float origin = -(N * cell) / 2.f;
    for (int iz = 0;iz <= N;iz++) for (int ix = 0;ix <= N;ix++) {
        float x = origin + ix * cell, z = origin + iz * cell, y = H(x, z);
        float eps = cell * .5f;
        float dydx = (H(x + eps, z) - H(x - eps, z)) / (2.f * eps);
        float dydz = (H(x, z + eps) - H(x, z - eps)) / (2.f * eps);
        glm::vec3 n = glm::normalize(glm::vec3(-dydx, 1.f, -dydz));
        verts.push_back(x); verts.push_back(y); verts.push_back(z);
        verts.push_back(n.x); verts.push_back(n.y); verts.push_back(n.z);
        verts.push_back((float)ix / N * 10.f);
        verts.push_back((float)iz / N * 10.f);
    }
    for (int iz = 0;iz < N;iz++) for (int ix = 0;ix < N;ix++) {
        unsigned int tl = iz * (N + 1) + ix, tr = tl + 1;
        unsigned int bl = (iz + 1) * (N + 1) + ix, br = bl + 1;
        idx.push_back(tl); idx.push_back(bl); idx.push_back(tr);
        idx.push_back(tr); idx.push_back(bl); idx.push_back(br);
    }
}

// Quad plat pe XZ
void addQuad(std::vector<float>& verts, std::vector<unsigned int>& idx,
    float x0, float z0, float x1, float z1, float y = 0.05f)
{
    int base = (int)verts.size() / 8;
    float pts[4][2] = { {x0,z0},{x1,z0},{x1,z1},{x0,z1} };
    for (int i = 0;i < 4;i++) {
        verts.push_back(pts[i][0]); verts.push_back(y); verts.push_back(pts[i][1]);
        verts.push_back(0.f); verts.push_back(1.f); verts.push_back(0.f);
        verts.push_back(pts[i][0] / 4.f); verts.push_back(pts[i][1] / 4.f);
    }
    idx.push_back(base + 0); idx.push_back(base + 1); idx.push_back(base + 2);
    idx.push_back(base + 0); idx.push_back(base + 2); idx.push_back(base + 3);
}

// Layout strazi:
//  Circuit exterior: Nord/Sud/Vest/Est
//  Strada centrala E-W
//  Strada centrala N-S
void generateRoads(std::vector<float>& verts, std::vector<unsigned int>& idx) {
    // Circuit exterior
    addQuad(verts, idx, -18.f, -14.f, 18.f, -10.f); // Nord
    addQuad(verts, idx, -18.f, 10.f, 18.f, 14.f); // Sud
    addQuad(verts, idx, -18.f, -10.f, -14.f, 10.f); // Vest
    addQuad(verts, idx, 14.f, -10.f, 18.f, 10.f); // Est
    // Strada centrala E-W
    addQuad(verts, idx, -14.f, -2.f, 14.f, 2.f);
    // Strada centrala N-S
    addQuad(verts, idx, -2.f, -10.f, 2.f, 10.f);
}

// Cub unitar [-0.5, 0.5]
void generateBox(std::vector<float>& verts, std::vector<unsigned int>& idx) {
    static const float F[6][4][8] = {
        {{-.5f,-.5f,.5f,0,0,1,0,0},{.5f,-.5f,.5f,0,0,1,1,0},{.5f,.5f,.5f,0,0,1,1,1},{-.5f,.5f,.5f,0,0,1,0,1}},
        {{.5f,-.5f,-.5f,0,0,-1,0,0},{-.5f,-.5f,-.5f,0,0,-1,1,0},{-.5f,.5f,-.5f,0,0,-1,1,1},{.5f,.5f,-.5f,0,0,-1,0,1}},
        {{-.5f,-.5f,-.5f,-1,0,0,0,0},{-.5f,-.5f,.5f,-1,0,0,1,0},{-.5f,.5f,.5f,-1,0,0,1,1},{-.5f,.5f,-.5f,-1,0,0,0,1}},
        {{.5f,-.5f,.5f,1,0,0,0,0},{.5f,-.5f,-.5f,1,0,0,1,0},{.5f,.5f,-.5f,1,0,0,1,1},{.5f,.5f,.5f,1,0,0,0,1}},
        {{-.5f,.5f,.5f,0,1,0,0,0},{.5f,.5f,.5f,0,1,0,1,0},{.5f,.5f,-.5f,0,1,0,1,1},{-.5f,.5f,-.5f,0,1,0,0,1}},
        {{-.5f,-.5f,-.5f,0,-1,0,0,0},{.5f,-.5f,-.5f,0,-1,0,1,0},{.5f,-.5f,.5f,0,-1,0,1,1},{-.5f,-.5f,.5f,0,-1,0,0,1}}
    };
    for (int face = 0;face < 6;face++) {
        unsigned int base = (unsigned int)(verts.size() / 8);
        for (int v = 0;v < 4;v++) for (int c = 0;c < 8;c++) verts.push_back(F[face][v][c]);
        idx.push_back(base + 0); idx.push_back(base + 1); idx.push_back(base + 2);
        idx.push_back(base + 0); idx.push_back(base + 2); idx.push_back(base + 3);
    }
}

void generateCylinder(std::vector<float>& verts, std::vector<unsigned int>& idx, int segs = 16) {
    const float PI = 3.14159265f;
    for (int i = 0;i <= segs;i++) {
        float a = 2.f * PI * i / segs, x = cosf(a), z = sinf(a);
        verts.push_back(x); verts.push_back(0.f); verts.push_back(z);
        verts.push_back(x); verts.push_back(0.f); verts.push_back(z);
        verts.push_back((float)i / segs); verts.push_back(0.f);
        verts.push_back(x); verts.push_back(1.f); verts.push_back(z);
        verts.push_back(x); verts.push_back(0.f); verts.push_back(z);
        verts.push_back((float)i / segs); verts.push_back(1.f);
    }
    for (int i = 0;i < segs;i++) {
        int b0 = i * 2, t0 = b0 + 1, b1 = (i + 1) * 2, t1 = b1 + 1;
        idx.push_back(b0); idx.push_back(b1); idx.push_back(t0);
        idx.push_back(t0); idx.push_back(b1); idx.push_back(t1);
    }
}

void generateCone(std::vector<float>& verts, std::vector<unsigned int>& idx, int segs = 16) {
    const float PI = 3.14159265f;
    verts.push_back(0.f); verts.push_back(1.f); verts.push_back(0.f);
    verts.push_back(0.f); verts.push_back(1.f); verts.push_back(0.f);
    verts.push_back(.5f); verts.push_back(1.f);
    for (int i = 0;i <= segs;i++) {
        float a = 2.f * PI * i / segs, x = cosf(a), z = sinf(a);
        glm::vec3 n = glm::normalize(glm::vec3(x, .5f, z));
        verts.push_back(x); verts.push_back(0.f); verts.push_back(z);
        verts.push_back(n.x); verts.push_back(n.y); verts.push_back(n.z);
        verts.push_back((float)i / segs); verts.push_back(0.f);
    }
    for (int i = 0;i < segs;i++) {
        idx.push_back(0); idx.push_back(1 + i); idx.push_back(2 + i);
    }
}

// ================================================================
// MAIN
// ================================================================
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H,
        "GPS Project – P3: Lighting + Shadows", NULL, NULL);
    if (!window) { glfwTerminate();return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { return -1; }
    glEnable(GL_DEPTH_TEST);

    // ── Shaders ──────────────────────────────────────────────────
    Shader shadowShader("shaders/shadow.vert", "shaders/shadow.frag");
    Shader sceneShader("shaders/scene.vert", "shaders/scene.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

    // ── Shadow map FBO ───────────────────────────────────────────
    unsigned int shadowFBO, shadowMap;
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = { 1,1,1,1 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ── Lumina (soare) ───────────────────────────────────────────
    glm::vec3 sunPos(-20.f, 45.f, -30.f);
    glm::vec3 sunDir = glm::normalize(-sunPos);                // directie spre scena
    glm::mat4 lightProj = glm::ortho(-45.f, 45.f, -45.f, 45.f, 1.f, 120.f);
    glm::mat4 lightView = glm::lookAt(sunPos, glm::vec3(0.f), glm::vec3(0, 1, 0));
    glm::mat4 lightSpaceMat = lightProj * lightView;

    // ── Stalpi de iluminat – pozitii (baza) ─────────────────────
    // 8 stalpi: 4 pe circuitul exterior + 4 la intersectii centrale
    glm::vec3 poleBase[] = {
        glm::vec3(-16.f,0.f,-12.f), glm::vec3(16.f,0.f,-12.f),
        glm::vec3(-16.f,0.f, 12.f), glm::vec3(16.f,0.f, 12.f),
        glm::vec3(-13.f,0.f,  0.f), glm::vec3(13.f,0.f,  0.f),
        glm::vec3(0.f,0.f, -9.f), glm::vec3(0.f,0.f,  9.f),
    };
    const int NUM_LIGHTS = 8;
    // Pozitia bulbului e la varful stalpului (h=5)
    glm::vec3 pointLightPos[NUM_LIGHTS];
    glm::vec3 pointLightCol[NUM_LIGHTS];
    for (int i = 0;i < NUM_LIGHTS;i++) {
        pointLightPos[i] = poleBase[i] + glm::vec3(0.f, 5.2f, 0.f);
        pointLightCol[i] = glm::vec3(1.0f, 0.92f, 0.60f); // galben cald
    }

    // ── Skybox VAO ───────────────────────────────────────────────
    float skyV[] = {
        -1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1,
        -1,-1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,
        1,-1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,
        -1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,1,
        -1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,-1,
        -1,-1,-1,-1,-1,1,1,-1,-1,1,-1,-1,-1,-1,1,1,-1,1
    };
    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO); glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyV), skyV, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // ── Generare mesh-uri ─────────────────────────────────────────
    std::vector<float> tV, rV, bV, cyV, coV;
    std::vector<unsigned int> tI, rI, bI, cyI, coI;
    generateTerrain(tV, tI, 100, 0.7f);
    generateRoads(rV, rI);
    generateBox(bV, bI);
    generateCylinder(cyV, cyI, 16);
    generateCone(coV, coI, 16);

    unsigned int terrVBO, terrEBO, roadVBO, roadEBO;
    unsigned int boxVBO, boxEBO, cylVBO, cylEBO, coneVBO, coneEBO;
    unsigned int terrVAO = makeVAO(tV, tI, terrVBO, terrEBO);
    unsigned int roadVAO = makeVAO(rV, rI, roadVBO, roadEBO);
    unsigned int boxVAO = makeVAO(bV, bI, boxVBO, boxEBO);
    unsigned int cylVAO = makeVAO(cyV, cyI, cylVBO, cylEBO);
    unsigned int coneVAO = makeVAO(coV, coI, coneVBO, coneEBO);
    int terrN = (int)tI.size(), roadN = (int)rI.size();
    int boxN = (int)bI.size(), cylN = (int)cyI.size(), coneN = (int)coI.size();

    unsigned int grassTex = createGrassTexture();
    unsigned int roadTex = createRoadTexture();

    // ── Cladiri (5) – in cele 4 cadrane + centru ─────────────────
    struct Bld { glm::vec3 pos, scale, color; };
    Bld buildings[] = {
        // Cadran NV
        {{-9.f,0.f,-6.f},{2.5f,8.f,2.5f},{0.72f,0.35f,0.25f}},
        // Cadran NE
        {{ 9.f,0.f,-6.f},{3.0f,6.f,3.0f},{0.55f,0.55f,0.62f}},
        // Cadran SV
        {{-9.f,0.f, 6.f},{3.0f,5.f,3.0f},{0.82f,0.74f,0.58f}},
        // Cadran SE
        {{ 9.f,0.f, 6.f},{2.5f,7.f,2.5f},{0.45f,0.52f,0.68f}},
        // Centru usor offset
        {{-4.5f,0.f,-5.f},{4.f,4.f,3.f},{0.70f,0.55f,0.40f}},
    };
    int numB = 5;

    // ── Copaci (6) – pe marginea exterioara ──────────────────────
    glm::vec3 treePts[] = {
        {-21.f,0.f,-8.f},{21.f,0.f,-8.f},
        {-21.f,0.f, 8.f},{21.f,0.f, 8.f},
        {  0.f,0.f,-18.f},{ 0.f,0.f,18.f},
    };
    int numT = 6;

    // ================================================================
    // RENDER LOOP
    // ================================================================
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame; lastFrame = now;
        processInput(window);

        glm::mat4 proj = glm::perspective(
            glm::radians(camera.Zoom), (float)SCR_W / (float)SCR_H, 0.1f, 500.f);
        glm::mat4 view = camera.GetViewMatrix();

        // ── Lambda: deseneaza un mesh (valabila pt ambele pass-uri) ─
        // Para shadUse=true: trimitem model la shadow shader
        // Para shadUse=false: trimitem toti uniformii la sceneShader
        auto drawMesh = [&](Shader& sh, unsigned int vao, int cnt,
            const glm::mat4& model,
            bool useTex, unsigned int tex, glm::vec3 col,
            bool shadowPass)
            {
                sh.setMat4("model", model);
                if (!shadowPass) {
                    sh.setInt("useTexture", useTex ? 1 : 0);
                    if (useTex) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, tex);
                        sh.setInt("texture1", 0);
                    }
                    else {
                        sh.setVec3("objectColor", col);
                    }
                }
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, cnt, GL_UNSIGNED_INT, 0);
            };

        // Lambda helper: deseneaza intreaga scena (fara skybox)
        auto renderScene = [&](Shader& sh, bool shadowPass) {
            // Teren
            drawMesh(sh, terrVAO, terrN, glm::mat4(1.f),
                true, grassTex, {}, shadowPass);
            // Strazi
            drawMesh(sh, roadVAO, roadN, glm::mat4(1.f),
                true, roadTex, {}, shadowPass);
            // Cladiri
            for (int b = 0;b < numB;b++) {
                glm::mat4 m = glm::translate(glm::mat4(1.f),
                    glm::vec3(buildings[b].pos.x,
                        buildings[b].scale.y * 0.5f,
                        buildings[b].pos.z));
                m = glm::scale(m, buildings[b].scale);
                drawMesh(sh, boxVAO, boxN, m,
                    false, 0, buildings[b].color, shadowPass);
            }
            // Copaci
            for (int t = 0;t < numT;t++) {
                glm::vec3 tp = treePts[t];
                // Trunchi
                glm::mat4 trunk = glm::translate(glm::mat4(1.f), tp);
                trunk = glm::scale(trunk, { .3f,2.5f,.3f });
                drawMesh(sh, cylVAO, cylN, trunk,
                    false, 0, { .42f,.27f,.12f }, shadowPass);
                // 3 niveluri coroana
                float rad[] = { 1.6f,1.3f,1.0f };
                float yo[] = { 2.f,3.f,4.f };
                glm::vec3 gr[] = { {.13f,.48f,.10f},{.17f,.54f,.13f},{.21f,.60f,.16f} };
                for (int l = 0;l < 3;l++) {
                    glm::mat4 c = glm::translate(glm::mat4(1.f),
                        glm::vec3(tp.x, tp.y + yo[l], tp.z));
                    c = glm::scale(c, { rad[l],1.8f,rad[l] });
                    drawMesh(sh, coneVAO, coneN, c,
                        false, 0, gr[l], shadowPass);
                }
            }
            // Stalpi de iluminat (cilindru subtire + sfera mica = cilindru lat)
            for (int i = 0;i < NUM_LIGHTS;i++) {
                glm::vec3 bp = poleBase[i];
                // Stâlp
                glm::mat4 pole = glm::translate(glm::mat4(1.f), bp);
                pole = glm::scale(pole, { .1f,5.f,.1f });
                drawMesh(sh, cylVAO, cylN, pole,
                    false, 0, { .65f,.65f,.68f }, shadowPass);
                // Lampa (cub mic la varf)
                glm::mat4 lamp = glm::translate(glm::mat4(1.f),
                    glm::vec3(bp.x, bp.y + 5.2f, bp.z));
                lamp = glm::scale(lamp, { .35f,.35f,.35f });
                drawMesh(sh, boxVAO, boxN, lamp,
                    false, 0, { 1.f,.95f,.6f }, shadowPass);
            }
            };

        // ============================================================
        // PASS 1 – Shadow map (perspectiva soarelui)
        // ============================================================
        glViewport(0, 0, SHADOW_W, SHADOW_H);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.f, 4.f);

        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMat);
        renderScene(shadowShader, true);

        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ============================================================
        // PASS 2 – Render normal
        // ============================================================
        glViewport(0, 0, SCR_W, SCR_H);
        glClearColor(.4f, .65f, .9f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.use();
        sceneShader.setMat4("view", view);
        sceneShader.setMat4("projection", proj);
        sceneShader.setMat4("lightSpaceMatrix", lightSpaceMat);
        sceneShader.setVec3("lightDir", sunDir);
        sceneShader.setVec3("lightColor", glm::vec3(1.f, .97f, .88f));
        sceneShader.setVec3("viewPos", camera.Position);
        sceneShader.setInt("numLights", NUM_LIGHTS);
        for (int i = 0;i < NUM_LIGHTS;i++) {
            std::string pi = "pointLightPos[" + std::to_string(i) + "]";
            std::string ci = "pointLightColor[" + std::to_string(i) + "]";
            sceneShader.setVec3(pi, pointLightPos[i]);
            sceneShader.setVec3(ci, pointLightCol[i]);
        }
        // Shadow map la texture unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        sceneShader.setInt("shadowMap", 1);

        renderScene(sceneShader, false);

        // ── Skybox ──────────────────────────────────────────────
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));
        skyboxShader.setMat4("projection", proj);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteFramebuffers(1, &shadowFBO);
    glDeleteTextures(1, &shadowMap);
    glDeleteTextures(1, &grassTex);
    glDeleteTextures(1, &roadTex);
    glDeleteVertexArrays(1, &skyVAO);  glDeleteBuffers(1, &skyVBO);
    glDeleteVertexArrays(1, &terrVAO); glDeleteBuffers(1, &terrVBO); glDeleteBuffers(1, &terrEBO);
    glDeleteVertexArrays(1, &roadVAO); glDeleteBuffers(1, &roadVBO); glDeleteBuffers(1, &roadEBO);
    glDeleteVertexArrays(1, &boxVAO);  glDeleteBuffers(1, &boxVBO);  glDeleteBuffers(1, &boxEBO);
    glDeleteVertexArrays(1, &cylVAO);  glDeleteBuffers(1, &cylVBO);  glDeleteBuffers(1, &cylEBO);
    glDeleteVertexArrays(1, &coneVAO); glDeleteBuffers(1, &coneVBO); glDeleteBuffers(1, &coneEBO);
    glfwTerminate();
    return 0;
}
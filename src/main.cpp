// ================================================================
// GPS Project – P2: Street circuit + static objects
// P1 pastrat: skybox gradient + silhoueta munti + teren + camera
// P2 adaugat:
//   - Circuit dreptunghiular cu textura asfalt
//   - 5 cladiri (cuburi colorate, scalate diferit)
//   - 6 copaci (cilindru trunchi + 3 conuri coroana)
//   Total obiecte statice: 11 (depaseste cerinta de min. 10)
// Controale: WASD = miscare, Mouse = rotire, E/Q = sus/jos, ESC = iesire
// ================================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <array>
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
    camera.ProcessMouseMovement(x - lastX, lastY - y);
    lastX = x; lastY = y;
}
void scroll_callback(GLFWwindow*, double, double yo) {
    camera.ProcessMouseScroll((float)yo);
}
void processInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);
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

// ── Textura iarba (P1, nemodificata) ─────────────────────────────
unsigned int createGrassTexture() {
    const int S = 256;
    std::vector<unsigned char> data(S * S * 3);
    srand(42);
    for (int y = 0; y < S; y++) for (int x = 0; x < S; x++) {
        int i = (y * S + x) * 3;
        float fx = (float)x / S, fy = (float)y / S;
        float v = 0.5f
            + 0.25f * sinf(fx * 30.0f) * cosf(fy * 21.0f)
            + 0.15f * sinf(fx * 60.0f + fy * 40.0f)
            + 0.10f * ((float)(rand() % 100) / 100.0f);
        v = std::max(0.0f, std::min(1.0f, v));
        data[i + 0] = (unsigned char)(25.0f + v * 35.0f);
        data[i + 1] = (unsigned char)(75.0f + v * 65.0f);
        data[i + 2] = (unsigned char)(12.0f + v * 22.0f);
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

// ── Textura asfalt (P2) ───────────────────────────────────────────
// Gri inchis, linie centrala alba punctata, borduri galbene
unsigned int createRoadTexture() {
    const int W = 256, H = 256;
    std::vector<unsigned char> data(W * H * 3);
    srand(7);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        int i = (y * W + x) * 3;
        unsigned char base = 55 + rand() % 14;      // zgomot asfalt
        data[i + 0] = base; data[i + 1] = base; data[i + 2] = base;
        // Linie centrala alba punctata
        if (x >= W / 2 - 6 && x <= W / 2 + 6 && (y / 28) % 2 == 0) {
            data[i + 0] = 215; data[i + 1] = 215; data[i + 2] = 215;
        }
        // Borduri galbene
        if (x < 9 || x > W - 9) {
            data[i + 0] = 210; data[i + 1] = 175; data[i + 2] = 0;
        }
    }
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// ================================================================
// GENERARE MESH-URI
// ================================================================

// ── Helper: creeaza VAO cu layout pos(3)+normal(3)+uv(2) ─────────
unsigned int makeVAO(const std::vector<float>& v,
    const std::vector<unsigned int>& idx,
    unsigned int& vbo, unsigned int& ebo)
{
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    // Pozitie
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // Normala
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    return vao;
}

// ── Teren procedural (P1, nemodificat) ───────────────────────────
void generateTerrain(std::vector<float>& verts, std::vector<unsigned int>& idx,
    int N, float cell)
{
    auto H = [](float x, float z) -> float {
        return 1.4f * sinf(x * 0.20f) * cosf(z * 0.20f)
            + 0.7f * sinf(x * 0.40f + 0.8f)
            + 0.4f * cosf(z * 0.33f + 1.2f)
            + 0.2f * sinf(x * 0.80f + z * 0.60f);
        };
    float origin = -(N * cell) / 2.0f;
    for (int iz = 0; iz <= N; iz++) for (int ix = 0; ix <= N; ix++) {
        float x = origin + ix * cell, z = origin + iz * cell, y = H(x, z);
        float eps = cell * 0.5f;
        float dydx = (H(x + eps, z) - H(x - eps, z)) / (2.0f * eps);
        float dydz = (H(x, z + eps) - H(x, z - eps)) / (2.0f * eps);
        glm::vec3 n = glm::normalize(glm::vec3(-dydx, 1.0f, -dydz));
        verts.push_back(x);   verts.push_back(y);   verts.push_back(z);
        verts.push_back(n.x); verts.push_back(n.y); verts.push_back(n.z);
        verts.push_back((float)ix / N * 10.0f);
        verts.push_back((float)iz / N * 10.0f);
    }
    for (int iz = 0; iz < N; iz++) for (int ix = 0; ix < N; ix++) {
        unsigned int tl = iz * (N + 1) + ix;
        unsigned int tr = tl + 1;
        unsigned int bl = (iz + 1) * (N + 1) + ix;
        unsigned int br = bl + 1;
        idx.push_back(tl); idx.push_back(bl); idx.push_back(tr);
        idx.push_back(tr); idx.push_back(bl); idx.push_back(br);
    }
}

// ── Circuit stradal dreptunghiular (P2) ──────────────────────────
// Outer: x[-15,15] z[-12,12]   Inner: x[-11,11] z[-8,8]
// 4 segmente: Nord, Sud, Vest, Est  (colturi se suprapun – ok vizual)
void generateRoad(std::vector<float>& verts, std::vector<unsigned int>& idx) {
    float y = 1.0f;   // usor deasupra centrului terenului

    // Adauga un quad plat; UV: u=x/4, v=z/4 → tile la fiecare 4 unitati
    auto addQuad = [&](float x0, float z0, float x1, float z1) {
        int base = (int)verts.size() / 8;
        float pts[4][2] = { {x0, z0}, {x1, z0}, {x1, z1}, {x0, z1} };
        for (int i = 0; i < 4; i++) {
            verts.push_back(pts[i][0]); verts.push_back(y);  verts.push_back(pts[i][1]);
            verts.push_back(0.0f);      verts.push_back(1.0f); verts.push_back(0.0f); // normal Y+
            verts.push_back(pts[i][0] / 4.0f);   // U
            verts.push_back(pts[i][1] / 4.0f);   // V
        }
        idx.push_back(base + 0); idx.push_back(base + 1); idx.push_back(base + 2);
        idx.push_back(base + 0); idx.push_back(base + 2); idx.push_back(base + 3);
        };

    addQuad(-15.0f, -12.0f, 15.0f, -8.0f);  // Nord
    addQuad(-15.0f, 8.0f, 15.0f, 12.0f);  // Sud
    addQuad(-15.0f, -8.0f, -11.0f, 8.0f);  // Vest
    addQuad(11.0f, -8.0f, 15.0f, 8.0f);  // Est
}

// ── Cub unitar [-0.5, 0.5] cu normale per fata (P2, cladiri) ─────
void generateBox(std::vector<float>& verts, std::vector<unsigned int>& idx) {
    // 6 fete × 4 vertex-uri, format: x y z  nx ny nz  u v
    static const float F[6][4][8] = {
        // Front  z+
        {{-.5f,-.5f,.5f, 0,0,1, 0,0},{.5f,-.5f,.5f, 0,0,1, 1,0},{.5f,.5f,.5f, 0,0,1, 1,1},{-.5f,.5f,.5f, 0,0,1, 0,1}},
        // Back   z-
        {{.5f,-.5f,-.5f,0,0,-1,0,0},{-.5f,-.5f,-.5f,0,0,-1,1,0},{-.5f,.5f,-.5f,0,0,-1,1,1},{.5f,.5f,-.5f,0,0,-1,0,1}},
        // Left   x-
        {{-.5f,-.5f,-.5f,-1,0,0,0,0},{-.5f,-.5f,.5f,-1,0,0,1,0},{-.5f,.5f,.5f,-1,0,0,1,1},{-.5f,.5f,-.5f,-1,0,0,0,1}},
        // Right  x+
        {{.5f,-.5f,.5f,1,0,0,0,0},{.5f,-.5f,-.5f,1,0,0,1,0},{.5f,.5f,-.5f,1,0,0,1,1},{.5f,.5f,.5f,1,0,0,0,1}},
        // Top    y+
        {{-.5f,.5f,.5f,0,1,0,0,0},{.5f,.5f,.5f,0,1,0,1,0},{.5f,.5f,-.5f,0,1,0,1,1},{-.5f,.5f,-.5f,0,1,0,0,1}},
        // Bottom y-
        {{-.5f,-.5f,-.5f,0,-1,0,0,0},{.5f,-.5f,-.5f,0,-1,0,1,0},{.5f,-.5f,.5f,0,-1,0,1,1},{-.5f,-.5f,.5f,0,-1,0,0,1}}
    };
    for (int face = 0; face < 6; face++) {
        unsigned int base = (unsigned int)(verts.size() / 8);
        for (int v = 0; v < 4; v++)
            for (int c = 0; c < 8; c++)
                verts.push_back(F[face][v][c]);
        idx.push_back(base + 0); idx.push_back(base + 1); idx.push_back(base + 2);
        idx.push_back(base + 0); idx.push_back(base + 2); idx.push_back(base + 3);
    }
}

// ── Cilindru unitar r=1, h=1, y∈[0,1] (P2, trunchiuri copaci) ────
void generateCylinder(std::vector<float>& verts, std::vector<unsigned int>& idx,
    int segs = 16)
{
    const float PI = 3.14159265f;
    for (int i = 0; i <= segs; i++) {
        float a = 2.0f * PI * i / segs;
        float x = cosf(a), z = sinf(a);
        // vertex jos
        verts.push_back(x); verts.push_back(0.0f); verts.push_back(z);
        verts.push_back(x); verts.push_back(0.0f); verts.push_back(z);
        verts.push_back((float)i / segs); verts.push_back(0.0f);
        // vertex sus
        verts.push_back(x); verts.push_back(1.0f); verts.push_back(z);
        verts.push_back(x); verts.push_back(0.0f); verts.push_back(z);
        verts.push_back((float)i / segs); verts.push_back(1.0f);
    }
    for (int i = 0; i < segs; i++) {
        int b0 = i * 2, t0 = b0 + 1, b1 = (i + 1) * 2, t1 = b1 + 1;
        idx.push_back(b0); idx.push_back(b1); idx.push_back(t0);
        idx.push_back(t0); idx.push_back(b1); idx.push_back(t1);
    }
}

// ── Con unitar r=1 la baza, varf la y=1 (P2, coroana copac) ──────
void generateCone(std::vector<float>& verts, std::vector<unsigned int>& idx,
    int segs = 16)
{
    const float PI = 3.14159265f;
    // Varf
    verts.push_back(0.0f); verts.push_back(1.0f); verts.push_back(0.0f);
    verts.push_back(0.0f); verts.push_back(1.0f); verts.push_back(0.0f);
    verts.push_back(0.5f); verts.push_back(1.0f);
    int apexIdx = 0;
    // Inel baza
    for (int i = 0; i <= segs; i++) {
        float a = 2.0f * PI * i / segs;
        float x = cosf(a), z = sinf(a);
        glm::vec3 n = glm::normalize(glm::vec3(x, 0.5f, z));
        verts.push_back(x);   verts.push_back(0.0f); verts.push_back(z);
        verts.push_back(n.x); verts.push_back(n.y);  verts.push_back(n.z);
        verts.push_back((float)i / segs); verts.push_back(0.0f);
    }
    for (int i = 0; i < segs; i++) {
        idx.push_back(apexIdx);
        idx.push_back(apexIdx + 1 + i);
        idx.push_back(apexIdx + 2 + i);
    }
}

// ================================================================
// MAIN
// ================================================================
int main() {
    // ── Init GLFW ──
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H,
        "GPS Project – P2: Street Circuit + Static Objects", NULL, NULL);
    if (!window) { std::cerr << "GLFW failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ── Init GLAD ──
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD failed\n"; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // ── Shadere (neschimbate fata de P1) ──
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
    Shader sceneShader("shaders/scene.vert", "shaders/scene.frag");

    // ── Skybox VAO (P1, identic) ──────────────────────────────────
    float skyVerts[] = {
        -1,1,-1, -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,1,-1, -1,1,-1,
        -1,-1,1, -1,-1,-1, -1,1,-1, -1,1,-1, -1,1,1, -1,-1,1,
        1,-1,-1, 1,-1,1,   1,1,1,   1,1,1,   1,1,-1,  1,-1,-1,
        -1,-1,1, -1,1,1,   1,1,1,   1,1,1,   1,-1,1, -1,-1,1,
        -1,1,-1, 1,1,-1,   1,1,1,   1,1,1,  -1,1,1,  -1,1,-1,
        -1,-1,-1,-1,-1,1,  1,-1,-1, 1,-1,-1,-1,-1,1,  1,-1,1
    };
    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO); glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVerts), skyVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // ── Generare si upload mesh-uri ───────────────────────────────
    std::vector<float>        terrV; std::vector<unsigned int> terrI;
    std::vector<float>        roadV; std::vector<unsigned int> roadI;
    std::vector<float>        boxV;  std::vector<unsigned int> boxI;
    std::vector<float>        cylV;  std::vector<unsigned int> cylI;
    std::vector<float>        coneV; std::vector<unsigned int> coneI;

    generateTerrain(terrV, terrI, 100, 0.7f);
    generateRoad(roadV, roadI);
    generateBox(boxV, boxI);
    generateCylinder(cylV, cylI, 16);
    generateCone(coneV, coneI, 16);

    unsigned int terrVBO, terrEBO, roadVBO, roadEBO;
    unsigned int boxVBO, boxEBO, cylVBO, cylEBO, coneVBO, coneEBO;

    unsigned int terrVAO = makeVAO(terrV, terrI, terrVBO, terrEBO);
    unsigned int roadVAO = makeVAO(roadV, roadI, roadVBO, roadEBO);
    unsigned int boxVAO = makeVAO(boxV, boxI, boxVBO, boxEBO);
    unsigned int cylVAO = makeVAO(cylV, cylI, cylVBO, cylEBO);
    unsigned int coneVAO = makeVAO(coneV, coneI, coneVBO, coneEBO);

    // Stocam dimensiunile pentru draw calls
    int terrIdxCnt = (int)terrI.size();
    int roadIdxCnt = (int)roadI.size();
    int boxIdxCnt = (int)boxI.size();
    int cylIdxCnt = (int)cylI.size();
    int coneIdxCnt = (int)coneI.size();

    // ── Texturi ───────────────────────────────────────────────────
    unsigned int grassTex = createGrassTexture();
    unsigned int roadTex = createRoadTexture();

    // ── Date cladiri: pozitie, scala, culoare (5 cladiri) ─────────
    // Toate plasate IN INTERIORUL circuitului (x in [-11,11], z in [-8,8])
    struct Building { glm::vec3 pos, scale, color; };
    Building buildings[] = {
        { glm::vec3(-4.0f, 1.0f, -3.0f), glm::vec3(2.5f, 8.0f, 2.5f), glm::vec3(0.72f, 0.35f, 0.25f) }, // caramiziu inalt
        { glm::vec3(4.0f, 1.0f, -3.0f), glm::vec3(3.0f, 5.0f, 3.0f), glm::vec3(0.55f, 0.55f, 0.60f) }, // gri mediu
        { glm::vec3(0.0f, 1.0f,  0.5f), glm::vec3(4.5f, 3.5f, 3.0f), glm::vec3(0.82f, 0.76f, 0.62f) }, // bej lat
        { glm::vec3(-5.5f, 1.0f,  4.5f), glm::vec3(2.0f, 7.0f, 2.0f), glm::vec3(0.45f, 0.52f, 0.65f) }, // albastru-gri inalt
        { glm::vec3(5.5f, 1.0f,  4.5f), glm::vec3(3.0f, 4.5f, 3.0f), glm::vec3(0.70f, 0.55f, 0.40f) }, // maro-portocaliu
    };
    int numBuildings = 5;

    // ── Pozitii copaci (6 copaci, in EXTERIORUL circuitului) ──────
    glm::vec3 treePosArr[] = {
        glm::vec3(-19.0f, 1.0f, -6.0f),
        glm::vec3(19.0f, 1.0f, -6.0f),
        glm::vec3(-19.0f, 1.0f,  6.0f),
        glm::vec3(19.0f, 1.0f,  6.0f),
        glm::vec3(0.0f, 1.0f,-17.0f),
        glm::vec3(0.0f, 1.0f, 17.0f),
    };
    int numTrees = 6;

    // ================================================================
    // RENDER LOOP
    // ================================================================
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;
        processInput(window);

        glClearColor(0.4f, 0.65f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(
            glm::radians(camera.Zoom), (float)SCR_W / (float)SCR_H, 0.1f, 500.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // Setam uniformele comune (view, projection, lumina)
        sceneShader.use();
        sceneShader.setMat4("view", view);
        sceneShader.setMat4("projection", proj);
        sceneShader.setVec3("lightDir", glm::vec3(-0.4f, -1.0f, -0.6f));
        sceneShader.setVec3("lightColor", glm::vec3(1.0f, 0.97f, 0.88f));

        // Lambda helper: deseneaza un mesh cu textura sau culoare solida
        auto draw = [&](unsigned int vao, int cnt, const glm::mat4& model,
            bool useTex, unsigned int tex, glm::vec3 col)
            {
                sceneShader.setMat4("model", model);
                sceneShader.setInt("useTexture", useTex ? 1 : 0);
                if (useTex) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    sceneShader.setInt("texture1", 0);
                }
                else {
                    sceneShader.setVec3("objectColor", col);
                }
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, cnt, GL_UNSIGNED_INT, 0);
            };

        // ── Teren (P1) ────────────────────────────────────────────
        draw(terrVAO, terrIdxCnt,
            glm::mat4(1.0f),
            true, grassTex, glm::vec3(0));

        // ── Circuit drum (P2) ─────────────────────────────────────
        draw(roadVAO, roadIdxCnt,
            glm::mat4(1.0f),
            true, roadTex, glm::vec3(0));

        // ── Cladiri (P2) – 5 cladiri, cuburi scalate diferit ──────
        for (int b = 0; b < numBuildings; b++) {
            // Centrul cubului unitar e la y=0; il mutam cu scale.y/2 ca baza sa fie la pos.y
            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                glm::vec3(buildings[b].pos.x,
                    buildings[b].pos.y + buildings[b].scale.y * 0.5f,
                    buildings[b].pos.z));
            model = glm::scale(model, buildings[b].scale);
            draw(boxVAO, boxIdxCnt, model,
                false, 0, buildings[b].color);
        }

        // ── Copaci (P2) – 6 copaci cu trunchi + 3 niveluri coroana ─
        for (int t = 0; t < numTrees; t++) {
            glm::vec3 tp = treePosArr[t];

            // Trunchi (cilindru brun, r=0.3, h=2.5)
            glm::mat4 trunk = glm::translate(glm::mat4(1.0f), tp);
            trunk = glm::scale(trunk, glm::vec3(0.3f, 2.5f, 0.3f));
            draw(cylVAO, cylIdxCnt, trunk,
                false, 0, glm::vec3(0.42f, 0.27f, 0.12f));

            // Coroana – 3 conuri suprapuse pentru aspect mai bogat
            float radii[] = { 1.6f, 1.3f, 1.0f };
            float yOffs[] = { 2.0f, 3.0f, 4.0f };
            glm::vec3 greens[] = {
                glm::vec3(0.13f, 0.48f, 0.10f),
                glm::vec3(0.17f, 0.54f, 0.13f),
                glm::vec3(0.21f, 0.60f, 0.16f),
            };
            for (int lv = 0; lv < 3; lv++) {
                glm::mat4 cone = glm::translate(glm::mat4(1.0f),
                    glm::vec3(tp.x, tp.y + yOffs[lv], tp.z));
                cone = glm::scale(cone, glm::vec3(radii[lv], 1.8f, radii[lv]));
                draw(coneVAO, coneIdxCnt, cone,
                    false, 0, greens[lv]);
            }
        }

        // ── Skybox (P1, ultimul randat) ───────────────────────────
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

    // ── Cleanup ───────────────────────────────────────────────────
    glDeleteVertexArrays(1, &skyVAO);  glDeleteBuffers(1, &skyVBO);
    glDeleteVertexArrays(1, &terrVAO); glDeleteBuffers(1, &terrVBO); glDeleteBuffers(1, &terrEBO);
    glDeleteVertexArrays(1, &roadVAO); glDeleteBuffers(1, &roadVBO); glDeleteBuffers(1, &roadEBO);
    glDeleteVertexArrays(1, &boxVAO);  glDeleteBuffers(1, &boxVBO);  glDeleteBuffers(1, &boxEBO);
    glDeleteVertexArrays(1, &cylVAO);  glDeleteBuffers(1, &cylVBO);  glDeleteBuffers(1, &cylEBO);
    glDeleteVertexArrays(1, &coneVAO); glDeleteBuffers(1, &coneVBO); glDeleteBuffers(1, &coneEBO);
    glDeleteTextures(1, &grassTex);
    glDeleteTextures(1, &roadTex);
    glfwTerminate();
    return 0;
}
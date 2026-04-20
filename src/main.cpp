// ================================================================
// GPS Project
// P1: Skybox gradient + silueta munti + teren plat in oras
// P2: Circuit stradal + strazi centrale + 5 cladiri + 6 copaci + 8 stalpi
// P3: Shadow mapping (soare) + 8 point lights (stalpi iluminat)
// C1: Masina verde controlabila cu sageti + AABB coliziune cladiri
// C2: Masina rosie pe circuit + masina albastra E-W + 3 pietoni aleatori
//
// Controale:
//   WASD + Mouse = camera libera    E/Q = sus/jos camera
//   Sageti = masina verde (C1)      ESC = iesire
// ================================================================

// opengl si librarii
// GLFW = fereastra + input, GLAD = incarca functiile OpenGL la runtime,
// GLM = matematica 3D (matrici, vectori), toate incluse mai jos:
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "Shader.h"
#include "Camera.h"

// Constante
const unsigned int SCR_W = 1280, SCR_H = 720;
const unsigned int SHADOW_W = 2048, SHADOW_H = 2048;

// Camera
Camera camera(glm::vec3(0.f, 8.f, 25.f));
float lastX = SCR_W / 2.f, lastY = SCR_H / 2.f;
bool  firstMouse = true;
float deltaTime = 0.f, lastFrame = 0.f;

// Structuri date

// structura cladirilor
// Fiecare cladire are pozitie (pos), dimensiuni (scale) si culoare (color).
// Sunt desenate ca un cub unitar scalat diferit cu glm::scale.
struct Bld { glm::vec3 pos, scale, color; };

// cum functioneaza masina controlata de jucator / structura masinii verzi 
// pos = pozitia in scena, yaw = unghiul de rotatie in grade, speed = viteza curenta
struct PlayerCar {
    glm::vec3 pos = { -10.f, 0.05f, 0.f };
    float     yaw = 0.f;   // grade; forward = (cos(yaw), 0, -sin(yaw))
    float     speed = 0.f;
} playerCar;

// cum merg masinile singure / structura masinilor automate 
// wps = lista de waypoints, wpIdx = waypoint-ul curent spre care merge masina,
// spd = viteza, masina se deplaseaza spre wps[wpIdx] si trece la urmatorul cand ajunge
struct AutoCar {
    std::vector<glm::vec3> wps;
    int wpIdx = 0; float spd;
    glm::vec3 pos, color;
    float yaw = 0.f;
};

// cum functioneaza pietonii / structura pietonilor 
// pos = pozitia curenta, target = destinatia random spre care merge,
// waitTimer = cat timp asteapta inainte sa aleaga o noua destinatie
struct Pedestrian {
    glm::vec3 pos, target, color;
    float spd = 1.3f, waitTimer = 0.f, yaw = 0.f;
};

// Callbacks
void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
void mouse_callback(GLFWwindow*, double xIn, double yIn) {
    float x = (float)xIn, y = (float)yIn;
    if (firstMouse) { lastX = x;lastY = y;firstMouse = false; }
    camera.ProcessMouseMovement(x - lastX, lastY - y);
    lastX = x;lastY = y;
}
void scroll_callback(GLFWwindow*, double, double yo) { camera.ProcessMouseScroll((float)yo); }

// cum controlezi masina / cum controlezi camera
// WASD + EQ = camera libera (6 grade de libertate)
// Sageti = masina verde: sus/jos = acceleratie/frana, stanga/dreapta = viraj
// Viteza are frecare aplicata automat (speed *= 1 - 2.8*dt) si e limitata intre -5 si 11
void processInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);
    // Camera – WASD
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) camera.ProcessKeyboard(DOWN, deltaTime);
    // Masina – sageti 
    float accel = 0.f, turn = 0.f;
    if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS) accel = 14.f;
    if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS) accel = -9.f;
    if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS) turn = 85.f;
    if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) turn = -85.f;
    playerCar.speed += accel * deltaTime;
    playerCar.speed *= (1.f - 2.8f * deltaTime);           // frecare
    playerCar.speed = glm::clamp(playerCar.speed, -5.f, 11.f);
    if (fabsf(playerCar.speed) > 0.15f)
        playerCar.yaw += turn * deltaTime * (playerCar.speed > 0 ? 1.f : -1.f);
}

// TEXTURI PROCEDURALE

// textura ierbii / podeaua verde
// Textura 256x256 generata procedural pe CPU cu functii sin/cos + zgomot random.
// Nu se incarca din fisier - pixelii sunt calculati matematic, canal verde dominant.
// Incarcata in GPU cu glTexImage2D si repetata cu GL_REPEAT (tiled pe tot terenul).
unsigned int createGrassTexture() {
    const int S = 256; std::vector<unsigned char> data(S * S * 3); srand(42);
    for (int y = 0;y < S;y++) for (int x = 0;x < S;x++) {
        int i = (y * S + x) * 3; float fx = (float)x / S, fy = (float)y / S;
        float v = 0.5f + 0.25f * sinf(fx * 30.f) * cosf(fy * 21.f)
            + 0.15f * sinf(fx * 60.f + fy * 40.f)
            + 0.10f * ((float)(rand() % 100) / 100.f);
        v = std::max(0.f, std::min(1.f, v));
        data[i + 0] = (unsigned char)(25.f + v * 35.f);
        data[i + 1] = (unsigned char)(75.f + v * 65.f);   // canal verde dominant
        data[i + 2] = (unsigned char)(12.f + v * 22.f);
    }
    unsigned int tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, S, S, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// textura drumului / textura asfalt
// Textura 256x256 generata procedural: fond gri cu zgomot,
// linie centrala alba punctata (la mijlocul axei U, intrerupta la fiecare 30px),
// borduri galbene la marginile U (primii si ultimii 10 pixeli pe latime).
unsigned int createRoadTexture() {
    const int W = 256, H = 256; std::vector<unsigned char> data(W * H * 3); srand(7);
    for (int y = 0;y < H;y++) for (int x = 0;x < W;x++) {
        int i = (y * W + x) * 3; unsigned char b = 50 + rand() % 14;
        data[i + 0] = b; data[i + 1] = b; data[i + 2] = b;
        // Linie centrala alba punctata de-a lungul V (y in textura)
        if (x >= W / 2 - 6 && x <= W / 2 + 6 && (y / 30) % 2 == 0)
        {
            data[i + 0] = 205;data[i + 1] = 205;data[i + 2] = 205;
        }
        // Borduri galbene la marginile U (x in textura = lat drum)
        if (x<10 || x>W - 10)
        {
            data[i + 0] = 210;data[i + 1] = 175;data[i + 2] = 0;
        }
    }
    unsigned int tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// GENERARE MESH-URI

// cum ai facut VAO si VBO / cum ai trimis datele la GPU
// VAO descrie structura datelor (pozitie la loc 0, normala la loc 1, UV la loc 2).
// VBO stocheaza datele vertexilor in memoria GPU.
// EBO (Element Buffer Object) stocheaza indicii pentru a refolosi vertecsi.
// Layout per vertex: [x,y,z, nx,ny,nz, u,v] = 8 floats
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);           // pozitie
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normala
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // UV
    return vao;
}

// functie ajutatoare pentru smoothstep (interpolare lina intre e0 si e1)
static float sStep(float e0, float e1, float x) {
    float t = std::max(0.f, std::min(1.f, (x - e0) / (e1 - e0)));
    return t * t * (3.f - 2.f * t);
}

// terenul verde / cum am generat podeaua
// Grila procedurala 100x100 celule, fiecare celula = 0.7 unitati.
// Inaltimea H(x,z) e calculata cu functii sin/cos (dealuri ondulate),
// dar multiplicata cu sStep(22,34,dist) = 0 in centrul orasului, 1 la margini.
// Asa terenul e PLAT in oras si are DEALURI doar la exterior.
// Normalele sunt calculate prin diferente finite pentru iluminare corecta.
void generateTerrain(std::vector<float>& verts, std::vector<unsigned int>& idx,
    int N, float cell)
{
    auto H = [](float x, float z)->float {
        float d = sqrtf(x * x + z * z), b = sStep(22.f, 34.f, d);
        return b * (1.4f * sinf(x * .20f) * cosf(z * .20f)
            + 0.7f * sinf(x * .40f + .8f)
            + 0.4f * cosf(z * .33f + 1.2f)
            + 0.2f * sinf(x * .80f + z * .60f));
        };
    float origin = -(N * cell) / 2.f;
    for (int iz = 0;iz <= N;iz++) for (int ix = 0;ix <= N;ix++) {
        float x = origin + ix * cell, z = origin + iz * cell, y = H(x, z);
        float eps = cell * .5f;
        float dydx = (H(x + eps, z) - H(x - eps, z)) / (2.f * eps);
        float dydz = (H(x, z + eps) - H(x, z - eps)) / (2.f * eps);
        glm::vec3 n = glm::normalize(glm::vec3(-dydx, 1.f, -dydz));
        verts.push_back(x);   verts.push_back(y);   verts.push_back(z);
        verts.push_back(n.x); verts.push_back(n.y); verts.push_back(n.z);
        verts.push_back((float)ix / N * 10.f);
        verts.push_back((float)iz / N * 10.f);
    }
    for (int iz = 0;iz < N;iz++) for (int ix = 0;ix < N;ix++) {
        unsigned int tl = iz * (N + 1) + ix, tr = tl + 1;
        unsigned int bl = (iz + 1) * (N + 1) + ix, br = bl + 1;
        idx.push_back(tl);idx.push_back(bl);idx.push_back(tr);
        idx.push_back(tr);idx.push_back(bl);idx.push_back(br);
    }
}

// strada / drumul
// Quad drum cu UV corect in functie de directie (Est-West sau Nord-Sud).
// type=0: drum E-W, U merge de-a latul (Z), V merge de-a lungul (X) cu tile la 6 unitati.
// type=1: drum N-S, U merge de-a latul (X), V merge de-a lungul (Z).
// Asta asigura ca linia centrala si bordurile galbene apar CORECT pe ambele directii.
void addRoadQuad(std::vector<float>& verts, std::vector<unsigned int>& idx,
    float x0, float z0, float x1, float z1, int type, float y = 0.05f)
{
    int base = (int)verts.size() / 8;
    float pts[4][2] = { {x0,z0},{x1,z0},{x1,z1},{x0,z1} };
    for (int i = 0;i < 4;i++) {
        float u, v;
        if (type == 0) {
            u = (pts[i][1] - z0) / (z1 - z0);  // 0..1 de-a latul (Z)
            v = pts[i][0] / 6.f;           // tiles de-a lungul (X)
        }
        else {
            u = (pts[i][0] - x0) / (x1 - x0);  // 0..1 de-a latul (X)
            v = pts[i][1] / 6.f;           // tiles de-a lungul (Z)
        }
        verts.push_back(pts[i][0]);verts.push_back(y);verts.push_back(pts[i][1]);
        verts.push_back(0.f);verts.push_back(1.f);verts.push_back(0.f);
        verts.push_back(u);verts.push_back(v);
    }
    idx.push_back(base + 0);idx.push_back(base + 1);idx.push_back(base + 2);
    idx.push_back(base + 0);idx.push_back(base + 2);idx.push_back(base + 3);
}

// circuitul / cum am organizat strazile
// 6 quad-uri: 4 formeaza circuitul exterior dreptunghiular,
// + strada centrala E-W si strada centrala N-S care se intersecteaza.
void generateRoads(std::vector<float>& verts, std::vector<unsigned int>& idx) {
    // Circuit exterior
    addRoadQuad(verts, idx, -18.f, -14.f, 18.f, -10.f, 0);      // Nord  EW
    addRoadQuad(verts, idx, -18.f, 10.f, 18.f, 14.f, 0);      // Sud   EW
    addRoadQuad(verts, idx, -18.f, -10.f, -14.f, 10.f, 1);      // Vest  NS
    addRoadQuad(verts, idx, 14.f, -10.f, 18.f, 10.f, 1);      // Est   NS
    // Strazi centrale
    addRoadQuad(verts, idx, -14.f, -2.f, 14.f, 2.f, 0);      // E-W centrala
    addRoadQuad(verts, idx, -2.f, -10.f, 2.f, 10.f, 1, 0.06f);// N-S centrala (y+0.01 evita z-fight)
}

// cladirile / forma geometrica a cladirilor
// Cub unitar [-0.5, 0.5] cu 6 fete, fiecare fata = 2 triunghiuri (4 vertecsi + 2 indici).
// Fiecare vertex contine: pozitie(3) + normala per fata(3) + UV(2) = 8 floats.
// In scena, cubul e scalat diferit pentru fiecare cladire cu glm::scale.
void generateBox(std::vector<float>& verts, std::vector<unsigned int>& idx) {
    static const float F[6][4][8] = {
        {{-.5f,-.5f,.5f,0,0,1,0,0},{.5f,-.5f,.5f,0,0,1,1,0},{.5f,.5f,.5f,0,0,1,1,1},{-.5f,.5f,.5f,0,0,1,0,1}},
        {{.5f,-.5f,-.5f,0,0,-1,0,0},{-.5f,-.5f,-.5f,0,0,-1,1,0},{-.5f,.5f,-.5f,0,0,-1,1,1},{.5f,.5f,-.5f,0,0,-1,0,1}},
        {{-.5f,-.5f,-.5f,-1,0,0,0,0},{-.5f,-.5f,.5f,-1,0,0,1,0},{-.5f,.5f,.5f,-1,0,0,1,1},{-.5f,.5f,-.5f,-1,0,0,0,1}},
        {{.5f,-.5f,.5f,1,0,0,0,0},{.5f,-.5f,-.5f,1,0,0,1,0},{.5f,.5f,-.5f,1,0,0,1,1},{.5f,.5f,.5f,1,0,0,0,1}},
        {{-.5f,.5f,.5f,0,1,0,0,0},{.5f,.5f,.5f,0,1,0,1,0},{.5f,.5f,-.5f,0,1,0,1,1},{-.5f,.5f,-.5f,0,1,0,0,1}},
        {{-.5f,-.5f,-.5f,0,-1,0,0,0},{.5f,-.5f,-.5f,0,-1,0,1,0},{.5f,-.5f,.5f,0,-1,0,1,1},{-.5f,-.5f,.5f,0,-1,0,0,1}}
    };
    for (int f = 0;f < 6;f++) {
        unsigned int base = (unsigned int)(verts.size() / 8);
        for (int v = 0;v < 4;v++) for (int c = 0;c < 8;c++) verts.push_back(F[f][v][c]);
        idx.push_back(base + 0);idx.push_back(base + 1);idx.push_back(base + 2);
        idx.push_back(base + 0);idx.push_back(base + 2);idx.push_back(base + 3);
    }
}

// copacii / forma geometrica cilindru pentru trunchi
// Cilindru unitar r=1, h=1, y in [0,1] cu 16 segmente.
// Folosit pentru: trunchiul copacilor (scala mica, culoare bruna)
//                 si stalpii de iluminat (scala subtire, culoare gri).
void generateCylinder(std::vector<float>& verts, std::vector<unsigned int>& idx, int segs = 16) {
    const float PI = 3.14159265f;
    for (int i = 0;i <= segs;i++) {
        float a = 2.f * PI * i / segs, x = cosf(a), z = sinf(a);
        verts.push_back(x);verts.push_back(0.f);verts.push_back(z);
        verts.push_back(x);verts.push_back(0.f);verts.push_back(z);
        verts.push_back((float)i / segs);verts.push_back(0.f);
        verts.push_back(x);verts.push_back(1.f);verts.push_back(z);
        verts.push_back(x);verts.push_back(0.f);verts.push_back(z);
        verts.push_back((float)i / segs);verts.push_back(1.f);
    }
    for (int i = 0;i < segs;i++) {
        int b0 = i * 2, t0 = b0 + 1, b1 = (i + 1) * 2, t1 = b1 + 1;
        idx.push_back(b0);idx.push_back(b1);idx.push_back(t0);
        idx.push_back(t0);idx.push_back(b1);idx.push_back(t1);
    }
}

// coroana copacilor / forma geometrica con pentru coroana
// Con unitar r=1 la baza, varf la y=1, cu 16 segmente.
// Fiecare copac are 3 conuri suprapuse la inaltimi diferite (yo[]) si
// raze diferite (rd[]) pentru un aspect mai bogat si natural.
void generateCone(std::vector<float>& verts, std::vector<unsigned int>& idx, int segs = 16) {
    const float PI = 3.14159265f;
    verts.push_back(0.f);verts.push_back(1.f);verts.push_back(0.f);
    verts.push_back(0.f);verts.push_back(1.f);verts.push_back(0.f);
    verts.push_back(.5f);verts.push_back(1.f);
    for (int i = 0;i <= segs;i++) {
        float a = 2.f * PI * i / segs, x = cosf(a), z = sinf(a);
        glm::vec3 n = glm::normalize(glm::vec3(x, .5f, z));
        verts.push_back(x);verts.push_back(0.f);verts.push_back(z);
        verts.push_back(n.x);verts.push_back(n.y);verts.push_back(n.z);
        verts.push_back((float)i / segs);verts.push_back(0.f);
    }
    for (int i = 0;i < segs;i++) { idx.push_back(0);idx.push_back(1 + i);idx.push_back(2 + i); }
}

// coliziunile / detectia coliziunilor
// AABB (Axis-Aligned Bounding Box) simplu: compara distanta dintre centrul masinii si centrul fiecarei cladiri pe axele X si Z.
// Daca masina se suprapune cu o cladire, functia returneaza true.
// In main: daca e coliziune, masina revine la pozitia anterioara si viteza se inverseaza partial.
bool checkCollision(glm::vec3 cp, Bld* blds, int n) {
    const float CHW = 1.0f, CHL = 1.9f;  // half-width si half-length masina
    for (int i = 0;i < n;i++) {
        float bw = blds[i].scale.x * .5f + .35f, bl = blds[i].scale.z * .5f + .35f;
        if (fabsf(cp.x - blds[i].pos.x) < CHW + bw && fabsf(cp.z - blds[i].pos.z) < CHL + bl)
            return true;
    }
    return false;
}

// cum merg masinile singure / algoritmul de miscare al masinilor automate
// Waypoint system: masina se deplaseaza spre wps[wpIdx].
// Cand distanta e < 0.6 unitati, trece la urmatorul waypoint (circular).
// Directia e normalizata, viteza e constanta (spd), yaw calculat din atan2.
void updateAutoCar(AutoCar& c, float dt) {
    glm::vec3 tgt = c.wps[c.wpIdx];
    glm::vec3 dir = tgt - c.pos;
    if (glm::length(dir) < 0.6f) { c.wpIdx = (c.wpIdx + 1) % (int)c.wps.size();return; }
    dir = glm::normalize(dir);
    c.pos += dir * c.spd * dt;
    c.yaw = glm::degrees(atan2f(dir.x, dir.z));
}

// cum functioneaza pietonii 
// Miscare random: pietonul merge spre o tinta random, se opreste (waitTimer),
// dupa care isi alege o noua tinta random in interiorul scenei.
// Cand waitTimer > 0, pietonul asteapta pe loc.
void updatePedestrian(Pedestrian& p, float dt) {
    glm::vec3 dir = p.target - p.pos;
    float dist = glm::length(glm::vec2(dir.x, dir.z));
    if (dist < 0.4f || p.waitTimer>0.f) {
        p.waitTimer -= dt;
        if (p.waitTimer <= 0.f) {
            // alege o noua tinta random in scena
            p.target = glm::vec3((float)(rand() % 180 - 90) * .09f, 0.05f,
                (float)(rand() % 140 - 70) * .09f);
            p.waitTimer = (float)(rand() % 3) + 0.5f;
        }
        return;
    }
    dir = glm::normalize(dir);
    p.pos += dir * p.spd * dt;
    p.yaw = glm::degrees(atan2f(dir.x, dir.z));
}

// MAIN
int main() {
    // initializare OpenGL / creare fereastra
    // GLFW creeaza fereastra si contextul OpenGL 3.3 Core Profile.
    // GLAD incarca la runtime toate pointerii catre functiile OpenGL.
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H,
        "GPS – City", NULL, NULL);
    if (!window) { glfwTerminate();return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { return -1; }
    glEnable(GL_DEPTH_TEST);

    // cum functioneaza shaderele
    // 3 shadere: shadowShader (pass umbra, doar adancime),
    //            sceneShader (randat normal cu lumina + umbra),
    //            skyboxShader (fundalul cerului).
    // Fiecare e compilat din fisiere .vert + .frag de clasa Shader.
    Shader shadowShader("shaders/shadow.vert", "shaders/shadow.frag");
    Shader sceneShader("shaders/scene.vert", "shaders/scene.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

    // umbrele / shadow mapping
    // Pass 1: scena e randata din perspectiva soarelui intr-un FBO (framebuffer object)
    //         care contine doar o textura de adancime (depth map) 2048x2048.
    // Pass 2: la randarea normala, fiecare fragment compara adancimea sa
    //         cu valoarea din depth map - daca e mai departe = e in umbra.
    // GL_CLAMP_TO_BORDER cu border alb (1,1,1,1) = zone in afara hurtii nu sunt umbrite.
    unsigned int shadowFBO, shadowMap;
    glGenFramebuffers(1, &shadowFBO);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_W, SHADOW_H, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float bord[] = { 1,1,1,1 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bord);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // lumina soarelui / directia luminii
    // Soarele e la pozitia (-30, 60, -25), priveste spre origine.
    // Proiectia e ortogoala (nu perspectiva) pentru ca razele soarelui sunt paralele.
    // lightSpaceMat = lProj * lView, trimisa la ambele shadere pentru shadow mapping.
    glm::vec3 sunPos(-30.f, 60.f, -25.f);
    glm::vec3 sunDir = glm::normalize(-sunPos);
    glm::mat4 lProj = glm::ortho(-70.f, 70.f, -70.f, 70.f, 0.5f, 220.f);
    glm::mat4 lView = glm::lookAt(sunPos, glm::vec3(0.f), glm::vec3(0, 1, 0));
    glm::mat4 lightSpaceMat = lProj * lView;

    // stalpii de iluminat / pozitiile si luminile stalpilor
    // 8 stalpi: 4 la colturile circuitului exterior, 4 la intersectii centrale.
    // Fiecare stalp genereaza un point light la y+5.0 (varful stalpului).
    // Culoarea e galben-cald (1.0, 0.92, 0.60).
    const int NUM_LIGHTS = 8;
    glm::vec3 poleBase[NUM_LIGHTS] = {
        {-16.f,0.f,-12.f},{16.f,0.f,-12.f},
        {-16.f,0.f, 12.f},{16.f,0.f, 12.f},
        {-12.f,0.f,  0.f},{12.f,0.f,  0.f},
        {  0.f,0.f, -9.f},{ 0.f,0.f,  9.f},
    };
    glm::vec3 plPos[NUM_LIGHTS], plCol[NUM_LIGHTS];
    for (int i = 0;i < NUM_LIGHTS;i++) {
        plPos[i] = poleBase[i] + glm::vec3(0.f, 5.0f, 0.f);
        plCol[i] = { 1.0f,0.92f,0.60f };
    }

    // fundalul / skybox VAO
    // Un cub de 36 vertecsi (6 fete x 2 triunghiuri x 3 vertecsi).
    // Pozitiile vertexilor servesc direct ca directii de sampling in skybox.frag.
    // Randat ultimul cu glDepthFunc(GL_LEQUAL) ca sa apara intotdeauna in fundal.
    float skyV[] = {
        -1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1,
        -1,-1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,
        1,-1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,
        -1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,1,
        -1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,-1,
        -1,-1,-1,-1,-1,1,1,-1,-1,1,-1,-1,-1,-1,1,1,-1,1
    };
    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyV), skyV, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // generare si upload mesh-uri pe GPU
    std::vector<float> tV, rV, bV, cyV, coV;
    std::vector<unsigned int> tI, rI, bI, cyI, coI;
    generateTerrain(tV, tI, 100, 0.7f);
    generateRoads(rV, rI);
    generateBox(bV, bI);
    generateCylinder(cyV, cyI, 16);
    generateCone(coV, coI, 16);

    unsigned int terrVBO, terrEBO, roadVBO, roadEBO,
        boxVBO, boxEBO, cylVBO, cylEBO, coneVBO, coneEBO;
    unsigned int terrVAO = makeVAO(tV, tI, terrVBO, terrEBO);
    unsigned int roadVAO = makeVAO(rV, rI, roadVBO, roadEBO);
    unsigned int boxVAO = makeVAO(bV, bI, boxVBO, boxEBO);
    unsigned int cylVAO = makeVAO(cyV, cyI, cylVBO, cylEBO);
    unsigned int coneVAO = makeVAO(coV, coI, coneVBO, coneEBO);
    int terrN = (int)tI.size(), roadN = (int)rI.size();
    int boxN = (int)bI.size(), cylN = (int)cyI.size(), coneN = (int)coI.size();

    unsigned int grassTex = createGrassTexture();
    unsigned int roadTex = createRoadTexture();

    // cladirile / datele cladirilor (pozitie, scala, culoare)
    // 5 cladiri plasate in cele 4 cadrane ale intersectiei + una in centru.
    // Fiecare e un cub unitar scalat: scale.y = inaltimea, scale.x/z = latimea.
    // Pozitia Y in model matrix = scale.y * 0.5f astfel incat baza sa stea pe pamant.
    Bld buildings[] = {
        {{-9.f,0.f,-6.f},{2.5f,8.f,2.5f},{0.72f,0.35f,0.25f}},   // caramiziu inalt NV
        {{ 9.f,0.f,-6.f},{3.0f,6.f,3.0f},{0.55f,0.55f,0.62f}},   // gri mediu NE
        {{-9.f,0.f, 6.f},{3.0f,5.f,3.0f},{0.82f,0.74f,0.58f}},   // bej SV
        {{ 9.f,0.f, 6.f},{2.5f,7.f,2.5f},{0.45f,0.52f,0.68f}},   // albastru-gri SE
        {{-4.5f,0.f,-4.f},{3.5f,3.5f,2.5f},{0.70f,0.55f,0.40f}}, // maro centru
    };
    const int numB = 5;

    // copacii / pozitiile copacilor
    // 6 copaci plasati la marginile exterioare ale scenei.
    // Fiecare copac = 1 cilindru (trunchi) + 3 conuri suprapuse (coroana).
    glm::vec3 treePts[] = {
        {-21.f,0.f,-8.f},{21.f,0.f,-8.f},
        {-21.f,0.f, 8.f},{21.f,0.f, 8.f},
        {  0.f,0.f,-18.f},{ 0.f,0.f,18.f},
    };
    const int numT = 6;

    // cum merg masinile singure / configurarea masinilor automate 
    // Masina rosie: circuit exterior in sensul acelor de ceasornic, viteza 7 unitati/s.
    // Masina albastra: dus-intors pe strada E-W centrala, viteza 5 unitati/s.
    AutoCar autoCars[2];
    autoCars[0].wps = { {-16.f,0.05f,-12.f},{16.f,0.05f,-12.f},
                     {16.f,0.05f,12.f},{-16.f,0.05f,12.f} };
    autoCars[0].pos = autoCars[0].wps[0]; autoCars[0].spd = 7.f;
    autoCars[0].color = { 0.85f,0.15f,0.15f };
    autoCars[1].wps = { {-12.f,0.05f,0.f},{12.f,0.05f,0.f} };
    autoCars[1].pos = autoCars[1].wps[0]; autoCars[1].spd = 5.f;
    autoCars[1].color = { 0.15f,0.35f,0.85f };

    // cum functioneaza pietonii / configurarea pietonilor 
    // 3 pietoni cu culori diferite, pornind din pozitii fixe,
    // se vor misca random in scena dupa primul frame.
    Pedestrian peds[3];
    peds[0] = { {-6.f,0.05f,-4.f},{-6.f,0.05f,-4.f},{0.80f,0.25f,0.10f} };
    peds[1] = { { 5.f,0.05f, 5.f},{ 5.f,0.05f, 5.f},{0.20f,0.60f,0.30f} };
    peds[2] = { {-3.f,0.05f, 7.f},{-3.f,0.05f, 7.f},{0.55f,0.20f,0.75f} };
    srand(123);

    // RENDER LOOP
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame; lastFrame = now;

        // cum controlezi masina / update masina jucator + coliziuni
        processInput(window);
        glm::vec3 prevPos = playerCar.pos;
        float r = glm::radians(playerCar.yaw);
        playerCar.pos += glm::vec3(cosf(r), 0.f, -sinf(r)) * playerCar.speed * deltaTime;
        playerCar.pos.x = glm::clamp(playerCar.pos.x, -25.f, 25.f);
        playerCar.pos.z = glm::clamp(playerCar.pos.z, -20.f, 20.f);
        // coliziunile - daca e coliziune, revine la pozitia anterioara
        if (checkCollision(playerCar.pos, buildings, numB)) {
            playerCar.pos = prevPos; playerCar.speed *= -0.25f;
        }

		// update miscare masini automate si pietoni
        for (int i = 0;i < 2;i++) updateAutoCar(autoCars[i], deltaTime);
        for (int i = 0;i < 3;i++) updatePedestrian(peds[i], deltaTime);

        // trimit matricele la GPU / matricele Model View Projection
        // proj = proiectie perspectiva (obiecte mai departe = mai mici),
        // view = pozitia si directia camerei (glm::lookAt in Camera.h)
        glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_W / (float)SCR_H, 0.1f, 500.f);
        glm::mat4 view = camera.GetViewMatrix();

        // lambda helper: deseneaza un mesh cu textura sau culoare solida
        // sp=true inseamna shadow pass (trimite doar model matrix, fara textura/culoare)
        auto drawMesh = [&](Shader& sh, unsigned int vao, int cnt,
            const glm::mat4& model,
            bool useTex, unsigned int tex, glm::vec3 col,
            bool sp)
            {
                sh.setMat4("model", model);
                if (!sp) {
                    sh.setInt("useTexture", useTex ? 1 : 0);
                    if (useTex) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, tex);
                        sh.setInt("texture1", 0);
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, shadowMap);
                    }
                    else {
                        sh.setVec3("objectColor", col);
                    }
                }
                glBindVertexArray(vao);
                glDrawElements(GL_TRIANGLES, cnt, GL_UNSIGNED_INT, 0);
            };

        // masinile / forma geometrica a masinilor
        // Masina = 6 cuburi: caroseria (jos + sus), 4 roti.
        // Rotatia vizuala e yaw+90 ca axa frontala sa fie aliniata cu directia de mers.
        auto drawCar = [&](Shader& sh, glm::vec3 pos, float yaw,
            glm::vec3 bCol, bool sp)
            {
                glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(yaw + 90.f), { 0,1,0 });
                auto part = [&](glm::vec3 off, glm::vec3 sc, glm::vec3 col) {
                    glm::mat4 m = glm::translate(glm::mat4(1.f), pos) * R;
                    m = glm::translate(m, off); m = glm::scale(m, sc);
                    drawMesh(sh, boxVAO, boxN, m, false, 0, col, sp);
                    };
                part({ 0.f, 0.35f, 0.f }, { 1.8f,0.55f,3.8f }, bCol);          // caroserie jos
                part({ 0.f, 0.85f,-0.2f }, { 1.4f,0.44f,2.1f }, bCol * 0.85f); // caroserie sus
                glm::vec3 wc = { 0.10f,0.10f,0.10f };
                part({ -0.85f,0.18f,-1.2f }, { 0.28f,0.42f,0.55f }, wc);  // roata SF
                part({ 0.85f,0.18f,-1.2f }, { 0.28f,0.42f,0.55f }, wc);   // roata DF
                part({ -0.85f,0.18f, 1.2f }, { 0.28f,0.42f,0.55f }, wc);  // roata SS
                part({ 0.85f,0.18f, 1.2f }, { 0.28f,0.42f,0.55f }, wc);   // roata DS
            };

        // pietonii / forma geometrica a pietonilor
        // Pieton = 1 cilindru (corp) + 1 cub mic (cap)
        auto drawPed = [&](Shader& sh, glm::vec3 pos, glm::vec3 col, bool sp) {
            auto pm = [&](unsigned int vao, int cnt, glm::vec3 off, glm::vec3 sc, glm::vec3 c) {
                glm::mat4 m = glm::translate(glm::mat4(1.f), pos + off);
                m = glm::scale(m, sc);
                drawMesh(sh, vao, cnt, m, false, 0, c, sp);
                };
            pm(cylVAO, cylN, { 0,0,0 }, { 0.18f,0.85f,0.18f }, col);             // corp cilindru
            pm(boxVAO, boxN, { 0,1.0f,0 }, { 0.22f,0.22f,0.22f }, { 0.85f,0.70f,0.55f }); // cap
            };

        // functie ce randeaza toata scena 
        auto renderScene = [&](Shader& sh, bool sp) {
            // teren verde cu textura iarba
            drawMesh(sh, terrVAO, terrN, glm::mat4(1.f), true, grassTex, {}, sp);
            // strazi cu textura asfalt
            drawMesh(sh, roadVAO, roadN, glm::mat4(1.f), true, roadTex, {}, sp);
            // cladirile - 5 cuburi scalate, baza la y=0
            for (int b = 0;b < numB;b++) {
                glm::mat4 m = glm::translate(glm::mat4(1.f),
                    glm::vec3(buildings[b].pos.x, buildings[b].scale.y * .5f, buildings[b].pos.z));
                m = glm::scale(m, buildings[b].scale);
                drawMesh(sh, boxVAO, boxN, m, false, 0, buildings[b].color, sp);
            }
            // copacii - 1 cilindru trunchi + 3 conuri coroana per copac
            for (int t = 0;t < numT;t++) {
                glm::vec3 tp = treePts[t];
                glm::mat4 tr = glm::translate(glm::mat4(1.f), tp);
                tr = glm::scale(tr, { .3f,2.5f,.3f });
                drawMesh(sh, cylVAO, cylN, tr, false, 0, { .42f,.27f,.12f }, sp); // trunchi
                float rd[] = { 1.6f,1.3f,1.f }, yo[] = { 2.f,3.f,4.f };
                glm::vec3 gr[] = { {.13f,.48f,.10f},{.17f,.54f,.13f},{.21f,.60f,.16f} };
                for (int l = 0;l < 3;l++) {
                    glm::mat4 c = glm::translate(glm::mat4(1.f), { tp.x,tp.y + yo[l],tp.z });
                    c = glm::scale(c, { rd[l],1.8f,rd[l] });
                    drawMesh(sh, coneVAO, coneN, c, false, 0, gr[l], sp); // nivel coroana
                }
            }
            // stalpii de iluminat - cilindru subtire + cub lampa deasupra
            for (int i = 0;i < NUM_LIGHTS;i++) {
                glm::vec3 bp = poleBase[i];
                glm::mat4 pole = glm::translate(glm::mat4(1.f), bp);
                pole = glm::scale(pole, { .10f,5.f,.10f });
                drawMesh(sh, cylVAO, cylN, pole, false, 0, { .65f,.65f,.68f }, sp); // stalp
                glm::mat4 lamp = glm::translate(glm::mat4(1.f), { bp.x, bp.y + 5.2f, bp.z });
                lamp = glm::scale(lamp, { .40f, .30f, .40f });
                drawMesh(sh, boxVAO, boxN, lamp, false, 0, { 1.f,.95f,.6f }, sp);   // lampa
            }
            // masina jucatorului (verde) si masinile automate (rosie si albastra)
            drawCar(sh, playerCar.pos, playerCar.yaw, { 0.15f,0.70f,0.20f }, sp);
            for (int i = 0;i < 2;i++) drawCar(sh, autoCars[i].pos, autoCars[i].yaw - 90.f, autoCars[i].color, sp);
            // pietoni
            for (int i = 0;i < 3;i++) drawPed(sh, peds[i].pos, peds[i].color, sp);
            };

        // umbrele / Pass 1: generare shadow map din perspectiva soarelui
        // Randarea se face in FBO-ul de adancime la rezolutia 2048x2048.
        // glPolygonOffset elimina shadow acne (artefacte de auto-umbrire).
        glViewport(0, 0, SHADOW_W, SHADOW_H);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(3.f, 6.f);
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMat);
        renderScene(shadowShader, true);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Pass 2: randare normala cu lumina, umbra si toate uniformele
        glViewport(0, 0, SCR_W, SCR_H);
        glClearColor(.4f, .65f, .9f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneShader.use();
        // trimit matricele la GPU
        sceneShader.setMat4("view", view);
        sceneShader.setMat4("projection", proj);
        sceneShader.setMat4("lightSpaceMatrix", lightSpaceMat);
        sceneShader.setVec3("lightDir", sunDir);
        sceneShader.setVec3("lightColor", { 1.f,.97f,.88f });
        sceneShader.setVec3("viewPos", camera.Position);
        // cum functioneaza lumina de la stalpi - trimite pozitia si culoarea fiecarui point light
        sceneShader.setInt("numLights", NUM_LIGHTS);
        for (int i = 0;i < NUM_LIGHTS;i++) {
            sceneShader.setVec3("pointLightPos[" + std::to_string(i) + "]", plPos[i]);
            sceneShader.setVec3("pointLightColor[" + std::to_string(i) + "]", plCol[i]);
        }
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        sceneShader.setInt("shadowMap", 1);

        renderScene(sceneShader, false);

        // fundalul / randare skybox
        // GL_LEQUAL permite skybox-ului sa treaca testul de adancime chiar si la z=1.
        // Translatie scoasa din view matrix (glm::mat3) ca skybox-ul sa nu se miste cu camera.
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

    // cleanup resurse GPU
    glDeleteFramebuffers(1, &shadowFBO);
    glDeleteTextures(1, &shadowMap);
    glDeleteTextures(1, &grassTex);
    glDeleteTextures(1, &roadTex);
    glDeleteVertexArrays(1, &skyVAO); glDeleteBuffers(1, &skyVBO);
    glDeleteVertexArrays(1, &terrVAO);glDeleteBuffers(1, &terrVBO);glDeleteBuffers(1, &terrEBO);
    glDeleteVertexArrays(1, &roadVAO);glDeleteBuffers(1, &roadVBO);glDeleteBuffers(1, &roadEBO);
    glDeleteVertexArrays(1, &boxVAO); glDeleteBuffers(1, &boxVBO); glDeleteBuffers(1, &boxEBO);
    glDeleteVertexArrays(1, &cylVAO); glDeleteBuffers(1, &cylVBO); glDeleteBuffers(1, &cylEBO);
    glDeleteVertexArrays(1, &coneVAO);glDeleteBuffers(1, &coneVBO);glDeleteBuffers(1, &coneEBO);
    glfwTerminate();
    return 0;
}

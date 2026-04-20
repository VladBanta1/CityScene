#pragma once
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

// cum functioneaza camera
// Camera cu 6 grade de libertate: WASD = miscare, E/Q = sus/jos, mouse = rotire.
// Yaw = rotatie stanga/dreapta, Pitch = rotatie sus/jos.
// Front/Right/Up sunt recalculate dupa fiecare miscare a mouse-ului (updateVectors).

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class Camera {
public:
    glm::vec3 Position, Front, Up, Right, WorldUp;
    float Yaw, Pitch, MovementSpeed, MouseSensitivity, Zoom;

    // cum initializez camera - pozitie initiala (0, 8, 25), priveste spre -Z
    Camera(glm::vec3 position = glm::vec3(0.0f, 3.0f, 10.0f))
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
        MovementSpeed(8.0f), MouseSensitivity(0.1f), Zoom(45.0f)
    {
        Position = position;
        WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        Yaw = -90.0f;
        Pitch = -10.0f;
        updateVectors();
    }

    // cum e calculata matricea de vizualizare
    // glm::lookAt(pozitia camerei, punctul spre care priveste, directia "sus")
    // Aceasta matrice transforma coordonatele din world space in camera space.
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // cum controlezi camera cu tastatura
    // Miscarea e inmultita cu deltaTime pentru viteza independenta de frame rate.
    void ProcessKeyboard(Camera_Movement dir, float dt) {
        float v = MovementSpeed * dt;
        if (dir == FORWARD)  Position += Front * v;
        if (dir == BACKWARD) Position -= Front * v;
        if (dir == LEFT)     Position -= Right * v;
        if (dir == RIGHT)    Position += Right * v;
        if (dir == UP)       Position += WorldUp * v;
        if (dir == DOWN)     Position -= WorldUp * v;
    }

    // cum controlezi camera cu mouse-ul
    // xoff/yoff = deplasarea mouse-ului fata de frame-ul anterior.
    // Pitch e limitat la [-89, 89] grade pentru a evita rasturnarea camerei (gimbal lock).
    void ProcessMouseMovement(float xoff, float yoff) {
        Yaw += xoff * MouseSensitivity;
        Pitch += yoff * MouseSensitivity;
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
        updateVectors();
    }

    // zoom cu scroll mouse - modifica field of view (FOV) al camerei
    void ProcessMouseScroll(float yoff) {
        Zoom -= yoff;
        if (Zoom < 10.0f) Zoom = 10.0f;
        if (Zoom > 90.0f) Zoom = 90.0f;
    }

private:
    // recalculeaza vectorii Front, Right, Up din Yaw si Pitch
    // Front = directia spre care priveste camera (calculata cu cos/sin din unghiuri)
    // Right = perpendicular pe Front si WorldUp (produs vectorial)
    // Up = perpendicular pe Right si Front (produs vectorial)
    void updateVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        f.y = sin(glm::radians(Pitch));
        f.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(f);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

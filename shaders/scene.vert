#version 330 core

// vertex shader / ce date primeste vertex shader-ul
// layout location corespunde cu glVertexAttribPointer din makeVAO():
// location 0 = pozitie (3 floats), location 1 = normala (3 floats), location 2 = UV (2 floats)
layout (location = 0) in vec3 aPos;       // pozitia vertexului in model space
layout (location = 1) in vec3 aNormal;    // normala vertexului
layout (location = 2) in vec2 aTexCoord;  // coordonate UV textura

// date trimise mai departe catre fragment shader (interpolate pe suprafata triunghiului)
out vec3 FragPos;            // pozitia in world space
out vec3 Normal;             // normala transformata in world space
out vec2 TexCoord;           // UV (pass-through, neschimbat)
out vec4 FragPosLightSpace;  // pozitia din perspectiva soarelui (pentru shadow mapping)

// model = transforma din model space in world space (pozitie + rotatie + scala obiect)
// view  = transforma din world space in camera space (pozitia camerei)
// projection = transforma din camera space in clip space (perspectiva)
// lightSpaceMatrix = projection * view din perspectiva soarelui (pentru shadow mapping)
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    // cum functioneaza pipeline-ul de transformare / MVP
    // Fiecare vertex trece prin lantul: model space -> world space -> camera space -> clip space
    vec4 worldPos     = model * vec4(aPos, 1.0);           // world space
    FragPos           = worldPos.xyz;
    // normala transformata corect (transpose(inverse(model)) pentru scalari neuniforme)
    Normal            = mat3(transpose(inverse(model))) * aNormal;
    TexCoord          = aTexCoord;
    // pozitia din perspectiva soarelui (folosita in fragment shader pentru shadow map)
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    // pozitia finala pe ecran
    gl_Position       = projection * view * worldPos;
}

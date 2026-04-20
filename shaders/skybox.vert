#version 330 core

// fundalul / vertex shader skybox
// Skybox-ul e un cub mare. Pozitiile vertexilor sunt folosite direct ca directii
// de sampling (TexCoords) in fragment shader, nu ca pozitii 3D reale.
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;  // directia trimisa la fragment shader

// view este fara translatie (glm::mat3 scoate componenta de translatie) -
// astfel skybox-ul "nu se misca" cu camera, ramane mereu in fundal.
uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;  // directia = pozitia vertexului cubului unitar
    // fundalul sa ramana mereu in spate
    // setam z = w, astfel dupa perspectiva divide z/w = 1.0 = adancime maxima.
    // Skybox-ul trece testul de adancime GL_LEQUAL si apare in spatele tuturor obiectelor.
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;  // z = w => adancime maxima dupa divide
}

#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    // Truc: setam z = w ca skybox-ul sa fie mereu la adancimea maxima (1.0 dupa divide)
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
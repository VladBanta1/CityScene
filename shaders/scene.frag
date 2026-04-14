#version 330 core
out vec4 FragColor;

in vec3  FragPos;
in vec3  Normal;
in vec2  TexCoord;
in float Height;

uniform vec3      objectColor;
uniform vec3      lightDir;
uniform vec3      lightColor;
uniform int       useTexture;
uniform sampler2D texture1;

void main()
{
    // Iluminare directionala (soare)
    vec3  norm    = normalize(Normal);
    vec3  sun     = normalize(-lightDir);
    float diff    = max(dot(norm, sun), 0.0);
    vec3  ambient = 0.38 * lightColor;
    vec3  diffuse = diff * lightColor;

    vec3 baseColor = (useTexture == 1)
        ? texture(texture1, TexCoord).rgb
        : objectColor;

    // Nuanta bazata pe inaltime: zone joase mai brune, zone inalte mai vii
    float hf      = clamp((Height + 1.5) / 4.5, 0.0, 1.0);
    vec3  lowTint = vec3(0.65, 0.55, 0.35);
    vec3  hiTint  = vec3(1.00, 1.00, 1.00);
    baseColor    *= mix(lowTint, hiTint, hf);

    vec3 result = (ambient + diffuse) * baseColor;
    FragColor   = vec4(result, 1.0);
}
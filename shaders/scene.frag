#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform vec3      objectColor;
uniform int       useTexture;
uniform sampler2D texture1;
uniform vec3      lightDir;
uniform vec3      lightColor;
uniform sampler2D shadowMap;

#define MAX_LIGHTS 8
uniform int  numLights;
uniform vec3 pointLightPos[MAX_LIGHTS];
uniform vec3 pointLightColor[MAX_LIGHTS];
uniform vec3 viewPos;

float calcShadow(vec4 fragPosLS, vec3 norm, vec3 sun)
{
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 0.0;
    float bias  = max(0.004 * (1.0 - dot(norm, sun)), 0.001);
    float shadow = 0.0;
    vec2  texel  = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            float pcf = texture(shadowMap, proj.xy + vec2(x,y)*texel).r;
            shadow += (proj.z - bias > pcf) ? 1.0 : 0.0;
        }
    return shadow / 9.0;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 sun  = normalize(-lightDir);

    vec3 baseColor = (useTexture == 1)
        ? texture(texture1, TexCoord).rgb
        : objectColor;

    float diff   = max(dot(norm, sun), 0.0);
    float shadow = calcShadow(FragPosLightSpace, norm, sun);

    // Ambient mic → umbre vizibile
    vec3 ambient = 0.15 * lightColor;
    vec3 diffuse = (1.0 - shadow) * diff * lightColor;
    vec3 result  = (ambient + diffuse) * baseColor;

    // Point lights (stalpi de iluminat)
    for (int i = 0; i < numLights; i++) {
        vec3  lv    = pointLightPos[i] - FragPos;
        float dist  = length(lv);
        float atten = 1.0 / (1.0 + 0.09*dist + 0.032*dist*dist);
        float pl    = max(dot(norm, normalize(lv)), 0.0);
        result += pl * atten * pointLightColor[i] * baseColor * 2.0;
    }

    FragColor = vec4(result, 1.0);
}
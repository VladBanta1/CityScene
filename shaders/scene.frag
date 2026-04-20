#version 330 core
out vec4 FragColor;

// date primite de la vertex shader (interpolate per fragment)
in vec3 FragPos;           // pozitia fragmentului in world space
in vec3 Normal;            // normala interpolata
in vec2 TexCoord;          // coordonate UV pentru textura
in vec4 FragPosLightSpace; // pozitia fragmentului din perspectiva soarelui (pentru shadow)

// materialele / uniforme pentru culoare si textura
uniform vec3      objectColor;   // culoarea solida (daca nu e textura)
uniform int       useTexture;    // 1 = foloseste textura, 0 = culoare solida
uniform sampler2D texture1;      // textura (iarba sau asfalt) la unit 0

// lumina soarelui 
uniform vec3      lightDir;      // directia luminii (spre soare)
uniform vec3      lightColor;    // culoarea luminii soarelui (alb-cald)

// umbrele / shadow map
uniform sampler2D shadowMap;     // depth map din perspectiva soarelui, la unit 1

// cum functioneaza lumina de la stalpi / point lights 
#define MAX_LIGHTS 8
uniform int  numLights;                       // numarul de stalpi activi (8)
uniform vec3 pointLightPos[MAX_LIGHTS];       // pozitia fiecarui bec (varful stalpului)
uniform vec3 pointLightColor[MAX_LIGHTS];     // culoarea luminii (galben-cald)
uniform vec3 viewPos;                         // pozitia camerei (pentru specular, daca e nevoie)

// umbrele / algoritm PCF shadow mapping
// Converteste pozitia din light space in coordonate [0,1] ale shadow map-ului.
// Compara adancimea fragmentului cu valoarea stocata in shadow map.
// PCF (Percentage Closer Filtering): face media pe un kernel 3x3 = umbre mai moi.
float calcShadow(vec4 fragPosLS, vec3 norm, vec3 sun)
{
    vec3 proj = fragPosLS.xyz / fragPosLS.w;  // perspectiva divide
    proj = proj * 0.5 + 0.5;                  // transforma din [-1,1] in [0,1]
    if (proj.z > 1.0) return 0.0;             // in afara frustumului = nu e umbrit
    float bias  = max(0.004 * (1.0 - dot(norm, sun)), 0.001); // offset anti-acne
    float shadow = 0.0;
    vec2  texel  = 1.0 / textureSize(shadowMap, 0); // dimensiunea unui texel
    // kernel 3x3 - esantionare in 9 puncte vecine pentru umbre mai moi
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            float pcf = texture(shadowMap, proj.xy + vec2(x,y)*texel).r;
            shadow += (proj.z - bias > pcf) ? 1.0 : 0.0;
        }
    return shadow / 9.0;  // media = cat la suta din kernel e in umbra
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 sun  = normalize(-lightDir);  // directia spre soare (inversul directiei luminii)

    // culoarea de baza: din textura sau culoare solida
    vec3 baseColor = (useTexture == 1)
        ? texture(texture1, TexCoord).rgb
        : objectColor;

    // umbre vizibile / iluminare directionala (soare)
    float diff   = max(dot(norm, sun), 0.0);       // cat de mult bate soarele pe fata asta
    float shadow = calcShadow(FragPosLightSpace, norm, sun);

    // ambient mic (0.15) = umbre sunt intunecate si vizibile
    vec3 ambient = 0.15 * lightColor;
    vec3 diffuse = (1.0 - shadow) * diff * lightColor;  // umbra reduce lumina difuza
    vec3 result  = (ambient + diffuse) * baseColor;

    // cum functioneaza lumina de la stalpi / calcul point lights (stalpi iluminat)
    // Atenuarea cu distanta: 1 / (1 + 0.09*d + 0.032*d^2) = lumina se reduce cu distanta.
    // Multiplicator 5.0 = lumina stalpilor e destul de puternica vizibil.
    for (int i = 0; i < numLights; i++) {
        vec3  lv    = pointLightPos[i] - FragPos;          // vector spre bec
        float dist  = length(lv);
        float atten = 1.0 / (1.0 + 0.09*dist + 0.032*dist*dist); // atenuare cu distanta
        float pl    = max(dot(norm, normalize(lv)), 0.0);          // cat bate lumina pe fata
        result += pl * atten * pointLightColor[i] * baseColor * 5.0;
    }

    FragColor = vec4(result, 1.0);
}

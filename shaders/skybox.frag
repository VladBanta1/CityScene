#version 330 core
out vec4 FragColor;
in  vec3 TexCoords;

void main()
{
    vec3 dir = normalize(TexCoords);

    // ── Gradient cer: albastru deschis la orizont, albastru inchis sus ──
    float t        = max(dir.y, 0.0);
    vec3 skyBottom = vec3(0.62, 0.84, 0.96);
    vec3 skyTop    = vec3(0.10, 0.28, 0.72);
    vec3 skyColor  = mix(skyBottom, skyTop, pow(t, 0.6));

    // ── Soare ──
    vec3 sunDir = normalize(vec3(0.5, 0.55, 0.3));
    float sd    = dot(dir, sunDir);
    if (sd > 0.9995) {
        FragColor = vec4(1.0, 0.97, 0.85, 1.0);
        return;
    }
    if (sd > 0.992) {
        float glow = (sd - 0.992) / 0.0075;
        skyColor   = mix(skyColor, vec3(1.0, 0.88, 0.55), glow * 0.45);
    }

    // ── Silhoueta munti la orizont ──
    float angle      = atan(dir.z, dir.x);
    float mLine      = 0.025
                     + 0.040 * (sin(angle * 4.0)  * 0.5 + 0.5)
                     + 0.025 * (sin(angle * 9.0)  * 0.5 + 0.5)
                     + 0.012 * (sin(angle * 19.0) * 0.5 + 0.5);

    if (dir.y < mLine + 0.005 && dir.y > -0.20) {
        float blend     = smoothstep(mLine - 0.008, mLine + 0.005, dir.y);
        vec3  mColor    = mix(vec3(0.28, 0.34, 0.40), skyColor, blend);
        FragColor = vec4(mColor, 1.0);
        return;
    }

    FragColor = vec4(skyColor, 1.0);
}
#version 330 core
out vec4 FragColor;
in  vec3 TexCoords;  // directia vertexului skybox (pozitia cubului folosita direct ca directie)

void main()
{
    vec3 dir = normalize(TexCoords);  // directia din centrul skybox-ului spre acest fragment

    // fundalul / gradientul cerului
    // t = cat de "sus" e directia (0 = orizont, 1 = zenit).
    // mix() interpoleaza intre albastru deschis (orizont) si albastru inchis (sus).
    float t        = max(dir.y, 0.0);
    vec3 skyBottom = vec3(0.62, 0.84, 0.96);  // albastru deschis la orizont
    vec3 skyTop    = vec3(0.10, 0.28, 0.72);  // albastru inchis sus
    vec3 skyColor  = mix(skyBottom, skyTop, pow(t, 0.6));

    // soarele si glow
    // dot(dir, sunDir) aproape de 1.0 = suntem in directia soarelui.
    // Pragul 0.9995 = discul alb al soarelui (foarte mic).
    // Pragul 0.992 = haloul (glow) portocaliu din jurul soarelui.
    vec3 sunDir = normalize(vec3(0.5, 0.55, 0.3));
    float sd    = dot(dir, sunDir);
    if (sd > 0.9995) {
        FragColor = vec4(1.0, 0.97, 0.85, 1.0);  // disc solar alb-galben
        return;
    }
    if (sd > 0.992) {
        float glow = (sd - 0.992) / 0.0075;
        skyColor   = mix(skyColor, vec3(1.0, 0.88, 0.55), glow * 0.45); // glow portocaliu
    }

    // muntii / silueta munti la orizont
    // mLine = linia de delimitare a muntelui, variaza cu unghiul (atan) pentru silueta.
    // 3 frecvente sinusoidale suprapuse = munti cu forme naturale.
    // Fragmentele sub mLine primesc culoarea gri-albastruie a muntelui.
    float angle      = atan(dir.z, dir.x);
    float mLine      = 0.025
                     + 0.040 * (sin(angle * 4.0)  * 0.5 + 0.5)   // forma de baza
                     + 0.025 * (sin(angle * 9.0)  * 0.5 + 0.5)   // detalii medii
                     + 0.012 * (sin(angle * 19.0) * 0.5 + 0.5);  // detalii fine

    if (dir.y < mLine + 0.005 && dir.y > -0.20) {
        // smoothstep = tranzitie lina intre culoarea muntelui si cer (anti-aliasing)
        float blend     = smoothstep(mLine - 0.008, mLine + 0.005, dir.y);
        vec3  mColor    = mix(vec3(0.28, 0.34, 0.40), skyColor, blend); // gri-albastru
        FragColor = vec4(mColor, 1.0);
        return;
    }

    FragColor = vec4(skyColor, 1.0);
}

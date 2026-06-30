#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform float time;
uniform vec4 shallowColor;
uniform vec4 deepColor;
uniform vec4 highlightColor;
uniform vec4 edgeMask;

float band(float value, float steps)
{
    return floor(value * steps) / steps;
}

void main()
{
    vec2 waterUv = fragTexCoord;
    vec2 localUv = fract(waterUv);
    vec2 blockUv = floor(waterUv * 8.0) / 8.0;
    float drift = time * 0.11;
    float waveA = sin((blockUv.x + drift) * 11.0 + blockUv.y * 3.5);
    float waveB = cos((blockUv.y - drift * 0.7) * 9.0 + blockUv.x * 2.5);
    float wave = band(waveA * 0.5 + waveB * 0.5 + 1.0, 6.0);
    float centerHighlight = smoothstep(0.86, 1.0, wave) * 0.18;

    float northEdge = edgeMask.x * (1.0 - smoothstep(0.018, 0.085, localUv.y));
    float eastEdge = edgeMask.y * (1.0 - smoothstep(0.018, 0.085, 1.0 - localUv.x));
    float southEdge = edgeMask.z * (1.0 - smoothstep(0.018, 0.085, 1.0 - localUv.y));
    float westEdge = edgeMask.w * (1.0 - smoothstep(0.018, 0.085, localUv.x));
    float shore = clamp(max(max(northEdge, eastEdge), max(southEdge, westEdge)), 0.0, 1.0);

    vec3 color = mix(deepColor.rgb, shallowColor.rgb, 0.62 + wave * 0.08);
    color = mix(color, highlightColor.rgb, centerHighlight + shore * 0.68);
    float alpha = mix(deepColor.a, shallowColor.a, 0.58) + shore * 0.3;

    finalColor = vec4(color, clamp(alpha, 0.0, 0.92)) * fragColor;
}

#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform float pixelSize;

void main()
{
    vec2 pixelCoord = floor(fragTexCoord * resolution / pixelSize) * pixelSize;
    vec2 pixelUv = (pixelCoord + pixelSize * 0.5) / resolution;
    vec4 color = texture(texture0, pixelUv);

    finalColor = color * fragColor;
}

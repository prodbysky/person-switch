// Stolen from raylib examples
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add your custom variables here

// NOTE: Render size values must be passed from code
uniform float renderWidth;
uniform float renderHeight;

uniform float pixelWidth = 2.0;
uniform float pixelHeight = 2.0;

void main()
{
    float dx = pixelWidth*(1.0/renderWidth);
    float dy = pixelHeight*(1.0/renderHeight);

    vec2 coord = vec2(dx*floor(fragTexCoord.x/dx), dy*floor(fragTexCoord.y/dy));

    vec3 tc = texture(texture0, coord).rgb;

    finalColor = vec4(tc, 1.0);
}

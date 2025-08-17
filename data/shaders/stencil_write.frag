#version 330 core
uniform sampler2D u_tex;
uniform float u_alphaThreshold;   // e.g. 0.5
in vec2 texCoords;                // SFML default varying name
out vec4 fragColor;

void main() {
    vec4 c = texture(u_tex, texCoords);
    if (c.a <= u_alphaThreshold) discard;   // emulate glAlphaFunc(GL_GREATER, threshold)
    fragColor = c;                          // color won't write when stencilOnly=true
}

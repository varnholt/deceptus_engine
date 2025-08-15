#version 330 core
uniform sampler2D u_tex;
uniform float u_alphaThreshold;
in vec2 v_texCoord;
out vec4 fragColor;

void main() {
    vec4 c = texture(u_tex, v_texCoord);
    if (c.a <= u_alphaThreshold) discard; // emulate glAlphaFunc(GL_GREATER, threshold)
    fragColor = c; // only write to stencil buffer with stencilOnly=true
}

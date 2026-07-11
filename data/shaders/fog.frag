#ifdef GL_ES
uniform sampler2D u_texture;
uniform float u_time;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main() {
   vec2 uv = sf_v_texCoord;
   uv.x += mod(u_time * 0.02, 1.0);
   sf_fragColor = texture(u_texture, uv);
}
#else
uniform sampler2D u_texture;
uniform float u_time;

void main() {
   vec2 uv = gl_TexCoord[0].xy;
   uv.x += mod(u_time * 0.02, 1.0);
   gl_FragColor = texture2D(u_texture, uv);
}
#endif

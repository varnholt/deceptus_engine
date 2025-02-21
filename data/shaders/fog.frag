#version 120
uniform sampler2D u_texture;
uniform float u_time;
uniform vec2 u_resolution;
uniform float u_uv_height;

void main() {
   vec2 uv = gl_TexCoord[0].xy;
   uv.x += mod(u_time * 0.02, 1.0);
   gl_FragColor = texture2D(u_texture, uv);	
}
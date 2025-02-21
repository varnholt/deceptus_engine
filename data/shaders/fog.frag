uniform sampler2D u_texture;
uniform vec2 u_resolution;
uniform float u_time;

void main() {
   vec2 uv = gl_TexCoord[0].xy;
   uv.x = mod(uv.x + mod(u_time * 0.02, 1.0), 1.0);
   gl_FragColor = texture2D(u_texture, uv);	
}

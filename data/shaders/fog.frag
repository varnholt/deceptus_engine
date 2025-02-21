uniform sampler2D u_texture;
uniform vec2 u_resolution;
uniform float u_time;

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    
    // Compute horizontal offset using time
	
	float speed = 0.02; // Adjust speed as needed
    float offset = mod(u_time * speed, 1.0);
    
    // Wrap the texture coordinates for infinite movement
    uv.x = mod(uv.x + offset, 1.0);
    
    // Sample the texture
    gl_FragColor = texture2D(u_texture, uv);
	
}
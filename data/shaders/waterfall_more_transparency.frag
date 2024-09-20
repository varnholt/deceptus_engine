uniform vec2 u_resolution;
uniform float u_time;
uniform float u_uv_height; // used as a uv stretch factor for the quad height
uniform sampler2D u_texture;

const vec4 col1 = vec4(0.510, 0.776, 0.486, 1.0);
const vec4 col2 = vec4(0.200, 0.604, 0.318, 1.0);
const vec4 col3 = vec4(0.145, 0.490 ,0.278, 1.0);
const vec4 col4 = vec4(0.059, 0.255, 0.251, 1.0);

// Simple 2D noise function to create jagged edges
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    float time = u_time * 0.3;

    // Apply pixelate effect
    vec2 uv_pixel = floor(uv * u_resolution * 2.0) / (u_resolution * 2.0);

    // Displacement on top of y
    vec3 displace = texture2D(u_texture, vec2(uv_pixel.x, (uv_pixel.y + time) * 0.05)).xyz;
    displace *= 0.1; // tweak this scale a bit?
    displace.x -= 1.0;
    displace.y -= 1.0;
    displace.y *= 0.5;

    // Color
    vec2 uv_tmp = uv_pixel;
    uv_tmp.y *= 0.2;
    uv_tmp.y += time;
    vec4 color = texture2D(u_texture, uv_tmp + displace.xy);

    // Match to colors
    vec4 noise_color = floor(color * 10.0) / 5.0;
    vec4 dark = mix(col1, col2, uv.y / u_uv_height);
    vec4 bright = mix(col3, col4, uv.y / u_uv_height);
    color = mix(dark, bright, noise_color);

    // Add gradients (top dark and transparent, bottom bright)
    color.xyz -= 0.45 * pow(uv_pixel.y / u_uv_height, 8.0);
    color.a -= 0.2 * pow(uv_pixel.y / u_uv_height, 8.0);

    // Create jagged transparency effect on the edges
    float edge_noise = noise(vec2(uv_pixel.x * 10.0, (uv_pixel.y + time * 0.1) * 5.0));
    float edge_factor = smoothstep(0.0, 0.1, uv.x) * smoothstep(1.0, 0.9, uv.x);
    color.a *= mix(1.0, 0.0, edge_noise * edge_factor);

    // Make waterfall transparent
    color.a -= 0.2;

    gl_FragColor = vec4(color);
}

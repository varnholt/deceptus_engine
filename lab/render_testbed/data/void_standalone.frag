#ifdef GL_ES
precision mediump float;
#endif

// === Uniforms ===
uniform vec2 resolution;       // screen resolution in pixels
uniform float time;            // time in seconds
 float pixel_size = 2.0;      // size of pixel blocks for pixelation
uniform sampler2D iChannel0;   // noise texture
uniform float alpha;
uniform float radius_factor;

// === Main Entry Point ===
void main()
{
    // Pixelate the screen coordinates
    vec2 frag_coord = gl_FragCoord.xy;
    vec2 pixelated_coord = floor(frag_coord / pixel_size) * pixel_size;

    // Convert to normalized screen space, centered at (0,0), scaled by vertical resolution
    vec2 normalized_coords = (pixelated_coord - 0.5 * resolution.xy) / resolution.y;

    // Convert to polar coordinates
    float radius = length(normalized_coords) * 1.5 + radius_factor* 0.000001;
    float angle = atan(normalized_coords.y, normalized_coords.x);
    angle += radius * 1.1; // swirl twist
    vec2 polar_coords = vec2(radius, angle);

    // Animated time offset
    float t = time * 0.2;
    float noise_scale = 15.0;

    // Generate layered sample positions for noise
    vec3 sample_pos_1 = vec3(sin(angle), cos(angle), pow(radius, 0.3) + t * 0.1);
    vec3 sample_pos_2 = vec3(cos(1.0 - angle), sin(1.0 - angle), pow(radius, 0.5) + t * 0.1);

    // Sample first layer of noise
    vec2 noise_uv_1 = fract((sample_pos_1.xy + sample_pos_1.z) / noise_scale);
    vec2 noise_uv_2 = fract((sample_pos_2.xy + sample_pos_2.z) / noise_scale);
    float noise_1 = texture2D(iChannel0, noise_uv_1).r;
    float noise_2 = texture2D(iChannel0, noise_uv_2).r;

    // Second noise layer using results from the first
    vec2 noise_uv_3 = fract((vec2(noise_1, noise_2) + radius + t * 0.3) / noise_scale);
    float refined_noise = texture2D(iChannel0, noise_uv_3).r;

    // Third noise layer for more depth
    vec2 noise_uv_4 = fract((vec2(refined_noise - noise_1, refined_noise - noise_2) + refined_noise + t * 0.3) / noise_scale);
    refined_noise = texture2D(iChannel0, noise_uv_4).r;

    // Combine noise and radial offset to compute brightness
    float brightness = (refined_noise + radius * 5.0) / 6.0;
    brightness = clamp(brightness, 0.0, 1.0);

 // gl_FragColor = vec4(vec3(brightness), 1.0);

    // Ring alpha based on brightness band
    float fade_in  = smoothstep(0.3, 0.5, brightness);
    float fade_out = 1.0 - smoothstep(0.5, 0.7, brightness);
    float ring_alpha = clamp(fade_in * fade_out, 0.0, 1.0);

    // Final color: cyan/petrol swirl ring
    vec3 swirl_color = vec3(0.0, 0.9, 0.8);
    vec3 effect_color = swirl_color;
    vec3 final_color = mix(vec3(0.0), effect_color, ring_alpha);

    // Output final color
   gl_FragColor = vec4(final_color, alpha); // Alpha ignored in SFML target
    //gl_FragColor = vec4(1,0,0,1);

}

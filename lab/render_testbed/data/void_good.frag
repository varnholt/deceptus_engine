float pixel_size = 4.0; // size of the blocks in pixels


void mainImage(out vec4 frag_color, in vec2 frag_coord)
{
    vec2 pixelated_coord = floor(frag_coord / pixel_size) * pixel_size;
    vec2 normalized_coords = (pixelated_coord - 0.5 * iResolution.xy) / iResolution.y;

    // Convert pixel coordinates to normalized screen space, centered
    // vec2 normalized_coords = (frag_coord - 0.5 * iResolution.xy) / iResolution.y;

    // Convert to polar coordinates: polar_coords.x = radius, polar_coords.y = angle
    vec2 polar_coords = vec2(length(normalized_coords) * 1.5, atan(normalized_coords.y, normalized_coords.x));

    // Apply swirl effect by modifying the angle based on radius
    polar_coords.y += polar_coords.x * 1.1;

    // Slowed down time for animation
    float time_slowed = iTime * 0.2;

    // Noise frequency scale (larger = coarser detail)
    float noise_scale = 10.0;

    // First 3D sample position (based on swirling polar coords)
    vec3 sample_position_1 = vec3(
        sin(polar_coords.y),
        cos(polar_coords.y),
        pow(polar_coords.x, 0.3) + time_slowed * 0.1
    );

    // Second 3D sample position with variation for layering
    vec3 sample_position_2 = vec3(
        cos(1.0 - polar_coords.y),
        sin(1.0 - polar_coords.y),
        pow(polar_coords.x, 0.5) + time_slowed * 0.1
    );

    // Project 3D positions into UV coordinates for noise texture
    vec2 noise_uv_1 = fract((sample_position_1.xy + sample_position_1.z) / noise_scale);
    vec2 noise_uv_2 = fract((sample_position_2.xy + sample_position_2.z) / noise_scale);

    // Sample noise texture
    float noise_sample_1 = texture(iChannel0, noise_uv_1).r;
    float noise_sample_2 = texture(iChannel0, noise_uv_2).r;

    // Combine samples and compute new UVs for more noise
    vec2 noise_uv_3 = fract((vec2(noise_sample_1, noise_sample_2) + polar_coords.x + time_slowed * 0.3) / noise_scale);
    float refined_noise = texture(iChannel0, noise_uv_3).r;

    // Feed back refined noise into next sample to add depth
    vec2 noise_uv_4 = fract((vec2(refined_noise - noise_sample_1, refined_noise - noise_sample_2) + refined_noise + time_slowed * 0.3) / noise_scale);
    refined_noise = texture(iChannel0, noise_uv_4).r;

    // Final brightness value combining noise and radial distance
    float brightness = (refined_noise + polar_coords.x * 5.0) / 6.0;brightness = clamp(brightness, 0.0, 1.0);

// Define visibility ring band
float brightness_inner = 0.3;
float brightness_peak = 0.5;
float brightness_outer = 0.7;

float fade_in  = smoothstep(brightness_inner, brightness_peak, brightness);
float fade_out = 1.0 - smoothstep(brightness_peak, brightness_outer, brightness);
float ring_alpha = clamp(fade_in * fade_out, 0.0, 1.0);

// Swirl color (petrol / cyan)
vec3 swirl_color = vec3(0.0, 0.9, 0.8);
vec3 effect_color = swirl_color; // constant color in ring

// Simulate alpha blending over black
vec3 final_color = mix(vec3(0.0), effect_color, ring_alpha);
frag_color = vec4(final_color, 1.0); // alpha ignored in ShaderToy



}

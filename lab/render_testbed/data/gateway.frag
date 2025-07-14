#version 120

uniform float time;
uniform float alpha;
uniform float radius;
uniform vec2 resolution;
uniform vec2 center;

const float pixelSize = 4.0;

float len(vec3 p) {
    return max(abs(p.x) * 0.5 + abs(p.z) * 0.5,
               max(abs(p.y) * 0.5 + abs(p.x) * 0.5,
                   abs(p.z) * 0.5 + abs(p.y) * 0.5));
}

// Helper to manually sample only once per block
// Computes the color for a single pixel block center.
// This uses a basic raymarching loop to simulate a glowing volumetric field.
vec3 computePixelColor(vec2 pixelBlockCenter)
{
    // Convert screen coordinates to normalized coordinates (-1 to +1)
    vec2 uv = (pixelBlockCenter - 0.5 * resolution) / resolution.y;

    // Camera origin (eye position)
    vec3 rayOrigin = vec3(0.0, 0.0, time);

    // Ray direction, pointing into the screen
    vec3 rayDir = normalize(vec3(uv, 1.0));

    // Final color to accumulate
    vec3 color = vec3(0.0);

    // Current distance along the ray
    float depth = 0.0;

    // Small variation to make the view change over time
    float viewWobble = cos(time * 0.05) * 0.15;

    // Step through space along the ray
    for (int i = 0; i < 60; i++) {
        // Compute the current sample point in 3D
        vec3 pos = rayOrigin + rayDir * depth;

        // Volumetric pattern using nested cosine distortion
        float distToShape = len(cos(pos * 0.6 + cos(pos * 0.3 + time))) - 0.75;

        // Brightness of this step (glow)
        float brightness = pow(max(0.0, 1.0 - abs(distToShape) * 10.25), 1.0);
        brightness /= float(i) + 10.0;

        // Color contribution from this sample
        vec3 glowColor = (cos(vec3(pos.xy, depth)) * 0.5 + 0.5 +
                          cos(vec3(depth + time, pos.yx) * 0.1) * 0.5 + 0.5 + 1.0) / 3.0;

        color += glowColor * brightness;

        // Move along the ray based on current distance
        depth += max(abs(distToShape), 0.35 + viewWobble);

        // Slightly bend the ray for a flowing feel
        vec3 swirl = vec3(sin(depth * 0.5), cos(depth * 0.5), 0.0);
        rayDir = normalize(rayDir + swirl * distToShape * 0.05 * clamp(depth - 1.0, 0.0, 1.0));
    }

    // Apply simple tone mapping to compress color values
    // return pow(color, vec3(1.7));
    return pow(color * 1.8, vec3(1.7)); // brighten before tonemapping
}


void main()
{
    vec2 fragCoord = gl_FragCoord.xy;

    // Snap to pixel block center
    vec2 blockCoord = floor(fragCoord / pixelSize) * pixelSize + pixelSize * 0.5;

    // Compute shared color for block
    vec3 color = computePixelColor(blockCoord);

    // Shared alpha falloff
    float dist = distance(blockCoord, center);
    float mask = smoothstep(radius * 0.5, radius, dist);
    float final_alpha = alpha * (1.0 - mask);

    gl_FragColor = vec4(color, final_alpha);
}

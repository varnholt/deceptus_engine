#if __VERSION__ >= 300
uniform float     u_time;
uniform vec2      u_resolution;
uniform sampler2D u_texture;
uniform float     u_ring_scale;
uniform float     u_pixel_size;   //!< pixel block size in screen pixels; 1.0 = no pixelation, 4.0 = coarse retro look
uniform vec3      u_flash_color;  //!< color to flash toward (0-1 per channel)
uniform float     u_flash_intensity; //!< 0 = no flash, 1 = full flash color
uniform float     u_touch_angle;     //!< angle the player is pressing against the band, radians
uniform float     u_touch_intensity; //!< how deep the band is pushed in there; 0 = untouched
uniform float     u_touch_width;     //!< angular falloff of the dent, radians
uniform vec2      u_push;            //!< whole-ring displacement in uv units, away from the last hit
uniform float     u_dissolve;        //!< raises the alpha cutoff; 0 = powered, 1 = gone

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

#define TIME (u_time * 0.15)


float noise(vec2 x)
{
    return texture(u_texture, x * 0.01).x;
}

float fbm(vec2 p)
{
    vec4 tt = fract(vec4(TIME * 2.0) + vec4(0.0, 0.25, 0.5, 0.75));

    vec2 p1 = p - normalize(p) * tt.x;
    vec2 p2 = vec2(1.0) + p - normalize(p) * tt.y;
    vec2 p3 = vec2(2.0) + p - normalize(p) * tt.z;
    vec2 p4 = vec2(3.0) + p - normalize(p) * tt.w;

    vec4 tr = vec4(1.0) - abs(tt - vec4(0.5)) * 2.0;
    float z = 2.0;
    vec4 rz = vec4(0.0);

    for (float i = 1.0; i < 4.0; i++)
    {
        rz += abs(vec4(noise(p1), noise(p2), noise(p3), noise(p4)) - 0.5) * 2.0 / z;
        z  *= 2.0;
        p1 *= 2.0;
        p2 *= 2.0;
        p3 *= 2.0;
        p4 *= 2.0;
    }

    return dot(rz, tr) * 0.25;
}

float circularEffect(vec2 p)
{
    float angle      = atan(p.y, p.x);
    float distortion = sin(angle * 6.0 + TIME) * 0.005 * sin(TIME * 10.0);
    float r          = (length(p) + distortion) * 5.0;
    r = 1.0 / r;
    return r * 5.0 - 12.0;
}

void main()
{
    vec2 frag_coord  = sf_v_texCoord * u_resolution;
    vec2 p_pixelated = floor(frag_coord / u_pixel_size) * u_pixel_size;
    vec2 p           = p_pixelated / u_resolution - 0.5;

    // the whole ring recoils away from whatever last hit it, then drifts back onto the sword
    p -= u_push;

    // the sway is a fixed distance on screen, not a fraction of the quad: the quad is sized for
    // the release to expand into, and a fraction of it would sway the ring several pixels
    p += vec2(sin(TIME * 15.0) * (0.6 / u_resolution.x), 0.0);
    p.x *= u_resolution.x / u_resolution.y;
    p   /= u_ring_scale;    // Shadertoy used p *= 5.0; equivalent when u_ring_scale = 0.2

    // the player pressing against the ring dents the band inward at that angle. scaling p up
    // means the band, which sits at a fixed length in p, lands at a smaller radius on screen.
    float fragment_angle    = atan(p.y, p.x);
    float angular_distance  = abs(fragment_angle - u_touch_angle);
    angular_distance        = min(angular_distance, 6.28318530718 - angular_distance);
    float touch_falloff     = exp(-(angular_distance * angular_distance) / (u_touch_width * u_touch_width));
    p *= (1.0 + u_touch_intensity * touch_falloff);

    // the band only ever lives in a thin annulus around length 14/12, however large the quad is.
    // everything outside it costs twelve texture fetches to produce nothing, so skip it: this is
    // what lets the quad be screen sized without paying for a screen of fbm every frame.
    float ring_space_radius = length(p);
    if (ring_space_radius < 0.6 || ring_space_radius > 2.2)
    {
        sf_fragColor = vec4(0.0);
        return;
    }

    float fbm_value = fbm(p);
    vec2  offset    = vec2(p.x / 14.0, p.y / 14.0);
    float effect    = abs(-circularEffect(offset));
    fbm_value      *= effect * effect * 2.0;

    vec3  base_color = vec3(0.2, 0.1, 0.4) / fbm_value;
    float brightness = dot(base_color, vec3(0.299, 0.587, 0.114));

    // flash only tints already-visible pixels; alpha stays gated on the original brightness
    // so background pixels remain transparent even at full flash intensity
    vec3 output_color = mix(base_color, u_flash_color, u_flash_intensity);

    // threshold cuts near-black pixels to fully transparent, preventing colour bleed onto adjacent layers
    sf_fragColor = vec4(output_color, clamp(brightness - 0.05 - u_dissolve, 0.0, 1.0));
}
#else
uniform float     u_time;
uniform vec2      u_resolution;
uniform sampler2D u_texture;
uniform float     u_ring_scale;
uniform float     u_pixel_size;   //!< pixel block size in screen pixels; 1.0 = no pixelation, 4.0 = coarse retro look
uniform vec3      u_flash_color;  //!< color to flash toward (0-1 per channel)
uniform float     u_flash_intensity; //!< 0 = no flash, 1 = full flash color
uniform float     u_touch_angle;     //!< angle the player is pressing against the band, radians
uniform float     u_touch_intensity; //!< how deep the band is pushed in there; 0 = untouched
uniform float     u_touch_width;     //!< angular falloff of the dent, radians
uniform vec2      u_push;            //!< whole-ring displacement in uv units, away from the last hit
uniform float     u_dissolve;        //!< raises the alpha cutoff; 0 = powered, 1 = gone

#define TIME (u_time * 0.15)


float noise(vec2 x)
{
    return texture2D(u_texture, x * 0.01).x;
}

float fbm(vec2 p)
{
    vec4 tt = fract(vec4(TIME * 2.0) + vec4(0.0, 0.25, 0.5, 0.75));

    vec2 p1 = p - normalize(p) * tt.x;
    vec2 p2 = vec2(1.0) + p - normalize(p) * tt.y;
    vec2 p3 = vec2(2.0) + p - normalize(p) * tt.z;
    vec2 p4 = vec2(3.0) + p - normalize(p) * tt.w;

    vec4 tr = vec4(1.0) - abs(tt - vec4(0.5)) * 2.0;
    float z = 2.0;
    vec4 rz = vec4(0.0);

    for (float i = 1.0; i < 4.0; i++)
    {
        rz += abs(vec4(noise(p1), noise(p2), noise(p3), noise(p4)) - 0.5) * 2.0 / z;
        z  *= 2.0;
        p1 *= 2.0;
        p2 *= 2.0;
        p3 *= 2.0;
        p4 *= 2.0;
    }

    return dot(rz, tr) * 0.25;
}

float circularEffect(vec2 p)
{
    float angle      = atan(p.y, p.x);
    float distortion = sin(angle * 6.0 + TIME) * 0.005 * sin(TIME * 10.0);
    float r          = (length(p) + distortion) * 5.0;
    r = 1.0 / r;
    return r * 5.0 - 12.0;
}

void main()
{
    vec2 frag_coord  = gl_TexCoord[0].xy * u_resolution;
    vec2 p_pixelated = floor(frag_coord / u_pixel_size) * u_pixel_size;
    vec2 p           = p_pixelated / u_resolution - 0.5;

    // the whole ring recoils away from whatever last hit it, then drifts back onto the sword
    p -= u_push;

    // the sway is a fixed distance on screen, not a fraction of the quad: the quad is sized for
    // the release to expand into, and a fraction of it would sway the ring several pixels
    p += vec2(sin(TIME * 15.0) * (0.6 / u_resolution.x), 0.0);
    p.x *= u_resolution.x / u_resolution.y;
    p   /= u_ring_scale;    // Shadertoy used p *= 5.0; equivalent when u_ring_scale = 0.2

    // the player pressing against the ring dents the band inward at that angle. scaling p up
    // means the band, which sits at a fixed length in p, lands at a smaller radius on screen.
    float fragment_angle    = atan(p.y, p.x);
    float angular_distance  = abs(fragment_angle - u_touch_angle);
    angular_distance        = min(angular_distance, 6.28318530718 - angular_distance);
    float touch_falloff     = exp(-(angular_distance * angular_distance) / (u_touch_width * u_touch_width));
    p *= (1.0 + u_touch_intensity * touch_falloff);

    // the band only ever lives in a thin annulus around length 14/12, however large the quad is.
    // everything outside it costs twelve texture fetches to produce nothing, so skip it: this is
    // what lets the quad be screen sized without paying for a screen of fbm every frame.
    float ring_space_radius = length(p);
    if (ring_space_radius < 0.6 || ring_space_radius > 2.2)
    {
        gl_FragColor = vec4(0.0);
        return;
    }

    float fbm_value = fbm(p);
    vec2  offset    = vec2(p.x / 14.0, p.y / 14.0);
    float effect    = abs(-circularEffect(offset));
    fbm_value      *= effect * effect * 2.0;

    vec3  base_color = vec3(0.2, 0.1, 0.4) / fbm_value;
    float brightness = dot(base_color, vec3(0.299, 0.587, 0.114));

    // flash only tints already-visible pixels; alpha stays gated on the original brightness
    // so background pixels remain transparent even at full flash intensity
    vec3 output_color = mix(base_color, u_flash_color, u_flash_intensity);

    // threshold cuts near-black pixels to fully transparent, preventing colour bleed onto adjacent layers
    gl_FragColor = vec4(output_color, clamp(brightness - 0.05 - u_dissolve, 0.0, 1.0));
}
#endif

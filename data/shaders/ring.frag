#version 120

uniform float     u_time;
uniform vec2      u_resolution;
uniform sampler2D u_texture;
uniform float     u_uv_height;
uniform float     u_ring_scale;
uniform float     u_pixel_size;  //!< pixel block size in screen pixels; 1.0 = no pixelation, 4.0 = coarse retro look

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

    p += vec2(sin(TIME * 15.0) * 0.01, 0.0);
    p.x *= u_resolution.x / u_resolution.y;
    p   /= u_ring_scale;    // Shadertoy used p *= 5.0; equivalent when u_ring_scale = 0.2

    float fbm_value = fbm(p);
    vec2  offset    = vec2(p.x / 14.0, p.y / 14.0);
    float effect    = abs(-circularEffect(offset));
    fbm_value      *= effect * effect * 2.0;

    vec3  col        = vec3(0.2, 0.1, 0.4) / fbm_value;
    float brightness = dot(col, vec3(0.299, 0.587, 0.114));

    gl_FragColor = vec4(col, clamp(brightness, 0.0, 1.0));
}

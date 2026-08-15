// tears the composited frame apart in short bursts: horizontal bands jump sideways,
// the color channels drift apart while a burst is active and scanlines darken the image.
// between bursts the frame passes through untouched apart from the scanlines.
// u_pixel_size is the size of one game pixel in uv space, so the channel separation and
// the scanline spacing stay tied to game pixels rather than to the window resolution.

#if __VERSION__ >= 300
uniform sampler2D u_texture;
uniform vec2 u_pixel_size;
uniform float u_time;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

float hash(float seed)
{
   return fract(sin(seed * 12.9898) * 43758.5453);
}

void main()
{
   vec2 uv = sf_v_texCoord;

   // most burst slots stay quiet, roughly every third one tears
   float burst_slot = floor(u_time * 2.0);
   float burst = step(0.72, hash(burst_slot));

   // horizontal bands, reshuffled several times within a single burst
   float band = floor(uv.y * 28.0);
   float band_noise = hash(band + burst_slot * 17.0 + floor(u_time * 18.0) * 3.0) - 0.5;
   float band_active = step(0.55, abs(band_noise) * 2.0);
   float displacement = band_noise * 0.06 * burst * band_active;
   uv.x += displacement;

   vec2 separation = vec2(6.0 * burst * band_active, 0.0) * u_pixel_size;
   float red = texture(u_texture, uv + separation).r;
   float green = texture(u_texture, uv).g;
   float blue = texture(u_texture, uv - separation).b;
   float alpha = texture(u_texture, uv).a;

   vec3 color = vec3(red, green, blue);
   color *= 1.0 - 0.12 * step(0.5, fract(uv.y / (u_pixel_size.y * 2.0)));

   sf_fragColor = vec4(color, alpha);
}
#else
uniform sampler2D u_texture;
uniform vec2 u_pixel_size;
uniform float u_time;

float hash(float seed)
{
   return fract(sin(seed * 12.9898) * 43758.5453);
}

void main()
{
   vec2 uv = gl_TexCoord[0].xy;

   // most burst slots stay quiet, roughly every third one tears
   float burst_slot = floor(u_time * 2.0);
   float burst = step(0.72, hash(burst_slot));

   // horizontal bands, reshuffled several times within a single burst
   float band = floor(uv.y * 28.0);
   float band_noise = hash(band + burst_slot * 17.0 + floor(u_time * 18.0) * 3.0) - 0.5;
   float band_active = step(0.55, abs(band_noise) * 2.0);
   float displacement = band_noise * 0.06 * burst * band_active;
   uv.x += displacement;

   vec2 separation = vec2(6.0 * burst * band_active, 0.0) * u_pixel_size;
   float red = texture2D(u_texture, uv + separation).r;
   float green = texture2D(u_texture, uv).g;
   float blue = texture2D(u_texture, uv - separation).b;
   float alpha = texture2D(u_texture, uv).a;

   vec3 color = vec3(red, green, blue);
   color *= 1.0 - 0.12 * step(0.5, fract(uv.y / (u_pixel_size.y * 2.0)));

   gl_FragColor = vec4(color, alpha);
}
#endif

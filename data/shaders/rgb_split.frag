// separates the color channels horizontally, like a badly converged crt.
// the separation breathes slowly so the fringe does not look like a static offset.
// u_pixel_size is the size of one game pixel in uv space, so the offset stays a game
// pixel count and the fringe keeps its width at any window scale.

#ifdef GL_ES
uniform sampler2D u_texture;
uniform vec2 u_pixel_size;
uniform float u_time;

// horizontal channel separation in game pixels
const float channel_offset = 2.0;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
   vec2 uv = sf_v_texCoord;

   float wobble = 1.0 + sin(u_time * 1.7) * 0.35;
   vec2 separation = vec2(channel_offset * wobble, 0.0) * u_pixel_size;

   float red = texture(u_texture, uv + separation).r;
   float green = texture(u_texture, uv).g;
   float blue = texture(u_texture, uv - separation).b;
   float alpha = texture(u_texture, uv).a;

   sf_fragColor = vec4(red, green, blue, alpha);
}
#else
uniform sampler2D u_texture;
uniform vec2 u_pixel_size;
uniform float u_time;

// horizontal channel separation in game pixels
const float channel_offset = 2.0;

void main()
{
   vec2 uv = gl_TexCoord[0].xy;

   float wobble = 1.0 + sin(u_time * 1.7) * 0.35;
   vec2 separation = vec2(channel_offset * wobble, 0.0) * u_pixel_size;

   float red = texture2D(u_texture, uv + separation).r;
   float green = texture2D(u_texture, uv).g;
   float blue = texture2D(u_texture, uv - separation).b;
   float alpha = texture2D(u_texture, uv).a;

   gl_FragColor = vec4(red, green, blue, alpha);
}
#endif

// renders the composited frame at game boy resolution and reduces it to the 4 color dmg palette.
// the grid is derived from u_resolution, the size of the game's own pixel grid, so the effect
// looks identical at every window scale.
//
// the tone curve is what makes this readable: the game is lit far below the palette thresholds,
// so without it almost every pixel collapses into the darkest color. it is a reinhard curve
// rather than a linear window on purpose - a linear one clips everything above its white point to
// the lightest color, which turned brightly lit sprites such as the player into flat silhouettes.
// this one never reaches 1.0, so highlights keep their relative differences and stay shaded.
// mid_grey is the input luminance that lands halfway up the palette, tuned against the catacombs
// so the player keeps its shading; raise it to darken the image.

#if __VERSION__ >= 300
uniform sampler2D u_texture;
uniform vec2 u_resolution;

// screen size in game boy pixels, expressed as a divisor of the view so it stays an integer
// divisor of the pixel grid at any window scale. raise it for chunkier pixels
const float grid_divisor = 1.0;

// tone curve constants, see the note above
const float black_point = 0.0;
const float mid_grey = 0.17;

const vec3 dmg_darkest = vec3(0.059, 0.220, 0.059);
const vec3 dmg_dark = vec3(0.188, 0.384, 0.188);
const vec3 dmg_light = vec3(0.545, 0.675, 0.059);
const vec3 dmg_lightest = vec3(0.608, 0.737, 0.059);

// a full palette step of dither turns the whole image into checkerboard noise, this only
// softens the edges between the four bands
const float dither_strength = 0.06;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

// closed form 2x2 bayer matrix: [[0.0, 0.5], [0.75, 0.25]]
float bayer2x2(vec2 pixel_position)
{
   vec2 cell = floor(pixel_position);
   return fract(cell.x * 0.5 + cell.y * cell.y * 0.75);
}

// 4x4 ordered dither threshold built from two nested 2x2 matrices. without it the
// luminance quantization collapses gradients into flat blobs instead of the stippled
// shading the real hardware produced.
float bayer4x4(vec2 pixel_position)
{
   return bayer2x2(pixel_position * 0.5) * 0.25 + bayer2x2(pixel_position);
}

void main()
{
   // snap to the low resolution grid and sample the center of each grid cell
   vec2 grid_size = u_resolution / grid_divisor;
   vec2 grid_position = floor(sf_v_texCoord * grid_size);
   vec2 uv = (grid_position + 0.5) / grid_size;

   vec4 color = texture(u_texture, uv);

   float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));
   luminance = max(luminance - black_point, 0.0);
   luminance = luminance / (luminance + mid_grey);
   luminance += (bayer4x4(grid_position) - 0.5) * dither_strength;

   vec3 palette_color = dmg_darkest;
   palette_color = mix(palette_color, dmg_dark, step(0.25, luminance));
   palette_color = mix(palette_color, dmg_light, step(0.50, luminance));
   palette_color = mix(palette_color, dmg_lightest, step(0.75, luminance));

   sf_fragColor = vec4(palette_color, color.a);
}
#else
uniform sampler2D u_texture;
uniform vec2 u_resolution;

// screen size in game boy pixels, expressed as a divisor of the view so it stays an integer
// divisor of the pixel grid at any window scale. raise it for chunkier pixels
const float grid_divisor = 1.0;

// tone curve constants, see the note above
const float black_point = 0.0;
const float mid_grey = 0.17;

const vec3 dmg_darkest = vec3(0.059, 0.220, 0.059);
const vec3 dmg_dark = vec3(0.188, 0.384, 0.188);
const vec3 dmg_light = vec3(0.545, 0.675, 0.059);
const vec3 dmg_lightest = vec3(0.608, 0.737, 0.059);

// a full palette step of dither turns the whole image into checkerboard noise, this only
// softens the edges between the four bands
const float dither_strength = 0.06;

// closed form 2x2 bayer matrix: [[0.0, 0.5], [0.75, 0.25]]
float bayer2x2(vec2 pixel_position)
{
   vec2 cell = floor(pixel_position);
   return fract(cell.x * 0.5 + cell.y * cell.y * 0.75);
}

// 4x4 ordered dither threshold built from two nested 2x2 matrices. without it the
// luminance quantization collapses gradients into flat blobs instead of the stippled
// shading the real hardware produced.
float bayer4x4(vec2 pixel_position)
{
   return bayer2x2(pixel_position * 0.5) * 0.25 + bayer2x2(pixel_position);
}

void main()
{
   // snap to the low resolution grid and sample the center of each grid cell
   vec2 grid_size = u_resolution / grid_divisor;
   vec2 grid_position = floor(gl_TexCoord[0].xy * grid_size);
   vec2 uv = (grid_position + 0.5) / grid_size;

   vec4 color = texture2D(u_texture, uv);

   float luminance = dot(color.rgb, vec3(0.299, 0.587, 0.114));
   luminance = max(luminance - black_point, 0.0);
   luminance = luminance / (luminance + mid_grey);
   luminance += (bayer4x4(grid_position) - 0.5) * dither_strength;

   vec3 palette_color = dmg_darkest;
   palette_color = mix(palette_color, dmg_dark, step(0.25, luminance));
   palette_color = mix(palette_color, dmg_light, step(0.50, luminance));
   palette_color = mix(palette_color, dmg_lightest, step(0.75, luminance));

   gl_FragColor = vec4(palette_color, color.a);
}
#endif

// light gradient texture and channel mask color from the sprite
uniform sampler2D u_texture;

// time in seconds, used to animate the dust
uniform float u_time;

// size (in pixels) and drift direction/speed for each dust layer
uniform float u_intensity;
uniform float u_layer_1_size;
uniform vec2 u_layer_1_speed;
uniform float u_layer_2_size;
uniform vec2 u_layer_2_speed;

// low-frequency noise multiplier that dims the whole beam to simulate organic flicker
uniform float u_flicker_speed;
uniform float u_flicker_amount;

// top-left corner and dimensions of the light sprite in world pixels,
// used to sample noise in world space so dust stays fixed as the player moves
uniform vec2 u_sprite_pos_px;
uniform vec2 u_sprite_size_px;

in vec2 sf_v_texCoord;
in vec4 sf_v_color;

layout(location = 0) out vec4 sf_fragColor;

// turns a 2d coordinate into a pseudo-random number
float hash(vec2 point)
{
   point = fract(point * vec2(127.1, 311.7));
   point += dot(point, point + 45.32);
   return fract(point.x * point.y);
}

// smooth noise: interpolates hash values across a grid
float noise(vec2 point)
{
   vec2 grid_cell = floor(point);
   vec2 grid_frac = fract(point);
   grid_frac = grid_frac * grid_frac * (3.0 - 2.0 * grid_frac); // smooth the interpolation curve
   return mix(
      mix(hash(grid_cell), hash(grid_cell + vec2(1.0, 0.0)), grid_frac.x),
      mix(hash(grid_cell + vec2(0.0, 1.0)), hash(grid_cell + vec2(1.0, 1.0)), grid_frac.x),
      grid_frac.y
   );
}

void main()
{
   vec2 uv = sf_v_texCoord;

   // sample the light gradient and apply the channel color (red, green, or blue mask)
   vec4 texel = texture(u_texture, uv) * sf_v_color;

   // convert uv to world pixel position so noise doesn't move with the player
   vec2 world_pos_px = u_sprite_pos_px + uv * u_sprite_size_px;

   // how bright the light is here — used to hide dust outside the lit area
   float light_mask = length(texel.rgb);

   // high pow() makes noise very sparse: most values collapse to near zero,
   // only the peaks survive as small bright specks
   float motes_1 = pow(noise((world_pos_px + u_layer_1_speed * u_time) / u_layer_1_size), 8.0);
   float motes_2 = pow(noise((world_pos_px + u_layer_2_speed * u_time) / u_layer_2_size), 10.0);

   // combine layers, scale by light brightness so dust only shows inside the beam
   float dust_intensity = (motes_1 * 0.7 + motes_2 * 0.5) * light_mask * u_intensity;
   dust_intensity = clamp(dust_intensity, 0.0, 1.0);

   // low-frequency noise dims the whole beam to simulate organic flame flicker
   float flicker_factor = 1.0 - u_flicker_amount * noise(vec2(u_time * u_flicker_speed, 0.42));

   // add dust on top of the light in the same channel, then apply flicker to the full beam
   sf_fragColor = vec4((texel.rgb + sf_v_color.rgb * dust_intensity) * flicker_factor, texel.a);
}

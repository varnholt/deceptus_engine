uniform sampler2D current_texture;
uniform sampler2D distortion_map_texture;
uniform sampler2D physics_texture;
uniform float time;
uniform float distortion_amplitude;

in vec2 sf_v_texCoord;
in vec4 sf_v_color;

layout(location = 0) out vec4 sf_fragColor;

vec4 distort(float amount)
{
   vec2 distortion_map_coordinate = sf_v_texCoord;

   // apply time-based movement to both x and y coordinates
   distortion_map_coordinate.x += sin(time * 1.5) * 0.1;
   distortion_map_coordinate.y += cos(time * 1.2) * 0.1;

   // sample the distortion map with updated coordinates
   vec4 distortionMapValue = texture(distortion_map_texture, distortion_map_coordinate);

   // create a distortion offset based on the sampled value from the map
   vec2 distortion_position_offset = distortionMapValue.xy;
   distortion_position_offset -= vec2(0.5f, 0.5f);  // center the distortion
   distortion_position_offset *= distortion_amplitude * amount;

   vec2 distorted_texture_coordinate = sf_v_texCoord.st + distortion_position_offset;
   return sf_v_color * texture(current_texture, distorted_texture_coordinate);
}

void main()
{
    vec4 effect_color = texture(physics_texture, sf_v_texCoord);

    float epsilon = 0.001;
    if (abs(effect_color.r - 0.0) < epsilon && effect_color.g > epsilon && effect_color.b > epsilon)
    {
        vec4 original_color = texture(current_texture, sf_v_texCoord);
        sf_fragColor = distort(effect_color.g);
    }
    else
    {
        vec4 ambient = vec4(0.0, 0.0, 0.0, 1.0);
        sf_fragColor = mix(ambient, sf_v_color, 1.0f) * texture(current_texture, sf_v_texCoord);
    }

    // water: 007f7f, 0.0f, 0.5f, 0.5f
    // solid: 49146a

    // debugging
    // sf_fragColor = effect_color;
}

uniform sampler2D color_map;
uniform sampler2D light_map_1;
uniform sampler2D light_map_2;
uniform sampler2D normal_map;

uniform vec2 u_resolution;
uniform vec4 u_ambient;

struct Light{
   vec3 _position;
   vec4 _color;
};

uniform int u_light_count;
uniform Light u_lights[50];

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
   vec2 uv = sf_v_texCoord;
   vec2 frag_coord_normalized = (gl_FragCoord.xy / u_resolution.xy);

   vec4 diffuse_color = texture(color_map,  uv);
   vec3 normal        = texture(normal_map, uv).rgb;

   // sample both light textures (RGB only, 6 lights total)
   vec3 light_mask1   = texture(light_map_1, uv).rgb;  // lights 0-2 (RGB)
   vec3 light_mask2   = texture(light_map_2, uv).rgb;  // lights 3-5 (RGB)

   vec3 light_sum = vec3(0.0);
   for (int i = 0; i < min(u_light_count, 6); i++) // limit to 6 lights (2 textures × RGB)
   {
      Light light = u_lights[i];

      // get mask for this light's channel from appropriate texture (RGB only)
      float mask;
      if (i == 0) mask = light_mask1.r;
      else if (i == 1) mask = light_mask1.g;
      else if (i == 2) mask = light_mask1.b;
      else if (i == 3) mask = light_mask2.r;
      else if (i == 4) mask = light_mask2.g;
      else if (i == 5) mask = light_mask2.b;

      // only calculate if mask is non-zero (sprite reaches this pixel)
      if (mask < 0.01) continue;

      vec2 light_pos_normalized = light._position.xy; // xy are already in 0..1

      vec3 light_dir = vec3(light_pos_normalized - frag_coord_normalized, light._position.z);
      light_dir.x *= u_resolution.x / u_resolution.y;

      // normalize normal and light vectors
      vec3 n = normalize(normal * 2.0 - 1.0);
      vec3 l = normalize(light_dir);

      // calculate lighting: color × surface angle (bump mapping) × sprite mask
      // mask provides falloff gradient from sprite texture
      vec3 diffuse_light = (light._color.rgb * light._color.a) * max(dot(n, l), 0.0) * mask;

      light_sum += diffuse_light;
   }

   sf_fragColor = vec4(u_ambient.rgb * diffuse_color.rgb + light_sum, diffuse_color.a);
}

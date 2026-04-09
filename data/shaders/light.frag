uniform sampler2D color_map;
uniform sampler2D light_map;
uniform sampler2D normal_map;

uniform vec2 u_resolution;
uniform vec4 u_ambient;

struct Light{
   vec3 _position;
   vec4 _color;
};

uniform int u_light_count;
uniform Light u_lights[50];


void main()
{
   vec2 uv = gl_TexCoord[0].xy;
   vec2 frag_coord_normalized = (gl_FragCoord.xy / u_resolution.xy);

   vec4 diffuse_color = texture2D(color_map,  uv);
   vec3 normal        = texture2D(normal_map, uv).rgb;
   float light_mask   = texture2D(light_map,  uv).r;

   vec3 light_sum = vec3(0.0);
   for (int i = 0; i < u_light_count; i++)
   {
      Light light = u_lights[i];

      vec2 light_pos_normalized = light._position.xy; // xy are already in 0..1

      vec3 light_dir = vec3(light_pos_normalized - frag_coord_normalized, light._position.z);
      light_dir.x *= u_resolution.x / u_resolution.y;

      // normalize normal and light vectors
      vec3 n = normalize(normal * 2.0 - 1.0);
      vec3 l = normalize(light_dir);

      // the light sprite texture already provides distance falloff via its gradient
      // shader only applies normal mapping (surface angle) and color
      vec3 diffuse_light = (light._color.rgb * light._color.a) * max(dot(n, l), 0.0);
      
      // simple addition - the sprite falloff prevents over-brightening
      light_sum += diffuse_light;
   }

   // apply sprite mask (provides shadow boundaries and distance falloff)
   light_sum *= light_mask;

   gl_FragColor = vec4(u_ambient.rgb * diffuse_color.rgb + light_sum, diffuse_color.a);
}


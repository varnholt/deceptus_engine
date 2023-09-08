uniform sampler2D color_map;
uniform sampler2D light_map;
uniform sampler2D normal_map;

uniform vec2 u_resolution;
uniform vec4 u_ambient;

struct Light{
   vec3 _position;
   vec4 _color;
   vec3 _falloff;
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
      vec3 light_falloff = light._falloff;

      vec3 light_dir = vec3(light_pos_normalized - frag_coord_normalized, light._position.z);
      light_dir.x *= u_resolution.x / u_resolution.y;

      float d = length(light_dir);

      // normalize normal and light vectors
      vec3 n = normalize(normal * 2.0 - 1.0);
      vec3 l = normalize(light_dir);

      // pre-multiply light color with its alpha then do 'n dot l' to determine diffuse
      //                        constant           linear                   quadratic
      float attenuation = 1.0 / (light_falloff.x + (light_falloff.y * d) + (light_falloff.z * d * d));
      vec3 diffuse_light = (light._color.rgb * light._color.a) * max(dot(n, l), 0.0);
      vec3 diffuse_light_weighted = diffuse_light * attenuation;

      light_sum += diffuse_light_weighted;
   }

   // apply light texture on top of light and apply shadow
   light_sum *= light_mask;

   gl_FragColor = vec4(u_ambient.rgb * diffuse_color.rgb + light_sum, diffuse_color.a);
}


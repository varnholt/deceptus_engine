uniform sampler2D color_map;
uniform sampler2D light_map;
uniform sampler2D normal_map;


// --

int light_count = 5;
struct Light{
   vec3 _position;
   vec4 _color;
};

uniform Light u_lights[5];
uniform vec2 u_resolution;
uniform vec4 u_ambient;
uniform vec4 u_vertex_color;

void light2()
{
   // check if that's the right UV
   vec2 uv = gl_TexCoord[0].xy;
   vec2 frag_coord_normalized = (gl_FragCoord.xy / u_resolution.xy);

   vec4 diffuse_color = texture2D(color_map, uv);
   vec3 normal = texture2D(normal_map, uv).rgb;
   float attenuation = texture2D(light_map, uv).r;

   vec3 sum = vec3(0.0);
   int light_count = 5;
   for (int i = 0; i < light_count; i++)
   {
      Light light = u_lights[i];

      vec3 light_pos = light._position;
      vec4 light_col = light._color;

      vec3 light_dir = vec3(light_pos.xy - frag_coord_normalized, light_pos.z);
      light_dir.x *= u_resolution.x / u_resolution.y;

      //normalize our vectors
      vec3 n = normalize(normal * 2.0 - 1.0);
      vec3 l = normalize(light_dir);

      //Pre-multiply light color with intensity
      //Then perform "N dot L" to determine our diffuse term
      vec3 Diffuse = (light_col.rgb * light_col.a) * max(dot(n, l), 0.0);

      //pre-multiply ambient color with intensity
      vec3 Ambient = u_ambient.rgb * u_ambient.a;

      vec3 intensity = Ambient + Diffuse * attenuation;
      vec3 final_color = diffuse_color.rgb * intensity;
      gl_FragColor = u_vertex_color * vec4(final_color, diffuse_color.a);
   }
}

// --

void main()
{
   float ambient = 1.0;

   vec2 uv = gl_TexCoord[0].xy;

   vec4 color = texture2D(color_map, uv);
   vec4 normal = texture2D(normal_map, uv);

   vec4 light = texture2D(light_map, uv);

   if (light.r < 0.001 && light.g < 0.001 && light.b < 0.001)
   {
      gl_FragColor = color * ambient;
   }
   else
   {
      vec4 light_bump = texture2D(light_map, uv + normal.xy - vec2(0.5, 0.5));
      gl_FragColor = color * ambient + 0.7* light + 0.3 * light_bump;
   }
}


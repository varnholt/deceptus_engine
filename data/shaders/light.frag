uniform sampler2D color_map;
uniform sampler2D light_map;
uniform sampler2D normal_map;

void main()
{
   float ambient = 1.0;

   vec2 uv = gl_TexCoord[0].xy;

   vec4 color = texture2D(color_map, uv);
   vec4 normal = texture2D(normal_map, uv);

   vec4 light = texture2D(light_map, uv);
   vec4 light_bump = texture2D(light_map, uv + normal.xy - vec2(0.5, 0.5));

   gl_FragColor = color * ambient + 0.7* light + 0.3 * light_bump;
}


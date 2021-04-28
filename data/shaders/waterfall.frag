uniform vec2 u_resolution;
uniform float u_time;
uniform sampler2D u_texture;

const vec4 col1 = vec4(0.510, 0.776, 0.486, 1.0);
const vec4 col2 = vec4(0.200, 0.604, 0.318, 1.0);
const vec4 col3 = vec4(0.145, 0.490 ,0.278, 1.0);
const vec4 col4 = vec4(0.059, 0.255, 0.251, 1.0);


void main()
{
   vec2 uv = gl_TexCoord[0].xy;

   float time = u_time * 0.3;

   // apply pixelate effect
   vec2 uv_pixel = floor(uv * u_resolution * 2) / (u_resolution * 2);

   // displacement on top of y
   vec3 displace = texture2D(u_texture, vec2(uv_pixel.x, (uv_pixel.y + time) * 0.05)).xyz;
   displace *= 0.1; // tweak this scale a bit?
   displace.x -= 1.0;
   displace.y -= 1.0;
   displace.y *= 0.5;

   // color
   vec2 uv_tmp = uv_pixel;
   uv_tmp.y *= 0.2;
   uv_tmp.y += time;
   vec4 color = texture2D(u_texture, uv_tmp + displace.xy);

   // match to colors
   vec4 noise = floor(color * 10.0) / 5.0;
   vec4 dark   = mix(col1, col2, uv.y);
   vec4 bright = mix(col3, col4, uv.y);
   color = mix(dark, bright, noise);

   // add gradients (top dark and transparent, bottom bright)
   color.xyz -= 0.45 * pow(uv_pixel.y, 8.0);
   color.a -= 0.2 * pow(uv_pixel.y, 8.0);

   // make waterfall transparent
   color.a -= 0.2;

   gl_FragColor = vec4(color);

   // hello world / troubleshooting
   // gl_FragColor = vec4(1,0,0,1);
}


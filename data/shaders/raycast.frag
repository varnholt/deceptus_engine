#version 120

varying vec4 lightpos;
varying vec4 pos;

void main()
{
   float aspect = 16.0 / 9.0;

   vec2 dist = vec2(pos - lightpos);

   float len = 1.0 - 1.1 * sqrt(dist.x * dist.x + dist.y * dist.y) * 0.75;

   if (gl_Color.r == 1.0 && gl_Color.g == 1.0 && gl_Color.b == 1.0)
      gl_FragColor = gl_Color * len;
   else
      gl_FragColor = gl_Color * 0.4;
}


#version 120

precision highp float;

uniform float time;

uniform sampler2D distort;
uniform sampler2D player;

void main() 
{
   vec2 uv = gl_TexCoord[0].xy;

   vec2 offset_shift = vec2(0.0, 0.7);
   vec2 offset_lookup = vec2(uv.x, uv.y);
   vec2 offset = texture2D(distort, offset_lookup + offset_shift).xy;
   vec4 texelColor = texture2D(player, uv.xy + time * offset);

   // fade out alpha
   texelColor.a = (1.0 - time);

   // remove crap
   if (texelColor.r < 0.1)
   {
      texelColor.a = 0.0;
   }
   else
   {
      texelColor = mix(texelColor, vec4(1.0, 1.0, 1.0, 1.0 - time), time);
   }

   gl_FragColor = texelColor;
}

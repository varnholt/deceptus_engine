#version 120

uniform float time;
uniform vec2 flowfield_offset;
varying vec2 vUv;

uniform sampler2D flowfield_1;
uniform sampler2D flowfield_2;
uniform sampler2D current_texture;

void main()
{
   float speed_1 = 2.0;
   float speed_2 = 1.0;

   vec2 offset_shift_1 = flowfield_offset;
   vec2 offset_shift_2 = vec2(0.0, 0.1);

   vec2 offset_lookup = vec2(vUv.x, vUv.y);

   vec2 offset_1 = time * speed_1 * texture2D(flowfield_1, (1.0 + time) * offset_lookup + offset_shift_1).xy;
   vec2 offset_2 = time * speed_2 * texture2D(flowfield_2, (1.0 + time) * offset_lookup + offset_shift_2).xy;

   vec4 texelColor = texture2D(
         current_texture,
           vUv.xy
         + offset_1 * 0.15
         + offset_2 * -0.3
      );

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

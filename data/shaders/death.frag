uniform float time;
uniform vec2 flowfield_offset;

uniform sampler2D flowfield_1;
uniform sampler2D flowfield_2;
uniform sampler2D current_texture;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
   float speed_1 = 2.0;
   float speed_2 = 1.0;

   vec2 offset_shift_1 = flowfield_offset;
   vec2 offset_shift_2 = vec2(0.0, 0.1);

   vec2 offset_lookup = vec2(sf_v_texCoord.x, sf_v_texCoord.y);

   vec2 offset_1 = time * speed_1 * texture(flowfield_1, (1.0 + time) * offset_lookup + offset_shift_1).xy;
   vec2 offset_2 = time * speed_2 * texture(flowfield_2, (1.0 + time) * offset_lookup + offset_shift_2).xy;

   vec4 texelColor = texture(
         current_texture,
           sf_v_texCoord.xy
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

   sf_fragColor = texelColor;
}

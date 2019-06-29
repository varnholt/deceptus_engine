#version 120
uniform sampler2D texture;

uniform int texture_width;
uniform int texture_height;
uniform float blur_radius;
uniform float add_factor;


float scurve(float x)
{
   x = x * 2.0 - 1.0;
   return -x * abs(x) * 0.5 + x + 0.5;
}


vec4 blurV(sampler2D source, vec2 size, vec2 uv, float radius)
{
   if (radius >= 1.0)
   {
      vec4 a = vec4(0.0);
      vec4 c = vec4(0.0);
      float height = 1.0 / size.y;
      float divisor = 0.0;
      float weight = 0.0;
      float radiusMultiplier = 1.0 / radius;

      for (float y = -radius; y <= radius; y++)
      {
         a = texture2D(source, uv + vec2(0.0, y * height));
         weight = scurve(1.0 - (abs(y) * radiusMultiplier));
         c += a * weight;
         divisor += weight;
      }

      return vec4(c.r / divisor, c.g / divisor, c.b / divisor, 1.0);
   }

   return texture2D(source, uv);
}


vec4 blurH(sampler2D source, vec2 size, vec2 uv, float radius)
{
   if (radius >= 1.0)
   {
      vec4 a = vec4(0.0);
      vec4 c = vec4(0.0);
      float width = 1.0 / size.x;
      float divisor = 0.0;
      float weight = 0.0;
      float radiusMultiplier = 1.0 / radius;

      for (float x = -radius; x <= radius; x++)
      {
         a = texture2D(source, uv + vec2(x * width, 0.0));
         weight = scurve(1.0 - (abs(x) * radiusMultiplier));
         c += a * weight;
         divisor += weight;
      }

      return vec4(c.r / divisor, c.g / divisor, c.b / divisor, 1.0);
   }

   return texture2D(source, uv);
}


void main()
{
   vec2 uv = gl_TexCoord[0].xy;
   vec4 h = blurH(texture, vec2(texture_width, texture_height), uv, blur_radius);
   vec4 v = blurV(texture, vec2(texture_width, texture_height), uv, blur_radius);
   gl_FragColor = (h + v) * add_factor;
}

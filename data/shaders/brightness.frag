#ifdef GL_ES
uniform float gamma;
uniform sampler2D u_texture;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
   const float invGamma = 1.0 / 2.2;

   vec4 col = texture(u_texture, sf_v_texCoord);

   float x = pow(col.x, invGamma);
   float y = pow(col.y, invGamma);
   float z = pow(col.z, invGamma);

   x = pow(x, gamma);
   y = pow(y, gamma);
   z = pow(z, gamma);

   sf_fragColor = vec4(x, y, z, col.w);
}
#else
uniform float gamma;
varying vec2 uv;
uniform sampler2D u_texture;

void main()
{
   const float invGamma = 1.0 / 2.2;

   vec4 col = texture2D(u_texture, gl_TexCoord[0].xy);

   float x = pow(col.x, invGamma);
   float y = pow(col.y, invGamma);
   float z = pow(col.z, invGamma);

   x = pow(x, gamma);
   y = pow(y, gamma);
   z = pow(z, gamma);

   gl_FragColor= vec4(x, y, z, col.w);
}
#endif

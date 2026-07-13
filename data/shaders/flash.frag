#ifdef GL_ES
uniform float flash;
uniform sampler2D u_texture;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
   vec4 texture_color = texture(u_texture, sf_v_texCoord);
   vec4 flash_color = vec4(1, 1, 1, 1);
   vec4 mixed = mix(texture_color, flash_color, flash);
   sf_fragColor = vec4(mixed.r, mixed.g, mixed.b, texture_color.a);
}
#else
uniform float flash;
uniform sampler2D u_texture;

void main()
{
   vec4 texture_color = texture2D(u_texture, gl_TexCoord[0].xy);
   vec4 flash_color = vec4(1, 1, 1, 1);
   vec4 mixed = mix(texture_color, flash_color, flash);
   gl_FragColor = vec4(mixed.r, mixed.g, mixed.b, texture_color.a);
}
#endif

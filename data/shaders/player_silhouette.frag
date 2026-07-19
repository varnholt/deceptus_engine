#ifdef GL_ES
uniform sampler2D u_texture;
uniform float u_alpha;

in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
   vec4 texture_color = texture(u_texture, sf_v_texCoord);
   sf_fragColor = vec4(1.0, 1.0, 1.0, texture_color.a * u_alpha);
}
#else
uniform sampler2D u_texture;
uniform float u_alpha;

void main()
{
   vec4 texture_color = texture2D(u_texture, gl_TexCoord[0].xy);
   gl_FragColor = vec4(1.0, 1.0, 1.0, texture_color.a * u_alpha);
}
#endif

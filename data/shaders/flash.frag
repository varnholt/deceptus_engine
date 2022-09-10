#version 120

uniform float flash;
uniform sampler2D texture;

void main()
{
   vec4 texture_color = texture2D(texture, gl_TexCoord[0].xy);
   vec4 flash_color = vec4(1, 1, 1, 1);
   vec4 mixed = mix(texture_color, flash_color, flash);
   gl_FragColor = vec4(mixed.r, mixed.g, mixed.b, texture_color.a); 
}

#ifdef GL_ES
uniform float time; // 0..1
uniform vec3 sf_u_mvpRow0;
uniform vec3 sf_u_mvpRow1;
uniform vec2 sf_u_invTextureSize;

layout(location = 0) in vec2 sf_a_position;
layout(location = 1) in vec4 sf_a_color;
layout(location = 2) in vec2 sf_a_texCoord;

out vec4 sf_v_color;
out vec2 sf_v_texCoord;

void main()
{
   sf_v_texCoord = sf_a_texCoord * sf_u_invTextureSize;

   vec2 displaced_position = sf_a_position;
   displaced_position.y -= (time * 128.0);

   vec3 pos = vec3(displaced_position, 1.0);
   gl_Position = vec4(dot(sf_u_mvpRow0, pos), dot(sf_u_mvpRow1, pos), 0.0, 1.0);
   sf_v_color = sf_a_color;
}
#else
uniform float time; // 0..1
varying vec2 vUv;

void main()
{
   vUv = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;

   vec4 pos = gl_Vertex;
   pos.y -= (time * 128.0);

   gl_Position = gl_ModelViewProjectionMatrix * pos;
}
#endif

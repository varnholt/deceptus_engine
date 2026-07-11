#ifdef GL_ES
uniform vec3 sf_u_mvpRow0;
uniform vec3 sf_u_mvpRow1;
uniform vec2 sf_u_invTextureSize;

layout(location = 0) in vec2 sf_a_position;
layout(location = 1) in vec4 sf_a_color;
layout(location = 2) in vec2 sf_a_texCoord;

out vec2 interpolated_uv;

void main()
{
    vec3 pos = vec3(sf_a_position, 1.0);
    gl_Position = vec4(dot(sf_u_mvpRow0, pos), dot(sf_u_mvpRow1, pos), 0.0, 1.0);
    interpolated_uv = sf_a_texCoord * sf_u_invTextureSize;
}
#else
varying vec2 interpolated_uv;

void main()
{
    gl_Position   = gl_ModelViewProjectionMatrix * gl_Vertex;
    interpolated_uv = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}
#endif

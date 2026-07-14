#ifdef GL_ES
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
   vec3 pos = vec3(sf_a_position, 1.0);
   gl_Position = vec4(dot(sf_u_mvpRow0, pos), dot(sf_u_mvpRow1, pos), 0.0, 1.0);
   sf_v_color = sf_a_color;
   // the ring uses its texcoord as a normalized 0..1 screen-space uv for the circular pattern, so it
   // must NOT be scaled by sf_u_invTextureSize. that built-in reflects whichever texture happens to be
   // bound for the draw (the layer binds none, so VRSFML falls back to an unrelated texture), which
   // would shrink the uv range and blow the ring up. the quad already supplies 0..1 texcoords, matching
   // the desktop gl_TexCoord[0] path.
   sf_v_texCoord = sf_a_texCoord;
}
#else
void main()
{
   gl_TexCoord[0] = gl_MultiTexCoord0;
   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
#endif

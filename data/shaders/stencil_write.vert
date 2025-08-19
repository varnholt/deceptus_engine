#version 330 compatibility

out vec2 interpolated_uv;

void main()
{
    gl_Position   = gl_ModelViewProjectionMatrix * gl_Vertex;
    interpolated_uv = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;
}

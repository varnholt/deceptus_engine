#version 120

uniform float time; // 0..1
varying vec2 vUv;

void main()
{
   vUv = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;

   vec4 pos = gl_Vertex;
   pos.y -= (time * 128.0);

   gl_Position = gl_ModelViewProjectionMatrix * pos;
}

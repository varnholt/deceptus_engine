#version 120

uniform float time; // 0..1

void main() 
{
   vec4 pos = gl_Vertex;
   pos.x += (time * 10.0);
   pos.y += (time * 15.0);

   gl_Position = gl_ModelViewProjectionMatrix * pos;
}


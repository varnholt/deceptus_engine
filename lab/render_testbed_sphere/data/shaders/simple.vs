#version 430

layout (location = 0) in vec3 VertexPosition;

// out vec3 Position;
out vec4 Color;

uniform mat4 MVP;
uniform vec4 u_color;


void main()
{
   Color = u_color;
   gl_Position = MVP * vec4(VertexPosition, 1.0);
}

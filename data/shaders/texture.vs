#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoord;

out vec3 FragPos;
out vec3 Normal_out;
out vec2 TexCoord_out;

uniform mat4 ModelViewMatrix;
uniform mat4 ModelMatrix;
uniform mat4 MVP;
uniform mat3 NormalMatrix;

void main()
{
    gl_Position = MVP * vec4(Position, 1.0);
    FragPos = vec3(ModelMatrix * vec4(Position, 1.0));
    Normal_out = normalize(NormalMatrix * Normal);
    TexCoord_out = TexCoord;
}
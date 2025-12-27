#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoord;
out vec3 ReflectDir;

uniform bool DrawSkyBox;
uniform vec3 WorldCameraPosition;

uniform mat4 ModelViewMatrix;
uniform mat4 ModelMatrix;
uniform mat3 NormalMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 MVP;

void main()
{
   if (DrawSkyBox)
   {
      ReflectDir = VertexPosition;
   }
   else
   {
      // environment mapping
      vec3 worldPos = vec3(ModelMatrix * vec4(VertexPosition,1.0));
      vec3 worldNorm = vec3(ModelMatrix * vec4(VertexNormal, 0.0));
      vec3 worldView = normalize(WorldCameraPosition - worldPos);
      ReflectDir = reflect(-worldView, worldNorm);
   }

   TexCoord = VertexTexCoord;
   Normal = normalize(NormalMatrix * VertexNormal);
   Position = vec3(ModelViewMatrix * vec4(VertexPosition,1.0));

   gl_Position = MVP * vec4(VertexPosition,1.0);
}

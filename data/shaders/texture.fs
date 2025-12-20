#version 330 core

struct Material
{
    vec3 Kd;            // Diffuse reflectivity
    vec3 Ks;            // Specular reflectivity
    vec3 Ka;            // Ambient reflectivity
    float Shininess;    // Specular shininess factor
};

struct Light
{
    vec4 Position;      // Position in eye coordinates
    vec3 Intensity;     // Light intensity (diffuse and ambient)
};

uniform Light Light;
uniform Material Material;
uniform bool useLighting = true;
uniform bool useAO = false;
uniform bool useSpecular = true;
uniform bool DrawSkyBox = false;
uniform vec3 WorldCameraPosition;

in vec3 FragPos;
in vec3 Normal_out;
in vec2 TexCoord_out;

out vec4 FragColor;

vec3 phongModel()
{
    vec3 ambient = Material.Ka * Light.Intensity;

    if (!useLighting) {
        return ambient;
    }

    vec3 s = normalize(vec3(Light.Position) - FragPos);
    vec3 v = normalize(WorldCameraPosition - FragPos);
    vec3 r = reflect(-s, Normal_out);
    
    // Only calculate specular if the surface is facing the light
    vec3 specular = vec3(0.0);
    if (useSpecular && dot(Normal_out, s) > 0.0) {
        specular = Material.Ks * Light.Intensity * pow(max(dot(r, v), 0.0), Material.Shininess);
    }
    
    vec3 diffuse = Material.Kd * Light.Intensity * max(dot(Normal_out, s), 0.0);

    return diffuse + specular + ambient;
}

void main()
{
    if (DrawSkyBox) {
        // For skybox rendering, we can have a special appearance
        FragColor = vec4(phongModel(), 1.0);
    } else {
        FragColor = vec4(phongModel(), 1.0);
    }
}
#version 330 core

struct LightInfo
{
    vec4 Position;      // Position in eye coordinates
    vec3 Intensity;     // Light intensity (diffuse and ambient)
};

struct MaterialInfo
{
    vec3 Kd;            // Diffuse reflectivity
    vec3 Ks;            // Specular reflectivity
    vec3 Ka;            // Ambient reflectivity
    float Shininess;    // Specular shininess factor
};

uniform LightInfo Light;
uniform MaterialInfo Material;
uniform bool useLighting = true;
uniform bool useAO = false;
uniform bool useSpecular = true;
uniform bool DrawSkyBox = false;
uniform vec3 WorldCameraPosition;

// Texture sampling
uniform sampler2D Tex1;
uniform bool useTexture = true;

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
    vec3 color = phongModel();

    if (useTexture) {
        // Sample the texture and modulate with lighting
        vec4 texColor = texture(Tex1, TexCoord_out);
        // Use the texture color for diffuse component
        color = texColor.rgb * color;
    }

    if (DrawSkyBox) {
        // For skybox rendering, we can have a special appearance
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(color, 1.0);
    }
}
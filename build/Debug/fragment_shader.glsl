#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 Color;

uniform sampler2D texture1;
uniform vec3 objectColor;
uniform int useTexture; // Changed to int
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Ambient lighting
    float ambientStrength = 0.5; // Increase ambient strength
    vec3 ambient = ambientStrength * vec3(1.0);

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    // Specular lighting
    float specularStrength = 1.0; // Increase specular strength
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 lighting = (ambient + diffuse + specular);

    if (useTexture == 1) { // Check if useTexture is 1
        vec4 texColor = texture(texture1, TexCoord);
        FragColor = vec4(lighting, 1.0) * texColor;
    } else {
        vec4 texColor = texture(texture1, TexCoord);
        FragColor = vec4(lighting * objectColor, 0.9)* texColor; // 0.6 alpha for transparency
    }
}

#version 430 core

in vec2 texcoord;
out vec4 fragColor;

uniform sampler2D u_diffuse;   // G-buffer color
uniform sampler2D u_normal;    // G-buffer normal  
uniform sampler2D u_position;  // G-buffer position
uniform sampler2D u_shadowMap; // Shadow map

uniform vec3 u_lightPos;
uniform mat4 u_lightMat;
uniform mat4 u_lightProjMat;

void main() {
    // Sample G-buffer
    vec3 albedo = texture(u_diffuse, texcoord).rgb;
    vec3 normal = normalize(texture(u_normal, texcoord).rgb);
    vec3 position = texture(u_position, texcoord).rgb;
    
    // Simple Blinn-Phong lighting
    vec3 lightDir = normalize(u_lightPos - position);
    vec3 viewDir = normalize(-position); // Assuming camera at origin
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo;
    
    // Specular
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * vec3(0.3);
    
    // Ambient
    vec3 ambient = 0.1 * albedo;
    
    // Simple shadow (just use the shadow map value)
    float shadow = 1.0; // For now, no shadows
    
    vec3 color = ambient + shadow * (diffuse + specular);
    
    fragColor = vec4(color, 1.0);
}

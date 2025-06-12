#version 430 core

in vec2 texcoord;
out vec4 fragColor;

uniform sampler2D color_texture;   // previously rendered scene
uniform sampler2D depth_texture;   // depth from geometry pass

uniform float focus_distance = 50.0;  // Focus distance in world units
uniform float focus_range = 20.0;     // Range around focus that's sharp
uniform float near_plane = 0.01;      // Camera near plane
uniform float far_plane = 100.0;      // Camera far plane
uniform bool debug_depth = false;     // Debug mode to visualize depth

// Convert non-linear depth buffer value to linear depth in world units
float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Convert from [0,1] to [-1,1]
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main() {
    float depth = texture(depth_texture, texcoord).r;
    float linearDepth = linearizeDepth(depth);
    
    // Calculate blur amount based on distance from focus
    float blur = clamp(abs(linearDepth - focus_distance) / focus_range, 0.0, 1.0);
    
    // Debug mode: visualize depth and blur
    if (debug_depth) {
        // Show depth as grayscale and blur as red overlay
        float normalizedDepth = clamp(linearDepth / far_plane, 0.0, 1.0);
        fragColor = vec4(normalizedDepth, normalizedDepth, normalizedDepth, 1.0);
        fragColor.r += blur * 0.5; // Add red tint for blur areas
        return;
    }
    
    vec3 color = texture(color_texture, texcoord).rgb;
    
    // If blur is very small, just return original color
    if (blur < 0.01) {
        fragColor = vec4(color, 1.0);
        return;
    }

    ivec2 size = textureSize(color_texture, 0);
    vec3 accum = vec3(0.0);
    float count = 0.0;

    // Enhanced blur kernel - larger for more pronounced DoF
    int kernelSize = int(blur * 5.0) + 1;
    for (int dx = -kernelSize; dx <= kernelSize; ++dx) {
        for (int dy = -kernelSize; dy <= kernelSize; ++dy) {
            vec2 offset = vec2(dx, dy) / vec2(size) * blur * 2.0;
            accum += texture(color_texture, texcoord + offset).rgb;
            count += 1.0;
        }
    }

    vec3 blurred = accum / count;
    fragColor = vec4(blurred, 1.0);
}

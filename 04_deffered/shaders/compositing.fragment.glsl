#version 430 core

in vec2 texcoord;
out vec4 fragColor;

uniform sampler2D color_texture;   // previously rendered scene
uniform sampler2D depth_texture;   // depth from geometry pass

uniform float focus_distance = 0.5;
uniform float focus_range = 0.2;

void main() {
	float depth = texture(depth_texture, texcoord).r;
	float blur = clamp(abs(depth - focus_distance) / focus_range, 0.0, 1.0);

	ivec2 size = textureSize(color_texture, 0);
	vec3 accum = vec3(0.0);
	float count = 0.0;

	// simple 3x3 blur
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			vec2 offset = vec2(dx, dy) / vec2(size) * blur;
			accum += texture(color_texture, texcoord + offset).rgb;
			count += 1.0;
		}
	}

	vec3 blurred = accum / count;
	fragColor = vec4(blurred, 1.0);
}

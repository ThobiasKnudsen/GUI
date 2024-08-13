#version 460 core

layout(location=0) in vec3 in_pos0;
layout(location=1) in vec3 in_pos1;
layout(location=2) in vec3 in_pos2;
layout(location=3) in vec4 in_tex_rect;
layout(location=4) in uint in_color;
layout(location=5) in uint in_tex_index;
layout(location=6) in float in_radius;

out VS_OUT {
	vec3 pos[4];
	vec4 tex_rect;
	vec4 color;
	uint tex_index;
	float radius;
} vs_out;

void main() {
	vs_out.pos[0] = in_pos0;	
	vs_out.pos[1] = in_pos1;	
	vs_out.pos[2] = in_pos2;
	vs_out.pos[3] = in_pos2 + in_pos1 - in_pos0;
	vs_out.tex_rect = in_tex_rect;
	vs_out.color = vec4(
		float((in_color>>24) & 0x000000FF),
		float((in_color>>16) & 0x000000FF),
		float((in_color>>8) & 0x000000FF),
		float(in_color & 0x000000FF)
	);
	vs_out.tex_index = in_tex_index;
	vs_out.radius = in_radius;
}

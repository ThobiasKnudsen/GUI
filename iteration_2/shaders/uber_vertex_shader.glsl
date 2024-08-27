#version 460 core

layout(location=0) in int in_x;
layout(location=1) in int in_y;
layout(location=2) in int in_w;
layout(location=3) in int in_h;
layout(location=4) in vec4 in_tex_rect;
layout(location=5) in uint in_color;
layout(location=6) in float in_rotation_360;
layout(location=7) in uint in_multiple_values;

uniform uint target_width;
uniform uint target_height;

out VS_OUT {
	vec2 pos[4];
	vec4 tex_rect;
	vec4 color;
	uint tex_index;
	float corner_radius_width;
	float corner_radius_height;
} vs_out;

vec2 rotate(vec2 point, vec2 origin, float rotation) {
	return vec2(	(point.x-origin.x)*cos(rotation) - (point.y-origin.y)*sin(rotation)+origin.x,  
			(point.x-origin.x)*sin(rotation) + (point.y-origin.y)*cos(rotation)+origin.y);
}

float norm(float value, float max_value) {
	return 2.0*float(value)/float(max_value)-1.0;
}

void main() {
	float r = in_rotation_360*3.1415926/180.0;
	vec2 origin = vec2(norm(float(in_x)+float(in_w)/2.0, float(target_width)), norm(float(in_y)+float(in_h)/2.0, float(target_height)));
	
	vs_out.pos[0] = vec2(2.0*float(in_x)/float(target_width)-1.0, 		2.0*float(in_y)/float(target_height)-1.0);
	vs_out.pos[0] = rotate(vs_out.pos[0], origin, r);
	vs_out.pos[1] = vec2(2.0*float(in_x+in_w)/float(target_width)-1.0,	2.0*float(in_y)/float(target_height) - 1.0);
	vs_out.pos[1] = rotate(vs_out.pos[1], origin, r);
	vs_out.pos[2] = vec2(2.0*float(in_x)/float(target_width)-1.0, 		2.0*float(in_y+in_h)/float(target_height)-1.0);
	vs_out.pos[2] = rotate(vs_out.pos[2], origin, r);
	vs_out.pos[3] = vec2(2.0*float(in_x+in_w)/float(target_width)-1.0, 	2.0*float(in_y+in_h)/float(target_height)-1.0);
	vs_out.pos[3] = rotate(vs_out.pos[3], origin, r);
	
	vs_out.tex_rect = in_tex_rect;
	vs_out.color = vec4(
		float(uint(in_color>>24) & 0x000000FFu)/256.0,
		float(uint(in_color>>16) & 0x000000FFu)/256.0,
		float(uint(in_color>>8) & 0x000000FFu)/256.0,
		float(uint(in_color>>0) & 0x000000FFu)/256.0
	);
	vs_out.tex_index = (uint(in_multiple_values>>8) & 0x000000FFu);
	float corner_radius_pixels = float(uint(in_multiple_values>>16) & 0x0000FFFFu);
	vs_out.corner_radius_width = corner_radius_pixels/float(in_w);
	vs_out.corner_radius_height = corner_radius_pixels/float(in_h);
}

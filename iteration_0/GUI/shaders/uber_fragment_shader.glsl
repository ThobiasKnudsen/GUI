#version 460 core

in vec2 tex_coords;
in vec4 color;
flat in uint tex_index;
in float corner_radius_width;
in float corner_radius_height;

out vec4 out_frag_color;

uniform sampler2D textures[16];

void main() {
	float x = tex_coords.x; x = x>0.5 ? 1.0-x : x;
	float y = tex_coords.y; y = y>0.5 ? 1.0-y : y;
	float edge = 0.003;
	if (corner_radius_width < 0.0001 || corner_radius_height < 0.0001 || x >= corner_radius_width+edge || y >= corner_radius_height+edge) {
		out_frag_color = color;
		return;
	}
	x=x-corner_radius_width;
	y=y-corner_radius_height;
	if ((pow(x/corner_radius_width,2)) + (pow(y/corner_radius_height,2)) < 1.0) {
		out_frag_color = color;
		return;
	}
	out_frag_color = vec4(color.rgb, 0.0);
}

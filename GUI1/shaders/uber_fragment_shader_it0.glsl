#version 460 core

in vec2 tex_coords;
in vec4 color;
flat in uint tex_index;
in float radius;

out vec4 out_frag_color;

uniform sampler2D textures[16];

void main() {
	float x = tex_coords.x; x = x>0.5 ? 1.0-x : x;
	float y = tex_coords.y; y = y>0.5 ? 1.0-y : y;
	float edge_width = 0.003;
	if (radius < 0.0001 || !(x < radius+edge_width && y < radius+edge_width)) {
		out_frag_color = color;
		return;
	}
	float a = color.a;
	a = (radius+edge_width-(length(vec2(x-radius, y-radius))))/(2*edge_width);
	//a = a>1.0 ? 1.0 : (a<0.0 ? 0.0 : a);
	out_frag_color = vec4(color.rgb, a);
}

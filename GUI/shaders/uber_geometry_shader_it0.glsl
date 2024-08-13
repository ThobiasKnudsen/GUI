#version 460 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in VS_OUT {
	vec3 pos[4];
	vec4 tex_rect;
	vec4 color;
	uint tex_index;
	float radius;
} gs_in[];

out vec2 tex_coords;
out vec4 color;
out uint tex_index;
out float radius;

void main() {

	vec2 tex_coords_tmp[4];
	tex_coords_tmp[0] = vec2(gs_in[0].tex_rect.x,    		     gs_in[0].tex_rect.y);
	tex_coords_tmp[1] = vec2(gs_in[0].tex_rect.x + gs_in[0].tex_rect.w,  gs_in[0].tex_rect.y);
	tex_coords_tmp[2] = vec2(gs_in[0].tex_rect.x,   		     gs_in[0].tex_rect.y + gs_in[0].tex_rect.z);
	tex_coords_tmp[3] = vec2(gs_in[0].tex_rect.x + gs_in[0].tex_rect.w,  gs_in[0].tex_rect.y + gs_in[0].tex_rect.z);

	for (uint i = 0; i<4; i++) {
		gl_Position = vec4(gs_in[0].pos[i],1.0f);
		tex_coords = tex_coords_tmp[i];
		color = gs_in[0].color;
		tex_index = gs_in[0].tex_index;
		radius = gs_in[0].radius;
		EmitVertex();
	}
	EndPrimitive();
}

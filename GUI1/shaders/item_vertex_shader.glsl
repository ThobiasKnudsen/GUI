#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in uint aColor;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in int aTexIndex;

out vec4 Color;
out vec2 TexCoords;
flat out int TexIndex;

void main() {
	gl_Position = vec4(aPos,1.0);
	Color = vec4(
		float(aColor/(256*256*256)) / 255.0,
		float(aColor/(256*256)%256) / 255.0,
		float(aColor/(256)%256) / 255.0,
		float(aColor%256) / 255.0
	);
	TexCoords = aTexCoords;
	TexIndex = aTexIndex;
}

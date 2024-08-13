#version 460 core

in vec4 Color;
in vec2 TexCoords;
flat in int TexIndex;

out vec4 FragColor;

uniform sampler2D textures[16];

void main() {
	if (TexIndex < 16 && TexIndex >= 0) {
		vec4 texColor = texture(textures[TexIndex], TexCoords);
		FragColor = texColor * Color;	
	}
	else {
		FragColor = Color;
	}
}

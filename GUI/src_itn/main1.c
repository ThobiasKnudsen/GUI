 #include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "functions.h"
#define DEBUG
#include "debug.h"
#include "gui.h"

// TODO:
// VBO+VAO+EBO draws all of text quads and rendertextures onto given target

// make one Vertex that can draw text, quad, candles, map, and graphs
// actually just make a vertex be a rectangle and not quad, which can then draw text, boxes and textures. baiscally everything thats ok with being a rectangle. when drawing these to a target there should be the same transformation to apply to each rectangle. 

// ubershader:
// round off corners. hover color.
// DONT DO THIS, TRUST ME/YOU: converts {vec3 pos, vec2 size, vec2 next_vec_dir, vec2 norm_vec, } to {vec3 pos[4]}


int main() {
	GLFWwindow* window = GUI_CreateWindow(800, 600, "OpenGL test");

	GLuint shaders[2];
    	debug(shaders[0] = GUI_CompileShader(GUI_ReadShaderSource("shaders/item_vertex_shader.glsl"), GL_VERTEX_SHADER););
    	debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/item_fragment_shader.glsl"), GL_FRAGMENT_SHADER););
	GLuint shader_program = GUI_CreateProgram(shaders, 2);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);
	
	Vertex vertices[] = {
		{{-0.5f, -0.5f,  0.f}, {0xFF0000FF}, {0.f,0.f}, GUI_NO_TEX_INDEX},
		{{ 0.5f, -0.5f,  0.f}, {0x00FF00FF}, {0.f,0.f}, GUI_NO_TEX_INDEX},
		{{-0.5f,  0.5f,  0.f}, {0x0000FFFF}, {0.f,0.f}, GUI_NO_TEX_INDEX},
		{{ 0.5f,  0.5f,  0.f}, {0xFF00FFFF}, {0.f,0.f}, GUI_NO_TEX_INDEX},
	};
	
	unsigned int indices[] = {
		0, 1, 2,
		1, 2, 3
	};
	
	// Generate and bind a Vertex Array Object
	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
	
	// color attribute
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT,    sizeof(Vertex), (void*)offsetof(Vertex, color));
	// position attribute
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));
	// position attribute
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT,    sizeof(Vertex), (void*)offsetof(Vertex, tex_index));
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Render loop
	while (!glfwWindowShouldClose(window)) {
	
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}
	
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// draw rectangle
		glUseProgram(shader_program);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shader_program);

	// Terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

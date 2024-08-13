 #include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "functions.h"
#define DEBUG
#include "debug.h"
#include "gui.h"


typedef struct {
	float pos[9]; // easily make the 4th point. parallellogram
	float tex_rect[4];
	unsigned int color;
	unsigned int tex_index;
	float radius;
} UberVertex;

UberVertex vertices[] = {
	{{-0.5f,-0.5f,0.0f,0.0f,-0.5f,0.0f,-0.5f,0.0f,0.0f}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFFFFFF, GUI_NO_TEX_INDEX, 0.1f},
	{{-0.0f,-0.0f,0.0f,0.5f,-0.0f,0.0f,-0.0f,0.5f,0.0f}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFFFFFF, GUI_NO_TEX_INDEX, 0.1f}
};

unsigned int indices[] = {
	0, 1
};

int main() {

	GLFWwindow* window = GUI_CreateWindow(800, 600, "OpenGL test");

	GLuint shaders[2];
    	debug(shaders[0] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_vertex_shader_test.glsl"), GL_VERTEX_SHADER););
    	//debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_geometry_shader.glsl"), GL_GEOMETRY_SHADER););
    	debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_fragment_shader_test.glsl"), GL_FRAGMENT_SHADER););
	GLuint shader_program = GUI_CreateProgram(shaders, 2);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);
//	glDeleteShader(shaders[2]);
	
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

	// pos attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 9, GL_FLOAT, GL_FALSE, sizeof(UberVertex), (void*)offsetof(UberVertex, pos));
	// tex_rect attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UberVertex), (void*)offsetof(UberVertex, tex_rect));
	// position attribute
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT,   sizeof(UberVertex), (void*)offsetof(UberVertex, color));
	// position attribute
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT,   sizeof(UberVertex), (void*)offsetof(UberVertex, tex_index));
	// radius attribute
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(UberVertex), (void*)offsetof(UberVertex, radius));
	
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
		glDrawElements(GL_POINT, 2, GL_UNSIGNED_INT, 0);
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

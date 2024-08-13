#include "GUI.h"
#include "private/GUI_GLFW_Simplified.h"
#include "private/GUI_Backbone.h"

#include <stdbool.h>

void GUI_Create(unsigned int width, unsigned int height, const char* title, unsigned int FPS) {
	private_window= GUI_CreateWindow(width, height, title);
	
	GUI_Item_ID window_item = private_GUI_Item_Create(WINDOW, 0); 
	GUI_Item_ID window_draw_order_item = private_GUI_Item_Create(DRAWORDER, 0);

	GLuint shaders[3];
	printf("vertex: \n"); debug(shaders[0] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_vertex_shader.glsl"), GL_VERTEX_SHADER););
    	printf("geometry: \n"); debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_geometry_shader.glsl"), GL_GEOMETRY_SHADER););
    	printf("fragment: \n"); debug(shaders[2] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_fragment_shader.glsl"), GL_FRAGMENT_SHADER););
	private_shader_program = GUI_CreateProgram(shaders, 3);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);
 	glDeleteShader(shaders[2]);
	
	PrivateVertex vertices[] = {
		{{120, 350, 170, 180}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFFFFFF, 30.0f, 0x00060000},
		{{100, 270, 100, 100}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFF00FF, 0.0f, 0x00320000},
		{{100, 300, 300, 100}, {0.0f,0.0f,1.0f,1.0f}, 0x0000FF80, 0.0f, 0x00060000},
		{{250, 130, 300, 200}, {0.0f,0.0f,1.0f,1.0f}, 0xFF000080, 70.0f, 0x00060000}
	};

	unsigned int indices[] = {2, 3, 0, 1};
	
	// Generate and bind a Vertex Array Object
	GLuint EBO;
	glGenVertexArrays(1, &private_VAO);
	glGenBuffers(1, &private_VBO);
	glGenBuffers(1, &EBO);
	
	glBindVertexArray(private_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, private_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// pos attribute
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 1, GL_INT, sizeof(PrivateVertex), (void*)(0*sizeof(int)));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_INT, sizeof(PrivateVertex), (void*)(1*sizeof(int)));
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_INT, sizeof(PrivateVertex), (void*)(2*sizeof(int)));
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_INT, sizeof(PrivateVertex), (void*)(3*sizeof(int)));
	// tex_rect attribute
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(PrivateVertex), (void*)(4*sizeof(float)));
	// color attribute
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(PrivateVertex), (void*)(8*sizeof(float)));
	// rotation_360 attribute
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(PrivateVertex), (void*)(9*sizeof(float)));
	// corner_radius and tex_index attribute
	glEnableVertexAttribArray(7);
	glVertexAttribIPointer(7, 1, GL_UNSIGNED_INT,  sizeof(PrivateVertex), (void*)(10*sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	
	glDeleteBuffers(1, &EBO);
}
void GUI_Show() {
	GLuint target_width_loc = glGetUniformLocation(private_shader_program, "target_width");
	GLuint target_height_loc = glGetUniformLocation(private_shader_program, "target_height");

	while (!glfwWindowShouldClose(private_window)) {
	
		if (glfwGetKey(private_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(private_window, true);
		}
		GUI_PrintFPS(private_window);
	
		// Clear the screen
		//glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw rectangles
		glUseProgram(private_shader_program);
		int width, height; 
		glfwGetWindowSize(private_window, &width, &height);
		glUniform1ui(target_width_loc, width);
		glUniform1ui(target_height_loc, height);
		glBindVertexArray(private_VAO);
		glDrawElements(GL_POINTS, 4, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Swap buffers and poll events
		glfwSwapBuffers(private_window);
		glfwPollEvents();	
	}
}
void GUI_Delete() {
	// Clean up
	glDeleteVertexArrays(1, &private_VAO);
	glDeleteBuffers(1, &private_VBO);
	glDeleteProgram(private_shader_program);

	// Terminate GLFW
	glfwDestroyWindow(private_window);
	glfwTerminate();
}

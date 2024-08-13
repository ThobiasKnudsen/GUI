
#include <stdbool.h>

#include "private/GUI_Backbone.h"
#include "private/GUI_Datastructures.h"
#include "private/GUI_GLFW_Simplified.h"
#define DEBUG
#include "private/debug.h"




// PRIVATE 
// ===============================================================================================================================================


typedef struct {
	float pos[9]; // easily make the 4th point. parallellogram. 8 bytes and you get 3D rectangle but there are many more calculations to do then and anyways this struct is 64 bytes now.
	float tex_rect[4];
	unsigned int color;
	unsigned int tex_index;
	float radius;
} PrivateVertex; // 64 bytes

typedef struct {
	// idea here is that every letter has the same width	
	// automatically calculated based on longest_line, line_count, tab_size, text_pixel_width and height
	unsigned int 	width; 
	unsigned int 	height;
	
	char* 		text;
	size_t		text_length;
	size_t		text_capacity;
	
	unsigned short	symbol_pixel_width;	
	unsigned short 	symbol_pixel_height;
	
	unsigned char* 	color_id_per_symbol; // size is text_length and capacity is text_capacity
	unsigned char*  colors; // index 0 1 2 3 is r g b a relatively, so max size is of this array is 4*256=1024
	unsigned char 	colors_count; 
	
	unsigned char*  visibility_per_symbol; // size is text_length and capacity is text_capacity
	
	unsigned char 	tab_size;
	unsigned int	current_writing_index;
	unsigned int 	sellecting_text_start;
	unsigned int 	longest_line;
	unsigned int	line_count;
	
} PrivateText;

// CPU ===========================================================================================================================================

static GLFWwindow* 	private_window = NULL;
static GUI_Item* 	private_root_item_ptr = NULL;
static size_t		private_root_item_array_size = 0;

static PrivateVertex*	private_vertices = NULL;
static size_t 		private_vertices_size = 0;
static size_t 		private_vertices_capacity = 0;

static PrivateText* 	private_text_array = NULL;
static size_t		private_text_array_size = 0;
static size_t		private_text_array_capacity = 0;

// GPU ==========================================================================================================================================

static GLuint 		private_shader_program;
static GLuint		private_VBO;
static GLuint		private_VAO;

// FUNCTIONS ====================================================================================================================================

void private_GUI_IncreaseItemArraySize() {
	size_t prev_size = 0;
	size_t new_size = 0;
	if (private_root_item_ptr == NULL) {
		prev_size = 0;
		new_size = 64;
	}
	else {
		prev_size = private_root_item_array_size;
		new_size = 2 * prev_size;
	}
	private_root_item_ptr = alloc(private_root_item_ptr, new_size);
	for (unsigned int i = prev_size; i < new_size; i++) {
		private_root_item_ptr[i].id = i;
		private_root_item_ptr[i].type = NONE;
	}
	private_root_item_array_size = new_size;
}



void GUI_Item_Print(GUI_Item* item_ptr) {
    printf("id = %d\n", item_ptr->id);
    printf("type = %d\n", item_ptr->type);
    printf("parent = %d\n", item_ptr->parent);
}
void GUI_PrintItems(GUI_Item* root_item_ptr) {
    for (unsigned int i = 0; i < private_root_item_array_size; i++) {
        debug(GUI_Item_Print(&private_root_item_ptr[i]););
    }
}
	
// PUBLIC
// ===============================================================================================================================================

void GUI_Create(unsigned int width, unsigned int height, const char* title, unsigned int FPS) {
	private_window= GUI_CreateWindow(width, height, title);

	GLuint shaders[3];
    	debug(shaders[0] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_vertex_shader.glsl"), GL_VERTEX_SHADER););
    	debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_geometry_shader.glsl"), GL_GEOMETRY_SHADER););
    	debug(shaders[2] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_fragment_shader.glsl"), GL_FRAGMENT_SHADER););
	private_shader_program = GUI_CreateProgram(shaders, 3);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);
 	glDeleteShader(shaders[2]);
	
	
	PrivateVertex vertices[] = {
		{{ -0.8f, -0.8f,0.0f, -0.8f, 0.0f,0.0f, 0.8f,-0.8f,0.0f}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFF00FF, 16, 0.5f},
		{{-0.0f,-0.0f,0.0f,0.5f,-0.0f,0.0f,-0.0f,0.5f,0.0f},      {0.0f,0.0f,1.0f,1.0f}, 0xFF00FFFF, 16, 0.04f}
	};

	unsigned int indices[] = {0, 1};
	
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PrivateVertex), (void*)(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PrivateVertex), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PrivateVertex), (void*)(6*sizeof(float)));
	// tex_rect attribute
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(PrivateVertex), (void*)(9*sizeof(float)));
	// color attribute
	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(PrivateVertex), (void*)(13*sizeof(float)));
	// tex_index attribute
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(PrivateVertex), (void*)(14*sizeof(float)));
	// radius attribute
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE,  sizeof(PrivateVertex), (void*)(15*sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	glDeleteBuffers(1, &EBO);
}
void GUI_Show() {

	while (!glfwWindowShouldClose(private_window)) {
	
		if (glfwGetKey(private_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(private_window, true);
		}
		GUI_PrintFPS(private_window);
	
		// Clear the screen
		//glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw rectangle
		glUseProgram(private_shader_program);
		glBindVertexArray(private_VAO);
		glDrawElements(GL_POINTS, 2, GL_UNSIGNED_INT, 0);
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

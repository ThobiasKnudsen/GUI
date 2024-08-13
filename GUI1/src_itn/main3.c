#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//#define DEBUG
#include "../include/private/debug.h"

void* alloc(void* ptr, size_t size) {
    void* tmp = ptr==NULL ?
				malloc(size) :
                realloc(ptr, size) ;
    if (!tmp) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    return tmp;
}


GLFWwindow* GUI_CreateWindow(unsigned int width, unsigned int height, const char* title) {
	// Initialize GLFW
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		exit(-1);
	}
	

	// Set GLFW window hints
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	
	// Create a GLFW window
	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!window) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		exit(-1);
	}

	// Make the OpenGL context current
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		exit(-1);
	}
	
	//glfwSwapInterval(0);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return window;
}
    
GLuint GUI_CompileShader(const char* shader_source, GLenum shader_type) {
	// Compile the shader
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, &shader_source, NULL);
	glCompileShader(shader);
	// Check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		printf("ERROR: shader %d compilation failed: %s\n", shader_type, infoLog);
	}
	return shader;
}

GLuint GUI_CreateProgram(GLuint* shaders, size_t number_of_shaders) {
	// Link shaders to create a shader program
	GLuint shader_program = glCreateProgram();
	for (size_t i = 0; i < number_of_shaders; i++) {
		glAttachShader(shader_program, shaders[i]);
	}
	glLinkProgram(shader_program);
	// Check for linking errors
	int success;
	char infoLog[512];
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
		printf("ERROR: shader program linking failed: %s\n", infoLog);
	}
	return shader_program;
}

int GUI_NDCtoPixel(float NDC, unsigned int max_pixels) {
	return (int)((float)max_pixels*((NDC+1.f)/2.f));
}
float GUI_PixeltoNDC(float pixel, unsigned int max_pixels) {
	return (2.f*(float)pixel/(float)max_pixels)-1.f;
}

char* GUI_ReadShaderSource(const char* shader_file) {
	FILE* file = fopen(shader_file, "r");
	if (!file) {
		printf("ERROR: could not open shader file %s\n", shader_file);
		exit(-1);
	}
	
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file,0,SEEK_SET);
	
	char* buffer = (char*)alloc(NULL, length+1);
	if(!buffer){
		printf("ERROR: could not allocatre memory for reading file %s\n", shader_file);
		fclose(file);
		exit(-1);
	}
	
	fread(buffer,1,length,file);
	buffer[length]='\0';
	fclose(file);
	
	if (!buffer) {
		printf("ERROR: buffer is NULL\n");
		exit(-1);
	}
	
	return buffer;
}

void GUI_PrintFPS(GLFWwindow* window) {
	static double previous_seconds = 0.0f;
	static int frame_count = 0;
	double current_seconds = glfwGetTime();
	double elapsed_seconds = current_seconds - previous_seconds;
	
	if (elapsed_seconds > 1.0) {
		previous_seconds = current_seconds;
		double fps = (double)frame_count / elapsed_seconds;
		char tmp[128];
		snprintf(tmp, sizeof(tmp), "FPS: %.2f", fps);
		printf("%s\n", tmp);
		frame_count = 0;
	}
	frame_count++;
}

typedef struct {
	float pos[9]; // easily make the 4th point. parallellogram
	float tex_rect[4];
	unsigned int color;
	unsigned int tex_index;
	float radius;
} UberVertex;

UberVertex vertices[] = {
	{{ -0.8f, -0.8f,0.0f, -0.8f, 0.0f,0.0f, 0.8f,-0.8f,0.0f}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFF00FF, 16, 0.5f},
	{{-0.0f,-0.0f,0.0f,0.5f,-0.0f,0.0f,-0.0f,0.5f,0.0f},      {0.0f,0.0f,1.0f,1.0f}, 0xFF00FFFF, 16, 0.04f}
};

unsigned int indices[] = {0, 1};

int main() {


	GLFWwindow* window = GUI_CreateWindow(600, 600, "OpenGL test");

	GLuint shaders[3];
    	debug(shaders[0] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_vertex_shader.glsl"), GL_VERTEX_SHADER););
    	debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_geometry_shader.glsl"), GL_GEOMETRY_SHADER););
    	debug(shaders[2] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_fragment_shader.glsl"), GL_FRAGMENT_SHADER););
	GLuint shader_program = GUI_CreateProgram(shaders, 3);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);
 	glDeleteShader(shaders[2]);
	
	
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(UberVertex), (void*)(0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(UberVertex), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(UberVertex), (void*)(6*sizeof(float)));
	// tex_rect attribute
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(UberVertex), (void*)(9*sizeof(float)));
	// color attribute
	glEnableVertexAttribArray(4);
	glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT, sizeof(UberVertex), (void*)(13*sizeof(float)));
	// tex_index attribute
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(UberVertex), (void*)(14*sizeof(float)));
	// radius attribute
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE,  sizeof(UberVertex), (void*)(15*sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	// Render loop
	while (!glfwWindowShouldClose(window)) {
	
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}
		GUI_PrintFPS(window);
	
		// Clear the screen
		//glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw rectangle
		glUseProgram(shader_program);
		glBindVertexArray(VAO);
		glDrawElements(GL_POINTS, 2, GL_UNSIGNED_INT, 0);
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



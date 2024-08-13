#ifndef GUI_GLFW_SIMPLIFIED_H
#define GUI_GLFW_SIMPLIFIED_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

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
}GLuint GUI_CompileShader(const char* shader_source, GLenum shader_type) {
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

#endif //  GUI_GLFW_SIMPLIFIED_H

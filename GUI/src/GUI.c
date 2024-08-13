#include "GUI.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>


#define DEBUG
#include "debug.h"


void* alloc(void* ptr, size_t length, size_t type_size) {
	size_t size = length*type_size;
    void* tmp = ptr==NULL ?
		malloc(size) :
                realloc(ptr, size) ;
    if (!tmp) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    return tmp;
}


// PRIVATE DATA ========================================================================================================================================

typedef struct {
	int                rect[4];
	float              tex_rect[4];
	unsigned int       color;
	float              rotation_360;
	unsigned int       multiple_values; //corner_radius and tex_index
	bool               in_use;
	bool							 VBO_upToDate;
	unsigned char      padding[2];
} Vertex; // 48 bytes (3 bytes padding)
typedef struct {
	// idea here is that every letter has the same width
	char*              symbol_array;
	unsigned char*     symbol_color_id_array;
	bool*              symbol_visibility_array;
	size_t             symbol_array_length;
	size_t             symbol_array_capacity;

	int*			   symbol_rect_array; // 4*symbol_array_length;

	unsigned int       text_pixel_width; // automatically calculated based on longest_line, line_count, tab_size, symbol_pixel_width -and height
	unsigned int       text_pixel_height;

	unsigned short     symbol_pixel_width;
	unsigned short     symbol_pixel_height;

	unsigned char*     colors_array; // index 0 1 2 3 is r g b a relatively, so max size is of this array is 4*256=1024
	unsigned char      colors_count;

	bool               in_use; // DOESNT FIT IN HERE EXEPT THAT IT FILLS AN EMPTY BYTE

	unsigned char      tab_size;
	unsigned int       current_writing_index;
	unsigned int       sellecting_text_start;
	unsigned int       longest_line;
	unsigned int       line_count;

	unsigned int       symbol_spacing;
	unsigned int       line_spacing;

	// you have to convert number_of_available_vertices to available_vertices_size or available_vertices_capacity ???????
	unsigned int       number_of_available_vertices; // THIS ONE IS NEW AND IMPORTANT
	unsigned int       available_vertices_size; // THIS ONE IS NEW AND IMPORTANT
	unsigned int       available_vertices_capacity; // THIS ONE IS NEW AND IMPORTANT

} Text;

static GLFWwindow* 	window = NULL;
static GUI_Item* 	item_array = NULL;
static size_t		item_array_size = 0;
static size_t		item_array_capacity = 0;
static size_t 		ID_count = 0;
#define ITEM_ARRAY_CAPACITY_START 8

static Vertex*		vertices = NULL;
static size_t 		vertices_size = 0;
static size_t 		vertices_capacity = 0;
static size_t 		vertices_prev_size = 0;
static size_t 		vertices_prev_capacity = 0;
static bool 		any_vertex_update = false;
#define VERTICES_CAPACITY_START 8

static Text* 		text_array = NULL;
static size_t		text_array_size = 0;
static size_t		text_array_capacity = 0;
#define TEXT_ARRAY_CAPACITY_START 4

// the items in this array can be either WINDOW, RENDERTEXTURE or DRAWORDER. it is like a tree. the first index, aka WINDOW, is the root then it branckes upwards as there are multiple draw_orders and targets for each target. Its important that a branch dont loops back into itself.
static GUI_Item_ID* draw_tree_array = NULL;
static size_t		draw_tree_array_size = 0;
static size_t		draw_tree_array_capacity = 0;
static bool			draw_tree_array_change = true;
#define DRAW_TREE_ARRAY_CAPACITY_START 16

// GPU ================================================================================================================================================
static GLuint 		shader_program;
static GLuint			VBO;
static GLuint			VAO;

static GLuint 		target_x_loc;
static GLuint 		target_y_loc;
static GLuint 		target_width_loc;
static GLuint 		target_height_loc;

// PUBLIC DATA ========================================================================================================================================

struct GUI_Item_Window {
	unsigned int 	width;
	unsigned int 	height;
	GUI_Item_ID 	draw_order_id; // as long as the new draworder is the same as previous and all items in this draw order is unchanged then this is not redrawn
};
struct GUI_Item_Normal {
	int 					rect[4];
	unsigned char color[4];
	float 				rotation_360;
	unsigned int 	corner_radius_pixels; // unsigned short in shader and vertex
};
struct GUI_Item_DrawOrder {

	// has to be unsigned int and not size_t so that this struct is less than 32 bytes
	GUI_Item_ID* 	item_id_array;
	unsigned short 	item_id_array_size;
	unsigned short 	item_id_array_capacity;

	GLuint			EBO;
	unsigned int*	indices;
	unsigned int	indices_size;
	unsigned int	indices_capacity;

	bool 			EBO_needsResize;
	bool			indices_needsUpdate;

}; // 28 bytes
struct GUI_Item_Text {
	unsigned int 	text_location; // this item needs its own struct so that it doesnt make GUI_Item size too large
};
struct GUI_Item_RenderTexture {
	int 			external_rect[4];
	unsigned short 	internal_width; // these has to be Uint16 so that the whole struct becomes 32 bytes
	unsigned short	internal_height;
	unsigned int 	draw_order_id; // as long as the new draworder is the same as previous and all items in this draw order is unchanged then this is not redrawn
	GLuint 			texture;
	GLuint 			FBO; // framebuffer
};
struct GUI_Item_TextureAtlas {
	// texture atlas
	// tex_rect for each texture inside the atlas so its easy to get the tex coords
};
struct GUI_Item_Custom {
	// the idea with this is that this function is executed every iteration as long as its inside the draw chain
	// even when this is inside the draw chain it is never visible but just changes other items which is defined
	// within custom_data_ptr. remember CustomFunction and custom_data_ptr can be what ever!

	// example: when ever normal_item1_id is drawn custom_item1_ptr is also "drawn". since custom_item1_ptr is drawn
	// custom_item1_ptr->Custom.CustomFunction is executed. within the custom_data is normal_item1_id stored and
	// CustomFunction changes the color of normal_item1_id to red if GUI_IsMouseHovering(normal_item1_id) otherwise
	// the color is set to blue

	// you could change CustomFunction to store last time mouse was hovering normal_item1_id inside custom_data_ptr
	// then gradually change the color from red to blue based on last time hovering.
	void* custom_data_ptr;
	void (*CustomFunction)(void* custom_data_ptr);
};
union GUI_Item_Union {
    GUI_Item_Window    		window;
    GUI_Item_Normal			normal;
    GUI_Item_DrawOrder		draw_order;
    GUI_Item_Text      		text;
    GUI_Item_RenderTexture  render_texture;
    GUI_Item_Custom 		custom;
};
struct GUI_Item {
	GUI_Item_ID 	id;
	GUI_Item_Type 	type;
	unsigned int 	vertex_location;
	bool 			changed;
	bool 			in_use;
	bool			vertex_updated;
	bool			empty;
	GUI_Item_Union 	item;
};



// PRIVATE FUNCTIONS ========================================================================================================================================

// WHATEVER!


GUI_Item* GUI_GetItemPtr(GUI_Item_ID item_id) {
	for (unsigned int i = 0; i < item_array_size; i++) {
		if (item_array[i].id == item_id && item_array[i].in_use) {
			return &item_array[i];
		}
	}
	printf("there is no item with id=%d that is in use.\n", item_id);
	exit(-1);
}

// PRINTING
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
void GUI_Item_Print(GUI_Item* item_ptr) {
    printf("id = %d | type = %d | vertex_location = %d | in_use = %d\n", item_ptr->id, item_ptr->type, item_ptr->vertex_location, item_ptr->in_use);
}
void GUI_PrintAllInfo() {

	printf("Items(%zu):\n", item_array_capacity);
	for (unsigned int i = 0; i < item_array_capacity; i++) {
		bool in_use = item_array[i].in_use;

		if (item_array[i].type == WINDOW) {
			printf("id = %d | type = WINDOW | draw_order_id = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].item.window.draw_order_id, in_use*item_array[i].changed, in_use);
		}
		else if (item_array[i].type == NORMAL) {
			printf("id = %d | type = NORMAL | vertex_location = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].vertex_location, in_use*item_array[i].changed, in_use);
		}
		else if (item_array[i].type == RENDERTEXTURE) {

		}
		else if (item_array[i].type == DRAWORDER) {
			printf("id = %d | type = DRAWORDER | draw_order_items_size = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].item.draw_order.item_id_array_size, in_use*item_array[i].changed, in_use);
			for (unsigned int j = 0; j < item_array[i].item.draw_order.item_id_array_size; j++) {
				GUI_Item* item_ptr = GUI_GetItemPtr(item_array[i].item.draw_order.item_id_array[j]);
				printf("	id = %d | type = %d | changed = %d | in_use = %d\n", in_use*item_ptr->id, in_use*item_ptr->type, in_use*item_ptr->changed, in_use);
			}
		}
		else if (item_array[i].type == NONE) {
			printf("id = %d | type = NONE | vertex_location = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].changed, in_use);
		}
		else if (item_array[i].type == CUSTOM) {

		}
		else {
			printf("THERE IS AN FLAW IN THE DESIGNE OF GUI. AN ITEM HAS AN UNKNOWN TYPE\n");
		}
	}
	printf("Vertices(%zu): \n", vertices_capacity);
	for (unsigned int i = 0; i < vertices_capacity; i++) {
		bool in_use = vertices[i].in_use;
		printf("rect = (%d,%d,%d,%d) | color = (%d,%d,%d,%d) | in_use = %d\n", in_use*vertices[i].rect[0], in_use*vertices[i].rect[1], in_use*vertices[i].rect[2], in_use*vertices[i].rect[3],
			in_use*((unsigned char*)(&vertices[i].color))[0], in_use*((unsigned char*)(&vertices[i].color))[1], in_use*((unsigned char*)(&vertices[i].color))[2], in_use*((unsigned char*)(&vertices[i].color))[3],
			vertices[i].in_use);
	}
	printf("Text(%zu): \n", text_array_capacity);
	for (unsigned int i = 0; i < text_array_capacity; i++) {
		bool in_use = text_array[i].in_use;
 		printf("text_length = %zu | text_capacity = %zu | in_use = %d \n", in_use*text_array[i].symbol_array_length, in_use*text_array[i].symbol_array_capacity, text_array[i].in_use);
	}
}
void GUI_PrintMaxTextureSize() {
	GLint maxTextureSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	printf("Maximum supported texture size: %d\n", maxTextureSize);
}

// GLFW simplified
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
	debug(GLuint shader = glCreateShader(shader_type););
	debug(glShaderSource(shader, 1, &shader_source, NULL););
	debug(glCompileShader(shader););
	// Check for shader compile errors
	int success;
	char infoLog[512];
	debug(glGetShaderiv(shader, GL_COMPILE_STATUS, &success););
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
	debug(FILE* file = fopen(shader_file, "rb"););
	if (!file) {
		printf("ERROR: could not open shader file %s\n", shader_file);
		exit(-1);
	}

	fseek(file,0,SEEK_END);
	long length = ftell(file);
	fseek(file,0,SEEK_SET);

	debug(char* buffer = (char*)alloc(NULL, length+1, 1););
	if(!buffer){
		printf("ERROR: could not allocatre memory for reading file %s\n", shader_file);
		fclose(file);
		exit(-1);
	}

	debug(fread(buffer,1,length,file););
	debug(buffer[length]='\0';);
	debug(fclose(file););

	if (!buffer) {
		printf("ERROR: buffer is NULL\n");
		exit(-1);
	}

	return buffer;
}

// GUI simplified
GUI_Item_ID GUI_FindItemLocation() {

	if (item_array == NULL || item_array_size >= item_array_capacity) {
		vertices_prev_size = vertices_size;
		vertices_prev_capacity = vertices_capacity;

		size_t prev_capacity = item_array_size;
		size_t new_capacity = 2*prev_capacity;
		if (item_array == NULL) {
			prev_capacity = 0;
			new_capacity = ITEM_ARRAY_CAPACITY_START;
		}
		item_array = alloc(item_array, new_capacity, sizeof(GUI_Item));
		item_array_capacity = new_capacity;
		for (unsigned int i = prev_capacity; i < new_capacity; i++) {
			item_array[i].type = NONE;
			item_array[i].vertex_location = 0;
			item_array[i].in_use = false;
		}
		item_array[prev_capacity].id = ID_count;
		item_array[prev_capacity].type = NONE;
		item_array[prev_capacity].changed= true;
		item_array[prev_capacity].in_use = true;
		ID_count++;
		item_array_size++;
		return item_array[prev_capacity].id;
	}

	for (unsigned int i = 0; i < item_array_size; i++) {
		if (!item_array[i].in_use) {
			item_array[i].id = ID_count;
			item_array[i].type = NONE;
			item_array[i].changed = true;
			item_array[i].in_use = true;
			ID_count++;
			return item_array[i].id;
		}
	}

	item_array[item_array_size].id = ID_count;
	item_array[item_array_size].type = NONE;
	item_array[item_array_size].changed= true;
	item_array[item_array_size].in_use = true;
	ID_count++;
	item_array_size++;
	return ID_count-1;
}
unsigned int GUI_FindVertexLocation(GUI_Item_ID item_id) {
	debug(GUI_Item* item_ptr = GUI_GetItemPtr(item_id););
	if (item_ptr->type == TEXT) {
		printf("You should not use this function for finding text allocation. Use GUI_FindVertexAndTextLocation instead\n");
		GUI_Item_Print(item_ptr);
		exit(-1);
	}
	if (item_ptr->type == DRAWORDER) {
		printf("Draw_order_item dont need vertex location, because it is never drawn itself!\n");
		GUI_Item_Print(item_ptr);
		exit(-1);
	}
	if (item_ptr->type == WINDOW) {
		// ?????????????????????????????????????????????????????????????????????????????????
		// CAN ONLY BE ONE WINDOW
		if (vertices != NULL) {
			if (vertices[0].in_use) {
				printf("You cant make a second window. there is only one window!\n");
				GUI_Item_Print(item_ptr);
				exit(-1);
			}
		}
	}
	if (item_ptr->vertex_location != 0) {
		printf("You have allready found vertex location for this item\n");
		GUI_Item_Print(item_ptr);
		exit(-1);
	}

	if (vertices == NULL || vertices_size >= vertices_capacity) {
		size_t prev_capacity = vertices_capacity;
		size_t new_capacity = 2*prev_capacity;
		if (vertices == NULL) {
			prev_capacity = 0;
			new_capacity = VERTICES_CAPACITY_START;
		}
		debug(vertices = alloc(vertices, new_capacity, sizeof(Vertex)););
		vertices_capacity = new_capacity;
		for (unsigned int i = prev_capacity; i < new_capacity; i++) {
			vertices[i].in_use = false;
		}
		item_ptr->vertex_location = prev_capacity;
		vertices[prev_capacity].changed = true;
		vertices[prev_capacity].in_use = true;
		vertices_size++;
		any_vertex_update = true;
		return prev_capacity;
	}
	for (unsigned int i = 0; i < vertices_size; i++) {
		if (!vertices[i].in_use) {
			item_ptr->vertex_location = i;
			vertices[i].changed = true;
			vertices[i].in_use = true;
			any_vertex_update = true;
			return i;
		}
	}
	item_ptr->vertex_location = vertices_size;
	vertices[vertices_size].changed = true;
	vertices[vertices_size].in_use = true;
	vertices_size++;
	any_vertex_update = true;
	return vertices_size-1;
}
unsigned int GUI_FindVertexAndTextLocation(GUI_Item_ID item_id, unsigned int number_of_available_vertices) {
	GUI_Item* item_ptr = GUI_GetItemPtr(item_id);
	if (item_ptr->type != TEXT) {
		printf("You tried to find text location for item that isnt text\n");
		GUI_Item_Print(item_ptr);
		exit(-1);
	}

	if (item_ptr->vertex_location != 0) {
		printf("You have allready found vertex and text location for this item\n");
		GUI_Item_Print(item_ptr);
		exit(-1);
	}

	bool found_text_location = false;
	for (unsigned int i = 0; i < text_array_size; i++) {
		if (!text_array[i].in_use) {
			item_ptr->item.text.text_location = i;
			text_array[i].number_of_available_vertices = number_of_available_vertices;
			text_array[i].in_use = true;
			found_text_location = true;
		}
	}
	if (!found_text_location && (text_array == NULL || text_array_size >= text_array_capacity)) {
		size_t prev_capacity = text_array_capacity;
		size_t new_capacity = 2*prev_capacity;
		if (text_array == NULL) {
			prev_capacity = 0;
			new_capacity = TEXT_ARRAY_CAPACITY_START;
		}
		text_array = alloc(text_array, new_capacity, sizeof(Text));
		text_array_capacity = new_capacity;
		for (unsigned int i = prev_capacity; i < new_capacity; i++) {
			text_array[i].in_use = false;
		}
		item_ptr->item.text.text_location = prev_capacity;
		text_array[prev_capacity].number_of_available_vertices = number_of_available_vertices;
		text_array[prev_capacity].in_use = true;
		text_array_size++;
	}
	else {
		item_ptr->item.text.text_location = text_array_size;
		text_array[text_array_size].number_of_available_vertices = number_of_available_vertices;
		text_array[text_array_size].in_use = true;
		text_array_size++;
	}

	// see if there is freed up vertices where text can be located in one chain =========================================================
	unsigned int consecutive_vertices = 0;
	for (unsigned int i = 0; i < vertices_size; i++) {
		// if vertex is free ========================================================================================================
		if (!text_array[i].in_use) {
			consecutive_vertices++;
			// if found long enough vertex chain ================================================================================
			if (consecutive_vertices == number_of_available_vertices) {
				item_ptr->vertex_location = i - consecutive_vertices + 1;
				// marking vertex chain as currently in use =================================================================
				for (unsigned int i = item_ptr->vertex_location; i < number_of_available_vertices; i++) {
					vertices[i].changed = true;
					vertices[i].in_use = true;
				}
				any_vertex_update = true;
				return item_ptr->vertex_location;
			}
		}
		// resets vertex chain length to 0 ==========================================================================================
		else {
			consecutive_vertices = 0;
		}
	}
	// increasing size of vertices until text fits inside =======================================================================
	size_t prev_capacity = vertices_capacity;
	while (vertices_size + number_of_available_vertices >= vertices_capacity) {
		if (vertices == NULL) {
			printf("text should not be the first item created. The GUI design is flawed!\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}
		vertices = alloc(vertices, 2*vertices_capacity, sizeof(Vertex));
		vertices_capacity *= 2;
	}
	// marking all newly allocated vertices as not in use ===============================================================================
	for (unsigned int i = prev_capacity; i < vertices_capacity; i++) {
		vertices[i].in_use = false;
	}
	item_ptr->vertex_location = vertices_size;
	vertices_size += number_of_available_vertices;
	// marking vertex chain as currently in use =========================================================================================
	for (unsigned int i = item_ptr->vertex_location; i < vertices_size; i++) {
		vertices[i].changed = true;
		vertices[i].in_use = true;
	}
	any_vertex_update = true;
	return item_ptr->vertex_location;
}
GUI_Item_ID GUI_Item_Create_Private(GUI_Item_Type item_type, unsigned int number_of_available_vertices_for_type_text_only){
	debug(GUI_Item_ID item_id = GUI_FindItemLocation(););
	debug(GUI_Item* item_ptr = GUI_GetItemPtr(item_id););
	item_ptr->type = item_type;
	if (item_type == DRAWORDER || item_type == WINDOW) {
		return item_id;
	}
	if (item_type == TEXT) {
		debug(GUI_FindVertexAndTextLocation(item_id, number_of_available_vertices_for_type_text_only););
		return item_id;
	}
	debug(GUI_FindVertexLocation(item_id););
	return item_id;
}
void GUI_WriteToVBO() {
	// if there is no vertex updated, then return
	if (!any_vertex_update) {
		return;
	}
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// if the length of VBO should be changed, then just vrite all of vertices to VBO
	if (vertices_prev_capacity != vertices_capacity) {
		glBufferData(GL_ARRAY_BUFFER, vertices_capacity*sizeof(Vertex), vertices, GL_DYNAMIC_DRAW);
		vertices_prev_capacity = vertices_capacity;
	}
 	// if length is not changed
	else {
		// check if any vertex is changed
		unsigned int continuous_vertices = 0;
		for (unsigned int i = 0; i < vertices_prev_size; i++) {
			if (vertices[i].changed) {
				continuous_vertices++;
				continue;
			}
			if (continuous_vertices >= 1) {
				glBufferSubData(GL_ARRAY_BUFFER, (i-continuous_vertices)*sizeof(Vertex), continuous_vertices*sizeof(Vertex), &vertices[i-continuous_vertices]);
				continuous_vertices = 0;
			}
		}
		// any new vertex must be updated
		if (vertices_prev_size < vertices_size) {
			glBufferSubData(GL_ARRAY_BUFFER, vertices_prev_size*sizeof(Vertex), (vertices_size-vertices_prev_size)*sizeof(Vertex), &vertices[vertices_prev_size]);
			vertices_prev_size = vertices_size;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	any_vertex_update = false;
}
void GUI_WriteFromItemToVertex(GUI_Item* item_ptr) {
	if (item_ptr == NULL) {
		printf("item_ptr == NULL\n");
		exit(-1);
	}
	if (item_ptr->type == WINDOW || item_ptr->type == DRAWORDER) {
		return; // WINDOW and DRAWORDER dont have vertices
	}
	Vertex* vertex_ptr = &vertices[item_ptr->vertex_location];
	GUI_Item_Normal* normal_ptr = &item_ptr->item.normal;
	if (item_ptr->type == NORMAL) {
		for (unsigned int i = 0; i<4; i++) {
			vertex_ptr->rect[i] = normal_ptr->rect[i];
			((unsigned char*)(&vertex_ptr->color))[i] = normal_ptr->color[i];
		}
		vertex_ptr->multiple_values = (unsigned int)normal_ptr->corner_radius_pixels << 16;
		vertex_ptr->changed = true;
		any_vertex_update = true;
		return;
	}
	if (item_ptr->type == TEXT) {
		// ??????????????????????????????????????????????????????????????????????????????????????????????????
		return;
	}
	if (item_ptr->type == RENDERTEXTURE) {
		// ??????????????????????????????????????????????????????????????????????????????????
		return;
	}
	if (item_ptr->type == CUSTOM) {
		// ??????????????????????????????????????????????????????????????????????????????????
		return;
	}
}
void GUI_DrawOntoTarget(GUI_Item_ID target_id) {
	debug(GUI_Item* target_ptr = GUI_GetItemPtr(target_id));
	// if the item is not a target
	if (target_ptr->type != WINDOW && target_ptr->type != RENDERTEXTURE) {
		printf("you tried to draw onto item that isnt a target\n");
		GUI_Item_Print(target_ptr);
		exit(-1);
	}

	// if target is WINDOW
	if (target_ptr->type == WINDOW) {
		debug(GUI_Item_DrawOrder* draw_order_ptr = &GUI_GetItemPtr(target_ptr->item.window.draw_order_id)->item.draw_order);

		int width = target_ptr->item.window.width;
		int height = target_ptr->item.window.height;
		glfwGetWindowSize(window, &width, &height);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0,0,width,height);

		glClear(GL_COLOR_BUFFER_BIT | height);
		// draw vertecies
		glUseProgram(shader_program);
		glUniform1ui(target_width_loc, width);
		glUniform1ui(target_height_loc, height);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_order_ptr->EBO);
		glDrawElements(GL_POINTS, draw_order_ptr->indices_size, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	// if target is RENDERTEXTURE
	else {
		debug(GUI_Item_DrawOrder* draw_order_ptr = &GUI_GetItemPtr(target_ptr->item.render_texture.draw_order_id)->item.draw_order);

		int internal_width = target_ptr->item.render_texture.internal_width;
		int internal_height = target_ptr->item.render_texture.internal_height;
		int external_width = target_ptr->item.render_texture.external_rect[2];
		int external_height = target_ptr->item.render_texture.external_rect[3];

		glBindFramebuffer(GL_FRAMEBUFFER, target_ptr->item.render_texture.FBO);
		glViewport(0,0,external_width,external_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// draw vertecies
		glUseProgram(shader_program);
		glUniform1ui(target_width_loc, internal_width);
		glUniform1ui(target_height_loc, internal_height);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_order_ptr->EBO);
		glDrawElements(GL_POINTS, draw_order_ptr->indices_size, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	//???????????????????????????????????
}

// RENDER LOOP FUNCTIONS ===================================================================

void GUI_UpdateEverythingNeededForDrawing() {
	// 	OVERVIEW:
	//		1. This is the first function to be called every frame because the drawing is done first then events are handled
	//		2. there should be "is_changed" in all items and (...) which this function depends upon to be fast.
	//		   Since you rearly need to update everything every frame, right?
	//		3. This function is devided into many parts:
	//			1. the first part is to update and validate the draw_tree if it is changed.
	//			   but wait. while validating the draw_tree you could more easily update indices inside visible draw_orders and vertices[_].changed=true.
	//			2. update EBO with indices in all visible draw_orders, VBO with vertices and VAO if EBO or VBO is updated
	//			3. ready to draw targets from furthest away towards window target ?????

	// Updating draw tree
	{
		// first and second is allways WINDOW and DRAWORDER
		draw_tree_array_size = 0;
		GUI_AddItemToDrawTree(0);
		GUI_AddItemToDrawTree(1);
		// this is used to show that a draw_order has ended and maybe a new will begin
		GUI_AddItemToDrawTree(0xFFFFFFFF); // 0xFFFFFFFF is the only id that is not valid but gets accepted

		GUI_Item* prev_item_ptr = NULL;
		GUI_Item* item_ptr = GUI_GetItemPtr(0);
		for (unsigned int i = 1; i < draw_tree_array_size; i++) {
			prev_item_ptr = item_ptr;
			item_ptr 	  = GUI_GetItemPtr(draw_tree_array[i]);

			// if not draw order then continue or draw order allready defined
			if (item_ptr->type == WINDOW || item_ptr->type == RENDERTEXTURE || prev_item_ptr->id == 0XFFFFFFFF) {
				goto skip;
			}

			#ifdef DEBUG
			// if item is not valid. not WINDOW, RENDERTEXTURE, DRAWORDER or 0xFFFFFFFF
			else if (item_ptr->type != DRAWORDER) {
				printf("design falw: the item is not valid\n");
				printf("current_item:\n");
				GUI_Item_Print(item_ptr);
				printf("prev item:\n");
				GUI_Item_Print(prev_item_ptr);
				exit(-1);
			}
			#endif

			// checking if draw_order is defined earlier
			GUI_Itme* prev_sub_item_ptr = NULL;
			GUI_Itme* sub_item_ptr 	    = GUI_GetItemPtr(item_ptr->item.draw_order.item_id_array[0]);
			for (unsigned int j = 1; j < i; j++) {

				prev_sub_item_ptr = sub_item_ptr;
				sub_item_ptr = GUI_GetItemPtr(item_ptr->item.draw_order.item_id_array[j]);

				// if draw order allready defined
				if (sub_item_ptr->type == DRAWORDER && prev_sub_item_ptr->id == 0XFFFFFFFF) {
					goto skip;
				}
			}

			// here draw order will be defined, meaning that all branches in the draw order will be written to draw_tree_array
			for (unsigned int j = 0; j < item_ptr->item.draw_order.item_id_array_size; j++) {

				sub_item_ptr = GUI_GetItemPtr(item_ptr->item.draw_order.item_id_array[j]);

				// if type is WINDOW then the draw order to the target must be found as well
				if (sub_item_ptr->type == WINDOW) {

					// adding target
					GUI_AddItemToDrawTree(sub_item_ptr->id);

					// adding draw order
					GUI_AddItemToDrawTree(sub_item_ptr->item.window.draw_order_id);

					// maybe you will have use for having window multiple places but for now the program should crash
					printf("the window should not be here\n");
					GUI_Item_Print(sub_item_ptr);
					exit(-1);
				}

				// if type is RENDERTEXTURE then the draw order to the target must be found as well
				else if (sub_item_ptr->type == RENDERTEXTURE) {

					// adding target
					GUI_AddItemToDrawTree(sub_item_ptr->id);

					// adding draw order
					GUI_AddItemToDrawTree(sub_item_ptr->item.render_texture.draw_order_id);
				}

				// if type is DRAWORDER
				else if (sub_item_ptr->type == DRAWORDER) {
					GUI_AddItemToDrawTree(sub_item_ptr->id);
				}
			}

			// this is used to show that a draw_order has ended and maybe a new will begin
			GUI_AddItemToDrawTree(0xFFFFFFFF); // 0xFFFFFFFF is the only id that is not valid but gets accepted

			skip:
		}
	}

	// Validate draw tree and while doing that also updating indices in all visible draw_orders
	// how and where should indices in draw_order be updated???????
	//		ANSWER:
	//			when draw_orders is marked as valid you should update indices as well
	//			the thing now is that you have to make an new dynamic array which tracs which indices needs update
	// 			so if an indices contains other indices that needs updates the indices itself needs an update as well
	//			HOW DO YOU CHECK THIS?
	//
	//			additionally an indices needs update if any of the items_id_array in the corresponding draw_order is changed
	//			i.e. if the size is changed or if new items has replaced old
	//			be be carfull because if a draw_order has a target infornt of it, its actually not in that draw_order that it seems to be within
	//		    HERE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	{
		debug(bool* is_draw_order_visited = alloc(NULL, draw_tree_array_size, sizeof(bool)));
		debug(bool* is_draw_order_valid = alloc(NULL, draw_tree_array_size, sizeof(bool)));

		// setting all values to false
		for (unsigned int i = 0; i < draw_tree_array_size; i++) {
			is_draw_order_visited[i] = false;
			is_draw_order_valid[i] = false;
			is_draw_order_indices_needing_update[i] = false;
		}

		unsigned int current = 1;
		while (is_draw_order_valid[1]==false) {

			// 	(1) is current valid?
			// 		(1.1) is current outside borders?
			//		(1.2) is current type DRAWORDER?
			//		(1.3) is current allready visited?

			//	(2) finding next draw_order
			//  	(2.1) if (next draw_order in next chunck is visited)
			//			(2.1.1) draw_tree is not valid
			//		(2.2) else if (current is first in current chunck || next draw_order in next chunck is valid)
			//			(2.2.1) if (there is no next draw_order in current chunck)
			//				(2.2.1.1) make all draw_order in current chunck valid and go back to the draw_order that entered this chunck
			//			(2.2.2) else
			//				(2.2.2.1) goto next draw_order in current chunck
			//		(2.3) else
			//			(2.3.1) goto next draw_order in next chunck

			GUI_Item* item_ptr = GUI_GetItemPtr(draw_tree_array[current]);

			// (1) is current valid?
			{
				#ifdef DEBUG
				// current should never be >= draw_tree_array_size. then its a design flaw
				else if (current >= draw_tree_array_size) {
					printf("design flaw: current+1 should never be greater than draw_tree_array_size\n");
					GUI_Item_Print(item_ptr);
					exit(-1);
				}

				// current should never be 0
				if (current == 0) {
					printf("design flaw: current should never == 0\n");
					GUI_Item_Print(item_ptr);
					exit(-1);
				}

				// if item is something other that draworder then there is a design flaw
				if (item_ptr->type != DRAWORDER) {
					printf("design flaw: current item should allways be DRAWORDER type\n");
					GUI_Item_Print(item_ptr);
					exit(-1);
				}
				#endif

				// should never visit a draw_order that is allready visited
				if (is_draw_order_visited[current] == true) {

					#ifdef DEBUG
					// if a branch is visited twice and also valid there is a design flaw
					if (is_draw_order_valid[current] == true) {
						printf("design flaw: A branch is visited twice and also valid, which shouldnt be possible\n");
						GUI_Item_Print(item_ptr);
						exit(-1);
					}
					#endif

					// should never visit a draw_order that is allready visited
					printf("user flaw: draw tree is not valid nr 1\n");
					GUI_Item_Print(item_ptr);
					exit(-1);
				}
			}

			// (2) finding next draw_order
			{
				// but first these must be found
				bool 			is_current_draw_order_first_in_chunck 			= false;
				unsigned int 	next_draw_order_in_next_chunck 					= 0;
				bool 			is_there_a_next_draw_order_in_current_chunck 	= false;
				unsigned int 	next_draw_order_in_current_chunck 				= 0;
				unsigned int 	first_draw_order_in_current_chunck 				= 0;
				GUI_Item* 		prev_item_ptr 									= NULL;
				unsigned int 	prev_item 										= 0;
				{
					// is current the first draw_order in currnet chunck
					if (draw_tree_array[current-1] == 0xFFFFFFFF) {
						is_current_draw_order_first_in_chunck = true;
					}
					else if ((draw_tree_array[current-2] == 0xFFFFFFFF) && (GUI_GetItemPtr(draw_tree_array[current-1])->type == WINDOW || GUI_GetItemPtr(draw_tree_array[current-1])->type == RENDERTEXTURE)) {
						is_current_draw_order_first_in_chunck = true;
					}

					// finding next draw_order in next chunck
					for (unsigned int i = 3; i<draw_tree_array_size; i++) {

						// if id is the same but index is not the same
						if (draw_tree_array[i] == draw_tree_array[current] && i != current) {

							// there should be an chunck separator item either i-1 xor i-2
							if ((draw_tree_array[i-1] == 0xFFFFFFFF || draw_tree_array[i-2] == 0xFFFFFFFF)
							&& !(draw_tree_array[i-1] == 0xFFFFFFFF && draw_tree_array[i-2] == 0xFFFFFFFF)) {

								// any item should never be first twice in a chunck
								#ifdef DEBUG
								if (next_draw_order_in_next_chunck != 0) {
									printf("design flaw: any item should never be first twice in a chunck\n");
									GUI_Item_Print(GUI_GetItemPtr(draw_tree_array[i]));
									exit(-1);
								}
								#endif

								next_draw_order_in_next_chunck = i;
							}
						}
					}

					#ifdef DEBUG
					if (next_draw_order_in_next_chunck == 0) {
						printf("design flaw: did not find next_draw_order_in_next_chunck. every draw_order should be first in only one chunck\n");
						GUI_Item_Print(item_ptr);
						exit(-1);
					}
					#endif

					// is_there_a_next_draw_order_in_current_chunck. if so what index is it at?
					GUI_Item* current_plus_1_ptr = GUI_GetItemPtr(draw_tree_array[current + 1]);
					GUI_Item* current_plus_2_ptr = GUI_GetItemPtr(draw_tree_array[current + 2]);
					// if next item is not id=0xFFFFFFFF means that there must be a next draw_order in current chunck
					if (current_plus_1_ptr->id != 0xFFFFFFFF) {

						// if current+1 is DRAWORDER
						if (current_plus_1_ptr->type == DRAWORDER) {

							#ifdef DEBUG
							// if next draw_order in current chunck is allready visited there is a design flaw
							if (is_draw_order_visited[current+1]) {
								printf("desgin flaw: next draw_order in current chunck is allready visited\n");
								printf("	current item\n");
								GUI_GetItemPtr(item_ptr);
								printf("	current+1:\n");
								GUI_GetItemPtr(current_plus_1_ptr);
								printf("	current+2:\n");
								GUI_GetItemPtr(current_plus_2_ptr);
								exit(-1);
							}
							#endif

							is_there_a_next_draw_order_in_current_chunck = true;
							next_draw_order_in_current_chunck = current+1;
						}

						#ifdef DEBUG
						// if design flaw: next item in current chunck is neither target, draw_order nor id==0xFFFFFFFF
						else if (!((current_plus_1_ptr->type == WINDOW) || (current_plus_1_ptr->type == RENDERTEXTURE))) {
							printf("design flaw: next item in current chunck is neither target, draw_order nor id==0xFFFFFFFF\n");
							printf("	current item\n");
							GUI_GetItemPtr(item_ptr);
							printf("	current+1:\n");
							GUI_GetItemPtr(current_plus_1_ptr);
							printf("	current+2:\n");
							GUI_GetItemPtr(current_plus_2_ptr);
							exit(-1);
						}
						#endif

						// if current+2 is draw_order
						else if (current_plus_2_ptr->type == DRAWORDER) {
							#ifdef DEBUG
							// if next draw_order in current chunck is allready visited there is a design flaw
							if (is_draw_order_visited[current+2]) {
								printf("desgin flaw: next draw_order in current chunck is allready visited\n");
								printf("	current item\n");
								GUI_GetItemPtr(item_ptr);
								printf("	current+1:\n");
								GUI_GetItemPtr(current_plus_1_ptr);
								printf("	current+2:\n");
								GUI_GetItemPtr(current_plus_2_ptr);
								exit(-1);
							}
							#endif

							is_there_a_next_draw_order_in_current_chunck = true;
							next_draw_order_in_current_chunck = current+2;
						}

						#ifdef DEBUG
						// if item after target is not draw_order then there is a design flaw
						else {
							printf("desing flaw: item after target is not draw_order\n");
							printf("	current item\n");
							GUI_GetItemPtr(item_ptr);
							printf("	current+1:\n");
							GUI_GetItemPtr(current_plus_1_ptr);
							printf("	current+2:\n");
							GUI_GetItemPtr(current_plus_2_ptr);
							exit(-1);
						}
						#endif
					}

					// to find the previous chunck you have to find the first draw_order in current chunck
					for (unsigned int i = current; i>=1; i--) {

						// id == 0xFFFFFFFF defines the chunck separator
						if (draw_tree_array[i] == 0xFFFFFFFF) {

							// remember the first item in the chunck can possibly be a target and not a draw_order
							// so you have to find the first draw_order in the chunck

							// if draw_order is the first item in chunck
							if (GUI_GetItemPtr(draw_tree_array[i+1])->type == DRAWORDER) {
								first_draw_order_in_current_chunck = i+1;
							}

							// if draw_order is the second item in chunck
							else if (GUI_GetItemPtr(draw_tree_array[i+1])->type == DRAWORDER) {
								first_draw_order_in_current_chunck = i+2;
							}

							// if neither the first nor second item in chunck is a draw_order then there is a design flaw
							#ifdef DEBUG
							else {
								printf("design flaw: neither first nor second item in chunck is a draw_order");
								GUI_Item_Print(item_ptr);
								exit(-1);
							}
							#endif

						}
					}

					// this is if the algorithm didnt find any first draw_order in current chunck
					#ifdef DEBUG
					if (first_draw_order_in_current_chunck == 0) {
						printf("design flaw: first_draw_order_in_current_chunck should never be 0 because it isnt a draw_order\n");
						GUI_Item_Print(item_ptr);
						exit(-1);
					}
					#endif

					// finding item that invoked current chunck in previous chunck
					for (unsigned int i = 1; i < draw_tree_array_size; i++) {

						// remember you do not want to find the first in chunck but rather any other index in previous chunck which invoked current chunck
						if (i == first_draw_order_in_current_chunck)
							continue;

						prev_item_ptr = GUI_GetItemPtr(draw_tree_array[i]);

						// has found the previous item
						if (prev_item_ptr->id == item_ptr->id) {
							prev_item = i;
							break;
						}
					}

					// prev_item_ptr should never be NULL
					#ifdef DEBUG
					if (prev_item_ptr == NULL) {
						printf("design flaw: prev_item_ptr should never be NULL\n");
						GUI_Item_Print(item_ptr);
						exit(-1);
					}
					#endif

					// i prev_item == 0 it means that prev_item_ptr never was found and so there is a design flaw
					#ifdef DEBUG
					if (prev_item == 0) {
						printf("design flaw: could not find prev_item\n");
						GUI_Item_Print(item_ptr);
						exit(-1);
					}
					#endif
				}

				// (2.1) if (first draw_order in next chunck is visited && current draw order is not first in current chunck)
				if (is_current_draw_order_first_in_chunck == false && is_draw_order_visited[next_draw_order_in_next_chunck] == true) {
					// (2.1.1) draw_tree is not valid
					{
						printf("user flaw: draw_tree is not valid");
						GUI_Item_Print(item_ptr);
						GUI_Item_Print
						exit(-1);
					}
				}
				// (2.2) else if (current draw order is first in current chunck || next draw_order in next chunck is valid)
				else if (is_current_draw_order_first_in_chunck == true
				|| is_draw_order_valid[next_draw_order_in_next_chunck] == true) {
					// (2.2.1) if (there is no next draw_order in current chunck)
					if (is_there_a_next_draw_order_in_current_chunck == false) {

						// (2.2.1.1) make all draw_order in current chunck valid and go back to the draw_order that entered this chunck
						{
							is_draw_order_valid[current] = true;
							is_draw_order_visited[current] = false;
							current = prev_item;

							// marking all draw_orders in current chunck as valid
							for (unsigned int i = first_draw_order_in_current_chunck; i < draw_tree_array_size; i++) {

								// figuring out if item i is a draw_order
								GUI_Item* sub_item_ptr = GUI_GetItemPtr(draw_tree_array[i]);
								if (sub_item_ptr->type == DRAWORDER) {
									is_draw_order_valid[i] = true;
									is_draw_order_visited[i] = false;
								}

								// item i is chunck separator then this loop is done
								else if (sub_item_ptr->if == 0xFFFFFFFF) {
									// not return ????????????????????????????????????????
									break;
								}

								#ifdef DEBUG
								else if (sub_item_ptr->type == WINDOW || sub_item_ptr->type == RENDERTEXTURE) {
									printf("design flaw: item in draw_tree_array is not valid\n");
									printf("current item:");
									GUI_Item_Print(item_ptr);
									printf("not valid item:");
									GUI_Item_Print(sub_item_ptr;);
									exit(-1);
								}
								#endif
							}
							// is there anything more to do before returning ?????????????


						}

						// updating indices in first draw_order in current chunck
						{
							// first_draw_order_in_current_chunck_ptr
							GUI_Item_DrawOrder* draw_order_ptr = &(GUI_GetItemPtr(first_draw_order_in_current_chunck)->item.draw_order);

							// if first draw_order in current chunck is not changed then check if any of the draw orer
							if (!draw_order_ptr->indices_needsUpdate) {
								for (unsigned int i = 0; i < draw_order_ptr->item_id_array_size; i++) {
									GUI_Item* sub_item_ptr = GUI_GetItemPtr(draw_order_ptr->item_id_array[i]);
									if (sub_item_ptr->type = DRAWORDER) {
										continue;
									}
									if (sub_item_ptr->item.draw_order.indices_needsUpdate) {
										draw_order_ptr->indices_needsUpdate = true;
										break;
									}
								}
							}

							// if first draw_rder in current chunck is changed then this will update the indices in that draw_order
							if (draw_order_ptr->indices_needsUpdate) {
								for (unsigned int i = 0; i < draw_order_ptr->item_id_array_size; i++) {

									// getting the current item in draw order
									GUI_Item* item_ptr = GUI_GetItemPtr(draw_order_ptr->item_id_array[i]);


									// start and end index are only for text
									size_t start_index = item_ptr->vertex_location;
									size_t end_index = start_index+1;
									size_t prev_vertices_size = draw_order_ptr->indices_size;

									// if item is text then there is more than one index
									if (item_ptr->id == TEXT) {
										end_index = start_index + text_array[item_ptr->item.text.text_location].number_of_available_vertices;
									}

									// same for draw_order. there will be more than one index
									if (item_ptr->id == DRAWORDER) {
										end_index = start_index + item_ptr->item.draw_order.item_id_array_size;
									}

									// figuring out what the new_capacity draw_order_ptr->indices is. most times its the same as previously
									size_t prev_capacity = draw_order_ptr->indices_capacity;
									size_t new_capacity = draw_order_ptr->indices_capacity == 0 ? 8 : draw_order_ptr->indices_capacity;
									while (draw_order_ptr->indices_size+(end_index-start_index) >= new_capacity) {
										new_capacity = 2*new_capacity;
									}

									// If resizing is needed, because capacity is increased
									if (draw_order_ptr->indices == NULL || new_capacity > prev_capacity) {

										// resizing indices to new capacity
										debug(draw_order_ptr->indices = alloc(draw_order_ptr->indices, new_capacity, sizeof(unsigned int)));
										draw_order_ptr->EBO_needsResize = true;

										// setting all new indices to 0xFFFFFFFF
										for (unsigned int i = prev_capacity; i < new_capacity; i++) {
											draw_order_ptr->indices[i] = 0xFFFFFFFF; // the NO_INDEX index
										}

										// writing the new item's vertex location. for text_item there are more than one vertex location, therefor there is a for loop here.
										for (unsigned int i = 0; i < end_index-start_index; i++) {
											draw_order_ptr->indices[draw_order_ptr->indices_size] = start_index+i;
											draw_order_ptr->indices_size++;
										}

										// resizing EBO
										draw_order_ptr->EBO_needsResize = true;

									}

									// writing the newly added item's vertex location to draw_order_ptr.
									{
										if (item_ptr->id == WINDOW) {
											printf("design flaw || user flaw: window item should no be here\n");
											printf("this window item ...\n");
											GUI_Item_Print(item_ptr);
											printf("was found in this draw_order item!\n");
											GUI_Item_Print(GUI_GetItemPtr(first_draw_order_in_current_chunck));
											exit(-1);
										}

										else if (item_ptr->id == CUSTOM) {
											// nothing?
										}

										else if (item_ptr->id == DRAWORDER) {

										}
										// all other items
										else {
											// for text_item there are more than one vertex location, therefor there is a for loop here.
											for (unsigned int i = 0; i < end_index-start_index; i++) {
												draw_order_ptr->indices[i+draw_order_ptr->indices_size] = start_index+i;
											}
										}
									}


									// for draw_order_item there could be more than one vertex location

									draw_order_ptr->indices_capacity = new_capacity;

									// writing new vertex location for new item to EBO
									glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_order_ptr->EBO);
									glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, prev_vertices_size, (end_index-start_index)*sizeof(unsigned int), &draw_order_ptr->indices[prev_vertices_size]);
									glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

								}

								// MAKE FIRST DRAWORDER IN PREVIOUS CHUNCK indices_needsUpdate = true SINCE FIRST DRAWORDER IN THIS CHUNCK IS CHANGED
								// NOOOOOOOOOOOOOOOOOOOOOOO!!! because this part checks if indices_needsUpdate==true in the sub_draw_orders

							}
						}

					}

					// (2.2.2) else
					else {
						// (2.2.2.1) goto next draw_order in current chunck and mark current as visited
						{
							is_draw_order_visited[current] = true;
							current = next_draw_order_in_current_chunck;
						}
					}
				}
				// (2.3) else
				else {
					// (2.3.1) goto next draw_order in next chunck and mark current as visited
					{
						is_draw_order_visited[current] = true;
						current = next_draw_order_in_next_chunck;
					}
				}

				// remember to mark current and other items as not visited !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			}
		}

		free(is_draw_order_visited);
		free(is_draw_order_valid);
	}

	// Update EBO in all visible draw_orders
	{
		for (unsigned int i = 0; i < draw_tree_array_size; i++) {
			GUI_Item* item_ptr = GUI_GetItemPtr(draw_tree_array[i]);
			if (item_ptr->type == DRAWORDER) {
				GUI_Item_DrawOrder* draw_order_ptr = &item_ptr->item.draw_order;

				// if EBO_needsResize then resizing EBO
				if (draw_order_ptr->EBO_needsResize) {
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_order_ptr->EBO);
					debug(glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_order_ptr->indices_capacity*sizeof(unsigned int), draw_order_ptr->indices, GL_DYNAMIC_DRAW));
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				}

				// there is no EBO_needsUpdate because this can be done when the indices are found

				else {
					glBufferSubData(draw_order_ptr->EBO, 0, draw_order_ptr->indices_size; draw_order_ptr->indices);
				}
			}
		}
	}

	// when setting indices you have to reset all indices_needsUpdate to false after it is not needed anymore for each frame
	// BUT you cannot do it right after updating the indices in one draw_order because most draw_orders depend on indices_needsUpdate in other draw_orders
	{

	}

	// Update vertices in draw_order that actually needs update
	{
		debug(bool* is_draw_order_visited = alloc(NULL, draw_tree_array_size, sizeof(bool)));
		debug(bool* is_draw_order_indices_needing_update = alloc(NULL, draw_tree_array_size, sizeof(bool)));


		free(is_draw_order_visited);
		free(is_draw_order_indices_needing_update):
	}



	// Update VBO
	{
		// remember VBO and vertices has all the same metadata exept that VBO is on the GPU and vertices is on the CPU
		unsigned int start;
		unsigned int end;
		for (unsigned int i = 0; i < vertices_size; i++) {
			if (vertices[i].VBO_upToDate) {
				if (start==end) {
					start++;
					end = start;
					continue;
				}

				// should update this range with glBufferSubData
				//glBufferSubData(target, offset, size, ptr);
				glBufferSubData(VBO, start, end-start, vertices[start]);

				start = i;
				end = i;
				continue;
			}

			consecutive_end++;
		}
	}



}

// DRAWTREE
void GUI_AddItemToDrawTree(GUI_Item_ID item_id) {

	// does item have the right type?
	GUI_Item* item_ptr = GUI_GetItemPtr(item_id);
	if (item_ptr->type != WINDOW && item_ptr->type != RENDERTEXTURE && item_ptr->type != DRAWORDER && item_ptr->type != NONE && item_id != 0xFFFFFFFF) {
		printf("item has to be WINDOW, RENDERTEXTURE, DRAWORDER or NONE\n");
		GUI_Item_Print(item_ptr);
		exit(-1);
	}

	// draw_tree_array cannot be NULL
	if (draw_tree_array == NULL) {

		// the first item must be the WINDOW
		if (item_id != 0) {
			printf("the first item in draw_tree_array must be WINDOW\n");
			GUI_Item_Print(GUI_GetItemPtr(item_id));
			exit(-1);
		}

		// allocating mem and writing every new item_id to 0xFFFFFFFF
		debug(draw_tree_array = alloc(draw_tree_array, DRAW_TREE_ARRAY_CAPACITY_START, sizeof(GUI_Item_ID)));
		draw_tree_array_capacity = DRAW_TREE_ARRAY_CAPACITY_START;
		for (unsigned int i = draw_tree_array_size; i < draw_tree_array_capacity; i++)
			draw_tree_array[i] = 0xFFFFFFFF;

		// if the previous item is not the same as this then a change is make and must be written
		if (draw_tree_array[draw_tree_array_size] != item_id)
			draw_tree_array_change = true;

		// writing item_id to draw_tree_array
		draw_tree_array[draw_tree_array_size] = item_id;
		draw_tree_array_size++;

		return;
	}

	// checking if draw_tree_array needs to be resized
	if (draw_tree_array_size >= draw_tree_array_capacity) {
		while (draw_tree_array_size >= draw_tree_array_capacity) {
			draw_tree_array_capacity *= 2;
		}
		debug(draw_tree_array = alloc(draw_tree_array, draw_tree_array_capacity, sizeof(GUI_Item_ID)));
		for (unsigned int i = draw_tree_array_size; i < draw_tree_array_capacity; i++)
			draw_tree_array[i] = 0xFFFFFFFF;
	}

	// add the new target to draw_tree_array
	draw_tree_array[draw_tree_array_size] = item_id;
	draw_tree_array_size++;

	// T 1 2 T 3 4 | 2 4 6 | 3 7 8 | 4 9 10 | 5 3 | 6 5 | 7 9 | 8 4 | 9  3 | 10 |
	//   1 1   0 0   1 1 0   1 1 0   1 1 0    0 0   0 0   1 1   0 0   1  1    0
	//   0 0   0 0   0 0 0   0 0 0   0 0 0    0 0   0 0   0 0   0 0   0  0    0
}
void GUI_UpdateDrawTree() {

	// first and second is allways WINDOW and DRAWORDER
	draw_tree_array_size = 0;
	GUI_AddItemToDrawTree(0);
	GUI_AddItemToDrawTree(1);
	// this is used to show that a draw_order has ended and maybe a new will begin
	GUI_AddItemToDrawTree(0xFFFFFFFF); // 0xFFFFFFFF is the only id that is not valid but gets accepted

	GUI_Item* prev_item_ptr = NULL;
	GUI_Item* item_ptr = GUI_GetItemPtr(0);
	for (unsigned int i = 1; i < draw_tree_array_size; i++) {
		prev_item_ptr = item_ptr;
		item_ptr 	  = GUI_GetItemPtr(draw_tree_array[i]);

		// if not draw order then continue or draw order allready defined
		if (item_ptr->type == WINDOW || item_ptr->type == RENDERTEXTURE || prev_item_ptr->id == 0XFFFFFFFF) {
			goto skip;
		}

		#ifdef DEBUG
		// if item is not valid. not WINDOW, RENDERTEXTURE, DRAWORDER or 0xFFFFFFFF
		else if (item_ptr->type != DRAWORDER) {
			printf("design falw: the item is not valid\n");
			printf("current_item:\n");
			GUI_Item_Print(item_ptr);
			printf("prev item:\n");
			GUI_Item_Print(prev_item_ptr);
			exit(-1);
		}
		#endif

		// checking if draw_order is defined earlier
		GUI_Itme* prev_sub_item_ptr = NULL;
		GUI_Itme* sub_item_ptr 	    = GUI_GetItemPtr(item_ptr->item.draw_order.item_id_array[0]);
		for (unsigned int j = 1; j < i; j++) {

			prev_sub_item_ptr = sub_item_ptr;
			sub_item_ptr = GUI_GetItemPtr(item_ptr->item.draw_order.item_id_array[j]);

			// if draw order allready defined
			if (sub_item_ptr->type == DRAWORDER && prev_sub_item_ptr->id == 0XFFFFFFFF) {
				goto skip;
			}
		}

		// here draw order will be defined, meaning that all branches in the draw order will be written to draw_tree_array
		for (unsigned int j = 0; j < item_ptr->item.draw_order.item_id_array_size; j++) {

			sub_item_ptr = GUI_GetItemPtr(item_ptr->item.draw_order.item_id_array[j]);

			// if type is WINDOW then the draw order to the target must be found as well
			if (sub_item_ptr->type == WINDOW) {

				// adding target
				GUI_AddItemToDrawTree(sub_item_ptr->id);

				// adding draw order
				GUI_AddItemToDrawTree(sub_item_ptr->item.window.draw_order_id);

				// maybe you will have use for having window multiple places but for now the program should crash
				printf("the window should not be here\n");
				GUI_Item_Print(sub_item_ptr);
				exit(-1);
			}

			// if type is RENDERTEXTURE then the draw order to the target must be found as well
			else if (sub_item_ptr->type == RENDERTEXTURE) {

				// adding target
				GUI_AddItemToDrawTree(sub_item_ptr->id);

				// adding draw order
				GUI_AddItemToDrawTree(sub_item_ptr->item.render_texture.draw_order_id);
			}

			// if type is DRAWORDER
			else if (sub_item_ptr->type == DRAWORDER) {
				GUI_AddItemToDrawTree(sub_item_ptr->id);
			}
		}

		// this is used to show that a draw_order has ended and maybe a new will begin
		GUI_AddItemToDrawTree(0xFFFFFFFF); // 0xFFFFFFFF is the only id that is not valid but gets accepted

		skip:
	}
}
void GUI_IsDrawTreeValid() {
	debug(bool* is_draw_order_visited = alloc(NULL, draw_tree_array_size, sizeof(bool)));
	debug(bool* is_draw_order_valid = alloc(NULL, draw_tree_array_size, sizeof(bool)));

	// setting all values to false
	for (unsigned int i = 0; i < draw_tree_array_size; i++) {
		is_draw_order_visited[i] = false;
		is_draw_order_valid[i] = false;
	}

	unsigned int current = 1;
	// ????????????????????????????????????????????? there is no brake inside this loop
	for (;;) {

		// 	(1) is current valid?
		// 		(1.1) is current outside borders?
		//		(1.2) is current type DRAWORDER?
		//		(1.3) is current allready visited?

		//	(2) finding next draw_order
		//  	(2.1) if (next draw_order in next chunck is visited)
		//			(2.1.1) draw_tree is not valid
		//		(2.2) else if (current is first in current chunck || next draw_order in next chunck is valid)
		//			(2.2.1) if (there is no next draw_order in current chunck)
		//				(2.2.1.1) make all draw_order in current chunck valid and go back to the draw_order that entered this chunck
		//			(2.2.2) else
		//				(2.2.2.1) goto next draw_order in current chunck
		//		(2.3) else
		//			(2.3.1) goto next draw_order in next chunck

		GUI_Item* item_ptr = GUI_GetItemPtr(draw_tree_array[current]);

		// (1) is current valid?

		#ifdef DEBUG
		// current should never be >= draw_tree_array_size. then its a design flaw
		else if (current >= draw_tree_array_size) {
			printf("design flaw: current+1 should never be greater than draw_tree_array_size\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}

		// current should never be 0
		if (current == 0) {
			printf("design flaw: current should never == 0\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}

		// if item is something other that draworder then there is a design flaw
		if (item_ptr->type != DRAWORDER) {
			printf("design flaw: current item should allways be DRAWORDER type\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}
		#endif

		// should never visit a draw_order that is allready visited
		if (is_draw_order_visited[current] == true) {

			#ifdef DEBUG
			// if a branch is visited twice and also valid there is a design flaw
			if (is_draw_order_valid[current] == true) {
				printf("design flaw: A branch is visited twice and also valid, which shouldnt be possible\n");
				GUI_Item_Print(item_ptr);
				exit(-1);
			}
			#endif

			// should never visit a draw_order that is allready visited
			printf("user flaw: draw tree is not valid nr 1\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}


		// (2) finding next draw_order

		// but first these must be found
		bool 			is_current_draw_order_first_in_chunck 			= false;
		unsigned int 	next_draw_order_in_next_chunck 					= 0;
		bool 			is_there_a_next_draw_order_in_current_chunck 	= false;
		unsigned int 	next_draw_order_in_current_chunck 				= 0;
		unsigned int 	first_draw_order_in_current_chunck 				= 0;
		GUI_Item* 		prev_item_ptr 									= NULL;
		unsigned int 	prev_item 										= 0;

		// is current the first draw_order in currnet chunck
		if (draw_tree_array[current-1] == 0xFFFFFFFF) {
			is_current_draw_order_first_in_chunck = true;
		}
		else if ((draw_tree_array[current-2] == 0xFFFFFFFF) && (GUI_GetItemPtr(draw_tree_array[current-1])->type == WINDOW || GUI_GetItemPtr(draw_tree_array[current-1])->type == RENDERTEXTURE)) {
			is_current_draw_order_first_in_chunck = true;
		}

		// finding next draw_order in next chunck
		for (unsigned int i = 3; i<draw_tree_array_size; i++) {

			// if id is the same but index is not the same
			if (draw_tree_array[i] == draw_tree_array[current] && i != current) {

				// there should be an chunck separator item either i-1 xor i-2
				if ((draw_tree_array[i-1] == 0xFFFFFFFF || draw_tree_array[i-2] == 0xFFFFFFFF)
				&& !(draw_tree_array[i-1] == 0xFFFFFFFF && draw_tree_array[i-2] == 0xFFFFFFFF)) {

					// any item should never be first twice in a chunck
					#ifdef DEBUG
					if (next_draw_order_in_next_chunck != 0) {
						printf("design flaw: any item should never be first twice in a chunck\n");
						GUI_Item_Print(GUI_GetItemPtr(draw_tree_array[i]));
						exit(-1);
					}
					#endif

					next_draw_order_in_next_chunck = i;
				}
			}
		}

		#ifdef DEBUG
		if (next_draw_order_in_next_chunck == 0) {
			printf("design flaw: did not find next_draw_order_in_next_chunck. every draw_order should be first in only one chunck\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}
		#endif

		// is_there_a_next_draw_order_in_current_chunck. if so what index is it at?
		GUI_Item* current_plus_1_ptr = GUI_GetItemPtr(draw_tree_array[current + 1]);
		GUI_Item* current_plus_2_ptr = GUI_GetItemPtr(draw_tree_array[current + 2]);
		// if next item is not id=0xFFFFFFFF means that there must be a next draw_order in current chunck
		if (current_plus_1_ptr->id != 0xFFFFFFFF) {

			// if current+1 is DRAWORDER
			if (current_plus_1_ptr->type == DRAWORDER) {

				#ifdef DEBUG
				// if next draw_order in current chunck is allready visited there is a design flaw
				if (is_draw_order_visited[current+1]) {
					printf("desgin flaw: next draw_order in current chunck is allready visited\n");
					printf("	current item\n");
					GUI_GetItemPtr(item_ptr);
					printf("	current+1:\n");
					GUI_GetItemPtr(current_plus_1_ptr);
					printf("	current+2:\n");
					GUI_GetItemPtr(current_plus_2_ptr);
					exit(-1);
				}
				#endif

				is_there_a_next_draw_order_in_current_chunck = true;
				next_draw_order_in_current_chunck = current+1;
			}

			#ifdef DEBUG
			// if design flaw: next item in current chunck is neither target, draw_order nor id==0xFFFFFFFF
			else if (!((current_plus_1_ptr->type == WINDOW) || (current_plus_1_ptr->type == RENDERTEXTURE))) {
				printf("design flaw: next item in current chunck is neither target, draw_order nor id==0xFFFFFFFF\n");
				printf("	current item\n");
				GUI_GetItemPtr(item_ptr);
				printf("	current+1:\n");
				GUI_GetItemPtr(current_plus_1_ptr);
				printf("	current+2:\n");
				GUI_GetItemPtr(current_plus_2_ptr);
				exit(-1);
			}
			#endif

			// if current+2 is draw_order
			else if (current_plus_2_ptr->type == DRAWORDER) {
				#ifdef DEBUG
				// if next draw_order in current chunck is allready visited there is a design flaw
				if (is_draw_order_visited[current+2]) {
					printf("desgin flaw: next draw_order in current chunck is allready visited\n");
					printf("	current item\n");
					GUI_GetItemPtr(item_ptr);
					printf("	current+1:\n");
					GUI_GetItemPtr(current_plus_1_ptr);
					printf("	current+2:\n");
					GUI_GetItemPtr(current_plus_2_ptr);
					exit(-1);
				}
				#endif

				is_there_a_next_draw_order_in_current_chunck = true;
				next_draw_order_in_current_chunck = current+2;
			}

			#ifdef DEBUG
			// if item after target is not draw_order then there is a design flaw
			else {
				printf("desing flaw: item after target is not draw_order\n");
				printf("	current item\n");
				GUI_GetItemPtr(item_ptr);
				printf("	current+1:\n");
				GUI_GetItemPtr(current_plus_1_ptr);
				printf("	current+2:\n");
				GUI_GetItemPtr(current_plus_2_ptr);
				exit(-1);
			}
			#endif
		}

		// to find the previous chunck you have to find the first draw_order in current chunck
		for (unsigned int i = current; i>=1; i--) {

			// id == 0xFFFFFFFF defines the chunck separator
			if (draw_tree_array[i] == 0xFFFFFFFF) {

				// remember the first item in the chunck can possibly be a target and not a draw_order
				// so you have to find the first draw_order in the chunck

				// if draw_order is the first item in chunck
				if (GUI_GetItemPtr(draw_tree_array[i+1])->type == DRAWORDER) {
					first_draw_order_in_current_chunck = i+1;
				}

				// if draw_order is the second item in chunck
				else if (GUI_GetItemPtr(draw_tree_array[i+1])->type == DRAWORDER) {
					first_draw_order_in_current_chunck = i+2;
				}

				// if neither the first nor second item in chunck is a draw_order then there is a design flaw
				#ifdef DEBUG
				else {
					printf("design flaw: neither first nor second item in chunck is a draw_order");
					GUI_Item_Print(item_ptr);
					exit(-1);
				}
				#endif

			}
		}

		// this is if the algorithm didnt find any first draw_order in current chunck
		#ifdef DEBUG
		if (first_draw_order_in_current_chunck == 0) {
			printf("design flaw: first_draw_order_in_current_chunck should never be 0 because it isnt a draw_order\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}
		#endif

		// finding item that invoked current chunck in previous chunck
		for (unsigned int i = 1; i < draw_tree_array_size; i++) {

			// remember you do not want to find the first in chunck but rather any other index in previous chunck which invoked current chunck
			if (i == first_draw_order_in_current_chunck)
				continue;

			prev_item_ptr = GUI_GetItemPtr(draw_tree_array[i]);

			// has found the previous item
			if (prev_item_ptr->id == item_ptr->id) {
				prev_item = i;
				break;
			}
		}

		// prev_item_ptr should never be NULL
		#ifdef DEBUG
		if (prev_item_ptr == NULL) {
			printf("design flaw: prev_item_ptr should never be NULL\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}
		#endif

		// i prev_item == 0 it means that prev_item_ptr never was found and so there is a design flaw
		#ifdef DEBUG
		if (prev_item == 0) {
			printf("design flaw: could not find prev_item\n");
			GUI_Item_Print(item_ptr);
			exit(-1);
		}
		#endif

		// (2.1) if (first draw_order in next chunck is visited && current draw order is not first in current chunck)
		if (is_current_draw_order_first_in_chunck == false && is_draw_order_visited[next_draw_order_in_next_chunck] == true) {
			// (2.1.1) draw_tree is not valid
			printf("user flaw: draw_tree is not valid");
			GUI_Item_Print(item_ptr);
			GUI_Item_Print
			exit(-1);

		}

		// (2.2) else if (current draw order is first in current chunck || next draw_order in next chunck is valid)
		else if (is_current_draw_order_first_in_chunck == true
		 || is_draw_order_valid[next_draw_order_in_next_chunck] == true) {
			// (2.2.1) if (there is no next draw_order in current chunck)
			if (is_there_a_next_draw_order_in_current_chunck == false) {
				// (2.2.1.1) make all draw_order in current chunck valid and go back to the draw_order that entered this chunck
				is_draw_order_valid[current] = true;
				is_draw_order_visited[current] = false;
				current = prev_item;

				// marking all draw_orders in current chunck as valid
				for (unsigned int i = first_draw_order_in_current_chunck; i < draw_tree_array_size; i++) {

					// figuring out if item i is a draw_order
					GUI_Item* sub_item_ptr = GUI_GetItemPtr(draw_tree_array[i]);
					if (sub_item_ptr->type == DRAWORDER) {
						is_draw_order_valid[i] = true;
						is_draw_order_visited[i] = false;
					}

					// item i is chunck separator then this loop is done
					else if (sub_item_ptr->if == 0xFFFFFFFF) {
						// not return ????????????????????????????????????????
						break;
					}

					#ifdef DEBUG
					else if (sub_item_ptr->type == WINDOW || sub_item_ptr->type == RENDERTEXTURE) {
						printf("design flaw: item in draw_tree_array is not valid\n");
						printf("current item:");
						GUI_Item_Print(item_ptr);
						printf("not valid item:");
						GUI_Item_Print(sub_item_ptr;);
						exit(-1);
					}
					#endif
				}
				// is there anything more to do before returning ?????????????
			}

			// (2.2.2) else
			else {
				// (2.2.2.1) goto next draw_order in current chunck and mark current as visited
				is_draw_order_visited[current] = true;
				current = next_draw_order_in_current_chunck;
			}
		}

		// (2.3) else
		else {
			// (2.3.1) goto next draw_order in next chunck and mark current as visited
			is_draw_order_visited[current] = true;
			current = next_draw_order_in_next_chunck;
		}

		// remember to mark current and other items as not visited !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	free(is_draw_order_visited);
	free(is_draw_order_valid);
}


//
void GUI_AddItemToIndicesInDrawOrder(GUI_Item_DrawOrder* draw_order_ptr, GUI_Item* adding_item_ptr) {

	// id the item to add is draw order then you have to add all indices in that draw order this asumes that the indices in the draw order which is being added is allready updated this iteration

	if (adding_item_ptr->type == DRAWORDER) {

		// resizeing if necessary
		if (draw_order_ptr->indices_size + adding_item_ptr->item.draw_order.indices.size >= draw_order_ptr->indices_capacity || draw_order_ptr->item_id_array == NULL) {
			if (draw_order_ptr->indices_capacity == 0) {
				draw_order_ptr->indices_capacity = 8;
			}
			while (draw_order_ptr->indices_size + adding_item_ptr->item.draw_order.indices.size >= draw_order_ptr->indices_capacity) {
				draw_order_ptr->indices_capacity *= 2;
			}
			debug(draw_order_ptr->item_id_array = alloc(draw_order_ptr->item_id_array, sizeof(unsigned int), draw_order_ptr->indices_capacity));
		}

		for (unsigned int i = 0; i < adding_item_ptr->item.draw_order.indices_size; i++) {
			// adding item to draw_order->indices
			draw_order_ptr->indices[draw_order_ptr->indices_size] = adding_item_ptr->item.draw_order.indices[i];
			draw_order_ptr->indices_size++;
		}

		return;
	}

	// resizeing if necessary
	if (draw_order_ptr->indices_size >= draw_order_ptr->indices_capacity
	 || draw_order_ptr->item_id_array == NULL) {
		if (draw_order_ptr->indices_capacity == 0) {
			draw_order_ptr->indices_capacity = 8;
		}
		while (draw_order_ptr->indices_size >= draw_order_ptr->indices_capacity) {
			draw_order_ptr->indices_capacity *= 2;
		}
		debug(draw_order_ptr->item_id_array = alloc(draw_order_ptr->item_id_array, sizeof(unsigned int), draw_order_ptr->indices_capacity));
	}

	// adding item to draw_order->indices
	draw_order_ptr->indices[draw_order_ptr->indices_size] = adding_item_ptr->vertex_location;
	draw_order_ptr->indices_size++;

}
void GUI_UpdateIndicesInDrawOrder(GUI_Item_DrawOrder* draw_order_ptr) {

	// where/when should you check if the item_id_array is changed?
	// should do it when draw_order->item_id_array is updated
	// but if one draw_order->item_id_array is changed then all other draw_orders which use it will also be changed. or the indices would be changed

	// (0) assuming indices in all draw orders within is updated this iteration

	// needs to wipe whole indices to then update the whole indices again
	// this must be changed later to identify if the prev indices is the same as the new ?????????????
	draw_order_ptr->indices_size = 0;

	for (unsigned int i = 0; i < item_ptr->item_id_array_size; i++) {
		GUI_Item* sub_item_ptr = GUI_GetItemPtr(item_ptr->item_id_array[i]);

		#ifdef DEBUG
		// if sub_item_ptr is WINDOW it would probably crash the program so just crash it now instead
		if (sub_item_ptr->type == WINDOW) {
			printf("design flaw: should not draw the window onto another target\n");
			printf("draw order:\n");
			GUI_Item_Print(item_ptr);
			printf("item inside draw order:\n");
			GUI_Item_Print(sub_item_ptr);
			exit(-1);
		}
		#endif

		// if the item to add is a draw_order then you have to add all indices in that draw_order.
		// this assumes that the indices in the draw order which is being added is allready updated this frame
		if (sub_item_ptr->type == DRAWORDER) {

			// resizeing if necessary
			if (draw_order_ptr->indices_size + sub_item_ptr->item.draw_order.indices.size >= draw_order_ptr->indices_capacity || draw_order_ptr->item_id_array == NULL) {
				if (draw_order_ptr->indices_capacity == 0) {
					draw_order_ptr->indices_capacity = 8;
				}
				while (draw_order_ptr->indices_size + sub_item_ptr->item.draw_order.indices.size >= draw_order_ptr->indices_capacity) {
					draw_order_ptr->indices_capacity *= 2;
				}
				debug(draw_order_ptr->item_id_array = alloc(draw_order_ptr->item_id_array, sizeof(unsigned int), draw_order_ptr->indices_capacity));
			}

			for (unsigned int i = 0; i < sub_item_ptr->item.draw_order.indices_size; i++) {
				// adding item to draw_order->indices
				draw_order_ptr->indices[draw_order_ptr->indices_size] = sub_item_ptr->item.draw_order.indices[i];
				draw_order_ptr->indices_size++;
			}

			continue;
		}

		// resizeing if necessary
		if (draw_order_ptr->indices_size >= draw_order_ptr->indices_capacity
		|| draw_order_ptr->item_id_array == NULL) {
			if (draw_order_ptr->indices_capacity == 0) {
				draw_order_ptr->indices_capacity = 8;
			}
			while (draw_order_ptr->indices_size >= draw_order_ptr->indices_capacity) {
				draw_order_ptr->indices_capacity *= 2;
			}
			debug(draw_order_ptr->item_id_array = alloc(draw_order_ptr->item_id_array, sizeof(unsigned int), draw_order_ptr->indices_capacity));
		}

		// adding item to draw_order->indices
		draw_order_ptr->indices[draw_order_ptr->indices_size] = sub_item_ptr->vertex_location;
		draw_order_ptr->indices_size++;
	}
}
void GUI_UpdateIndicesInAllVisibleDrawOrders() {

	// EXPLANATION:
	// this will update indices for all visible draw_orders
	// to do this correctly when draw_orders contain draw_orders the outmost draw_orders must be updated first
	// before this function is executed this frame, GUI_IsDrawTreeValid must be executed first

	// (0) asuming draw_tree_array is validated for this frame
	// (1) find every target in draw_tree_array
	// (1.1)

	for (unsigned int i = 0; i < draw_tree_array_size; i++) {
		GUI_Item* item_ptr = GUI_GetItemPtr(draw_tree_array[i]);
		if (item_ptr->type == WINDOW) {

		}
		else if (item_pt->type == RENDERTEXTURE) {

		}
	}

}

/*
void GUI_FindAllBranches() {

	// the items in draw_tree_array can be either WINDOW, RENDERTEXTURE, DRAWORDER or NONE. it is like a tree. the first index, aka WINDOW, is the root then it branckes upwards as there are multiple draw_orders and/or targets for each draw_order. Its important that a branch dont loops back into the tree.

	// draw_tree_array cannot be NULL
	if (draw_tree_array == NULL) {
		debug(draw_tree_array = alloc(draw_tree_array, DRAW_TREE_ARRAY_CAPACITY_START, sizeof(GUI_Item_ID)));
		draw_tree_array_capacity = DRAW_TREE_ARRAY_CAPACITY_START;
		draw_tree_array_size = 1; // 1 becasue window must be added anyways
		draw_tree_array[0] = 0; // id 0 is allways the window id!
	}

	// itterating through all found target's to find new targets
	for (unsigned int working_on_target = 0; working_on_target < draw_tree_array_size; working_on_target++) {

		// getting the current target_ptr in target array
		GUI_Item* target_ptr = GUI_GetItemPtr(draw_tree_array[working_on_target]);

		// getting the draw_order_ptr for the current target
		GUI_Item* draw_order_ptr;
		if (target_ptr->type == WINDOW)
			draw_order_ptr = GUI_GetItemPtr(target_ptr->item.window.draw_order_id);
 		if (target_ptr->type == RENDERTEXTURE)
			draw_order_ptr = GUI_GetItemPtr(target_ptr->item.render_texture.draw_order_id);

		// iterating through all items in draw order
		for (unsigned int item = 0; item < draw_order_ptr->item_id_array_size; item++) {

			// getting the current item in draw order
			GUI_Item* item_ptr = GUI_GetItemPtr(draw_order_ptr->item_id_array[item]);

			// is current item in draw order a target?
			if (item_ptr->type == WINDOW || item_ptr->type == RENDERTEXTURE) {

				// check that its not allreading in the array because if it is the the program must crash or not render this target
				for (unsigned int target = 0; target < draw_tree_array_size; target++) {
					if (item_ptr->id == GUI_GetItemPtr(all_visible_draw_order_id_array[target])->id) {
						printf("the same target ");
					}
				}

				// checking if draw_tree_array needs to be resized
				if (draw_tree_array_size >= draw_tree_array_capacity) {
					while (draw_tree_array_size >= draw_tree_array_capacity) {
						draw_tree_array_capacity *= 2;
					}
					debug(draw_tree_array = alloc(draw_tree_array, draw_tree_array_capacity, sizeof(GUI_Item_ID)));
				}

				// add the new target to draw_tree_array
				draw_tree_array[draw_tree_array_size] = item_ptr->id;
				draw_tree_array_size++;
			}
		}

		// adding item of type NONE for indicating that the branch has stopped
			// checking if draw_tree_array needs to be resized
			if (draw_tree_array_size >= draw_tree_array_capacity) {
				while (draw_tree_array_size >= draw_tree_array_capacity) {
					draw_tree_array_capacity *= 2;
				}
				debug(draw_tree_array = alloc(draw_tree_array, draw_tree_array_capacity, sizeof(GUI_Item_ID)));
			}

			// add the new target to draw_tree_array
			draw_tree_array[draw_tree_array_size] = item_ptr->id;
			draw_tree_array_size++;
	}
}
*/

// PUBLIC FUNCTIONS ==============================================================================================================================
GUI_Item_ID GUI_Get_WindowDrawOrder() {
	return item_array[1].id;
}
GUI_Item_ID GUI_Item_DrawOrder_Create() {
	GUI_Item_ID new_draw_order_id = GUI_Item_Create_Private(DRAWORDER, 0);

	GUI_Item* new_item_ptr 	= GUI_GetItemPtr(new_draw_order_id);
	new_item_ptr->changed 	= true;
	new_item_ptr->in_use 	= true;

	GUI_Item_DrawOrder* new_draw_order_ptr 		= &new_item_ptr->item.draw_order;
	new_draw_order_ptr->item_id_array 			= NULL;
	new_draw_order_ptr->item_id_array_size 		= 0;
	new_draw_order_ptr->item_id_array_capacity 	= 0;
	new_draw_order_ptr->indices 				= NULL;
	new_draw_order_ptr->indices_size 			= 0;
	new_draw_order_ptr->indices_capacity 		= 0;
	new_draw_order_ptr->EBO 					= 0;

	return new_draw_order_id;
}
void GUI_Item_DrawOrder_AddItem(GUI_Item_ID draw_order_id, GUI_Item_ID item_id) {

	GUI_Item* draw_order_item_ptr = GUI_GetItemPtr(draw_order_id);
	GUI_Item_DrawOrder* draw_order_ptr = &draw_order_item_ptr->item.draw_order;
	GUI_Item* item_ptr = GUI_GetItemPtr(item_id);

	if (draw_order_item_ptr->type != DRAWORDER) {
		printf("You tried to add item to a draw_order_item that isnt a draw_order\n");
		GUI_Item_Print(draw_order_item_ptr);
		exit(-1);
	}
	if (!draw_order_item_ptr->in_use) {
		printf("You tried to add item to a draw_order_item that isnt in use.\n");
		GUI_Item_Print(draw_order_item_ptr);
		exit(-1);
	}
	if (!item_ptr->in_use) {
		printf("You tried to add an item that isnt in use.\n");
		GUI_Item_Print(draw_order_item_ptr);
		exit(-1);
	}
	if (draw_order_ptr->item_id_array_size > draw_order_ptr->item_id_array_capacity) {
		printf("given draw_order is corrupt. this should never be true: item_id_array_size > item_id_array_capacity.\n");
		GUI_Item_Print(draw_order_item_ptr);
		exit(-1);
	}
	if (draw_order_ptr->indices_size > draw_order_ptr->indices_capacity) {
		printf("given draw_order is corrupt. this should never be true: indices_size > indices_capacity.\n");
		GUI_Item_Print(draw_order_item_ptr);
		exit(-1);
	}

	// ensuring that item_id_array is large enough
	if (draw_order_ptr->item_id_array == NULL || draw_order_ptr->item_id_array_size == draw_order_ptr->item_id_array_capacity) {
		size_t prev_capacity = draw_order_ptr->item_id_array_capacity;
		size_t new_capacity = 2*draw_order_ptr->item_id_array_capacity;
		if (draw_order_ptr->item_id_array == NULL) {
			prev_capacity = 0;
			new_capacity = 32;
		}
		// resizing item_id_array
		debug(draw_order_ptr->item_id_array = alloc(draw_order_ptr->item_id_array, new_capacity, sizeof(GUI_Item_ID)));

		// setting all new unused id's to 0xFFFFFFFF
		for (unsigned int i = prev_capacity; i < new_capacity; i++) {
			draw_order_ptr->item_id_array[i] = 0xFFFFFFFF; // the NO_ID id
		}
	}

	// setting new item
	draw_order_ptr->item_id_array[draw_order_ptr->item_id_array_size] = item_id;
	draw_order_ptr->item_id_array_size++;
	draw_order_ptr->indices_needsUpdate = true;

	// ALL OF THIS IS RATHER DONE IN THE GUI_UpdateEverythingNeededForDrawing
	/*
		size_t start_index = item_ptr->vertex_location;
		size_t end_index = start_index+1;
		size_t prev_vertices_size = draw_order_ptr->indices_size;
		if (item_ptr->id == TEXT) {
			end_index = start_index + text_array[item_ptr->item.text.text_location].number_of_available_vertices;
		}

		// figuring out what the new_capacity is. most times its the same as previously
		size_t prev_capacity = draw_order_ptr->indices_capacity;
		size_t new_capacity = draw_order_ptr->indices_capacity == 0 ? 8 : draw_order_ptr->indices_capacity;
		while (draw_order_ptr->indices_size+(end_index-start_index) >= new_capacity) {
			new_capacity = 2*new_capacity;
		}

		// If resizing is needed, because capacity is increased
		if (draw_order_ptr->indices == NULL || new_capacity > prev_capacity) {

			// resizing indices to new capacity
			debug(draw_order_ptr->indices = alloc(draw_order_ptr->indices, new_capacity, sizeof(unsigned int)));
			draw_order_ptr->EBO_needsResize = true;

			// setting all new indices to 0xFFFFFFFF
			for (unsigned int i = prev_capacity; i < new_capacity; i++) {
				draw_order_ptr->indices[i] = 0xFFFFFFFF; // the NO_INDEX index
			}

			// writing the new item's vertex location. for text_item there are more than one vertex location, therefor there is a for loop here.
			for (unsigned int i = 0; i < end_index-start_index; i++) {
				draw_order_ptr->indices[draw_order_ptr->indices_size] = start_index+i;
				draw_order_ptr->indices_size++;
			}

			// resizing EBO
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_order_ptr->EBO);
			debug(glBufferData(GL_ELEMENT_ARRAY_BUFFER, new_capacity*sizeof(unsigned int), draw_order_ptr->indices, GL_DYNAMIC_DRAW));
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			draw_order_ptr->indices_needsUpdate = true;

			return;
		}

		// If no resizing is needed
		// writing the newly added item's vertex location. for text_item there are more than one vertex location, therefor there is a for loop here.
		for (unsigned int i = 0; i < end_index-start_index; i++) {
			draw_order_ptr->indices[i+draw_order_ptr->indices_size] = start_index+i;
		}

		draw_order_ptr->indices_capacity = new_capacity;
		draw_order_ptr->indices_needsUpdate = true;

		// writing new vertex location for new item to EBO
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_order_ptr->EBO);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, prev_vertices_size, (end_index-start_index)*sizeof(unsigned int), &draw_order_ptr->indices[prev_vertices_size]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	*/
}
void GUI_Item_DrawOrder_RemoveItem(GUI_Item_ID draw_order_id, GUI_Item_ID item_id) {
	GUI_Item_DrawOrder* draw_order_ptr = &GUI_GetItemPtr(draw_order_id)->item.draw_order;
	for (unsigned int i = 0; i < draw_order_ptr->item_id_array_size; i++) {
		// if item i is not given item_id just skip to next iteration
		if (draw_order_ptr->item_id_array[i] != item_id) {
			continue;
		}
		// if item i is the given item_id
		for (unsigned int j = i+1; j < draw_order_ptr->item_id_array_size; j++) {
			draw_order_ptr->item_id_array[j-1] = draw_order_ptr->item_id_array[j];
		}
		if (draw_order_ptr->item_id_array_size >= 1) {
			draw_order_ptr->item_id_array_size-=1;
		}
		break;
	}
	draw_order_ptr->indices_needsUpdate = true;
}
void GUI_Item_DrawOrder_ReplaceItem(GUI_Item_ID draw_order_id, unsigned int index, GUI_Item_ID replace_with_item_id) {

	GUI_Item* replace_with_item_ptr = GUI_GetItemPtr(draw_order_id);
	if (!replace_with_item_ptr->in_use) {
		printf("user flaw: the given item to be replacing with is not in use\n");
		GUI_Item_Print(replace_with_item_ptr);
		exit(-1);
	}

	GUI_Item_DrawOrder* draw_order_ptr = &(GUI_GetItemPtr(draw_order_id)->item.draw_order);
	if (index >= draw_order_ptr->item_id_array_size) {
		printf("user flaw: the index to replcae in given draw order is out of bounds\n");
		GUI_Item_Print(GUI_GetItemPtr(draw_order_id));
		exit(-1);
	}

	draw_order_ptr->item_id_array[index] = replace_with_item_id;
	draw_order_ptr->indices_needsUpdate = true;
}
void GUI_Item_Normal_SetValues(GUI_Item_ID item_id, int x, int y, int w, int h, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, float rotation_360, unsigned int corner_radius_pixels){
	GUI_Item* item_ptr = GUI_GetItemPtr(item_id);

	if (item_ptr->type != NORMAL) {
		printf("GUI_Item_Normal_SetValues is not meant to be applied to any other item than NORMAL items!\n");
		GUI_Item_Print(item_ptr);
		exit(-1);
	}

	item_ptr->changed = true;
	GUI_Item_Normal* normal_ptr = &item_ptr->item.normal;
	normal_ptr->rect[0] = x;
	normal_ptr->rect[1] = y;
	normal_ptr->rect[2] = w;
	normal_ptr->rect[3] = h;
	normal_ptr->color[0] = red;
	normal_ptr->color[1] = green;
	normal_ptr->color[2] = blue;
	normal_ptr->color[3] = alpha;
	normal_ptr->rotation_360 = rotation_360;
	normal_ptr->corner_radius_pixels = corner_radius_pixels;

	GUI_WriteFromItemToVertex(item_ptr);
}
GUI_Item_ID GUI_Item_Normal_Create(int x, int y, int w, int h, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, GUI_Item_ID draw_order_id) {

	GUI_Item_ID new_item_id = GUI_Item_Create_Private(NORMAL, 0);

	GUI_Item_Normal_SetValues(new_item_id, x,y,w,h,red,green,blue,alpha,0.0f,10);

	GUI_Item_DrawOrder_AddItem(draw_order_id, new_item_id);

	return new_item_id;
}
void GUI_Create(unsigned int width, unsigned int height, const char* title, unsigned int FPS) {
	window = GUI_CreateWindow(width, height, title);

	// setting up window_item
	debug(GUI_Item_ID window_item = GUI_Item_Create_Private(WINDOW, 0));
	debug(GUI_Item_Window* window_ptr = &GUI_GetItemPtr(window_item)->item.window);
	window_ptr->width  = width;
	window_ptr->height = height;

	// setting up first draw_order_item
	debug(GUI_Item_ID window_draw_order_item = GUI_Item_DrawOrder_Create());
	window_ptr->draw_order_id = window_draw_order_item;


	// testing items creation and setup
	debug(GUI_Item_Normal_Create(10,10,200,100,200,64,0,255,GUI_Get_WindowDrawOrder()));




	GLuint shaders[3];
    printf("vertex: \n"); debug(shaders[0] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_vertex_shader.glsl"), GL_VERTEX_SHADER));
	printf("geometry: \n"); debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_geometry_shader.glsl"), GL_GEOMETRY_SHADER));
    printf("fragment: \n"); debug(shaders[2] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_fragment_shader.glsl"), GL_FRAGMENT_SHADER));
	shader_program = GUI_CreateProgram(shaders, 3);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);
 	glDeleteShader(shaders[2]);

	// Generate and bind a Vertex Array Object
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// pos attribute
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 1, GL_INT, sizeof(Vertex), (void*)(0*sizeof(int)));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_INT, sizeof(Vertex), (void*)(1*sizeof(int)));
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_INT, sizeof(Vertex), (void*)(2*sizeof(int)));
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_INT, sizeof(Vertex), (void*)(3*sizeof(int)));
	// tex_rect attribute
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(4*sizeof(float)));
	// color attribute
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)(8*sizeof(float)));
	// rotation_360 attribute
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(9*sizeof(float)));
	// corner_radius and tex_index attribute
	glEnableVertexAttribArray(7);
	glVertexAttribIPointer(7, 1, GL_UNSIGNED_INT,  sizeof(Vertex), (void*)(10*sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	target_width_loc = glGetUniformLocation(shader_program, "target_width");
	target_height_loc = glGetUniformLocation(shader_program, "target_height");

	printf("\n\n");
	printf("size of GUI_Item = %d\n", sizeof(GUI_Item));
	printf("size of GUI_Item_Union = %d\n\n", sizeof(GUI_Item_Union));

	GUI_PrintAllInfo();
}
void GUI_Create_0(unsigned int width, unsigned int height, const char* title, unsigned int FPS) {
	window = GUI_CreateWindow(width, height, title);



	debug(GUI_Item_ID window_item = GUI_Item_Create_Private(WINDOW, 0););
	debug(GUI_Item_ID window_draw_order_item = GUI_Item_Create_Private(DRAWORDER, 0););
	debug(GUI_Item_Normal_Create(10,10,200,100,200,64,0,255,GUI_Get_WindowDrawOrder()););
	GUI_PrintAllInfo();

	GLuint shaders[3];
    printf("vertex: \n"); debug(shaders[0] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_vertex_shader.glsl"), GL_VERTEX_SHADER););
	printf("geometry: \n"); debug(shaders[1] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_geometry_shader.glsl"), GL_GEOMETRY_SHADER););
    printf("fragment: \n"); debug(shaders[2] = GUI_CompileShader(GUI_ReadShaderSource("shaders/uber_fragment_shader.glsl"), GL_FRAGMENT_SHADER););
	shader_program = GUI_CreateProgram(shaders, 3);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);
 	glDeleteShader(shaders[2]);

	Vertex vertices2[] = {
		{{120, 350, 170, 180}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFFFFFF, 30.0f, 0x00060000},
		{{100, 270, 100, 100}, {0.0f,0.0f,1.0f,1.0f}, 0xFFFF00FF, -60.0f, 0x00320000},
		{{100, 300, 300, 100}, {0.0f,0.0f,1.0f,1.0f}, 0x0000FF80, 0.0f, 0x00060000},
		{{250, 130, 300, 200}, {0.0f,0.0f,1.0f,1.0f}, 0xFF000080, 70.0f, 0x00060000}
	};

	unsigned int indices[] = {2, 3, 0, 1};

	// Generate and bind a Vertex Array Object
	GLuint EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// pos attribute
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 1, GL_INT, sizeof(Vertex), (void*)(0*sizeof(int)));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(1, 1, GL_INT, sizeof(Vertex), (void*)(1*sizeof(int)));
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(2, 1, GL_INT, sizeof(Vertex), (void*)(2*sizeof(int)));
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_INT, sizeof(Vertex), (void*)(3*sizeof(int)));
	// tex_rect attribute
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(4*sizeof(float)));
	// color attribute
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)(8*sizeof(float)));
	// rotation_360 attribute
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(9*sizeof(float)));
	// corner_radius and tex_index attribute
	glEnableVertexAttribArray(7);
	glVertexAttribIPointer(7, 1, GL_UNSIGNED_INT,  sizeof(Vertex), (void*)(10*sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	glDeleteBuffers(1, &EBO);
}
void GUI_Show() {
	while (!glfwWindowShouldClose(window)) {

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}
		debug(GUI_PrintFPS(window));

		// 0) OK: find all targets
		// youve got draw_tree_order now
		// 1) every item that will be drawn must be written to indices in its draw_order_item
		//	  then indices in draw_order_item must by written to EBO in draw_order_item
		// 2) all vertices that will be drawn must be written to VBO
		// 3) if VBO is resized then the attributes must be updated
		// 4) draw onto all visible targets

		// the right EBO must be used and it must be populated correctly

		debug(GUI_DrawOntoTarget(0));

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
void GUI_Delete() {
	// Clean up
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shader_program);

	// Terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();
}

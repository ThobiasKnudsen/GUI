#include "gui.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define DEBUG
#include "debug.h"
#include "array.h"



// TYPES
{
    // VERTEX
    typedef struct {
        int             rect[4];
        float           tex_rect[4];
        unsigned int    color;
        float           rotation_360;
        unsigned int    multiple_values; //corner_radius and tex_index
        bool            in_use;
        bool            VBO_upToDate;
        unsigned char   padding[2];
    } Vertex; // 48 bytes (3 bytes padding)

    // TEXTURE ATLAS
    typedef struct {
        GLuint          texture_atlas;
        unsigned int    width;
        unsigned int    height;
        unsigned int    padding;

        Array           rect_per_texture;
        Array           id_per_texture;

        size_t          texture_count;
    } TextureAtlas;

    // GUI_STATICIMAGE
    typedef struct {
        unsigned int texture_atlas_index;
        unsigned int texture_id;
    } GUI_StaticImage;

    // GUI_DYNAMICIMAGE
    typedef struct {
        GLuint texture;
        unsigned int width;
        unsigned int height;

    } GUI_DynamicImage;

    // GUI_SHAPE 28 bytes
    typedef struct {
        unsigned int    rect[4];
        unsigned char   color[4];
        float           rotation_360;
        unsigned int    corner_radius_pixels;
    } GUI_Shape;

    // GUI_SEQUENCE
    typedef struct {

    } GUI_Sequence;
}


static GLFWwindow* 	window = NULL;
static GLuint       FBO = 0;

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

void GUI_RenderToTexture(GUI_Texture_Id texture_id, GUI_Sequence_Id) {

    // Update VAO
    {
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
    }
}

void GUI_Initialize(unsigned int width, unsigned int height, const char* title) {
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

	target_width_loc = glGetUniformLocation(shader_program, "target_width");
	target_height_loc = glGetUniformLocation(shader_program, "target_height");

	GUI_PrintAllInfo();
}

void GUI_RenderLoop {
    while (!glfwWindowShouldClose(window)) {

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}
		debug(GUI_PrintFPS(window));


        // PrepareForRendering
        {
             FindDrawTree
            ValidateDrawTree_And_SetIndicesInSequences
            SetEBOinSequences                           | ValidateDrawTree_And_SetIndicesInSequences
            ResetIndicesNeedsUpdateInSequences          | SetEBOinSequences
            UpdateVerticesInSeqeunces                   | ValidateDrawTree_And_SetIndicesInSequences
            Update                                      | UpdateVerticesInSeqeunces
        }
        // Rendering
        {

        }
        // UserEvents
        {

        }
        // ReactionToUserEvents
        {

        }


		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

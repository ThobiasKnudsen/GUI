#ifndef GUI_ITEM_DATATYPES_H
#define GUI_ITEM_DATATYPES_H

#include "GUI.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// PRIVATE =======================================================================================================================================

typedef struct {
	int 		rect[4];
	float 		tex_rect[4];
	unsigned int 	color;
	float 		rotation_360;
	unsigned int 	multiple_values; //corner_radius and tex_index
	bool		in_use;
	unsigned char 	padding[3];
} PrivateVertex; // 48 bytes (3 bytes padding)

typedef struct {
	// idea here is that every letter has the same width	
	char* 		symbol_array;
	unsigned char* 	symbol_color_id_array; 
	bool*  		symbol_visibility_array;
	size_t		symbol_array_length;
	size_t		symbol_array_capacity;
	
	unsigned int 	text_pixel_width; // automatically calculated based on longest_line, line_count, tab_size, symbol_pixel_width -and height
	unsigned int 	text_pixel_height;
	
	unsigned short	symbol_pixel_width;	
	unsigned short 	symbol_pixel_height;
	
	unsigned char*  colors_array; // index 0 1 2 3 is r g b a relatively, so max size is of this array is 4*256=1024
	unsigned char 	colors_count; 
	
	bool		in_use; // DOESNT FIT IN HERE EXEPT THAT IT FILLS AN EMPTY BYTE
	
	unsigned char 	tab_size;
	unsigned int	current_writing_index;
	unsigned int 	sellecting_text_start;
	unsigned int 	longest_line;
	unsigned int	line_count;
	
	unsigned int 	symbol_spacing;
	unsigned int 	line_spacing;
	
	unsigned int 	number_of_available_vertices; // THIS ONE IS NEW AND IMPORTANT
	
} PrivateText;

// PUBLIC ========================================================================================================================================

struct GUI_Item_Window {
	unsigned int 	width;
	unsigned int 	height;
	unsigned int 	previous_DrawOrder_id; // as long as the new draworder is the same as previous and all items in this draw order is unchanged then this is not redrawn
};
struct GUI_Item_Normal {
	int 		rect[4];
	unsigned char 	color[4];
	float 		rotation_360;
	unsigned int 	corner_radius_pixels;
};
struct GUI_Item_DrawOrder {
	unsigned int* 	item_id_list;
	unsigned int 	item_id_list_size;
};
struct GUI_Item_Text {
	unsigned int 	text_location; // this item needs its own struct so that it doesnt make GUI_Item size too large
};
struct GUI_Item_RenderTexture {
	int 		external_rect[4];
	int 		internal_width;
	int	 	internal_height;
	unsigned int 	previous_DrawOrder_id; // as long as the new draworder is the same as previous and all items in this draw order is unchanged then this is not redrawn
	GLuint 		texture;
	GLuint 		frame_buffer;
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
    GUI_Item_Normal		normal;
    GUI_Item_DrawOrder		draw_order;
    GUI_Item_Text      		text;
    GUI_Item_RenderTexture   	render_texture;
    GUI_Item_Custom 		custom;
};

struct GUI_Item {
	GUI_Item_ID 		id;
	GUI_Item_Type 		type;
	unsigned int 		vertex_location;
	bool 			changed;
	bool 			in_use;
	GUI_Item_Union 		item;
};


#endif

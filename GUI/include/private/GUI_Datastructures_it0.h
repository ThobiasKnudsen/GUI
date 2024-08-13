#ifndef GUI_ITEM_DATATYPES_H
#define GUI_ITEM_DATATYPES_H

#include "GUI.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct GUI_Item_Root {
	unsigned int 	width;
	unsigned int 	height;
	const char* 	title;
	GLuint 		EBO;
	unsigned int 	items_size;
};
struct GUI_Item_Normal {
	unsigned int rect[4];
	unsigned char color[4];
};
struct GUI_Item_Prolific {
	unsigned int rect[4];
	unsigned char color[4];
	unsigned int* children_id_list;
	unsigned int children_id_list_size;
};

struct GUI_Item_Text {
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
};
struct GUI_Item_Frame {
	unsigned int temp;
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
	
	void (*CustomFunction)(void* custom_data_ptr);
};

union GUI_Item_Union {
    GUI_Item_Root    		root;
    GUI_Item_Normal		normal;
    GUI_Item_Prolific		prolific;
    GUI_Item_Text      		text;
    GUI_Item_Frame   		frame;
    GUI_Item_Custom 		custom;
};

struct GUI_Item {
	unsigned int 	id;
	GUI_Item_Type 	type;
	unsigned int 	parent; // every item has parent exept root. root.parent == 0 and whenever you get that a parent is 0 then you must also check if the id is 0
	unsigned int 	VBO_index;
	bool 		is_updated;
	GUI_Item_Union item;
};


#endif

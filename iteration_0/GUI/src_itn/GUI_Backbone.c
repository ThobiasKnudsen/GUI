
#include <stdbool.h>

#include "private/GUI_Backbone.h"
#include "private/GUI_Datastructures.h"
#include "private/GUI_GLFW_Simplified.h"
#define DEBUG
#include "private/debug.h"




// PRIVATE ====================================================================================================================================

// CPU ========================================================================================================================================

static GLFWwindow* 	private_window = NULL;
static GUI_Item* 	private_item_array = NULL;
static size_t		private_item_array_size = 0;
static size_t		private_item_array_capacity = 0;
static size_t 		private_ID_count = 0;
#define PRIVATE_ITEM_ARRAY_CAPACITY_START 32

static PrivateVertex*	private_vertices = NULL;
static size_t 		private_vertices_size = 0;
static size_t 		private_vertices_capacity = 0;
#define PRIVATE_VERTICES_CAPACITY_START 256

static PrivateText* 	private_text_array = NULL;
static size_t		private_text_array_size = 0;
static size_t		private_text_array_capacity = 0;
#define PRIVATE_TEXT_ARRAY_CAPACITY_START 8

// GPU =========================================================================================================================================

static GLuint 		private_shader_program;
static GLuint		private_VBO;
static GLuint		private_VAO;

// FUNCTIONS ===================================================================================================================================


void private_GUI_Item_Print(GUI_Item* item_ptr) {
    printf("id = %d\n", item_ptr->id);
    printf("type = %d\n", item_ptr->type);
    printf("vertex_location = %d\n", item_ptr->vertex_location);
    printf("inuse = %d\n", item_ptr->in_use);
}
void private_GUI_PrintItems(GUI_Item* window_item_ptr) {
    for (unsigned int i = 0; i < private_item_array_size; i++) {
        debug(GUI_Item_Print(&private_item_array[i]););
    }
}
GUI_Item* private_GUI_GetItemPtr(GUI_Item_ID item_id) {
	for (unsigned int i = 0; i < private_item_array_size; i++) {
		if (private_item_array[i].id == item_id) {
			return &private_item_array[i];
		}
	}
	printf("there is no item with id %d\n", item_id);
	exit(-1);
}


GUI_Item_ID private_GUI_FindItemLocation() {

	for (unsigned int i = 0; i < private_item_array_size; i++) {
		if (!private_item_array[i].in_use) {
			private_item_array[i].id = private_ID_count;
			private_item_array[i].type = NONE;
			private_item_array[i].in_use = true;
			private_ID_count++;
			return private_item_array[i].id;
		}
	}
	
	if (private_item_array == NULL || private_item_array_size >= private_item_array_capacity) {
		size_t prev_capacity = private_item_array_size;
		size_t new_capacity = 2*prev_capacity;
		if (private_item_array == NULL) {
			prev_capacity = 0;
			new_capacity = PRIVATE_ITEM_ARRAY_CAPACITY_START;
		}
		private_item_array = alloc(private_item_array, new_capacity);
		private_item_array_capacity = new_capacity;
		for (unsigned int i = prev_capacity; i < new_capacity; i++) {
			private_item_array[i].type = NONE;
			private_item_array[i].vertex_location = 0;
			private_item_array[i].in_use = false;
		}
		private_item_array[prev_capacity].id = private_ID_count;
		private_item_array[prev_capacity].type = NONE;
		private_item_array[prev_capacity].in_use = true;
		private_ID_count++;
		private_item_array_size++;
		return private_item_array[prev_capacity].id;
	}
	
	private_item_array[private_item_array_size].id = private_ID_count;
	private_item_array[private_item_array_size].type = NONE;
	private_item_array[private_item_array_size].in_use = true;
	private_ID_count++;
	private_item_array_size++;
	return private_ID_count-1;
}

void private_GUI_FindVertexLocation(GUI_Item_ID item_id) {
	GUI_Item* item_ptr = private_GUI_GetItemPtr(item_id);
	if (item_ptr->type == TEXT) {
		printf("You should not use this function for finding text allocation. Use private_GUI_FindVertexAndTextLocation instead\n");
		private_GUI_Item_Print(item_ptr);
		exit(-1);
	}
	if (item_ptr->type == DRAWORDER) {
		printf("Draw_order_item dont need vertex location, because it is never drawn itself!\n");
		private_GUI_Item_Print(item_ptr);
		exit(-1);
	}
	if (item_ptr->type == WINDOW) {
		// ?????????????????????????????????????????????????????????????????????????????????
		// CAN ONLY BE ONE WINDOW
		if (private_vertices != NULL) {
			if (private_vertices[0].in_use) {
				printf("You cant make a second window. there is only one window!\n");
				private_GUI_Item_Print(item_ptr);
				exit(-1);
			}
		}
	}
	if (item_ptr->vertex_location != 0) {
		printf("You have allready found vertex location for this item\n");
		private_GUI_Item_Print(item_ptr);
		exit(-1);
	}
	
	for (unsigned int i = 0; i < private_vertices_size; i++) {
		if (!private_vertices[i].in_use) {
			item_ptr->vertex_location = i;
			private_vertices[i].in_use = true;
			return;
		}
	}
	if (private_vertices == NULL || private_vertices_size >= private_vertices_capacity) {
		size_t prev_capacity = private_vertices_capacity;
		size_t new_capacity = 2*prev_capacity;
		if (private_vertices == NULL) {
			prev_capacity = 0;
			new_capacity = PRIVATE_VERTICES_CAPACITY_START;
		}
		private_vertices = alloc(private_vertices, new_capacity);
		private_vertices_capacity = new_capacity;
		for (unsigned int i = prev_capacity; i < new_capacity; i++) {
			private_vertices[i].in_use = false;
		}
		item_ptr->vertex_location = prev_capacity;
		private_vertices[prev_capacity].in_use = true;
		private_vertices_size++;
		return;
	}
	item_ptr->vertex_location = private_vertices_size;
	private_vertices[private_vertices_size].in_use = true;
	private_vertices_size++;
	return;
}

void private_GUI_FindVertexAndTextLocation(GUI_Item_ID item_id, unsigned int number_of_available_vertices) {
	GUI_Item* item_ptr = private_GUI_GetItemPtr(item_id);
	if (item_ptr->type != TEXT) {
		printf("You tried to find text location for item that isnt text\n");
		private_GUI_Item_Print(item_ptr);
		exit(-1);
	}
	
	if (item_ptr->vertex_location != 0) {
		printf("You have allready found vertex and text location for this item\n");
		private_GUI_Item_Print(item_ptr);
		exit(-1);
	}
	
	for (unsigned int i = 0; i < private_text_array_size; i++) {
		if (!private_text_array[i].in_use) {
			item_ptr->item.text.text_location = i;
			private_text_array[i].number_of_available_vertices = number_of_available_vertices;
			private_text_array[i].in_use = true;
			goto found_text_location;
		}
	}
	if (private_text_array == NULL || private_text_array_size >= private_text_array_capacity) {
		size_t prev_capacity = private_text_array_capacity;
		size_t new_capacity = 2*prev_capacity;
		if (private_text_array == NULL) {
			prev_capacity = 0;
			new_capacity = PRIVATE_TEXT_ARRAY_CAPACITY_START;
		}
		private_text_array = alloc(private_text_array, new_capacity);
		private_text_array_capacity = new_capacity;
		for (unsigned int i = prev_capacity; i < new_capacity; i++) {
			private_text_array[i].in_use = false;
		}
		item_ptr->item.text.text_location = prev_capacity;
		private_text_array[prev_capacity].number_of_available_vertices = number_of_available_vertices;
		private_text_array[prev_capacity].in_use = true;
		private_text_array_size++;
		goto found_text_location;
	}
	item_ptr->item.text.text_location = private_text_array_size;
	private_text_array[private_text_array_size].number_of_available_vertices = number_of_available_vertices;
	private_text_array[private_text_array_size].in_use = true;
	private_text_array_size++;
	
	found_text_location: // GOTO
	
	// see if there is freed up vertices where text can be located in one chain =========================================================
	unsigned int consecutive_vertices = 0;
	for (unsigned int i = 0; i < private_vertices_size; i++) {
		// if vertex is free ========================================================================================================
		if (!private_text_array[i].in_use) {
			consecutive_vertices++;
			// if found long enough vertex chain ================================================================================
			if (consecutive_vertices == number_of_available_vertices) {
				item_ptr->vertex_location = i - consecutive_vertices + 1;
				// marking vertex chain as currently in use =================================================================
				for (unsigned int i = item_ptr->vertex_location; i < number_of_available_vertices; i++) {
					private_vertices[i].in_use = true;
				}
				return;
			}
		}
		// resets vertex chain length to 0 ==========================================================================================
		else {
			consecutive_vertices = 0;
		}
	}
	// increasing size of private_vertices until text fits inside =======================================================================
	size_t prev_capacity = private_vertices_capacity;
	while (private_vertices_size + number_of_available_vertices >= private_vertices_capacity) {
		if (private_vertices == NULL) {
			printf("text should not be the first item created. The GUI design is flawed!\n");
			private_GUI_Item_Print(item_ptr);
			exit(-1);
		}
		private_vertices = alloc(private_vertices, 2*private_vertices_capacity);
		private_vertices_capacity *= 2;
	}
	// marking all newly allocated vertices as not in use ===============================================================================
	for (unsigned int i = prev_capacity; i < private_vertices_capacity; i++) {
		private_vertices[i].in_use = false;
	}
	item_ptr->vertex_location = private_vertices_size;
	private_vertices_size += number_of_available_vertices;
	// marking vertex chain as currently in use =========================================================================================
	for (unsigned int i = item_ptr->vertex_location; i < private_vertices_size; i++) {
		private_vertices[i].in_use = true;
	}
}



GUI_Item_ID private_GUI_Item_Create(GUI_Item_Type item_type, unsigned int number_of_available_vertices_for_type_text_only) {
	GUI_Item_ID item_id = private_GUI_FindItemLocation();
	GUI_Item* item_ptr = private_GUI_GetItemPtr(item_id);
	item_ptr->type = item_type;
	if (item_type == DRAWORDER) {
		return item_id;
	}
	if (item_type == TEXT) {
		private_GUI_FindVertexAndTextLocation(item_id, number_of_available_vertices_for_type_text_only);
		return item_id;
	}
	private_GUI_FindVertexLocation(item_id);		
	return item_id;
}





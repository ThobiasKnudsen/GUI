#include "gui.h"

#define DEBUG
#include "debug.h"

// GOAL:
// make rendering a part of sequences
// FBO static image and dynamic image. texture atlas

// when should you use float or int for rect

// implement mipmap option for images

// sequence does not support function yet

// 		FIRST PRIORITY: MAKE DYNAMIC_IMAGE STATIC_IMAGE AND RENDERING WORK

ID Sequence_1() {

	debug(ID sequence_1 = GUI_Sequence_Create());
	debug(ID shape_1 = GUI_Shape_Create(10,10,100,100,255,255,255,255,0.0f,10,NO_ID));
	debug(ID shape_2 = GUI_Shape_Create(120,10,200,100,255,255,0,255,0.0f,10,NO_ID));
	debug(GUI_Sequence_AddID(sequence_1, shape_1));
	debug(GUI_Sequence_AddID(sequence_1, shape_2));

	debug(ID sequence_2 = GUI_Sequence_Create());
	debug(ID shape_3 = GUI_Shape_Create(10,210,100,100,255,0,255,255,0.0f,10,NO_ID));
	debug(ID shape_4 = GUI_Shape_Create(120,210,100,200,255,0,0,255,0.0f,10,NO_ID));
	debug(GUI_Sequence_AddID(sequence_2, shape_3));
	debug(GUI_Sequence_AddID(sequence_2, shape_4));
	debug(GUI_Sequence_AddID(sequence_1, sequence_2));

	return sequence_1;
}

ID Sequence_2() {

	int x = 300;
	int y = 300;

	debug(ID sequence_1 = GUI_Sequence_Create());
	debug(GUI_Sequence_AddID(sequence_1, GUI_Shape_Create(x+10,y+10,100,100,255,255,255,255,0.0f,10,NO_ID)));
	debug(GUI_Sequence_AddID(sequence_1, GUI_Shape_Create(x+120,y+10,200,100,255,255,0,255,0.0f,10,NO_ID)));
	//debug(ID rendering_1 = GUI_Rendering_Create(NO_ID, sequence_1));
	debug(ID sequence_2 = GUI_Sequence_Create());
	debug(GUI_Sequence_AddID(sequence_2, GUI_Shape_Create(x+10,y+210,100,100,255,0,255,255,0.0f,10,NO_ID)));
	debug(GUI_Sequence_AddID(sequence_2, GUI_Shape_Create(x+120,y+210,100,200,255,0,0,255,0.0f,10,NO_ID)));
	debug(GUI_Sequence_AddID(sequence_1, sequence_2));

	return sequence_1;
}

int main() {

	debug(GUI_Window_Create(600, 600, "BTC - Better Than CAS", 60));
	debug(GUI_AddItemToWindow(Sequence_1()));
	debug(GUI_AddItemToWindow(Sequence_2()));
	debug(GUI_Window_Show());
	debug(GUI_Window_Delete());

	return 0;
}



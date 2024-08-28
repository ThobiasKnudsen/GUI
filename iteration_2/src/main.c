#include "gui.h"

#define DEBUG
#include "debug.h"

// GOAL:
// make rendering a part of groups
// FBO static image and dynamic image. texture atlas

// when should you use float or int for rect

// implement mipmap option for images

// group does not support function yet

// 		FIRST PRIORITY: MAKE DYNAMIC_IMAGE STATIC_IMAGE AND RENDERING WORK

ID Group_1() {

	debug(ID group_1 = GUI_Group_Create());
	debug(ID shape_1 = GUI_Shape_Create(10,10,100,100,255,255,255,255,45.0f,10,NO_ID));
	debug(ID shape_2 = GUI_Shape_Create(120,10,200,100,255,255,0,255,0.0f,10,NO_ID));
	debug(GUI_Group_AddID(group_1, shape_1));
	debug(GUI_Group_AddID(group_1, shape_2));

	debug(ID group_2 = GUI_Group_Create());
	debug(GUI_Group_AddID(group_2, GUI_Shape_Create(10,210,100,100,255,0,255,255,0.0f,10,NO_ID)));
	debug(GUI_Group_AddID(group_2, GUI_Shape_Create(120,210,100,200,255,0,0,255,0.0f,10,NO_ID)));
	debug(GUI_Group_AddID(group_2, GUI_Shape_Create(120,210,100,200,255,0,0,255,90.0f,10,NO_ID)));
	debug(GUI_Group_AddID(group_1, group_2));

	return group_1;
}

ID Group_2() {

	int x = 300;
	int y = 300;

	debug(ID group_1 = GUI_Group_Create());
	debug(GUI_Group_AddID(group_1, GUI_Shape_Create(x+10,y+10,100,100,255,255,255,255,0.0f,10,NO_ID)));
	debug(GUI_Group_AddID(group_1, GUI_Shape_Create(x+120,y+10,200,100,255,255,0,255,0.0f,10,NO_ID)));
	//debug(ID rendering_1 = GUI_Rendering_Create(NO_ID, group_1));
	debug(ID group_2 = GUI_Group_Create());
	debug(GUI_Group_AddID(group_2, GUI_Shape_Create(x+10,y+210,100,100,255,0,255,255,32.0f,10,NO_ID)));
	debug(GUI_Group_AddID(group_2, GUI_Shape_Create(x+120,y+110,400,200,255,0,0,100,12.0f,10,NO_ID)));
	debug(ID group_3 = GUI_Group_Create());
	debug(GUI_Group_AddID(group_3, GUI_Shape_Create(x+10,-y+210,100,100,255,0,255,255,0.0f,10,NO_ID)));
	debug(GUI_Group_AddID(group_3, GUI_Shape_Create(x+120,y+10,10,200,255,0,0,255,0.0f,10,NO_ID)));
	debug(ID group_4 = GUI_Group_Create());
	debug(GUI_Group_AddID(group_4, GUI_Shape_Create(x+10,y+410,100,100,255,0,255,255,0.0f,10,NO_ID)));
	debug(GUI_Group_AddID(group_4, GUI_Shape_Create(x+420,210,100,200,255,0,0,255,0.0f,10,NO_ID)));

	debug(GUI_Group_AddID(group_1, group_2));
	debug(GUI_Group_AddID(group_1, group_3));
	debug(GUI_Group_AddID(group_1, group_4));

	return group_1;
}

int main() {

	debug(GUI_Window_Create(1200, 800, "BTC - Better Than CAS", 60));
	debug(GUI_AddItemToWindow(Group_1()));
	debug(GUI_AddItemToWindow(Group_2()));
	debug(GUI_Window_Show());
	debug(GUI_Window_Delete());

	return 0;
}



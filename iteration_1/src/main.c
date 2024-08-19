#include "gui.h"

#define DEBUG
#include "debug.h"

ID sequence_1() {

	ID sequence_1 = GUI_Sequence_Create();
	ID shape_1 = GUI_Shape_Create(10,10,100,100,255,255,255,255,0.0f,10,NO_ID);
	ID shape_2 = GUI_Shape_Create(120,10,200,100,255,255,0,255,0.0f,10,NO_ID);
	GUI_AddItemToSequence(sequence_1, shape_1);
	GUI_AddItemToSequence(sequence_1, shape_2);

	ID sequence_2 = GUI_Sequence_Create();
	ID shape_3 = GUI_Shape_Create(10,210,100,100,255,0,255,255,0.0f,10,NO_ID);
	ID shape_4 = GUI_Shape_Create(120,210,100,200,255,0,0,255,0.0f,10,NO_ID);
	GUI_AddItemToSequence(sequence_2, shape_3);
	GUI_AddItemToSequence(sequence_2, shape_4);
	GUI_AddItemToSequence(sequence_1, sequence_2);

	return sequence_1;
}

ID sequence_2() {

	ID sequence_1 = GUI_Sequence_Create();
	GUI_AddItemToSequence(sequence_1, GUI_Shape_Create(10,10,100,100,255,255,255,255,0.0f,10,NO_ID));
	GUI_AddItemToSequence(sequence_1, GUI_Shape_Create(120,10,200,100,255,255,0,255,0.0f,10,NO_ID));
	ID sequence_2 = GUI_Sequence_Create();
	GUI_AddItemToSequence(sequence_2, GUI_Shape_Create(10,210,100,100,255,0,255,255,0.0f,10,NO_ID));
	GUI_AddItemToSequence(sequence_2, GUI_Shape_Create(120,210,100,200,255,0,0,255,0.0f,10,NO_ID));
	GUI_AddItemToSequence(sequence_1, sequence_2);

	return sequence_1;
}

int main() {

	ID window = GUI_Window_Create(600, 600, "BTC - Better Than CAS", 60);
	ID render_1 = GUI_Render_Create(window, sequence_2());
	GUI_Window_Show();
	GUI_Window_Delete();

	return 0;
}



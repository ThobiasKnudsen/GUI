#ifndef GUI_H
#define GUI_H

#include <stdbool.h>

typedef struct {
    unsigned int    id;
    unsigned short  loc;
    unsigned char   type;
    bool            active;
} ID;

extern const ID NO_ID;

ID      GUI_Sequence_Create();
ID      GUI_Shape_Create(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float rotation_360, unsigned int corner_radius_pixels, ID image_id);
void    GUI_AddItemToSequence(ID sequence_id, ID id);
ID      GUI_Frame_Create(ID target_id, ID sequence_id);
void    GUI_Frame_AddFunction(ID frame_id, ID function_id);

ID      GUI_Window_Create(unsigned int width, unsigned int height, const char* title, unsigned int FPS);
void    GUI_AddItemToWindow(ID id);
void    GUI_Window_Show();
void    GUI_Window_Close();
void    GUI_Window_Delete();

#endif // GUI_H

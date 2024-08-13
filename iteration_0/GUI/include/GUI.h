#ifndef GUI_H
#define GUI_H

typedef enum  {
    NONE,
    WINDOW,
    NORMAL,
    DRAWORDER,
    TEXT,
    RENDERTEXTURE,
    CUSTOM
} GUI_Item_Type;
typedef unsigned int GUI_Item_ID;
typedef struct GUI_Item_Window          GUI_Item_Window;
typedef struct GUI_Item_Normal          GUI_Item_Normal;
typedef struct GUI_Item_DrawOrder       GUI_Item_DrawOrder;
typedef struct GUI_Item_Text            GUI_Item_Text;
typedef struct GUI_Item_RenderTexture   GUI_Item_RenderTexture;
typedef struct GUI_Item_Custom          GUI_Item_Custom;
typedef union  GUI_Item_Union           GUI_Item_Union;
typedef struct GUI_Item                 GUI_Item;


void GUI_Create(unsigned int width, unsigned int height, const char* title, unsigned int FPS);
void GUI_Show();
void GUI_Close();
void GUI_Delete();


GUI_Item_ID GUI_Get_WindowDrawOrder();
GUI_Item_ID GUI_Item_Create();



#endif // GUI_H

#ifndef GUI_BACKBONE_H
#define GUI_BACKBONE_H

#include "GUI.h"

GUI_Item_ID 	private_GUI_Item_Create();
GUI_Item* 	private_GUI_GetItemPtr(GUI_Item_ID item_id);
void 		private_GUI_Item_Print(GUI_Item* item_ptr);
void 		private_GUI_PrintItems(GUI_Item* window_item_ptr);


#endif // GUI_BACKBONE_H

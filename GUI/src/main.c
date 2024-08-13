#include "GUI.h"

#define DEBUG
#include "debug.h"


// ISSUE: 
//	1. how will you determin what happens when keys are pressed.
// 	2. if you make GUI_Item_Custom that custom then the user would have to write unecessary code. But actually 

// GOAL
// ===============================================================================================================================================

// 23. juli 2024
// do you really need parent-child strure if the draw order should be stored as a large array
// 	actually if you dont have parent-child structure its much harder to make a mental model of how the GUI will look like and 
// 	the user would need to write extra code for explicitly handling the draw order.
// 	at the same time you get more flexibility by explicitly handling the draw order.
//	if you choose to not have parent-child structure then you must make a new item which stores the draw order and that will be
//	more complicated for the user to learn. 
//	How would it look like to have a draw_order_item?
// 		It would actually not be that complicated for the user. He would make the draw_order_item, then when making other items, one of the 
//		arguments is the id of the draw_order_item.
//			would imply runtime errors as to compile time, if not the draw_order_item is valid as argument.
//		then to draw to the window (GUI_DrawToTarget(target, draw_order_items))
//		and remember this doesnt imidiatly draw to to window, its an request. 
//		implementing draw_order_item would remove the need for prolofic_item and parent-child structure.
//		note: the items in draw_order can be draw_order_item, right. that opens up for morphisism, but be carefull that it doesnt make an
//		an endless loop.
//		

// 23. juli 2024
// Right now vertices is statically made inside the GUI_Create function.
// how would you implement the actual version?
//	there is private_VBO and there is private_vertices. both contain the exact same data but VBO is on the GPU and vertices is on the CPU.
//	when a new item is created it gets its own vertex in vertices and that item will be marked as changed so when it is drawn, the corresponding.
//	vertex on the VBO must be changed alongside all other item changes that will be drawn.
//

// 23. juli 2024 
// make this so that the user can read/write the 

// 23. juli 2024 
// do not implement the 3 matricies into targets and the shader program. that is something that can be done with the custom_item
// earlier you wanted it so it would be possbile to make MilMap with this GUI framework. But thats such a specifc case actually, 
// so in that case just use custom_item. 
// previous Vertex:
//	{float pos[9], float tex_rect[4], uint color, uint tex_index, float corner_radius} = 64 bytes
// new Vertex:
//	it1: {uint rect[4], float tex_rect[4]?, uint color, float corner_radius, uchar tex_index, byte padding[7]} = 41 bytes + 7 bytes padding
// GUI_Item should be 64 bytes!
//	NOOOOOOOO! make it 48 bytes!

// 23. juli 2024
// Now when vertex pos is 2d rect and not 3d parallellogram means that you can only draw lines horizontally and vertically.
//	but actually there is 7 bytes free space so just add float rotation

// 24. juli 2024
// How do you justify that text has one vertex location?

// 25. Juli 2024 
// The only GUI_Item pointer that should exist is private_item_array because if the pointer is used any other place and private_item_array is
// upodated that means that all other pointers are compromized. You should only use GUI_Item_ID and make GUI handle the item location for you

// 25. juli 2024
// if items are deleted you shouldnt try to regain the space the items were before because the id of that item is store with the user
// and to avoid drawing something that is deleted you shouldnt use the same space again
//	actually it is completely fine to reuse the space but you cant reuse the id, sonce the user has is stored.
//	and if the user wants to dynamically make items then delete them after they are no longer needed for almost every frame then there would be 
//	so much unused space after a while.

// 25. Juli 2024
// How should you find the right vertex location for a given item?
// what should you do when an item is deleted. how should you handle the free vertex location?
// what about text_item? it has a whole array of vertices within private_vertices which starts at the vertex location in the GUI_Item struct?
// now ive added (bool in_use) in the PrivateVertex struct, so when you need to find a free vertex you just search through untill
// you find a free vertex

// 26. Juli 2024
// should you update vertices every frame or right before a draw_order is drawn?
// SHOULD DO: should check that no size is ever larger than capacity. it so the program must crash

// 27. Juli 2024
// should be possible to have one item controll multiple vertices like text item while also being costomizable like custom_item
// for every iteration that there is no loop in the target's draw orders, which implies infinite time for each iteration
// make shure that nothing is redrawn if its exactly the same as the previous frame

// 30. Juli 2024
// you actually can have more than 16 textures, but drawing max 16 per target. when drawing onto a new target you can bind 16 new textures
// 		10. August 2024
//		you can also use texture atlases so you can tecnically have more than 16 textures binded at once

// 31. Juli 2024
// TALK ABOUT MYSTERIES AND COUNTER ARGUMENTS

// 7. august 2024
// you have to change text_item so that its just a normal_item with texcoords. This opens up for more possibilities

// 10. august 2024
// when setting indices you have to reset all indices_needsUpdate to false after it is not needed anymore for each frame
// BUT you cannot do it right after updating the indices in one draw_order because most draw_orders depend on indices_needsUpdate in other draw_orders
//
// TextureAtlas isnt something that is drawn directly and just is there without doing anything it seems, but the position can be used to know that the
// TextureAtlas has that place in textures in shaders for the draw_order its within
// how would that be implemented for the user?
//		Texture texture1 =
//		Item item1 = CreateItem(x,y,w,h,color);
//		ItemSetTexture(x,y,w,h, texture);
//		draw(item, draw_order)
//
// should TextureAtlas and RenderTexture be given a number from 0 to 15 to determin their location in the shaders
// so that you can then check if there allready is a texture at that number for the given draw order.
//
// so there should be an StaticImage and an DynamicImage where both has an internal rect for the internal drawing.
// none of them should have an external rect because the Rect item will handle the external rect for the images.
// they both can be rendered to with a FBO but StaticImage is stored inside an atlas while DynamicImage has its own texture.
// draw order should be name sequence instead
// target instead of RenderTexture and Window.
// target contains FBO and FBO=0 is the window so it actually works
// it contains one sequence. when setting the indices for the sequences it should recugnize all textures that are used and indexing them from 0 to 15
//
// TODAYS INTELLECTUAL ADVANCMENTS
//		1. Normal 			-> Shape
//		2. RenderTexture 	-> DynamicImage + FBO
//		3. Window 			-> FBO_window
//		4. TextureAtlas 	-> StaticImage + FBO_temp
//		5. Draw_Order 		-> Sequence
//		6. Text 			-> Text
//
//		Target contains FBO and indices to textures to bind. when doing this the texture index in VBO must be changed


// ===============================================================================================================================================

int main() {

	debug(GUI_Create(600, 600, "BTC - Better Than CAS", 60););
	
	//debug(GUI_Item_ID window_draw_order = GUI_Get_WindowDrawOrder();); // window_draw_order is allways drawn on the window each frame
	//debug(GUI_Item_ID rectangle0        = GUI_Item_Normal_Create(100, 100, 300, 100, 255, 64, 0, 255, window_draw_order););
	
	
	debug(GUI_Show(););
	debug(GUI_Delete(););

	return 0;
}



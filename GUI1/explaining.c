do you c everything?

window, static_texture and dynamic_texture could be a target
a target is used to render onto
static_texture is within a texture atlas
dynamic_texture is its own texture within Textures alongside texture atlases
a texture atlas is just a large texture which contains smaller textures

rendering requires a target and a sequence

a sequence stores a list of shapes or other sequences
a sequence should find which indices in VBO is used
a sequence should find which indices in Textures is used

to render onto a target with a sequence you must attach the sequence to the target

GUI_Window
GUI_Shape
GUI_Sequence
GUI_StaticImage
GUI_DynamicImage
GUI_Function

Function will only execute when it is attached to a Sequence that is attached to a Target that eventually will be rendered to window this frame
When it executes it executes after the user events are registered

GUI_Render contains 1x Target, 1x GUI_Sequence, 1x EBO, Textures and GUI_Functions
Inside GUI_Render EBO, Textures and GUI_Functions is determined through the GUI_Sequence
GUI_Render are stored in random sequence in its own array but the sequence is determined through which Targets must be rendered first
GUI_Sequence could contain in the sequence itself GUI_Shape, GUI_Sequence and besides the sequence it could contain GUI_Functions
it could contain GUI_Render because that determines which

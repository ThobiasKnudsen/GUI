#include "gui.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h> // pow(a,b)

#define DEBUG
#include "debug.h"
#include "array.h"

// GOAL
// MAKE A SIMPLE VERSION THAT USES VBO, VAO, EBO, FBO, INDICES IN EACH SEQENCE, VERTICES IN EACH SHAPE AND WITHOUT IMAGES TO MAKE THINGS SIMPLER


// TYPES
#define VERTEX 0
#define SHAPE 1
#define SEQUENCE 2
#define RENDERING 3
#define ATLAS_IMAGE 4
#define DYNAMIC_IMAGE 5
#define STATIC_IMAGE 6
#define IMAGE 7
#define FUNCTION 8
#define WINDOW 9
#define NONE 10



// STRUCTURES
    typedef struct {

        int             rect[4];
        float           tex_rect[4];
        unsigned int    color;
        float           rotation_360;
        unsigned int    multiple_values; // 2 bytes corner_radius and 1 byte texture_loc
        bool            in_use;
        bool            synced_with_VBO;

        // 46 bytes

    } Vertex;
    typedef struct {

        ID              id;
        unsigned int    vertex_loc;
        unsigned int    image_loc; // its in the global image_array. if image_loc == 0 then it uses no image

    } Shape; // 16 bytes
    typedef struct {

        ID              id;

        ID*             id_array; // types are only SEQUENCE and SHAPE
        unsigned int    id_array_size;
        unsigned int    id_array_capacity;

        unsigned int*   indices;
        unsigned int    indices_size;
        unsigned int    indices_capacity;

    } Sequence;
    typedef struct {

        ID              id;

        ID              target_image_id; // type Image
        ID              sequence_id;

        bool            window_is_target;

        unsigned int*   image_source_array;
        unsigned int    image_source_array_size;
        unsigned int    image_source_array_capacity;

        unsigned int*   function_array;
        unsigned int    function_array_size;
        unsigned int    function_array_capacity;

    } Rendering;
    typedef struct {

        ID              id;

        GLuint          texture;
        unsigned int    width;
        unsigned int    height;

        unsigned int    padding_pixels;

        unsigned int*   rect_each_image; // 4x unsigned int
        unsigned int    rect_each_image_size;
        unsigned int    rect_each_image_capacity;

        unsigned int    image_count;

    } AtlasImage;
    typedef struct {

        ID              id;
        GLuint          texture;
        unsigned int    width;
        unsigned int    height;

    } DynamicImage;
    typedef struct {
        ID                  id;
        unsigned int        atlas_image_loc;
        unsigned int        image_loc;
    } StaticImage;
    typedef struct {
        ID  id;
        ID  image_id; // dynamic or static
    } Image;
    typedef struct {
        ID          id;
    } Function;
    typedef struct {
        GLFWwindow*     window;

        Rendering*          rendering;

        GLuint          shader_program;
        GLuint          target_x_loc;
        GLuint          target_y_loc;
        GLuint          target_width_loc;
        GLuint          target_height_loc;

        GLuint          VAO;
        GLuint          VBO;
        unsigned int    VBO_size;
        unsigned int    VBO_capacity;
        GLuint          EBO;
        unsigned int    EBO_size;
        unsigned int    EBO_capacity;
        GLuint          FBO;
    } Window;



// DATA

    static ID*              id_array = NULL;
    static unsigned int     id_array_size = 0;
    static unsigned int     id_array_capacity = 0;
    static unsigned int     id_count = 0;
    const ID                NO_ID = {NONE, 0, 0, false};

    static Vertex*          vertices = NULL;
    static unsigned int     vertices_size = 0;
    static unsigned int     vertices_capacity = 0;

    static Shape*           shape_array = NULL;
    static unsigned int     shape_array_size = 0;
    static unsigned int     shape_array_capacity = 0;

    static Sequence*        sequence_array = NULL;
    static unsigned int     sequence_array_size = 0;
    static unsigned int     sequence_array_capacity = 0;

    static Rendering*           rendering_array = NULL;
    static unsigned int     rendering_array_size = 0;
    static unsigned int     rendering_array_capacity = 0;

    static AtlasImage*      atlas_image_array = NULL;
    static unsigned int     atlas_image_array_size = 0;
    static unsigned int     atlas_image_array_capacity = 0;

    static DynamicImage*    dynamic_image_array = NULL;
    static unsigned int     dynamic_image_array_size = 0;
    static unsigned int     dynamic_image_array_capacity = 0;

    static StaticImage*     static_image_array = NULL;
    static unsigned int     static_image_array_size = 0;
    static unsigned int     static_image_array_capacity = 0;

    static Image*           image_array = NULL;
    static unsigned int     image_array_size = 0;
    static unsigned int     image_array_capacity = 0;

    static Function*        function_array = NULL;
    static unsigned int     function_array_size = 0;
    static unsigned int     function_array_capacity = 0;

    static Window           window;
    static bool             window_created = false;

// OPENGL ERROR HANDLING
void OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    // Print message source
    const char* sourceStr;
    switch (source) {
        case GL_DEBUG_SOURCE_API:             sourceStr = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceStr = "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceStr = "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     sourceStr = "Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           sourceStr = "Other"; break;
        default:                              sourceStr = "Unknown"; break;
    }

    // Print message type
    const char* typeStr;
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               typeStr = "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated Behavior"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Undefined Behavior"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              typeStr = "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               typeStr = "Other"; break;
        default:                                typeStr = "Unknown"; break;
    }

    // Print message severity
    const char* severityStr;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         severityStr = "High"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "Medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          severityStr = "Low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "Notification"; break;
        default:                             severityStr = "Unknown"; break;
    }

    // Print the complete debug message
    printf("OpenGL Debug Message\n");
    printf("Source: %s\n", sourceStr);
    printf("Type: %s\n", typeStr);
    printf("ID: %u\n", id);
    printf("Severity: %s\n", severityStr);
    printf("Message: %s\n", message);
    printf("\n");

    exit(-1);
}


// FUNCTIONS
    // GLFW SIMPLIFIED
        GLFWwindow* CreateWindow(unsigned int width, unsigned int height, const char* title) {
            // Initialize GLFW
            if (!glfwInit()) {
                printf("Failed to initialize GLFW\n");
                exit(-1);
            }


            // Set GLFW window hints
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_SAMPLES, 4);
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

            // Create a GLFW window
            GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
            if (!window) {
                printf("Failed to create GLFW window\n");
                glfwTerminate();
                exit(-1);
            }

            // Make the OpenGL context current
            glfwMakeContextCurrent(window);

            // Initialize GLEW
            if (glewInit() != GLEW_OK) {
                printf("Failed to initialize GLEW\n");
                exit(-1);
            }

            //glfwSwapInterval(0);

            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(OpenGLDebugCallback, NULL);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            return window;
        }
        GLuint CompileShader(const char* shader_source, GLenum shader_type) {
            // Compile the shader
            debug(GLuint shader = glCreateShader(shader_type));
            debug(glShaderSource(shader, 1, &shader_source, NULL));
            debug(glCompileShader(shader));
            // Check for shader compile errors
            int success;
            char infoLog[512];
            debug(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
            if (!success) {
                glGetShaderInfoLog(shader, 512, NULL, infoLog);
                printf("ERROR: shader %d compilation failed: %s\n", shader_type, infoLog);
            }
            return shader;
        }
        GLuint CreateProgram(GLuint* shaders, size_t number_of_shaders) {
            // Link shaders to create a shader program
            GLuint shader_program = glCreateProgram();
            for (size_t i = 0; i < number_of_shaders; i++) {
                glAttachShader(shader_program, shaders[i]);
            }
            glLinkProgram(shader_program);
            // Check for linking errors
            int success;
            char infoLog[512];
            glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
                printf("ERROR: shader program linking failed: %s\n", infoLog);
            }
            return shader_program;
        }
        int NDCtoPixel(float NDC, unsigned int max_pixels) {
            return (int)((float)max_pixels*((NDC+1.f)/2.f));
        }
        float PixeltoNDC(float pixel, unsigned int max_pixels) {
            return (2.f*(float)pixel/(float)max_pixels)-1.f;
        }
        char* ReadShaderSource(const char* shader_file) {
            debug(FILE* file = fopen(shader_file, "rb"););
            if (!file) {
                printf("ERROR: could not open shader file %s\n", shader_file);
                exit(-1);
            }

            fseek(file,0,SEEK_END);
            long length = ftell(file);
            fseek(file,0,SEEK_SET);

            debug(char* buffer = alloc(NULL, length+1));
            if(!buffer){
                printf("ERROR: could not allocatre memory for reading file %s\n", shader_file);
                fclose(file);
                exit(-1);
            }

            debug(fread(buffer,1,length,file););
            debug(buffer[length]='\0';);
            debug(fclose(file););

            if (!buffer) {
                printf("ERROR: buffer is NULL\n");
                exit(-1);
            }

            return buffer;
        }

    // PRIVATE
        void* GetItemPointer(ID id) {

            void* ptr = NULL;

            switch(id.type) {
                case VERTEX:
                    ptr = &vertices[id.loc];
                    break;
                case SHAPE:
                    ptr = &shape_array[id.loc];
                    #ifdef DEBUG
                    if (((Shape*)ptr)->id.id != id.id) {
                        printf("design flaw: id.loc(%d) ((Shape*)ptr)->id.id(%d) id.id(%d)\n", id.loc, ((Shape*)ptr)->id.id, id.id);
                        exit(-1);
                    }
                    #endif
                    break;
                case SEQUENCE:
                    ptr = &sequence_array[id.loc];
                    #ifdef DEBUG
                    if (sequence_array == NULL) {
                        printf("design flaw: sequence_array == NULL\n");
                        exit(-1);
                    }
                    if (((Sequence*)ptr)->id.id != id.id) {
                        printf("design flaw: id.loc(%d) ((Sequence*)ptr)->id.id(%d) id.id(%d)\n", id.loc, ((Sequence*)ptr)->id.id, id.id);
                        exit(-1);
                    }
                    #endif
                    break;
                case RENDERING:
                    ptr = &rendering_array[id.loc];
                    #ifdef DEBUG
                    if (rendering_array == NULL) {
                        printf("design flaw: rendering_array == NULL\n");
                        exit(-1);
                    }
                    if (((Rendering*)ptr)->id.id != id.id) {
                        printf("design flaw: id.loc(%d) ((Rendering*)ptr)->id.id(%d) id.id(%d)\n", id.loc, ((Rendering*)ptr)->id.id, id.id);
                        exit(-1);
                    }
                    #endif
                    break;
                case ATLAS_IMAGE:
                    ptr = &atlas_image_array[id.loc];
                    #ifdef DEBUG
                    if (((AtlasImage*)ptr)->id.id != id.id) {
                        printf("design flaw: id.loc(%d) ((AtlasImage*)ptr)->id.id(%d) id.id(%d)\n", id.loc, ((AtlasImage*)ptr)->id.id, id.id);
                        exit(-1);
                    }
                    #endif
                    break;
                case DYNAMIC_IMAGE:
                    ptr = &dynamic_image_array[id.loc];
                    #ifdef DEBUG
                    if (((DynamicImage*)ptr)->id.id != id.id) {
                        printf("design flaw: id.loc(%d) ((DynamicImage*)ptr)->id.id(%d) id.id(%d)\n", id.loc, ((DynamicImage*)ptr)->id.id, id.id);
                        exit(-1);
                    }
                    #endif
                    break;
                case STATIC_IMAGE:
                    ptr = &static_image_array[id.loc];
                    #ifdef DEBUG
                    if (((StaticImage*)ptr)->id.id != id.id) {
                        printf("design flaw: id.loc(%d) ((StaticImage*)ptr)->id.id(%d) id.id(%d)\n", id.loc, ((StaticImage*)ptr)->id.id, id.id);
                        exit(-1);
                    }
                    #endif
                    break;
                case IMAGE:
                    ptr = &image_array[id.loc];
                    break;
                case FUNCTION:
                    ptr = &function_array[id.loc];
                    #ifdef DEBUG
                    if (((Function*)ptr)->id.id != id.id) {
                        printf("design flaw: id.loc(%d) ((Function*)ptr)->id.id(%d) id.id(%d)\n", id.loc, ((Function*)ptr)->id.id, id.id);
                        exit(-1);
                    }
                    #endif
                    break;
            }

            if (ptr == NULL) {
                printf("design flaw: ptr == NULL\n");
                exit(-1);
            }

            return ptr;
        }
        ID ID_Create(unsigned char type) {
            ID new_id;
            new_id.id = id_count;
            id_count++;
            new_id.type = type;
            new_id.active = true;
            switch (type) {
                case VERTEX:
                    debug(vertices = Array_ManageMemory(vertices, &vertices_size, &vertices_capacity, sizeof(Vertex)));
                    new_id.loc = vertices_size;
                    vertices_size++;
                    break;
                case SHAPE:
                    debug(shape_array = Array_ManageMemory(shape_array, &shape_array_size, &shape_array_capacity, sizeof(Shape)));
                    new_id.loc = shape_array_size;
                    shape_array[new_id.loc].id = new_id;
                    shape_array_size++;
                    break;
                case SEQUENCE:
                    debug(sequence_array = Array_ManageMemory(sequence_array, &sequence_array_size, &sequence_array_capacity, sizeof(Sequence)));
                    new_id.loc = sequence_array_size;
                    sequence_array[new_id.loc].id = new_id;
                    sequence_array_size++;
                    break;
                case RENDERING:
                    debug(rendering_array = Array_ManageMemory(rendering_array, &rendering_array_size, &rendering_array_capacity, sizeof(Rendering)));
                    new_id.loc = rendering_array_size;
                    rendering_array[new_id.loc].id = new_id;
                    rendering_array_size++;
                    break;
                case ATLAS_IMAGE:
                    debug(atlas_image_array = Array_ManageMemory(atlas_image_array, &atlas_image_array_size, &atlas_image_array_capacity, sizeof(AtlasImage)));
                    new_id.loc = atlas_image_array_size;
                    atlas_image_array[new_id.loc].id = new_id;
                    atlas_image_array_size++;
                    break;
                case DYNAMIC_IMAGE:
                    debug(dynamic_image_array = Array_ManageMemory(dynamic_image_array, &dynamic_image_array_size, &dynamic_image_array_capacity, sizeof(DynamicImage)));
                    new_id.loc = dynamic_image_array_size;
                    dynamic_image_array[new_id.loc].id = new_id;
                    dynamic_image_array_size++;
                    break;
                case STATIC_IMAGE:
                    debug(static_image_array = Array_ManageMemory(static_image_array, &static_image_array_size, &static_image_array_capacity, sizeof(StaticImage)));
                    new_id.loc = static_image_array_size;
                    static_image_array[new_id.loc].id = new_id;
                    static_image_array_size++;
                    break;
                case IMAGE:
                    debug(image_array = Array_ManageMemory(image_array, &image_array_size, &image_array_capacity, sizeof(Image)));
                    new_id.loc = image_array_size;
                    image_array_size++;
                    break;
                case FUNCTION:
                    debug(function_array = Array_ManageMemory(function_array, &function_array_size, &function_array_capacity, sizeof(Function)));
                    new_id.loc = function_array_size;
                    function_array[new_id.loc].id = new_id;
                    function_array_size++;
                    break;
            }
            #ifdef DEBUG
            debug(id_array = Array_ManageMemory(id_array, &id_array_size, &id_array_capacity, sizeof(ID)));
            debug(id_array[id_array_size] = new_id);
            id_array_size++;
            #endif
            return new_id;
        }
        ID Vertex_Create(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float rotation_360, unsigned int corner_radius_pixels) {

            ID new_vertex_id = ID_Create(VERTEX);
            Vertex* vertex_ptr = GetItemPointer(new_vertex_id);

            vertex_ptr->rect[0] = x;
            vertex_ptr->rect[1] = y;
            vertex_ptr->rect[2] = width;
            vertex_ptr->rect[3] = height;
            vertex_ptr->tex_rect[0] = 0.0f;
            vertex_ptr->tex_rect[1] = 0.0f;
            vertex_ptr->tex_rect[2] = 1.0f;
            vertex_ptr->tex_rect[3] = 1.0f;
            vertex_ptr->color = (unsigned int)pow(2, 24)*((unsigned int)r) +
                                (unsigned int)pow(2, 16)*((unsigned int)g) +
                                (unsigned int)pow(2, 8)*((unsigned int)b) +
                                (unsigned int)pow(2, 0)*((unsigned int)a);
            vertex_ptr->rotation_360 = rotation_360;
            vertex_ptr->multiple_values = (vertex_ptr->multiple_values&0x0000FFFF) + (unsigned int)pow(2,16) * corner_radius_pixels;
            vertex_ptr->in_use = true;
            vertex_ptr->synced_with_VBO = false;

            return new_vertex_id;

        }

    // PRINTING

        void GUI_PrintFPS() {
            static double previous_seconds = 0.0f;
            static int rendering_count = 0;
            double current_seconds = glfwGetTime();
            double elapsed_seconds = current_seconds - previous_seconds;

            if (elapsed_seconds > 1.0) {
                previous_seconds = current_seconds;
                double fps = (double)rendering_count / elapsed_seconds;
                char tmp[128];
                snprintf(tmp, sizeof(tmp), "FPS: %.2f", fps);
                printf("%s\n", tmp);
                rendering_count = 0;
            }
            rendering_count++;
        }
        /*
        void GUI_Item_Print(GUI_Item* item_ptr) {
            printf("id = %d | type = %d | vertex_location = %d | in_use = %d\n", item_ptr->id, item_ptr->type, item_ptr->vertex_location, item_ptr->in_use);
        }
        */
        void PrintItem(ID id) {
            switch(id.type) {
                case VERTEX: {
                    Vertex* ptr = GetItemPointer(id);

                    printf("Vertex id(%d) loc(%d) active(%d) rect(%d %d %d %d) tex_rect(%.3f %.3f %.3f %.3f) color(%d %d %d %d) rotation_360(%f)\n", id.id, id.loc, id.active, ptr->rect[0], ptr->rect[1], ptr->rect[2], ptr->rect[3],  ptr->tex_rect[0], ptr->tex_rect[1], ptr->tex_rect[2], ptr->tex_rect[3], ((unsigned char*)(&ptr->color))[0], ((unsigned char*)(&ptr->color))[1], ((unsigned char*)(&ptr->color))[2], ((unsigned char*)(&ptr->color))[3], ptr->rotation_360);

                    break;
                }
                case SHAPE: {

                    Shape* ptr = GetItemPointer(id);
                    printf("Shape id(%d) loc(%d) active(%d) vertex_loc(%d) image_loc(%d)\n", id.id, id.loc, id.active, ptr->vertex_loc, ptr->image_loc);

                    Vertex* vertex = &vertices[ptr->vertex_loc];
                    printf("    Vertex rect(%d %d %d %d) tex_rect(%.3f %.3f %.3f %.3f) color(%d %d %d %d) rotation_360(%f)\n", vertex->rect[0], vertex->rect[1], vertex->rect[2], vertex->rect[3],  vertex->tex_rect[0], vertex->tex_rect[1], vertex->tex_rect[2], vertex->tex_rect[3], ((unsigned char*)(&vertex->color))[0], ((unsigned char*)(&vertex->color))[1], ((unsigned char*)(&vertex->color))[2], ((unsigned char*)(&vertex->color))[3], vertex->rotation_360);

                    {
                        printf("    Image id(%d) loc(%d) active(%d)\n", id.id, id.loc, id.active);
                        if (ptr->image_loc == 0) {
                            break;
                        }
                        Image* image = &image_array[ptr->image_loc];
                        ID sub_id = image->image_id;

                        if (image->image_id.type == NONE) {
                            break;
                        }

                        if (sub_id.type == DYNAMIC_IMAGE) {
                            DynamicImage* sub_ptr = GetItemPointer(sub_id);

                            printf("        DynamicImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d)\n", id.id, id.loc, id.active, sub_ptr->texture, sub_ptr->width, sub_ptr->height);
                        }

                        else if (sub_id.type == STATIC_IMAGE) {
                            StaticImage* sub_ptr1 = GetItemPointer(sub_id);

                            printf("        StaticImage id(%d) loc(%d) active(%d) atlas_image_loc(%d) image_loc(%d)\n", id.id, id.loc, id.active, sub_ptr1->atlas_image_loc, sub_ptr1->image_loc);

                            AtlasImage* sub_ptr2 = &atlas_image_array[sub_ptr1->atlas_image_loc];

                            printf("        AtlasImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d) padding_pixels(%d) rect_each_image(%p %d %d) image_count(%d)\n", sub_ptr2->id.id, sub_ptr2->id.loc, sub_ptr2->id.active, sub_ptr2->texture, sub_ptr2->width, sub_ptr2->height, sub_ptr2->padding_pixels, sub_ptr2->rect_each_image, sub_ptr2->rect_each_image_size, sub_ptr2->rect_each_image_capacity, sub_ptr2->image_count);

                            if (sub_ptr2->rect_each_image != NULL && sub_ptr2->rect_each_image_size != 0) {
                                unsigned int i = sub_ptr1->atlas_image_loc;
                                printf("        Rect %d (%d %d %d %d)\n", i, sub_ptr2->rect_each_image[4*i + 0], sub_ptr2->rect_each_image[4*i + 1], sub_ptr2->rect_each_image[4*i + 2], sub_ptr2->rect_each_image[4*i + 3]);
                            }
                        }
                        #ifdef DEBUG
                        else {
                            printf("design flaw: image_id in type IMAGE is neither DYNAMIC_IMAGE or STATIC_IMAGE\n");
                            exit(-1);
                        }
                        #endif
                    }

                    break;
                }
                case SEQUENCE: {
                    Sequence* ptr = GetItemPointer(id);
                    printf("Sequence id(%d) loc(%d) active(%d) id_array(%p %d %d) vertices(%p %d %d)\n", id.id, id.loc, id.active, ptr->id_array, ptr->id_array_size, ptr->id_array_capacity, ptr->indices, ptr->indices_size, ptr->indices_capacity);
                    if (ptr->id_array != NULL && ptr->id_array_size != 0) {
                        for (unsigned int i = 0; i < ptr->id_array_size; i++) {
                            ID sub_id = ptr->id_array[i];
                            switch(sub_id.type) {
                                case SHAPE: {

                                    Shape* sub_ptr = GetItemPointer(sub_id);
                                    printf("    Shape id(%d) loc(%d) active(%d) vertex_loc(%d) image_loc(%d)\n", sub_id.id, sub_id.loc, sub_id.active, sub_ptr->vertex_loc, sub_ptr->image_loc);

                                    break;
                                }
                                case SEQUENCE: {

                                    Sequence* sub_ptr = GetItemPointer(sub_id);
                                    printf("    Sequence id(%d) loc(%d) active(%d) id_array(%p %d %d) vertices(%p %d %d)\n", sub_id.id, sub_id.loc, sub_id.active, sub_ptr->id_array, sub_ptr->id_array_size, sub_ptr->id_array_capacity, sub_ptr->indices, sub_ptr->indices_size, sub_ptr->indices_capacity);

                                    break;
                                }
                                case RENDERING: {

                                    Rendering* sub_ptr = GetItemPointer(sub_id);
                                    printf("Rendering id(%d) loc(%d) active(%d) image_source_array(%p %d %d) function_array(%p %d %d)\n", id.id, id.loc, id.active, sub_ptr->image_source_array, sub_ptr->image_source_array_size, sub_ptr->image_source_array_capacity, sub_ptr->function_array, sub_ptr->function_array_size, sub_ptr->function_array_capacity);

                                    break;
                                }
                                default: {

                                    printf("    id(%d) loc(%d) type(%d) active(%d). type is undefined!!!\n", sub_id.id, sub_id.loc, sub_id.type, sub_id.active);

                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case RENDERING: {
                    Rendering* ptr = GetItemPointer(id);

                    printf("Rendering id(%d) loc(%d) active(%d) window_is_target(%d) image_source_array(%p %d %d) function_array(%p %d %d)\n", id.id, id.loc, id.active, ptr->window_is_target, ptr->image_source_array, ptr->image_source_array_size, ptr->image_source_array_capacity, ptr->function_array, ptr->function_array_size, ptr->function_array_capacity);

                    Sequence* sequence = GetItemPointer(ptr->sequence_id);

                    printf("    Sequence id(%d) loc(%d) active(%d) id_array(%p %d %d) vertices(%p %d %d)\n", ptr->sequence_id.id, ptr->sequence_id.loc, ptr->sequence_id.active, sequence->id_array, sequence->id_array_size, sequence->id_array_capacity, sequence->indices, sequence->indices, sequence->indices_capacity);

                    {
                        Image* image = GetItemPointer(ptr->target_image_id);
                        printf("    Image id(%d) loc(%d) active(%d)\n", id.id, id.loc, id.active);
                        ID sub_id = image->image_id;

                        if (ptr->target_image_id.type == NONE) {
                            break;
                        }

                        if (sub_id.type == DYNAMIC_IMAGE) {
                            DynamicImage* sub_ptr = GetItemPointer(sub_id);

                            printf("        DynamicImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d)\n", id.id, id.loc, id.active, sub_ptr->texture, sub_ptr->width, sub_ptr->height);
                        }

                        else if (sub_id.type == STATIC_IMAGE) {
                            StaticImage* sub_ptr1 = GetItemPointer(sub_id);

                            printf("        StaticImage id(%d) loc(%d) active(%d) atlas_image_loc(%d) image_loc(%d)\n", id.id, id.loc, id.active, sub_ptr1->atlas_image_loc, sub_ptr1->image_loc);

                            AtlasImage* sub_ptr2 = &atlas_image_array[sub_ptr1->atlas_image_loc];

                            printf("        AtlasImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d) padding_pixels(%d) rect_each_image(%p %d %d) image_count(%d)\n", sub_ptr2->id.id, sub_ptr2->id.loc, sub_ptr2->id.active, sub_ptr2->texture, sub_ptr2->width, sub_ptr2->height, sub_ptr2->padding_pixels, sub_ptr2->rect_each_image, sub_ptr2->rect_each_image_size, sub_ptr2->rect_each_image_capacity, sub_ptr2->image_count);

                            if (sub_ptr2->rect_each_image != NULL && sub_ptr2->rect_each_image_size != 0) {
                                unsigned int i = sub_ptr1->atlas_image_loc;
                                printf("        Rect %d (%d %d %d %d)\n", i, sub_ptr2->rect_each_image[4*i + 0], sub_ptr2->rect_each_image[4*i + 1], sub_ptr2->rect_each_image[4*i + 2], sub_ptr2->rect_each_image[4*i + 3]);
                            }
                        }
                        #ifdef DEBUG
                        else {
                            printf("design flaw: image_id in type IMAGE is neither DYNAMIC_IMAGE or STATIC_IMAGE\n");
                            exit(-1);
                        }
                        #endif
                    }

                    break;
                }
                case ATLAS_IMAGE: {

                    AtlasImage* ptr = GetItemPointer(id);

                    printf("AtlasImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d) padding_pixels(%d) rect_each_image(%p %d %d) image_count(%d)\n", id.id, id.loc, id.active, ptr->texture, ptr->width, ptr->height, ptr->padding_pixels, ptr->rect_each_image, ptr->rect_each_image_size, ptr->rect_each_image_capacity, ptr->image_count);

                    if (ptr->rect_each_image != NULL && ptr->rect_each_image_size != 0) {
                        for (unsigned int i = 0; i < ptr->rect_each_image_size; i++) {
                            printf("rect %d (%d %d %d %d)\n", i, ptr->rect_each_image[4*i + 0], ptr->rect_each_image[4*i + 1], ptr->rect_each_image[4*i + 2], ptr->rect_each_image[4*i + 3]);
                        }
                    }
                    break;
                }
                case DYNAMIC_IMAGE: {

                    DynamicImage* ptr = GetItemPointer(id);
                    printf("DynamicImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d)\n", id.id, id.loc, id.active, ptr->texture, ptr->width, ptr->height);

                    break;
                }
                case STATIC_IMAGE: {
                    StaticImage* ptr = GetItemPointer(id);

                    printf("StaticImage id(%d) loc(%d) active(%d) atlas_image_loc(%d) image_loc(%d)\n", id.id, id.loc, id.active, ptr->atlas_image_loc, ptr->image_loc);

                    AtlasImage* sub_ptr = &atlas_image_array[ptr->atlas_image_loc];

                    printf("    AtlasImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d) padding_pixels(%d) rect_each_image(%p %d %d) image_count(%d)\n", id.id, id.loc, id.active, sub_ptr->texture, sub_ptr->width, sub_ptr->height, sub_ptr->padding_pixels, sub_ptr->rect_each_image, sub_ptr->rect_each_image_size, sub_ptr->rect_each_image_capacity, sub_ptr->image_count);

                    if (sub_ptr->rect_each_image != NULL && sub_ptr->rect_each_image_size != 0) {
                        unsigned int i = ptr->atlas_image_loc;
                        printf("rect %d (%d %d %d %d)\n", i, sub_ptr->rect_each_image[4*i + 0], sub_ptr->rect_each_image[4*i + 1], sub_ptr->rect_each_image[4*i + 2], sub_ptr->rect_each_image[4*i + 3]);
                    }

                    break;
                }
                case IMAGE: {

                    Image* ptr = GetItemPointer(id);
                    printf("Image id(%d) loc(%d) active(%d)\n", id.id, id.loc, id.active);
                    ID sub_id = ptr->image_id;

                    if (ptr->image_id.type == NONE) {
                        break;
                    }

                    if (sub_id.type == DYNAMIC_IMAGE) {
                        DynamicImage* sub_ptr = GetItemPointer(sub_id);

                        printf("    DynamicImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d)\n", id.id, id.loc, id.active, sub_ptr->texture, sub_ptr->width, sub_ptr->height);
                    }

                    else if (sub_id.type == STATIC_IMAGE) {
                        StaticImage* sub_ptr1 = GetItemPointer(sub_id);

                        printf("    StaticImage id(%d) loc(%d) active(%d) atlas_image_loc(%d) image_loc(%d)\n", id.id, id.loc, id.active, sub_ptr1->atlas_image_loc, sub_ptr1->image_loc);

                        AtlasImage* sub_ptr2 = &atlas_image_array[sub_ptr1->atlas_image_loc];

                        printf("        AtlasImage id(%d) loc(%d) active(%d) texture(%d) width(%d) height(%d) padding_pixels(%d) rect_each_image(%p %d %d) image_count(%d)\n", sub_ptr2->id.id, sub_ptr2->id.loc, sub_ptr2->id.active, sub_ptr2->texture, sub_ptr2->width, sub_ptr2->height, sub_ptr2->padding_pixels, sub_ptr2->rect_each_image, sub_ptr2->rect_each_image_size, sub_ptr2->rect_each_image_capacity, sub_ptr2->image_count);

                        if (sub_ptr2->rect_each_image != NULL && sub_ptr2->rect_each_image_size != 0) {
                            unsigned int i = sub_ptr1->atlas_image_loc;
                            printf("            Rect %d (%d %d %d %d)\n", i, sub_ptr2->rect_each_image[4*i + 0], sub_ptr2->rect_each_image[4*i + 1], sub_ptr2->rect_each_image[4*i + 2], sub_ptr2->rect_each_image[4*i + 3]);
                        }
                    }
                    #ifdef DEBUG
                    else {
                        printf("design flaw: image_id in type IMAGE is neither DYNAMIC_IMAGE or STATIC_IMAGE\n");
                        exit(-1);
                    }
                    #endif
                    break;
                }
                case FUNCTION: {
                    Function* ptr = GetItemPointer(id);
                    printf("Function id(%d) loc(%d) active(%d)\n", id.id, id.loc, id.active);
                    break;
                }
                case NONE: {
                    printf("None id(%d) loc(%d) active(%d)\n", id.id, id.loc, id.active);
                    break;
                }
                default: {
                    printf("id(%d) loc(%d) type(%d) active(%d). type is undefined!!!\n", id.type, id.loc, id.type, id.active);
                    break;
                }
            }
        }
        void GUI_PrintAllInfo() {

            printf("printing all info\n");
            // VERTEX
            printf("vertices (%d): \n", vertices_size);
            for (unsigned int i = 0; i < vertices_size; i++) {
                printf("    rect = (%d,%d,%d,%d) | color = (%d,%d,%d,%d) | in_use = %d\n", vertices[i].rect[0], vertices[i].rect[1], vertices[i].rect[2], vertices[i].rect[3], ((unsigned char*)(&vertices[i].color))[0], ((unsigned char*)(&vertices[i].color))[1], ((unsigned char*)(&vertices[i].color))[2], ((unsigned char*)(&vertices[i].color))[3], vertices[i].in_use);
            }

            // SHAPE
            printf("shape_array (%d): \n", shape_array_size);
            for (unsigned int i = 0; i < shape_array_size; i++) {
                PrintItem(shape_array[i].id);
                printf("    vertex_loc = %d | image_loc = %d\n", shape_array[i].vertex_loc, shape_array[i].image_loc);
            }

            printf("DONE printing all infor ============\n");
        }
        void GUI_PrintMaxTextureSize() {
            GLint maxTextureSize;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
            printf("Maximum supported texture size: %d\n", maxTextureSize);
        }
        /*
        void GUI_PrintAllInfo() {

            printf("Items(%zu):\n", item_array_capacity);
            for (unsigned int i = 0; i < item_array_capacity; i++) {
                bool in_use = item_array[i].in_use;

                if (item_array[i].type == WINDOW) {
                    printf("id = %d | type = WINDOW | draw_order_id = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].item.window.draw_order_id, in_use*item_array[i].changed, in_use);
                }
                else if (item_array[i].type == NORMAL) {
                    printf("id = %d | type = NORMAL | vertex_location = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].vertex_location, in_use*item_array[i].changed, in_use);
                }
                else if (item_array[i].type == RENDERINGTEXTURE) {

                }
                else if (item_array[i].type == DRAWORDER) {
                    printf("id = %d | type = DRAWORDER | draw_order_items_size = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].item.draw_order.item_id_array_size, in_use*item_array[i].changed, in_use);
                    for (unsigned int j = 0; j < item_array[i].item.draw_order.item_id_array_size; j++) {
                        GUI_Item* item_ptr = GetItemPointer(item_array[i].item.draw_order.item_id_array[j]);
                        printf("	id = %d | type = %d | changed = %d | in_use = %d\n", in_use*item_ptr->id, in_use*item_ptr->type, in_use*item_ptr->changed, in_use);
                    }
                }
                else if (item_array[i].type == NONE) {
                    printf("id = %d | type = NONE | vertex_location = %d | changed = %d | in_use = %d\n", in_use*item_array[i].id, in_use*item_array[i].changed, in_use);
                }
                else if (item_array[i].type == CUSTOM) {

                }
                else {
                    printf("THERE IS AN FLAW IN THE DESIGNE OF GUI. AN ITEM HAS AN UNKNOWN TYPE\n");
                }
            }
            printf("Vertices(%zu): \n", vertices_capacity);
            for (unsigned int i = 0; i < vertices_capacity; i++) {
                bool in_use = vertices[i].in_use;
                printf("rect = (%d,%d,%d,%d) | color = (%d,%d,%d,%d) | in_use = %d\n", in_use*vertices[i].rect[0], in_use*vertices[i].rect[1], in_use*vertices[i].rect[2], in_use*vertices[i].rect[3],
                    in_use*((unsigned char*)(&vertices[i].color))[0], in_use*((unsigned char*)(&vertices[i].color))[1], in_use*((unsigned char*)(&vertices[i].color))[2], in_use*((unsigned char*)(&vertices[i].color))[3],
                    vertices[i].in_use);
            }
            printf("Text(%zu): \n", text_array_capacity);
            for (unsigned int i = 0; i < text_array_capacity; i++) {
                bool in_use = text_array[i].in_use;
                printf("text_length = %zu | text_capacity = %zu | in_use = %d \n", in_use*text_array[i].symbol_array_length, in_use*text_array[i].symbol_array_capacity, text_array[i].in_use);
            }
        }
        */



    // PUBLIC
        ID GUI_Sequence_Create() {

            debug(ID new_sequence_id = ID_Create(SEQUENCE));
            debug(Sequence* sequence_ptr = GetItemPointer(new_sequence_id));

            sequence_ptr->id_array = NULL;
            sequence_ptr->id_array_size = 0;
            sequence_ptr->id_array_capacity = 0;
            sequence_ptr->indices = NULL;
            sequence_ptr->indices_size = 0;
            sequence_ptr->indices_capacity = 0;

            return new_sequence_id;
        }
        ID GUI_Shape_Create(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float rotation_360, unsigned int corner_radius_pixels, ID image_id) {

            ID new_vertex_id = Vertex_Create(x,y,width,height,r,g,b,a,rotation_360,corner_radius_pixels);

            ID new_shape_id = ID_Create(SHAPE);
            Shape* shape_ptr = GetItemPointer(new_shape_id);
            shape_ptr->image_loc = image_id.loc;
            shape_ptr->vertex_loc = new_vertex_id.loc;

            printf("Shape_Create:\n");
            PrintItem(new_shape_id);

            return new_shape_id;
        }
        // when image_data_rgba_ptr is not NULL the size of the array should be the same as width and height that is provided
        // enable_mipmap is not supported yet !!!
        ID GUI_DynamicImage_Create(unsigned int width, unsigned int height, void* image_data_rgba_ptr, bool enable_mipmap) {

            DynamicImage* dynamic_image = GetItemPointer(ID_Create(DYNAMIC_IMAGE));

            dynamic_image->width = width;
            dynamic_image->height = height;
            glGenTextures(1, &dynamic_image->texture);
            glBindTexture(GL_TEXTURE_2D, dynamic_image->texture);

            // Specify the texture format and dimensions
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture when done

            return dynamic_image->id;

        }
        ID GUI_Rendering_Create(ID image_id, ID sequence_id) {

            if (image_id.type != DYNAMIC_IMAGE && image_id.type != STATIC_IMAGE) {
                printf("user flaw: You provided an id for an image that is neither a DYNAMIC_IMAGE or STATIC_IMAGE\n");
                exit(-1);
            }

            debug(Rendering* rendering_ptr = GetItemPointer(ID_Create(RENDERING)));

            if (sequence_id.id == NO_ID.id) {
                rendering_ptr->sequence_id = GUI_Sequence_Create();
            } else {
                rendering_ptr->sequence_id = sequence_id;
            }

            rendering_ptr->window_is_target = false; // this variable is needed for the one case that a rendering is for the window
            rendering_ptr->target_image_id = image_id;

            return rendering_ptr->id;
        }
        ID GUI_Window_Create(unsigned int width, unsigned int height, const char* title, unsigned int FPS) {

            if (window_created) {
                printf("user false: you have allready created the window!\n");
                exit(-1);
            }

            //glGetIntegerv(GL_MAX_IMAGE_SIZE, &maxTextureSize);

            window.window = CreateWindow(width, height, title);

            window.VBO_size = 0;
            window.VBO_capacity = 0;
            window.EBO_size = 0;
            window.EBO_capacity = 0;

            window.rendering                                = GetItemPointer(ID_Create(RENDERING));
            window.rendering->sequence_id                   = GUI_Sequence_Create();
            window.rendering->window_is_target              = true;
            window.rendering->target_image_id               = NO_ID;
            window.rendering->image_source_array            = NULL;
            window.rendering->image_source_array_size       = 0;
            window.rendering->image_source_array_capacity   = 0;
            window.rendering->function_array                = NULL;
            window.rendering->function_array_size           = 0;
            window.rendering->function_array_capacity       = 0;

            // shader program

            GLuint shaders[3];
            debug(shaders[0] = CompileShader(ReadShaderSource("shaders/uber_vertex_shader.glsl"), GL_VERTEX_SHADER));
            debug(shaders[1] = CompileShader(ReadShaderSource("shaders/uber_geometry_shader.glsl"), GL_GEOMETRY_SHADER));
            debug(shaders[2] = CompileShader(ReadShaderSource("shaders/uber_fragment_shader.glsl"), GL_FRAGMENT_SHADER));
            window.shader_program = CreateProgram(shaders, 3);
            glDeleteShader(shaders[0]);
            glDeleteShader(shaders[1]);
            glDeleteShader(shaders[2]);

            // Generate and bind a Vertex Array Object
            glGenVertexArrays(1, &window.VAO);
            glGenBuffers(1, &window.VBO);
            glGenBuffers(1, &window.EBO);

            glBindVertexArray(window.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, window.VBO);

            // pos attribute
            glEnableVertexAttribArray(0);
            glVertexAttribIPointer(0, 1, GL_INT, sizeof(Vertex), (void*)(0*sizeof(int)));
            glEnableVertexAttribArray(1);
            glVertexAttribIPointer(1, 1, GL_INT, sizeof(Vertex), (void*)(1*sizeof(int)));
            glEnableVertexAttribArray(2);
            glVertexAttribIPointer(2, 1, GL_INT, sizeof(Vertex), (void*)(2*sizeof(int)));
            glEnableVertexAttribArray(3);
            glVertexAttribIPointer(3, 1, GL_INT, sizeof(Vertex), (void*)(3*sizeof(int)));
            // tex_rect attribute
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(4*sizeof(float)));
            // color attribute
            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)(8*sizeof(float)));
            // rotation_360 attribute
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(9*sizeof(float)));
            // corner_radius and tex_loc attribute
            glEnableVertexAttribArray(7);
            glVertexAttribIPointer(7, 1, GL_UNSIGNED_INT,  sizeof(Vertex), (void*)(10*sizeof(float)));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            window.target_width_loc = glGetUniformLocation(window.shader_program, "target_width");
            window.target_height_loc = glGetUniformLocation(window.shader_program, "target_height");


            //PrintAllInfo();

            ID id;
            id.type = WINDOW;

            window_created = true;

            return id;
        }
        void GUI_AddItemToSequence(ID sequence_id, ID id) {

            if (id.type != SHAPE && id.type != SEQUENCE) {
                printf("user flaw: item to add to sequence is not of type sequence or shape\n");
                exit(-1);
            }
            if (sequence_id.type != SEQUENCE) {
                printf("user flaw: given ID for sequence GUI_AddItemToSequence is not of type SEQUENCE\n");
                exit(-1);
            }
            if (sequence_id.id == id.id) {
                printf("user flaw: cant add itself it itself\n");
                exit(-1);
            }

            Sequence* sequence_ptr = GetItemPointer(sequence_id);
            debug(sequence_ptr->id_array = Array_ManageMemory(sequence_ptr->id_array, &sequence_ptr->id_array_size, &sequence_ptr->id_array_capacity, sizeof(ID)));
            sequence_ptr->id_array[sequence_ptr->id_array_size] = id;
            sequence_ptr->id_array_size++;

            printf("AddItemToSequence:\n");
            PrintItem(sequence_id);
            PrintItem(id);
        }
        void GUI_AddItemToWindow(ID id) {

            if (id.type != SHAPE && id.type != SEQUENCE && id.type != RENDERING) {
                printf("user flaw: item to add to sequence is not of type sequence or shape or rendering\n");
                exit(-1);
            }

            Sequence* sequence_ptr = GetItemPointer(window.rendering->sequence_id);

            debug(sequence_ptr->id_array = Array_ManageMemory(sequence_ptr->id_array, &sequence_ptr->id_array_size, &sequence_ptr->id_array_capacity, sizeof(ID)));
            sequence_ptr->id_array[sequence_ptr->id_array_size] = id;
            sequence_ptr->id_array_size++;

            printf("AddItemToWindow:\n");
            PrintItem(window.rendering->sequence_id);
            PrintItem(id);

        }
        void GUI_Window_Show() {

            GUI_PrintAllInfo();

            unsigned int*   sequence_loc_array = NULL;
            unsigned int    sequence_loc_array_size = 0;
            unsigned int    sequence_loc_array_capacity = 0;

            while (!glfwWindowShouldClose(window.window)) {

                if (glfwGetKey(window.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                    glfwSetWindowShouldClose(window.window, true);
                }
                debug(GUI_PrintFPS());

                {
                    for (int i = rendering_array_size-1; i >= 0; i--) {

                        //  skip it not active
                        if (!rendering_array[i].id.active) {
                            continue;
                        }

                        // everything needed for drawing onto target including any necessary data update.
                        {
                            // making a list of all used sequences then finding the indices for every sequence
                            {
                                #ifdef DEBUG
                                if (rendering_array[i].sequence_id.type != SEQUENCE) {
                                    printf("design flaw: there is no sequence inside rendering\n");
                                    exit(-1);
                                }
                                #endif

                                sequence_loc_array_size = 1;
                                debug(sequence_loc_array = Array_ManageMemory(sequence_loc_array, &sequence_loc_array_size, &sequence_loc_array_capacity, sizeof(unsigned int)));
                                sequence_loc_array[0] = rendering_array[i].sequence_id.loc;

                                // finding all sequences used for this rendering
                                for (unsigned int j = 0; j < sequence_loc_array_size; j++) {

                                    Sequence* sequence_ptr = &(sequence_array[sequence_loc_array[j]]);

                                    for (unsigned int k = 0; k < sequence_ptr->id_array_size; k++) {

                                        ID id_in_sequence = sequence_ptr->id_array[k];

                                        if (id_in_sequence.type == SHAPE) {
                                            continue;
                                        }
                                        #ifdef DEBUG
                                        else if (id_in_sequence.type != SEQUENCE) {
                                            printf("design flaw: id inside sequence should be either SEQUENCE or SHAPE\n");
                                            exit(-1);
                                        }
                                        #endif

                                        // if type==SEQUENCE then append location
                                        debug(sequence_loc_array = Array_ManageMemory(sequence_loc_array, &sequence_loc_array_size, &sequence_loc_array_capacity, sizeof(unsigned int)));
                                        sequence_loc_array[sequence_loc_array_size] = id_in_sequence.loc;
                                        sequence_loc_array_size++;
                                    }
                                }

                                // updating indices in all sequences just found. starting from the last found sequence
                                for (int j = sequence_loc_array_size-1; j>=0; j--) {
                                    Sequence* sequence_ptr = &(sequence_array[sequence_loc_array[j]]);
                                    sequence_ptr->indices_size = 0;
                                    for (unsigned int k = 0; k < sequence_ptr->id_array_size; k++) {

                                        ID id_in_sequence = sequence_ptr->id_array[k];

                                        // if SHAPE then only appending the index in that shape
                                        if (id_in_sequence.type == SHAPE) {
                                            Shape* shape_ptr = GetItemPointer(id_in_sequence);
                                            debug(sequence_ptr->indices = Array_ManageMemory(sequence_ptr->indices, &sequence_ptr->indices_size, &sequence_ptr->indices_capacity, sizeof(unsigned int)));
                                            sequence_ptr->indices[sequence_ptr->indices_size] = shape_ptr->vertex_loc;
                                            sequence_ptr->indices_size++;
                                        }

                                        // if SEQUENCE then append all indices in that other sequence
                                        else if (id_in_sequence.type == SEQUENCE) {
                                            Sequence* other_sequence_ptr = GetItemPointer(id_in_sequence);
                                            for (unsigned int l = 0; l < other_sequence_ptr->indices_size; l++) {
                                                debug(sequence_ptr->indices = Array_ManageMemory(sequence_ptr->indices, &sequence_ptr->indices_size, &sequence_ptr->indices_capacity, sizeof(unsigned int))); // resizes only if needed
                                                sequence_ptr->indices[sequence_ptr->indices_size] = other_sequence_ptr->indices[l];
                                                sequence_ptr->indices_size++;
                                            }
                                        }
                                    }
                                }
                                //free(sequence_loc_array);
                            }

                            // now indices in the sequence for this rendering is updated

                            Sequence* sequence_ptr = GetItemPointer(rendering_array[i].sequence_id);

                            // updating VBO
                            {
                                glBindBuffer(GL_ARRAY_BUFFER, window.VBO);
                                if (vertices_capacity > window.VBO_capacity) {
                                    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*vertices_capacity, vertices, GL_DYNAMIC_DRAW);
                                    window.VBO_capacity = vertices_capacity;
                                    for (unsigned int j = 0; j < sequence_ptr->indices_size; j++) {
                                        vertices[sequence_ptr->indices[j]].synced_with_VBO = true;
                                    }
                                }
                                else {
                                    for (unsigned int j = 0; j < sequence_ptr->indices_size; j++) {
                                        if (vertices[sequence_ptr->indices[j]].synced_with_VBO) {
                                            continue;
                                        }
                                        glBufferSubData(GL_ARRAY_BUFFER, sizeof(Vertex)*sequence_ptr->indices[j], sizeof(Vertex), &vertices[sequence_ptr->indices[j]]);
                                        vertices[sequence_ptr->indices[j]].synced_with_VBO = true;
                                    }
                                }
                                window.VBO_size = vertices_size;
                                glBindBuffer(GL_ARRAY_BUFFER, 0);
                            }

                            // updating EBO
                            {
                                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, window.EBO);
                                if (sequence_ptr->indices_size >= window.EBO_capacity) {
                                    debug(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sequence_ptr->indices_capacity*sizeof(unsigned int), sequence_ptr->indices, GL_DYNAMIC_DRAW));
                                    window.EBO_capacity = sequence_ptr->indices_capacity;
                                }
                                else {
                                    debug(glBufferSubData(window.EBO, 0, sequence_ptr->indices_size, sequence_ptr->indices));
                                }
                                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                                window.EBO_size = sequence_ptr->indices_size;
                            }

                            // rendering
                            {
                                if (rendering_array[i].window_is_target) {

                                    int width, height;
                                    glfwGetWindowSize(window.window, &width, &height);

                                    glBindRenderingbuffer(GL_RENDERINGBUFFER, 0);
                                    glViewport(0, 0, width, height);

                                    glClear(GL_COLOR_BUFFER_BIT);

                                    glUseProgram(window.shader_program);

                                    glUniform1ui(window.target_width_loc, width);
                                    glUniform1ui(window.target_height_loc, height);

                                    glBindVertexArray(window.VAO);
                                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, window.EBO);
                                    glDrawElements(GL_POINTS, window.EBO_size, GL_UNSIGNED_INT, 0);

                                    glBindVertexArray(0);
                                }

                                // if now window it is static or dynamic image
                                else {

                                    int width, height;
                                    glfwGetWindowSize(window.window, &width, &height);

                                    glBindRenderingbuffer(GL_RENDERINGBUFFER, window.FBO);
                                    glViewport(0, 0, width, height);

                                    glClear(GL_COLOR_BUFFER_BIT);

                                    glUseProgram(window.shader_program);

                                    glUniform1ui(window.target_width_loc, width);
                                    glUniform1ui(window.target_height_loc, height);

                                    glBindVertexArray(window.VAO);
                                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, window.EBO);
                                    glDrawElements(GL_POINTS, window.EBO_size, GL_UNSIGNED_INT, 0);

                                    glBindVertexArray(0);

                                }
                            }
                        }
                    }
                }
                if (window.window == NULL) {
                    printf("window==NULL\n");
                }
                // Swap buffers and poll events
                debug(glfwSwapBuffers(window.window));
                glfwPollEvents();
            }

        }
        void GUI_Window_Delete() {

        }

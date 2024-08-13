#include "array.h"
#include "debug.h"

typedef struct {
	void* ptr;
	size_t size;
	size_t capacity;
	size_t type_size;
} Array;

void* alloc(void* ptr, size_t length, size_t type_size) {
	size_t size = length*type_size;
    void* tmp = ptr==NULL ?
		malloc(size) :
                realloc(ptr, size) ;
    if (!tmp) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    return tmp;
}
Array CreateArray(size_t type_size) {

	#ifdef DEBUG
	if (type_size == 0) {
		printf("user flaw: type_size should never be 0\n")
		exit(-1);
	}
	#endif

	Array array;
	array.ptr = NULL;
	array.size = 0;
	array.capacity = 0;
	array.type_size = type_size;
	return array;
}
void Array_Append(Array* array_ptr, void* data, size_t data_size, size_t type_size) {

	// error handling
	#ifdef DEBUG
	if (array_ptr == NULL) {
		printf("user flaw: array_ptr == NULL\n");
		exit(-1);
	}
	if (data == NULL) {
		printf("user flaw: data == NULL\n");
		exit(-1);
	}
	if (data_size == 0) {
		printf("user flaw: data_size == 0\n");
		exit(-1);
	}
	if (type_size == 0) {
		printf("user flaw: type_size == 0\n");
		exit(-1);
	}
	if (array_ptr->capacity*array_ptr->type_size != DebugGetSizeBytes(array_ptr->ptr)) {
		printf("user flaw: array_ptr->capacity*array_ptr->type_size != DebugGetSizeBytes(array_ptr->ptr)");
		exit(-1);
	}
	if (data_size*type_size != DebugGetSizeBytes(data)) {
		printf("user flaw: data_size*type_size != DebugGetSizeBytes(data)");
		exit(-1);
	}
	#endif

	// resize if necessary
	if (array_ptr->capacity < array_ptr->size + data_size) {
		if (array_ptr->capacity == 0) {
			array_ptr->capacity = 1;
		}
		while (array_ptr->capacity < array_ptr->size + data_size*type_size) {
			array_ptr->capacity *= 2;
		}
		debug(array_ptr->data = alloc(array_ptr->data, array_ptr->capacity, array_ptr->type_size));
	}

	// adding data to array
	memcpy(	&(((unsigned char*)array_ptr->ptr)[array_ptr->size*array_ptr->type_size]),
					data,
					data_size*type_size);

	array_ptr->size += data_size;

}
void Array_RemoveMatchingData(Array* array_ptr, void* data, size_t data_size, size_t type_size, size_t search_stride_bytes) {

}
void Array_RemoveIndex() {

}
void Array_ReplaceMatchingData(Array* array_ptr, void* old_data, void* new_data, size_t data_size, size_t type_size, size_t search_stride_bytes) {

}
void Array_ReplaceIndex() {

}

#include "debug.h"
#include <string.h>

void* alloc(void* ptr, unsigned int size) {
    void* tmp = (ptr==NULL ? malloc(size) : realloc(ptr, size));
    if (!tmp) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    return tmp;
}


void* Array_ManageMemory(void* ptr, unsigned int* size_ptr, unsigned int* capacity_ptr, unsigned int element_size) {

#ifdef DEBUG
	if (size_ptr == NULL) {
		printf("design flaw: size_ptr == NULL\n");
		exit(-1);
	}
	if (capacity_ptr == NULL) {
		printf("design flaw: capacity_ptr == NULL\n");
		exit(-1);
	}
	if (element_size==0) {
		printf("design flaw: element_size==0\n");
		exit(-1);
	}
#endif

	if (ptr == NULL) {
		*size_ptr = 0;
		*capacity_ptr = 1;
		debug(ptr = alloc(ptr, *capacity_ptr*element_size));
		return ptr;
	}
	if (*size_ptr < *capacity_ptr) {
		return ptr;
	}
	if (*capacity_ptr == 0) {
		*capacity_ptr = 1;
	}
	while (*size_ptr >= *capacity_ptr) {
		*capacity_ptr *= 2;
	}
	debug(ptr = alloc(ptr, *capacity_ptr*element_size));
	return ptr;

}
void Array_Append(void* ptr, unsigned int size, unsigned int capacity, void* new_data_ptr, unsigned int new_data_size) {

	// error handling
	#ifdef DEBUG
	if (ptr == NULL) {
		printf("user flaw: ptr == NULL\n");
		exit(-1);
	}
	if (new_data_ptr == NULL) {
		printf("user flaw: new_data_ptr == NULL\n");
		exit(-1);
	}
	if (new_data_size == 0) {
		printf("user flaw: new_data_size == 0\n");
		exit(-1);
	}
	if (capacity != DebugGetSizeBytes(ptr)) {
		printf("user flaw: capacity != DebugGetSizeBytes(ptr) or ptr is not allocated in the standard way with debug.h included\n");
		exit(-1);
	}
	if (new_data_size != DebugGetSizeBytes(new_data_ptr)) {
		printf("user flaw: new_data_size != DebugGetSizeBytes(new_data_ptr) or new_data_ptr is not allocated in the standard way qwith debug.h included\n");
		exit(-1);
	}
	#endif

	// resize if necessary
	if (capacity < size + new_data_size) {
		if (capacity == 0) {
			capacity = 1;
		}
		while (capacity < size + new_data_size) {
			capacity *= 2;
		}
		debug(ptr = alloc(ptr, capacity));
	}

	// adding data to array
	memcpy(&(((unsigned char*)ptr)[size]), new_data_ptr, new_data_size);

	size += new_data_size;

}

#include "include_ffi.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// ffi_type** ffi_type_alloc_array(size_t size, ffi_type** types)
// {
//     ffi_type** copy = malloc((size + 1) 

void ffi_type_destroy(ffi_type* type)
{
    if (type == NULL) return;

    if (type->type == FFI_TYPE_STRUCT || type->type == FFI_TYPE_COMPLEX) {
        ffi_type_destroy_array(type->elements);
        free(type);
    }
}

void ffi_type_destroy_array(ffi_type** types)
{
    for (ffi_type** curr = types; *curr != NULL; ++curr)
        ffi_type_destroy(*curr);

    free(types);
}

ffi_type* ffi_type_clone(ffi_type* type)
{
    if (type == NULL) return NULL;

    if (type->type == FFI_TYPE_STRUCT || type->type == FFI_TYPE_COMPLEX) {
        ffi_type* copy = malloc(sizeof(ffi_type));
        assert(copy != NULL);

        memcpy(copy, type, sizeof(ffi_type));
        copy->elements = ffi_type_clone_array(type->elements);

        return copy;
    } else
        return type;
}

ffi_type** ffi_type_clone_array(ffi_type** types)
{
    size_t size = 1;
    for (ffi_type** curr = types; *curr != NULL; ++curr)
        ++size;

    ffi_type** copy = malloc(size * sizeof(ffi_type*));
    assert(copy != NULL);

    for (size_t i = 0; i < size; ++i)
        copy[i] = ffi_type_clone(types[i]);

    return copy;
}

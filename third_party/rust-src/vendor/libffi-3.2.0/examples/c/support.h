#include <ffi.h>

void ffi_type_destroy(ffi_type*);
void ffi_type_destroy_array(ffi_type**);

ffi_type* ffi_type_clone(ffi_type*);
ffi_type** ffi_type_clone_array(ffi_type**);

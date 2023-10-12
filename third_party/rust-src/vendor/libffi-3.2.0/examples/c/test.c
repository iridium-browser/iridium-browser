#include <assert.h>
#include <stdio.h>
#include <ffi.h>

void callback(ffi_cif* cif, void* result, void** args, void* userdata)
{
    *(int*)result = *(int*)userdata + **(int**)args;
}

int main()
{
    ffi_cif cif;
    ffi_type* args[1] = { &ffi_type_sint };
    ffi_closure* closure;
    int (*fn)(int);
    int env = 5;

    assert(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1,
                        &ffi_type_sint, args)
            == FFI_OK);
    closure = ffi_closure_alloc(sizeof(ffi_closure), (void**)&fn);
    assert(ffi_prep_closure_loc(closure, &cif, callback, &env, fn)
            == FFI_OK);

    printf("%d\n", fn(6));
    printf("%d\n", fn(7));
}

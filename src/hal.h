#include <hal/libhal.h>

LibHalContext *get_hal_ctx(void);
void put_hal_ctx(LibHalContext *hal_ctx);
char* get_uuid(LibHalContext *hal_ctx);

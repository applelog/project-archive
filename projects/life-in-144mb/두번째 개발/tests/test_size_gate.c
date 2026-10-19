#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define MAX_ALLOWED_BYTES 1474560

int main(void) {
    printf("[TEST] Size gate limit constant check...\n");
    assert(MAX_ALLOWED_BYTES == 1474560);
    assert(MAX_ALLOWED_BYTES == 1440 * 1024);
    
    printf("[PASS] test_size_gate passed! 1.44MB = %d bytes\n", MAX_ALLOWED_BYTES);
    return 0;
}

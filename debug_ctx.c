#include "exceptions.h"
void test(struct exception_context *ctx) {
    uint64_t far = ctx->far;
    (void)far;
}

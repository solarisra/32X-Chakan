#ifndef CHAKAN_SH_TEST_WIDE_H
#define CHAKAN_SH_TEST_WIDE_H
#include <stdint.h>
#include "gdl/gdl.h"
void test_wide_init(void);
void test_wide_update(uint16_t pad);
const GdlFrame *test_wide_render_frame(void);
#endif

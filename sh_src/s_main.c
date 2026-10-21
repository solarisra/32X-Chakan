#include "mars.h"
#include "game_config.h"
#if GAME_TEST_WIDE
#include "gdl/scheduler.h"
#endif

void s_main(void) {
#if GAME_TEST_WIDE
    gdl_slave_loop();
#else
    for (;;) {}
#endif
}

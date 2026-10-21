#include "mars.h"
#include "game_config.h"
#if GAME_SH2
#include "gdl/scheduler.h"
#endif

void s_main(void) {
#if GAME_SH2
    gdl_slave_loop();
#else
    for (;;) {}
#endif
}

#ifndef CHAKAN_INPUT_CONTROLS_H
#define CHAKAN_INPUT_CONTROLS_H
#include <stdint.h>
#define BUTTON_UP    0x0001
#define BUTTON_DOWN  0x0002
#define BUTTON_LEFT  0x0004
#define BUTTON_RIGHT 0x0008
#define BUTTON_B     0x0010
#define BUTTON_C     0x0020
#define BUTTON_A     0x0040
#define BUTTON_START 0x0080
#define BUTTON_RETURN_HUB (BUTTON_START | BUTTON_A | BUTTON_C)
#define BUTTON_JUMP (BUTTON_A | BUTTON_B | BUTTON_C)
extern uint16_t prevJoyInput;
void input_init(void);
/* MarsDev main reads the controller once per frame and supplies its state. */
void processInput(uint16_t joyState);
void joyHandlerStatePlay(uint16_t changed, uint16_t state);
#endif

#ifndef MYOS_STARBOARD_EVENT_ABI_H
#define MYOS_STARBOARD_EVENT_ABI_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
/* Pinned Starboard 18 x64 wire declarations. Fixtures compile the upstream
 * event/input/key headers independently and check these layouts and values. */
enum { SB_PRELOAD=0,SB_START=1,SB_BLUR=2,SB_FOCUS=3,SB_CONCEAL=4,SB_REVEAL=5,
       SB_FREEZE=6,SB_UNFREEZE=7,SB_STOP=8,SB_INPUT=9 };
struct sb_event { int type;int64_t timestamp;void *data; };
struct sb_start_data { char **argument_values;int argument_count;const char *link; };
struct sb_vector { float x,y; };
struct sb_input_data {
    void *window;int type,device_type,device_id,key;int32_t character;
    int key_location;unsigned key_modifiers;
    struct sb_vector position,delta;float pressure;
    struct sb_vector size,tilt;const char *input_text;bool is_composing;
};
_Static_assert(sizeof(struct sb_event)==24&&offsetof(struct sb_event,timestamp)==8,"SbEvent ABI");
_Static_assert(sizeof(struct sb_start_data)==24&&offsetof(struct sb_start_data,link)==16,"SbEventStartData ABI");
_Static_assert(sizeof(struct sb_input_data)==88&&offsetof(struct sb_input_data,input_text)==72,"SbInputData ABI");
int SbRunStarboardMain(int,char **,void (*)(const struct sb_event *));
uint32_t SbEventSchedule(void (*)(void *),void *,int64_t);
void SbEventCancel(uint32_t);
void SbSystemRequestStop(int);
void SbSystemRequestConceal(void);
void SbSystemRequestReveal(void);
void SbSystemRequestBlur(void);
void SbSystemRequestFocus(void);
bool SbSystemSupportsResume(void);
#endif

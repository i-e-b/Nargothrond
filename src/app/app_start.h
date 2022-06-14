#ifndef SdlBase_App_Start_h
#define SdlBase_App_Start_h

#include <SDL.h>
#include "scene.h"
#include "types/general.h"
#include "shared_types.h"

/******************************************
 * Application settings                   *
 ******************************************/

// Screen dimension constants
const int SCREEN_WIDTH = 800;//1280;
const int SCREEN_HEIGHT = 600;//720;

// Ideal frame duration for frame limit, in milliseconds
#define FRAME_TIME_TARGET 15
// If set, renderer will try to hit the ideal frame time (by delaying frames, or postponing input events as required)
// Otherwise, drawing will be as fast as possible, and events are handled every frame
#define FRAME_LIMIT 1
// If defined, renderer will run in a parallel thread. Otherwise, draw and render will run in sequence
#define MULTI_THREAD 1
// If defined, the output screen will remain visible after the test run is complete
//#define WAIT_AT_END 1


/******************************************
 * Main application implementation points *
 ******************************************/

// Called once at app start
void StartUp(volatile ApplicationGlobalState *state);
// Called for every frame. Update the scene ready for next render
void UpdateModel(volatile ApplicationGlobalState *state, uint32_t frame, uint32_t frameTime);

// Called for every frame. Draw scene to buffer
void RenderFrame(volatile ApplicationGlobalState *state,SDL_Surface *screen);

// Called when an SDL event is consumed
void HandleEvent(SDL_Event *event, volatile ApplicationGlobalState *state);

// Called once at app stop
void Shutdown(volatile ApplicationGlobalState *state);

#endif //SdlBase_App_Start_h

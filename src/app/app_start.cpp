#include "app_start.h"

#include "src/gui_core/ScanBufferFont.h"
#include "src/types/MemoryManager.h"
#include "src/types/String.h"
#include "scene.h"

// Handy SDL docs: https://wiki.libsdl.org/


void HandleEvent(SDL_Event *event, volatile ApplicationGlobalState *state) {
    // see lib/SDL2-devel-2.0.9-VC/SDL2-2.0.9/include/SDL_events.h
    //     https://wiki.libsdl.org/SDL_EventType

    if (event->type == SDL_KEYDOWN || event->type == SDL_QUIT) {
        state->running = false;
        return;
    }
}

void UpdateModel(volatile ApplicationGlobalState *state, int frame, uint32_t frameTime) {
    MMPush(1 MEGABYTE); // prepare a per-frame bump allocator

    // todo: stuff
    if (state == nullptr) return;
    if (frame < 0) return;
    if (frameTime < 0) return;

    state->t++;
    if (state->t > 10) state->t = 0;

    MMPop(); // wipe out anything we allocated in this frame.
}

void RenderFrame(volatile ApplicationGlobalState *state,SDL_Surface *screen){
    // todo: stuff
    if (state == nullptr) return;
    if (screen == nullptr) return;

    auto base = (BYTE*)screen->pixels;

    int x = 0, c = state->t * 25, j;

    int rowBytes = screen->pitch;

    while (x < screen->w && x < screen->h){
        j = (x*4) + (x*rowBytes);
        base[j++] = (char)c; // B
        base[j++] = (char)c; // G
        base[j++] = (char)c; // R
        x++;
    }
}

void StartUp(volatile ApplicationGlobalState *state) {
    StartManagedMemory(); // use the semi-auto memory helper

    if (state == nullptr) return;
    state->t = 1;
}

void Shutdown(volatile ApplicationGlobalState *state) {
    ShutdownManagedMemory(); // deallocate everything
    if (state == nullptr) return;
}


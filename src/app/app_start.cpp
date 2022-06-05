#include "app_start.h"

#include <cstdlib>

#include "src/types/MemoryManager.h"
#include "scene.h"
#include "synth/map_synth.h"

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
//    auto hmap = state->heightMap;
    auto cmap = state->colorMap;

    int j;

    int rowBytes = screen->pitch;

    // for now, show the height map
    for (int y = 0; y < 512; ++y) {
        int mapY = y*512*3;
        int bmpY = y*rowBytes;

        for (int x = 0; x < 512; ++x) {
            //char h = hmap[x+mapY];
            int i = (x*3)+mapY;

            BYTE r = cmap[i++];
            BYTE g = cmap[i++];
            BYTE b = cmap[i++];

            j = (x*4) + bmpY;
            base[j++] = b; // B
            base[j++] = g; // G
            base[j++] = r; // R
        }
    }
}

void StartUp(volatile ApplicationGlobalState *state) {
    StartManagedMemory(); // use the semi-auto memory helper
    //MMPush(10 MEGABYTE); // memory for global state

    if (state == nullptr) return;
    state->t = 1;

    MapSynthInit();

    // synthesise maps. This should be dynamic based on wider area later
    //state->heightMap = (char*)ArenaAllocateAndClear(MMCurrent(), 512*512); // the very basic allocator doesn't handle big maps.
    state->heightMap = (BYTE*)malloc(512*512);
    state->colorMap = (BYTE*)malloc(512*512*3);

    GenerateHeight(512, 5, state->heightMap);
    GenerateColor(512, state->heightMap, state->colorMap);
}

void Shutdown(volatile ApplicationGlobalState *state) {
    MapSynthDispose();

    ShutdownManagedMemory(); // deallocate everything

    if (state == nullptr) return;
}


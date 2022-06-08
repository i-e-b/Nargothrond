#include "app_start.h"

#include <cstdlib>

#include "src/types/MemoryManager.h"
#include "scene.h"
#include "synth/map_synth.h"

// Handy SDL docs: https://wiki.libsdl.org/


void HandleEvent(SDL_Event *event, volatile ApplicationGlobalState *state) {
    // see lib/SDL2-devel-2.0.9-VC/SDL2-2.0.9/include/SDL_events.h
    //     https://wiki.libsdl.org/SDL_EventType

    if (event->type == SDL_KEYDOWN) {
        auto sym = event->key.keysym.sym;
        state->showColor = false;
        state->showHeight = false;

        if (sym == SDLK_c) {
            state->showColor = true;
        } else if (sym == SDLK_h){
            state->showHeight = true;
        } else if (sym == SDLK_LEFT){
            state->scene->camAngle += 0.1;
        } else if (sym == SDLK_RIGHT){
            state->scene->camAngle -= 0.1;
        }
    }

    if (event->type == SDL_QUIT) { // window close button
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

    state->scene->camX += 1;
    if (state->scene->camX >= 1024) state->scene->camX = 0;

    MMPop(); // wipe out anything we allocated in this frame.
}

void showColorMap(volatile ApplicationGlobalState *state, SDL_Surface *screen){

    auto base = (BYTE*)screen->pixels;
    auto cmap = state->colorMap;

    int rowBytes = screen->pitch;

    // for now, show the height map
    for (int y = 0; y < 512; ++y) {
        int mapY = y*512*3;
        int bmpY = y*rowBytes;

        for (int x = 0; x < 512; ++x) {
            int i = (x*3)+mapY;

            BYTE r = cmap[i++];
            BYTE g = cmap[i++];
            BYTE b = cmap[i++];

            int j = (x*4) + bmpY;
            base[j++] = b; // B
            base[j++] = g; // G
            base[j++] = r; // R
        }
    }
}

void showHeightMap(volatile ApplicationGlobalState *state, SDL_Surface *screen){
    auto base = (BYTE*)screen->pixels;
    auto hmap = state->heightMap;

    int rowBytes = screen->pitch;

    // for now, show the height map
    for (int y = 0; y < 512; ++y) {
        int mapY = y*512;
        int bmpY = y*rowBytes;

        for (int x = 0; x < 512; ++x) {
            BYTE h = hmap[x+mapY];

            int j = (x*4) + bmpY;
            base[j++] = h; // B
            base[j++] = h; // G
            base[j++] = h; // R
        }
    }
}


void RenderFrame(volatile ApplicationGlobalState *state, SDL_Surface *screen){
    // todo: stuff
    if (state == nullptr) return;
    if (screen == nullptr) return;

    if (state->showColor){
        showColorMap(state, screen);
    } else if (state->showHeight) {
        showHeightMap(state, screen);
    } else {
        // render the scene
        RenderScene(state, screen);
    }

}

void StartUp(volatile ApplicationGlobalState *state) {
    StartManagedMemory(); // use the semi-auto memory helper
    //MMPush(10 MEGABYTE); // memory for global state

    if (state == nullptr) return;

    MapSynthInit();

    // synthesise maps. This should be dynamic based on wider area later
    //state->heightMap = (char*)ArenaAllocateAndClear(MMCurrent(), 512*512); // the very basic allocator doesn't handle big maps.
    state->heightMap = (BYTE*)malloc(512*512);
    state->colorMap = (BYTE*)malloc(512*512*3);

    state->mapSize = 512;
    GenerateHeight(512, 5, state->heightMap);
    GenerateColor(512, state->heightMap, state->colorMap);
    InitScene(state);
}

void Shutdown(volatile ApplicationGlobalState *state) {
    MapSynthDispose();

    ShutdownManagedMemory(); // deallocate everything

    if (state == nullptr) return;
}


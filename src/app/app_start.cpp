
#include "app_start.h"
#include "src/types/MemoryManager.h"
#include "scene.h"
#include "synth/map_synth.h"

// Handy SDL docs: https://wiki.libsdl.org/

void HandleEvent(SDL_Event *event, volatile ApplicationGlobalState *state) {
    // see lib/SDL2-devel-2.0.9-VC/SDL2-2.0.9/include/SDL_events.h
    //     https://wiki.libsdl.org/SDL_EventType

    if (event->type == SDL_KEYDOWN) { // both down and repeat

        auto sym = event->key.keysym.sym;
        state->showColor = false;
        state->showHeight = false;
        state->showShadow = false;

        if (sym == SDLK_q) {
            state->scene->shadowAngle += 1;
            // update shadow with time. This should really be done on another thread, and rarely.
            GenerateShadow(512, state->heightMap, state->shadowMap, state->scene->shadowAngle);
        } else if (sym == SDLK_e){
            state->scene->shadowAngle -= 1;
            // update shadow with time. This should really be done on another thread, and rarely.
            GenerateShadow(512, state->heightMap, state->shadowMap, state->scene->shadowAngle);
        }

        if (event->key.repeat > 0) return;

        if (sym == SDLK_c) {
            state->showColor = true;
        } else if (sym == SDLK_h) {
            state->showHeight = true;
        } else if (sym == SDLK_l) {
            state->showShadow = true;
        } else if (sym == SDLK_LEFT) {
            state->scene->moveTurnLeft = 1;
        } else if (sym == SDLK_RIGHT) {
            state->scene->moveTurnLeft = -1;
        } else if (sym == SDLK_UP) {
            state->scene->moveLookUp = 1;
        } else if (sym == SDLK_DOWN) {
            state->scene->moveLookUp = -1;
        } else if (sym == SDLK_LEFTBRACKET) {
            state->scene->moveUp = -1;
        } else if (sym == SDLK_RIGHTBRACKET) {
            state->scene->moveUp = 1;
        } else if (sym == SDLK_w) {
            state->scene->moveForward = 1;
        } else if (sym == SDLK_s) {
            state->scene->moveForward = -1;
        } else if (sym == SDLK_d) {
            state->scene->moveStrafeLeft = -1;
        } else if (sym == SDLK_a) {
            state->scene->moveStrafeLeft = 1;
        }
    }

    if (event->type == SDL_KEYUP) {
        auto sym = event->key.keysym.sym;
        if (sym == SDLK_LEFT) {
            state->scene->moveTurnLeft = 0;
        } else if (sym == SDLK_RIGHT) {
            state->scene->moveTurnLeft = 0;
        } else if (sym == SDLK_UP) {
            state->scene->moveLookUp = 0;
        } else if (sym == SDLK_DOWN) {
            state->scene->moveLookUp = 0;
        } else if (sym == SDLK_LEFTBRACKET) {
            state->scene->moveUp = 0;
        } else if (sym == SDLK_RIGHTBRACKET) {
            state->scene->moveUp = 0;
        } else if (sym == SDLK_w) {
            state->scene->moveForward = 0;
        } else if (sym == SDLK_s) {
            state->scene->moveForward = 0;
        } else if (sym == SDLK_d) {
            state->scene->moveStrafeLeft = 0;
        } else if (sym == SDLK_a) {
            state->scene->moveStrafeLeft = 0;
        }
    }

    if (event->type == SDL_QUIT) { // window close button
        state->running = false;
        return;
    }
}

void UpdateModel(volatile ApplicationGlobalState *state, uint32_t frame, uint32_t frameTime) {
    //MMPush(1 MEGABYTE); // prepare a per-frame bump allocator
    if (state == nullptr) return;
    if (frame < 0) return;
    if (frameTime < 0) return;

    state->scene->sceneTime += (double) frameTime / 1000;

    // movement
    auto dx = sin(state->scene->camAngle);
    auto dy = cos(state->scene->camAngle);
    // strafe
    state->scene->camX -= dy * state->scene->moveStrafeLeft;
    state->scene->camY += dx * state->scene->moveStrafeLeft;
    // walk
    state->scene->camX -= 1.5 * dx * state->scene->moveForward;
    state->scene->camY -= 1.5 * dy * state->scene->moveForward;
    // fly up/down
    state->scene->camHeight += 5 * state->scene->moveUp;
    // look up/down (pitch)
    state->scene->camPitch += 10 * state->scene->moveLookUp;
    // turn
    state->scene->camAngle += 0.03 * state->scene->moveTurnLeft;


    //MMPop(); // wipe out anything we allocated in this frame.
}

void showColorMap(volatile ApplicationGlobalState *state, SDL_Surface *screen) {

    auto base = (BYTE *) screen->pixels;
    auto cmap = state->colorMap;

    int rowBytes = screen->pitch;

    // for now, show the height map
    for (int y = 0; y < 512; ++y) {
        int mapY = y * 512 * 3;
        int bmpY = y * rowBytes;

        for (int x = 0; x < 512; ++x) {
            int i = (x * 3) + mapY;

            BYTE r = cmap[i++];
            BYTE g = cmap[i++];
            BYTE b = cmap[i++];

            int j = (x * 4) + bmpY;
            base[j++] = b; // B
            base[j++] = g; // G
            base[j++] = r; // R
        }
    }
}

void showHeightMap(volatile ApplicationGlobalState *state, SDL_Surface *screen) {
    auto base = (BYTE *) screen->pixels;
    auto hmap = state->heightMap;

    int rowBytes = screen->pitch;

    // for now, show the height map
    for (int y = 0; y < 512; ++y) {
        int mapY = y * 512;
        int bmpY = y * rowBytes;

        for (int x = 0; x < 512; ++x) {
            BYTE h = hmap[x + mapY];

            int j = (x * 4) + bmpY;
            base[j++] = h; // B
            base[j++] = h; // G
            base[j++] = h; // R
        }
    }
}

void showShadowMap(volatile ApplicationGlobalState *state, SDL_Surface *screen) {
    auto base = (BYTE *) screen->pixels;
    auto shadowMap = state->shadowMap;

    int rowBytes = screen->pitch;

    for (int y = 0; y < 512; ++y) {
        int mapY = y * 512;
        int bmpY = y * rowBytes;

        for (int x = 0; x < 512; ++x) {
            BYTE h = shadowMap[x + mapY];

            int j = (x * 4) + bmpY;
            base[j++] = h * 0xFF; // B
            base[j++] = h * 0xFF; // G
            base[j++] = h * 0xFF; // R
        }
    }
}


void RenderFrame(volatile ApplicationGlobalState *state, SDL_Surface *screen) {
    if (state == nullptr) return;
    if (screen == nullptr) return;

    if (state->showColor) {
        showColorMap(state, screen);
    } else if (state->showHeight) {
        showHeightMap(state, screen);
    } else if (state->showShadow) {
        showShadowMap(state, screen);
    } else {
        // render the scene
        RenderScene(state, screen);
    }

}

void StartUp(volatile ApplicationGlobalState *state) {
    StartManagedMemory(); // use the semi-auto memory helper
    MMPush(10 MEGABYTE); // memory for global state

    if (state == nullptr) return;

    MapSynthInit();

    // synthesise maps. This should be dynamic based on wider area later
    state->heightMap = (BYTE *) MMAllocate(512 * 512);
    state->colorMap = (BYTE *) MMAllocate(512 * 512 * 3);
    state->shadowMap = (BYTE *) MMAllocate(512 * 512);

    // screen-to-map lookups
    state->depthMap = (BYTE*) MMAllocate(SCREEN_HEIGHT*SCREEN_WIDTH);
    state->coordMap = (uint32_t*) MMAllocate(SCREEN_HEIGHT*SCREEN_WIDTH*sizeof(uint32_t));
    InitScene(state);

    state->mapSize = 512;
    GenerateHeight(512, 5, state->heightMap);
    GenerateColor(512, state->heightMap, state->colorMap);
    GenerateShadow(512, state->heightMap, state->shadowMap, state->scene->shadowAngle);
}

void Shutdown(volatile ApplicationGlobalState *state) {
    state->scene = nullptr;
    MapSynthDispose();

    ShutdownManagedMemory(); // deallocate everything
}


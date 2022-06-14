#include <SDL.h>
#include <SDL_mutex.h>
#include <SDL_thread.h>

#include <iostream>
#include <app/app_start.h>

using namespace std;

// Two-thread rendering stuff:
SDL_mutex* gDataLock = nullptr; // Data access semaphore, for the read buffer
SDL_Window* window; //The window we'll be rendering to
SDL_Surface* screenSurface;// The surface contained by the window

volatile bool quit = false; // Quit flag
volatile bool drawDone = false; // Quit complete flag
volatile int writeBuffer = 0; // which buffer is being written (other will be read)
volatile int frameWait = 0; // frames waiting

uint64_t renderTicks = 0;
uint64_t renderedFrames = 0;
char* base = nullptr; // graphics base
int rowBytes = 0;

// User/Core shared data:
volatile ApplicationGlobalState gState = {};

// voxel rendering on a separate thread
int RenderWorker(void*)
{
    while (base == nullptr) {
        SDL_Delay(5);
    }
    SDL_Delay(150); // delay wake up
    while (!quit) {
        while (!quit && frameWait < 1) {
            SDL_Delay(1); // pause the thread until a new frame is ready
        }

        uint32_t st = SDL_GetTicks();

        RenderFrame(&gState, screenSurface);
        SDL_UpdateWindowSurface(window); // update the surface -- need to do this every frame.

        uint32_t frameSplit = SDL_GetTicks();
        renderTicks += frameSplit - st;

        SDL_LockMutex(gDataLock);
        frameWait = 0;
        SDL_UnlockMutex(gDataLock);
        renderedFrames++;
    }
    drawDone = true;
    return 0;
}

void HandleEvents() {
    SDL_PumpEvents();
    SDL_Event next_event;
    while (SDL_PollEvent(&next_event)) {
        HandleEvent(&next_event, &gState);
    }
}

// We undefine the `main` macro in SDL_main.h, because it confuses the linker.
#undef main

int main()
{

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "SDL initialization failed. SDL Error: " << SDL_GetError();
        return 1;
    } else {
        cout << "SDL initialization succeeded!\r\n";
    }

    // Create window
    window = SDL_CreateWindow("SDL project base", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        cout << "Window could not be created! SDL_Error: " << SDL_GetError();
        return 1;
    }

    // Let the app startup
    StartUp(&gState);

    gDataLock = SDL_CreateMutex(); // Initialize lock, one reader at a time
    screenSurface = SDL_GetWindowSurface(window); // Get window surface

    base = (char*)screenSurface->pixels;
    int w = screenSurface->w;
    rowBytes = screenSurface->pitch;
    int pixBytes = rowBytes / w;

    cout << "\r\nScreen format: " << SDL_GetPixelFormatName(screenSurface->format->format);
    cout << "\r\nBytesPerPixel: " << (pixBytes) << ", exact? " << (((screenSurface->pitch % pixBytes) == 0) ? "yes" : "no");

    // run the rendering thread
#ifdef MULTI_THREAD
    SDL_Thread* threadA = SDL_CreateThread(RenderWorker, "RenderThread", nullptr);
#endif

    // Used to calculate the frames per second
    uint32_t startTicks = SDL_GetTicks();
    uint32_t idleTime = 0;
    uint32_t frame = 0;
    uint32_t fTime = FRAME_TIME_TARGET;
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Draw loop                                                                                      //
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    gState.running = true;
    while (gState.running) {
        uint32_t fst = SDL_GetTicks();
        // Wait for frame render to finish, then swap buffers and do next

#ifdef MULTI_THREAD
        if (frameWait < 1) {
            // Swap buffers, we will render one to pixels while we're issuing draw commands to the other
            // If render can't keep up with frameWait, we skip this frame and draw to the same buffer.
            SDL_LockMutex(gDataLock);                               // lock
            writeBuffer = 1 - writeBuffer;                          // switch buffer

            frameWait = 1;                                          // signal to the other thread that the buffer has changed
            SDL_UnlockMutex(gDataLock);                             // unlock
        }
#endif

        // Pick the write buffer and set switch points:
        UpdateModel(&gState, frame++, fTime);

#ifndef MULTI_THREAD
        // if not threaded, render immediately
        RenderBuffer(writingScanBuf, base);
        SDL_UpdateWindowSurface(window);
#endif


        // Event loop and frame delay
#ifdef FRAME_LIMIT
        fTime = SDL_GetTicks() - fst;
        if (fTime < FRAME_TIME_TARGET) { // We have time after the frame
            HandleEvents(); // spend some budget on events
            auto left = SDL_GetTicks() - fst;
            if (left < FRAME_TIME_TARGET) SDL_Delay(FRAME_TIME_TARGET - left); // still got time? Then wait
            idleTime += FRAME_TIME_TARGET - fTime; // indication of how much slack we have
        }
        fTime = SDL_GetTicks() - fst;
#else
        fTime = SDL_GetTicks() - fst;
        HandleEvents();
#endif
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    quit = true;
    frameWait = 100;

    long endTicks = SDL_GetTicks();
    float avgFPS = static_cast<float>(frame) / (static_cast<float>(endTicks - startTicks) / 1000.0f);
    float logicIdleRatio = static_cast<float>(idleTime) / (15.0f*static_cast<float>(frame));
    float drawIdleAve = static_cast<float>(renderTicks) / (static_cast<float>(renderedFrames));
    cout << "\r\nFPS ave = " << avgFPS << "\r\nLogic idle % = " << (100 * logicIdleRatio);
    cout << "\r\nFrame drawn = " << renderedFrames << "\r\nDraw time ave = " << (drawIdleAve) << "ms (greater than 15 is under-speed)";

    // Let the app deallocate etc
    Shutdown(&gState);

#ifdef MULTI_THREAD
    while (!drawDone) { SDL_Delay(100); }// wait for the renderer to finish
#endif

#ifdef WAIT_AT_END
    // Wait for user to close the window
    SDL_Event close_event;
    while (SDL_WaitEvent(&close_event)) {
        if (close_event.type == SDL_QUIT) {
            break;
        }
    }
#endif

#ifdef MULTI_THREAD
    SDL_WaitThread(threadA, nullptr);
#endif
    SDL_DestroyMutex(gDataLock);
    gDataLock = nullptr;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#pragma comment(linker, "/subsystem:Console")

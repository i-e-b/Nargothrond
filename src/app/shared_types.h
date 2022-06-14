//
// Created by iain on 08/06/22.
//

#ifndef SDLBASE_SHARED_TYPES_H
#define SDLBASE_SHARED_TYPES_H

#include <cstdint>
#include "types/general.h"

typedef struct Vec3 {
    int32_t x,y,z;
} Vec3;


typedef struct NgScene {
    int VIEW_DISTANCE; // how far to draw. More is slower but you can see further (range: 400 to 2000)

    bool doInterlacing; // render alternate columns per frame for motion blur
    bool doJitter; // scatter color sample points
    bool doFog; // fade to background near draw limit
    bool doSmoothing; // fade between textels on contiguous slopes
    bool sharperPeaks; // change scaling to make hills into mountains


    int interlace = 1;
    int aspect = 512; // camera aspect. Smaller = fisheye
    double heightScale = 1.1; // scale of slopes. Higher = taller mountains.

    // camera
    int camX = 256;
    int camY = 256;
    int camHeight = 400;
    double camAngle = 3.14; // yaw angle
    double camPitch = 0.0; // pitch. Positive = looking down

} NgScene;

typedef NgScene* NgScenePtr;

// This is the global state shared between the core and your app.
// The running flag is required, you can add extra fields as you need.
typedef struct ApplicationGlobalState {
    bool running;

    int mapSize; // height and width of color and height maps
    BYTE* colorMap; // color map
    BYTE* shadowMap; // light and shadow values. TODO: move out of colorMap.
    BYTE* heightMap; // height map
    bool showColor;
    bool showHeight;
    NgScenePtr scene;
} ApplicationGlobalState;


#endif //SDLBASE_SHARED_TYPES_H

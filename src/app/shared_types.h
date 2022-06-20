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

#define SET_IN_INIT  0

typedef struct NgScene {
    int VIEW_DISTANCE = SET_IN_INIT; // how far to draw. More is slower, but you can see further (range: 400 to 2000)

    bool doInterlacing = SET_IN_INIT; // render alternate columns per frame for motion blur
    bool doJitter = SET_IN_INIT; // scatter color sample points
    bool doFog = SET_IN_INIT; // fade to background near draw limit
    bool doSmoothing = SET_IN_INIT; // fade between texels on contiguous slopes
    bool sharperPeaks = SET_IN_INIT; // change scaling to make hills into mountains


    double waterLevel = SET_IN_INIT; // global water level. Treated as underwater if below this 0..255
    double shadowAngle = SET_IN_INIT; // 0..180 angle of the sun in sky. Affects shadow shape and darkness

    int interlace = 1;
    int aspect = 512; // camera aspect. Smaller = fisheye
    double heightScale = 1.1; // scale of slopes. Higher = taller mountains.

    // camera
    double camX = 256;
    double camY = 256;
    double camHeight = 400;
    double camAngle = 3.14; // yaw angle
    double camPitch = 0.0; // pitch. Positive = looking down

    double sceneTime = 0.0;
    int sky_R = 0, sky_G = 0, sky_B = 0; // sky color, set based on time of day.

    // viewpoint movement
    int moveForward = 0; // -1..1, -1 being backwards
    int moveStrafeLeft = 0;
    int moveTurnLeft = 0;
    int moveUp = 0;
    int moveLookUp = 0;
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
    bool showShadow;
    NgScenePtr scene;
} ApplicationGlobalState;


#endif //SDLBASE_SHARED_TYPES_H

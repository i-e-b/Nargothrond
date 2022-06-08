//
// Created by Iain on 05/06/2022.
//
#include "scene.h"


typedef struct NgScene {
    int MAX_HEIGHT = 400;
    int MIN_HEIGHT = 10;
    int MAX_PITCH = -200;
    int MIN_PITCH = 150;

    int VIEW_DISTANCE = 600; // how far to draw. More is slower but you can see further (range: 400 to 2000)

    bool doInterlacing = true; // render alternate columns per frame for motion blur
    bool doJitter = true; // scatter color sample points
    bool doFog = true; // fade to background near draw limit
    bool doSmoothing = true; // fade between texels on contiguous slopes
    bool interpolateHeightMap = true; // sample multiple height map points for a smoother render
    bool sharperPeaks = true; // change scaling to make hills into mountains
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


int wrapToBounds(int v, int limit){
    // mirroring at edges, so we don't need to have a nice wrapping texture
    if (v < 0) v = -v;
    v = v % (limit * 2);
    if (v >= limit) v = limit - (v - limit);
    return v;
}



void InitScene(NgScenePtr scene) {

}

void RenderScene(volatile ApplicationGlobalState *state, SDL_Surface *screen) {
    auto base = (BYTE*)screen->pixels;
    auto cmap = state->colorMap;

    int rowBytes = screen->pitch;
    auto scene = state->scene;

    // draw terrain
    double sinAngle = sin(scene.camAngle);
    double cosAngle = cos(scene.camAngle);

    double y3d = -(scene.aspect) * 1.5;
    int di = scene.doInterlacing ? 2 : 1;
    int width = screen->w;

    int camX = scene.camX;
    int camY = scene.camX;
    double camAngle = scene.camAngle;

    for (int i = scene.interlace; i < width; i+= di){ //increment by 2 for interlacing
        double x3d = (i - width / 2.0) * 2.25;

        double rotX =  cosAngle * x3d + sinAngle * y3d;
        double rotY = -sinAngle * x3d + cosAngle * y3d;

        rayCast(i, camX, camY,
                camX + rotX, camY + rotY,
                y3d / sqrt(x3d * x3d + y3d * y3d),
                camAngle);
    }

    // alternate scanlines each frame
    scene.interlace = 1 - scene.interlace;
}

void MoveCamera(NgScenePtr scene, Vec3 &eye, Vec3 &lookAt) {

}



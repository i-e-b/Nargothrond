//
// Created by Iain on 04/06/2022.
//

#ifndef SDLBASE_SCENE_H
#define SDLBASE_SCENE_H

#include <cstdint>
#include "app_start.h"
#include "scene.h"

typedef struct Vec3 {
    int32_t x,y,z;
} Vec3;

typedef struct NgScene NgScene;
typedef NgScene* NgScenePtr;

void InitScene(NgScenePtr scene);

void MoveCamera(NgScenePtr scene, Vec3 &eye, Vec3 &lookAt);
void RenderScene(volatile ApplicationGlobalState *state, SDL_Surface *screen);

#endif //SDLBASE_SCENE_H

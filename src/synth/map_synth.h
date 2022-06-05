//
// Created by Iain on 04/06/2022.
//

#ifndef SDLBASE_MAP_SYNTH_H
#define SDLBASE_MAP_SYNTH_H

#include "types/general.h"

void MapSynthInit();
void MapSynthDispose();

// Create a random height field from a seed
void GenerateHeight(int size, int seed, BYTE* map);

// Create a color map to match a height map
void GenerateColor(int size, const BYTE* height, BYTE* color);

#endif //SDLBASE_MAP_SYNTH_H

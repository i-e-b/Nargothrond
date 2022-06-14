//
// Created by Iain on 04/06/2022.
//

#include <cmath>
#include "map_synth.h"
#include "types/MemoryManager.h"
#include "types/Vector.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

RegisterVectorStatics(vec)
RegisterVectorFor(double, vec)

int p[512];

typedef struct rgbSample {
    int r;
    int g;
    int b;
} rgbSample;

double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double lerp(double t, double a, double b) {
    return a + t * (b - a);
}

double grad(int hash, double x, double y, double z) {
    int h = hash & 15; // CONVERT LO 4 BITS OF HASH CODE
    double u = h < 8 ? x : y, // INTO 12 GRADIENT DIRECTIONS.
    v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double scale(double n) {
    return (1.0 + n) / 2.0;
}

// This is a port of Ken Perlin's Java code. The
// original Java code is at http://cs.nyu.edu/%7Eperlin/noise/.
// Note that in this version, a number from 0 to 1 is returned.
double noise(double x, double y, double z) {
    int X = (int)floor(x) & 255, // FIND UNIT CUBE THAT
    Y = (int)floor(y) & 255, // CONTAINS POINT.
    Z = (int)floor(z) & 255;
    x -= (int)floor(x); // FIND RELATIVE X,Y,Z
    y -= (int)floor(y); // OF POINT IN CUBE.
    z -= (int)floor(z);
    double u = fade(x), // COMPUTE FADE CURVES
    v = fade(y), // FOR EACH OF X,Y,Z.
    w = fade(z);
    int A = p[X] + Y,
            AA = p[A] + Z,
            AB = p[A + 1] + Z, // HASH COORDINATES OF
    B = p[X + 1] + Y,
            BA = p[B] + Z,
            BB = p[B + 1] + Z; // THE 8 CUBE CORNERS,

    return scale(lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), // AND ADD
                                      grad(p[BA], x - 1, y, z)), // BLENDED
                              lerp(u, grad(p[AB], x, y - 1, z), // RESULTS
                                   grad(p[BB], x - 1, y - 1, z))), // FROM  8
                      lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), // CORNERS
                                   grad(p[BA + 1], x - 1, y, z - 1)), // OF CUBE
                           lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                grad(p[BB + 1], x - 1, y - 1, z - 1)))));
}


int heightFunction(int seed, int x, int y) {
// three octaves of noise:

    double z = seed; // slice through the noise space. This is your 'seed'
    double lowfreq = 0.01;
    double midfreq = 0.05;
    double highfreq = 0.5;

    double a = noise(x * lowfreq, y * lowfreq, z);
    double b = noise(x * midfreq, y * midfreq, z);
    double c = noise(x * highfreq, y * highfreq, z);

    double v = 1.2 * // overdrive a little
               (a * 0.6) + // main mountain ranges
               (a * b * 0.4) + // more ridges at higher elevations
               (a * a * c * 0.1) - // crinkles only at the top
               0.1; // adjust water line

    //if (v < 0.2) v = 0.199; // water line TODO: handle water-level limit in render phase

    int iv = (int)(v*255);
    if (iv > 255) return 255;
    if (iv < 0) return 0;
    return iv;
}

inline void set(BYTE* map, int idx, int r, int g, int b){
    map[idx++]=(BYTE)r;
    map[idx++]=(BYTE)g;
    map[idx++]=(BYTE)b;
}
inline rgbSample get(const BYTE* map, int idx){
    rgbSample s = {};
    s.r = (BYTE)map[idx++];
    s.g = (BYTE)map[idx++];
    s.b = (BYTE)map[idx++];
    return s;
}

void heightToColor(int size, const BYTE* heightMap, BYTE* colorMap) {
    int rowWidth = size * 3; // number of bytes in an image row

    // main color
    for (int y = 0; y < size; y++) {
        int yoff = y * rowWidth;
        int yoffH = y * size;

        for (int x = 0, xoff = 0; x < size; x++, xoff += 3) {
            int hidx = yoffH + x;
            int cidx = yoff + xoff;

            double s = heightMap[hidx] / 255.0; // sample height, put in range 0..1

            // really simple green->white
            double r = s * s * s * 800 - 50;
            double g = max(r, 50 + ((s * s) * 127));
            double b = s * s * s * 800 - 50;

            if (s < 0.2) { // water
                r = 20;
                g = 80;
                b = 120;
            } else if (s < 0.3) { // mud
                double ms = 1.0 - (s * 2);
                double gs = s * 2;
                r = 140 * ms + r * gs;
                g = 120 * ms + g * gs;
                b = 110 * ms + b * gs;
            }

            // pin to range
            r = max(0, min(255, r));
            g = max(0, min(255, g));
            b = max(0, min(255, b));

            set(colorMap, cidx, (int) r, (int) g, (int) b);
        }
    }
}

// falloff = how steep the shadows are. lower = sun is closer to horizon.
void heightToShadow(int size, int direction, double falloff, const BYTE* heightMap, BYTE* shadowMap) {
    int start = 0;
    int end = size-1;
    int initial = direction > 0 ? start : end;

    for (int y = 0; y < size; y++) {
        int yoff = y * size;
        double shadow = 0; // what height is in shadow

        for (int x = initial; x >= start && x <= end; x += direction) {
            int idx = yoff + x;

            double s = heightMap[idx] / 255.0; // sample height, put in range 0..1
            shadow = max(s, shadow) - falloff;

            // darker if in shadow
            if (shadow > s) {
                shadowMap[idx] = (BYTE)1;
            } else {
                shadowMap[idx] = (BYTE)0;
            }
        }
    }
}

void blur(int size, BYTE* colorMap) {
    int rowWidth = size * 3; // number of bytes in an image row

    // simple kernel blur
    for (int y = 1; y < size - 1; y++) {
        int yoff_1 = (y - 1) * rowWidth;
        int yoff_2 = (y) * rowWidth;
        int yoff_3 = (y + 1) * rowWidth;

        for (int x = 1, xoff = 0; x < size - 1; x++, xoff += 3) {
            auto c_tl = get(colorMap, yoff_1 + xoff - 3);
            auto c_ml = get(colorMap, yoff_2 + xoff - 3);
            auto c_bl = get(colorMap, yoff_3 + xoff - 3);


            auto c_tc = get(colorMap, yoff_1 + xoff);
            auto c_mc = get(colorMap, yoff_2 + xoff);
            auto c_bc = get(colorMap, yoff_3 + xoff);

            auto c_tr = get(colorMap, yoff_1 + xoff + 3);
            auto c_mr = get(colorMap, yoff_2 + xoff + 3);
            auto c_br = get(colorMap, yoff_3 + xoff + 3);

            double r = c_tl.r * 0.5 + c_tc.r * 0.75 + c_tr.r * 0.5 +
                    c_ml.r * 0.75 + c_mc.r * 1.0 + c_mr.r * 0.75 +
                    c_bl.r * 0.5 + c_bc.r * 0.75 + c_br.r * 0.5;

            double g = c_tl.g * 0.5 + c_tc.g * 0.75 + c_tr.g * 0.5 +
                    c_ml.g * 0.75 + c_mc.g * 1.0 + c_mr.g * 0.75 +
                    c_bl.g * 0.5 + c_bc.g * 0.75 + c_br.g * 0.5;

            double b = c_tl.b * 0.5 + c_tc.b * 0.75 + c_tr.b * 0.5 +
                    c_ml.b * 0.75 + c_mc.b * 1.0 + c_mr.b * 0.75 +
                    c_bl.b * 0.5 + c_bc.b * 0.75 + c_br.b * 0.5;


            set(colorMap, yoff_2 + xoff, (int)r / 6, (int)g / 6, (int)b / 6);
        }
    }
}

void GenerateHeight(int size, int seed, BYTE *map) {
    for (int y = 0; y < size; ++y) {
        int yoff = y * size;
        for (int x = 0; x < size; ++x) {
            map[x + yoff] = (BYTE)heightFunction(seed, x,y);
        }
    }
}

void GenerateColor(int size, const BYTE *height, BYTE *color) {
    if (color == nullptr) return;
    if (height == nullptr) return;
    if (size < 1) return;

    heightToColor(size, height, color);
    blur(size, color);
}

// sun angle 0..180
void GenerateShadow(int size, const BYTE *height, BYTE *shadow, double sunAngle) {
    if (shadow == nullptr) return;
    if (height == nullptr) return;
    if (size < 1) return;

    // TODO: pass in a sun-point, or time of day and calculate the falloff and direction
    double sunrad = 0.017453 * sunAngle;
    double falloff = sin(sunrad) / 100;
    int direction = sunAngle < 90 ? 1 : -1;

    heightToShadow(size, direction, falloff, height, shadow);
}

void MapSynthInit() {
    // Setup for perlin noise function
    int permutation[/*256*/] = {
            151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37,
            240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
            57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48,
            27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46,
            245, 40, 244, 102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
            200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250,
            124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182,
            189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43,
            172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246,
            97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14,
            239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4,
            150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
    };
    for (int i = 0; i < 256; i++) {
        p[256 + i] = p[i] = permutation[i];
    }
}

void MapSynthDispose() {

}


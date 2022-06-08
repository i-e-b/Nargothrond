//
// Created by Iain on 05/06/2022.
//
#include "scene.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

// set a pixel in the SDL surface
inline void setPixel(SDL_Surface *screen, int x, int y, int r, int g, int b){
    BYTE* base = (BYTE*)screen->pixels;
    int idx = (y * screen->pitch) + (x*4);
    base[idx++] = (BYTE)b;
    base[idx++] = (BYTE)g;
    base[idx  ] = (BYTE)r;
}


inline int wrapToBounds(double dv, int limit){
    int v = (int)dv;

    // mirroring at edges, so we don't need to have a nice wrapping texture
    if (v < 0) v = -v;
    v = v % (limit * 2);
    if (v == limit) return limit - 1;
    if (v > limit) v = limit - (v - limit);
    return v;
}

// line = vertical span index (x in render space)
// x1,y1 = camera location (in map space)
// x2,y2 = camera look
// d = height of camera
// xDir = camera direction
void rayCast(volatile ApplicationGlobalState *state, NgScenePtr scene, SDL_Surface *screen,
             int line, double x1, double y1, double x2, double y2, double d, double xDir) {

    int height = screen->h;
    int width = screen->w;
    BYTE* heights = state->heightMap; // todo: this should be in scene
    BYTE* colors = state->colorMap; // todo: this should be in scene

    // x1, y1, x2, y2 are the start and end points on map for ray
    double dx = x2 - x1;
    double dy = y2 - y1;

    double dp = abs(d) / 100.0;
    double persp = 0;

    // calculate stepsize in x and y direction
    double dr = sqrtf64(dx * dx + dy * dy); // distance between start and end point
    dx = dx / dr;
    dy = dy / dr;

    int ymin = height; // last place we ended drawing a vertical line
    // also the highest thing we've drawn to prevent over-paint (0 is max height)
    double z3 = ymin+1;     // projected Z height of point under consideration
    double h=0;
    int pr=0,pg=0,pb=0;
    int gap = 1; // marks when we should break slope colour interpolation
    int hbound = height - 1;
    int viewDistance = scene->VIEW_DISTANCE;

    // sky texture x coord
    //int sx = floor( (-xDir*(1 / 3.141592) + line) % skyWidth)*skyWidth;

    // fog parameters
    double dlimit = viewDistance * 0.7;
    double dfog = 1 / (viewDistance * 0.3);
    double fo=0,fs = 1;
    int lastI = 0;

    int x,y,idx,cidx;

    // local references
    double camHeight = scene->camHeight;
    double camV = scene->camPitch;

    int mapWidth = state->mapSize;
    int mapHeight = state->mapSize;

    // MAIN LOOP
    // we draw from near to far
    // first we do a tight loop through the height map looking for a position
    // that would be visible behind anything else we've drawn
    // then we loop through the pixels to draw and color them
    for (int i = 0; i < viewDistance; i++) {

        // step to next position, wrapped for out-of-bounds
        x1 = x1+dx;
        y1 = y1+dy;

        x = wrapToBounds(x1, mapWidth);
        y = wrapToBounds(y1, mapHeight);
        idx = (y * mapWidth) + x;
        cidx = idx * 3;

        // get height
        double terrainHeight;
        if (scene->interpolateHeightMap) {

            double fx = x1 - trunc(x1);
            double fy = y1 - trunc(y1);

            if (dx < 0) fx = 1 - fx;
            if (dy < 0) fy = 1 - fy;

            double dfx = 1 - fx;
            double dfy = 1 - fy;

            int ix = wrapToBounds(x1+dx, mapWidth);
            int iy = wrapToBounds(y1+dy, mapHeight);

            int s1 = heights[( y * width) +  x];
            int s2 = heights[( y * width) + ix];
            int s3 = heights[(iy * width) +  x];
            int s4 = heights[(iy * width) + ix];

            terrainHeight = (s1 * dfx * dfy) + (s2 * fx * dfy) +
                            (s3 * dfx *  fy) + (s4 * fx * fy);

            terrainHeight *= scene->heightScale;
        } else {
            terrainHeight = heights[idx] * scene->heightScale;
        }
        if (scene->sharperPeaks) {
            terrainHeight *= terrainHeight / 127.0;
        }
        h = camHeight - terrainHeight;  // lack of interpolation here causes banding artifacts close up

        // perspective calculation where d is the correction parameter
        persp = persp + dp;
        z3 = (h / persp) - camV;

        // is this position is visible?
        if (z3 < ymin) { // (if you wanted to mark visible/invisible positions you could do it here)
            z3 = floor(z3); // get on to pixel bounds

            // bounds of vertical strip, limited to buffer bounds
            int ir = min(hbound, max(0,z3));
            int iz = min(hbound, ymin);

            // read color from image
            int r = colors[cidx], g = colors[cidx+1], b =colors[cidx+2];

            // fog effect
            if ((scene->doFog) && (i > dlimit) ){ // near the fog limit
                fo = dfog*(i-dlimit); // calculate the fog blend by distance
                fs = 1 - fo;
                //idx = (sx) + (ir % skyHeight)
                r = (int)((r * fs) + (fo * 127));//sky_R[idx]) //127,127,255
                g = (int)((g * fs) + (fo * 127));//sky_G[idx])
                b = (int)((b * fs) + (fo * 255));//sky_B[idx])
            }

            if (ir+1 < iz) { // large textels, interpolate for smoothness
                // get the next color, interpolate between that and the previous
                // TODO: instead of smoothing, we should use a 'fine' texture

                // Jitter samples to make smoothing look better (otherwise orthagonal directions look stripey)
                if ((scene->doJitter) && (i < dlimit)) { // don't jitter if drawing fog
                    // pull nearby sample to blend
                    int jx = wrapToBounds(x1+(dy/2), mapWidth);
                    int jy = wrapToBounds(y1+(dx/2), mapHeight);
                    int jidx = 3 * ((jy*mapWidth)+jx);
                    r = (r + colors[jidx]) / 2;
                    g = (g + colors[jidx+1]) / 2;
                    b = (b + colors[jidx+2]) / 2;
                }

                if (scene->doSmoothing) {
                    if (gap > 0) { pr=r;pg=g;pb=b; } // no prev colors
                    int pc = (iz - ir) + 1;
                    int sr = (r - pr)/pc;
                    int sg = (g - pg)/pc;
                    int sb = (b - pb)/pc;
                    for (int k = iz; k >= ir; k--) {
                        setPixel(screen, line, k, pr,pg,pb);
                        pr = pr + sr;
                        pg = pg + sg;
                        pb = pb + sb;
                    }
                } else {// no smoothing, just fill in with sample color
                    for (int k = iz; k >= ir; k--) {
                        setPixel(screen, line, 0|k, r,g,b);
                    }
                }

            } else { // small textels. Could supersample for quality?
                pr=r;pg=g;pb=b; // copy previous colors
                setPixel(screen, line, ir, r,g,b);
            }
            gap = 0;
        } else { // obscured
            gap = 1;
        }
        ymin = min(ymin, z3);
        if (ymin < 1) { break; } // early exit: the screen is full
    } // end of draw distance

    // now if we didn't get to the top of the screen, fill in with sky
    for (int i = ymin; i >= 0; i--) {
        //idx = (sx) + (i % skyHeight)
        //imageData:setPixel(line, i % height, sky_R[idx],sky_G[idx],sky_B[idx])
        // todo: synth a sky texture too
        setPixel(screen, line, i % height, 127,127,255);
    }
}

void InitScene(volatile ApplicationGlobalState *state){
    auto scene = (NgScenePtr)malloc(sizeof(NgScene));
    scene-> MAX_HEIGHT = 400;
    scene-> MIN_HEIGHT = 10;
    scene-> MAX_PITCH = -200;
    scene-> MIN_PITCH = 150;

    scene-> VIEW_DISTANCE = 600; // how far to draw. More is slower but you can see further (range: 400 to 2000)

    scene-> doInterlacing = true; // render alternate columns per frame for motion blur
    scene-> doJitter = true; // scatter color sample points
    scene-> doFog = true; // fade to background near draw limit
    scene-> doSmoothing = true; // fade between texels on contiguous slopes
    scene-> interpolateHeightMap =false; // sample multiple height map points for a smoother render
    scene-> sharperPeaks = false; // change scaling to make hills into mountains
    scene-> interlace = 1;
    scene-> aspect = 512; // camera aspect. Smaller = fisheye
    scene-> heightScale = 1.1; // scale of slopes. Higher = taller mountains.

    // camera
    scene-> camX = 256;
    scene-> camY = 256;
    scene-> camHeight = 400;
    scene-> camAngle = 3.14; // yaw angle
    scene-> camPitch = 0.0; // pitch. Positive = looking down

    state->scene = scene;
}

void RenderScene(volatile ApplicationGlobalState *state, SDL_Surface *screen) {
    if (state == nullptr) return;
    auto scene = state->scene;
    if (scene == nullptr) return;

    // draw terrain
    double sinAngle = sin(scene->camAngle);
    double cosAngle = cos(scene->camAngle);

    double y3d = -(scene->aspect) * 1.5;
    int di = scene->doInterlacing ? 2 : 1;
    int width = screen->w;

    double camX = scene->camX;
    double camY = scene->camY;
    double camAngle = scene->camAngle;

    for (int i = scene->interlace; i < width; i+= di){ //increment by 2 for interlacing
        double hw = width / 2.0;
        double x3d = (i - hw) * 2.25;

        double rotX =  cosAngle * x3d + sinAngle * y3d;
        double rotY = -sinAngle * x3d + cosAngle * y3d;


        rayCast(state, scene, screen,
                i, camX, camY,
                camX + rotX, camY + rotY,
                y3d / sqrtf64(x3d * x3d + y3d * y3d),
                camAngle);
    }

    // alternate scanlines each frame
    scene->interlace = 1 - scene->interlace;
}


void MoveCamera(NgScenePtr scene, Vec3 &eye, Vec3 &lookAt) {

}



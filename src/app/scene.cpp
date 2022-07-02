//
// Created by Iain on 05/06/2022.
//
#include "scene.h"
#include "types/MemoryManager.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

// line = vertical span index (x in render space)
// x1,y1 = camera location (in map space)
// x2,y2 = camera look
// d = height of camera
// xDir = camera direction
void rayCast(volatile ApplicationGlobalState *state, NgScenePtr scene, SDL_Surface *screen, int shadowDarkness,
             int line, double x1, double y1, double x2, double y2, double d /*,double xDir*/) { //xDir used for sky texture

    if (state == nullptr || scene == nullptr) return;
    int height = screen->h;

    // Maps we read from:
    BYTE* heights = state->heightMap; // todo: this should be in scene, not state
    BYTE* colors = state->colorMap; // todo: this should be in scene, not state
    BYTE* shadows = state->shadowMap;

    // Maps we write to
    uint32_t* depths = state->depthMap;
    uint32_t* coords = state->coordMap;

    // output pixel map
    BYTE* base = (BYTE*)screen->pixels;
    int xpos = line * 4; // pixel column
    int rowBytes = screen->pitch;
    int scrWidth = screen->w;

    // x1, y1, x2, y2 are the start and end points on map for ray
    double dx = x2 - x1;
    double dy = y2 - y1;

    double dp = fabs(d) / 100.0;
    double persp = 0;

    // calculate step size in x and y direction
    double dr = sqrt(dx * dx + dy * dy); // distance between start and end point
    dx = dx / dr;
    dy = dy / dr;

    int ymin = height; // last place we ended drawing a vertical line
    // also the highest thing we've drawn to prevent over-paint (0 is max height)
    double z3 = ymin+1;     // projected Z height of point under consideration
    double h=0;
    int hbound = height - 1;
    int viewDistance = scene->VIEW_DISTANCE;
    int yline = hbound * scrWidth;
    int ypos = hbound * rowBytes; // y-offset in output pixel map. We tick this down, so have to be careful not to overdraw anywhere.

    // sky texture x coord
    //int sx = floor( (-xDir*(1 / 3.141592) + line) % skyWidth)*skyWidth;
    int skyR = scene->sky_R;
    int skyG = scene->sky_G;
    int skyB = scene->sky_B;

    // fog parameters
    double dlimit = viewDistance * 0.7;
    double dfog = 1 / (viewDistance * 0.3);
    double fo=0,fs = 1;

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

        // TODO: pick a map from a tileset based on global X,Y
        //x = ((int)fabs(x1)) % mapWidth; // repeat, mirrored across 0
        //y = ((int)fabs(y1)) % mapHeight;
        x = (int)x1;
        y = (int)y1;
        if (x < 0 || x >= mapWidth) break; // show only one tile
        if (y < 0 || y >= mapHeight) break;
        idx = (y * mapWidth) + x;
        cidx = idx * 3;

        // get height
        double terrainHeight;
        terrainHeight = heights[idx] * scene->heightScale;

        bool water = heights[idx] <= scene->waterLevel;
        if (water) terrainHeight = scene->waterLevel;

        int shadow = (int)shadows[idx]; // 1 if dark, 0 if bright. Used to bit shift

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
            int ir = (int)(min(hbound, max(0,z3)));
            int iz = (int)(min(hbound, ymin));
            //ypos = iz * rowBytes; // if this is needed, we're overdrawing somewhere?

            // read color from image
            int r = colors[cidx], g = colors[cidx+1], b =colors[cidx+2];

            if (shadow){
                // TODO: idea: have a dark & light floor texture, and blend between them.
                r = max(0, r - shadowDarkness);
                g = max(0, g - shadowDarkness);
                b = max(0, b - shadowDarkness);
            }

            // fog effect
            if ((scene->doFog) && (i > dlimit) ){ // near the fog limit
                fo = dfog*(i-dlimit); // calculate the fog blend by distance
                fs = 1 - fo;
                //idx = (sx) + (ir % skyHeight)
                r = (int)((r * fs) + (fo * skyR));//sky_R[idx])
                g = (int)((g * fs) + (fo * skyG));//sky_G[idx])
                b = (int)((b * fs) + (fo * skyB));//sky_B[idx])
            }

            if (ir+1 < iz) { // large texels, repeat texture sample
                // TODO: instead of repeating, we should use a 'fine' texture
                //       this could be based on another map, which would let us do walls etc.
                for (int k = iz; k > ir; k--) {
                    base[ypos+xpos  ] = (BYTE)b; base[ypos+xpos+1] = (BYTE)g; base[ypos+xpos+2] = (BYTE)r;

                    // TODO: back-maps
                    depths[yline+line] = i;
                    //coords[???] = ???
                    yline -= scrWidth;
                    ypos -= rowBytes;
                }
            } else { // small texels. Could super-sample for quality?
                base[ypos+xpos  ] = (BYTE)b; base[ypos+xpos+1] = (BYTE)g; base[ypos+xpos+2] = (BYTE)r;

                // TODO: back-maps
                depths[yline+line] = i;
                //coords[???] = ???
                yline -= scrWidth;
                ypos -= rowBytes;
            }
        } else { // obscured
            //gap = 1;
        }
        ymin = min(ymin, (int)z3);
        if (ymin < 1) { break; } // early exit: the screen is full
    } // end of draw distance

    // now if we didn't get to the top of the screen, fill in with sky
    while (ypos > 0) {
        base[ypos+xpos  ] = (BYTE)skyB; base[ypos+xpos+1] = (BYTE)skyG; base[ypos+xpos+2] = (BYTE)skyR;
        depths[yline+line] = (BYTE)-1;

        yline -= scrWidth;
        ypos -= rowBytes;
    }
}

void InitScene(volatile ApplicationGlobalState *state){
    auto scene = (NgScenePtr)ArenaAllocate(MMCurrent(),sizeof(NgScene));
    scene-> VIEW_DISTANCE = 600; // how far to draw. More is slower but you can see further (range: 400 to 2000)

    scene-> doInterlacing = true; // render alternate columns per frame for motion blur
    scene-> doFog = true; // fade to background near draw limit
    scene-> sharperPeaks = false; // change scaling to make hills into mountains

    scene->waterLevel = 51; //51;
    scene->shadowAngle = 45;

    scene-> interlace = 0;
    scene-> aspect = SCREEN_WIDTH; // camera aspect. Smaller = fisheye. Ideally equal to screen width
    scene-> heightScale = 1.1; // scale of slopes. Higher = taller mountains.

    // camera
    scene-> camX = 256;
    scene-> camY = 256;
    scene-> camHeight = 400;
    scene-> camAngle = 3.14; // yaw angle
    scene-> camPitch = 0.0; // pitch. Positive = looking down

    state->scene = scene;
}

void SetSkyColor(volatile const ApplicationGlobalState *state, NgScenePtr scene) {
    scene->sky_R = 80; // general gloom
    scene->sky_G = 20;
    scene->sky_B = 0;
    double sunrad = 0.017453 * state->scene->shadowAngle;
    if (state->scene->shadowAngle > 0 && state->scene->shadowAngle < 180) {
        double csr = cos(sunrad + 3.1415);
        int b = 300 - (int) fabs( csr * 300);
        int g = 275 - (int)fabs(csr * 255);
        int r = 155 - (int)fabs(csr * 75);

        scene->sky_R = min(127, r);
        scene->sky_G = min(127, g);
        scene->sky_B = min(255, b);
    }
}

void RenderScene(volatile ApplicationGlobalState *state, SDL_Surface *screen) {
    auto scene = state->scene;
    if (scene == nullptr) return;

    // draw terrain
    double sinAngle = sin(scene->camAngle);
    double cosAngle = cos(scene->camAngle);

    double y3d = -(scene->aspect) * 1.5;
    int di = (scene->doInterlacing) ? (2) : (1);
    int width = screen->w;

    double camX = scene->camX;
    double camY = scene->camY;
    //double camAngle = scene->camAngle;
    int angleOffNoon = (int)(90 - state->scene->shadowAngle);
    int shadowDarkness = min(255, max(0, (angleOffNoon*angleOffNoon) / 200));

    SetSkyColor(state, scene);

    for (int i = scene->interlace; i < width; i+= di){ //increment by 2 for interlacing
        double hw = width / 2.0;
        double x3d = (i - hw) * 2.25;

        double rotX =  cosAngle * x3d + sinAngle * y3d;
        double rotY = -sinAngle * x3d + cosAngle * y3d;

        rayCast(state, scene, screen,
                shadowDarkness,
                i, camX, camY,
                camX + rotX, camY + rotY,
                y3d / sqrt(x3d * x3d + y3d * y3d));
        /*, camAngle);*/ // for sky texture
    }

    // alternate scanlines each frame
    scene->interlace = 1 - scene->interlace;
}





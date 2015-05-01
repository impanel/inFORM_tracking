//
//  HybridTokens.cpp
//  Relief2
//
//  Created by Daniel Windham on 3/18/15.
//
//

#include "HybridTokens.h"

HybridTokens::HybridTokens(KinectTracker *tracker) {
    kinectTracker = tracker;
    pinHeightMapImage.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);

    // swords schema default
    useStaticSecondSword = true;
    blockadeSword = true;
}

void HybridTokens::drawHeightMap() {
    pinHeightMapImage.draw(0, 0);
}

void HybridTokens::drawGraphics() {
}

void HybridTokens::update(float dt) {
    pinHeightMapImage.begin();
    ofBackground(0);
    ofSetColor(255);
    drawSwordsHeightMap(RELIEF_PROJECTOR_SIZE_X);
    drawCubeRisers(RELIEF_PROJECTOR_SIZE_X);
    pinHeightMapImage.end();
}

// lift cubes slightly above neighboring pins to facilitate smooth sliding
void HybridTokens::drawCubeRisers(float lengthScale) {
    ofSetColor(40);
    for (vector<Cube>::iterator cube = kinectTracker->redCubes.begin(); cube < kinectTracker->redCubes.end(); cube++) {
        int left = (cube->center.x - pinSize) * lengthScale;
        int top = (cube->center.y - pinSize) * lengthScale;
        ofRect(left, top, pinSize * 2 * lengthScale, pinSize * 2 * lengthScale);
    }
}

void HybridTokens::drawSwordsHeightMap(float lengthScale) {
    // known width and height of our cubes
    float cubeWidth = 4 * pinSize;
    float cubeHeight = 4 * pinSize;
    
    // for now, assume a ready cube is flat and aligned to the coordinate axes
    if (kinectTracker->redCubes.size() == 1) {
        // dynamic cube
        Cube cube = kinectTracker->redCubes[0];
        
        // for now, hardcode a description of the static block
        ofPoint fixedCenter(0.17, 0.34);
        
        // draw dynamic sword up
        ofSetColor(140);
        int left, right, top, bottom;
        left = (cube.center.x - 0.07) * lengthScale;
        right = (cube.center.x + 0.07) * lengthScale;
        top = (cube.center.y - 0.07 - 3 * cubeHeight) * lengthScale;
        bottom = (cube.center.y - 0.07 - 0.3 * cubeHeight) * lengthScale;
        ofRect(left, top, right - left, bottom - top);

        if (useStaticSecondSword) {
            // draw static sword right
            ofSetColor(140);
            int fixedLeft, fixedRight, fixedTop, fixedBottom;
            fixedLeft = (fixedCenter.x + 0.07 + 0.3 * cubeHeight) * lengthScale;
            fixedRight = (fixedCenter.x + 0.07 + 3 * cubeHeight) * lengthScale;
            fixedTop = (fixedCenter.y - 0.07) * lengthScale;
            fixedBottom = (fixedCenter.y + 0.07) * lengthScale;
            ofRect(fixedLeft, fixedTop, fixedRight - fixedLeft, fixedBottom - fixedTop);
            
            // draw blockade
            if (blockadeSword) {
                ofSetColor(140);
                int closeDistance = 0.07 * lengthScale;
                if (left < fixedRight + closeDistance) {
                    int adjWidth = cubeWidth * lengthScale;
                    int adjHeight = cubeHeight * lengthScale;
                    int cubeBottom = (cube.center.y + 0.07) * lengthScale;
                    ofRect(fixedRight - adjWidth / 4, cubeBottom - adjHeight, adjWidth / 4, adjHeight);
                }
            }
        }
    }
}

void HybridTokens::keyPressed(int key) {
    if(key == 's') {
        useStaticSecondSword = !useStaticSecondSword;
    }

    if(key == 'b') {
        blockadeSword = !blockadeSword;
    }
}

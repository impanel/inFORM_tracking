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
    pinHeightMapImage.allocate(lengthScale, lengthScale, GL_RGBA);
    cubeFootprintsFbo.allocate(lengthScale, lengthScale, GL_RGBA);
    cubeFootprintPixels.allocate(lengthScale, lengthScale, GL_RGBA);
    pinHeightMapContentPixels.allocate(lengthScale, lengthScale, GL_RGBA);
    pinGraphicsPixels.allocate(lengthScale, lengthScale, GL_RGBA);

    mode = BOOLEAN_SWORDS;

    // swords schema default
    booleanSwordsSchema = SUM;

    // flexible swords extension parameter
    flexibleExtensionSize = 0.8;

    // calculate the rectangle corresponding to a cube sword in the cube's reference frame.
    // a sword is a cube footprint extended up by three extra cube lengths
    int left, right, top, bottom;
    left = -0.5 * cubeEdgeLength * lengthScale;
    right = 0.5 * cubeEdgeLength * lengthScale;
    top = (-0.5 - 3) * cubeEdgeLength * lengthScale;
    bottom = 0.5 * cubeEdgeLength * lengthScale;
    swordRectangle = Rectangle(left, top, right - left, bottom - top);
}

void HybridTokens::setMode(HybridTokensMode mode) {
    this->mode = mode;
}

void HybridTokens::drawHeightMap() {
    pinHeightMapImage.draw(0, 0);
}

void HybridTokens::drawGraphics() {
    int minX = RELIEF_PROJECTOR_SIZE_X * projectorMinXCoordinateBoundaryAtCubeHeight;
    int minY = RELIEF_PROJECTOR_SIZE_Y * projectorMinYCoordinateBoundaryAtCubeHeight;
    ofImage(reprojectedPinGraphicsPixels).draw(minX, minY);
}

void HybridTokens::update(float dt) {
    // draw height map content
    pinHeightMapImage.begin();
    ofBackground(0);
    ofSetColor(255);

    if (mode == BOOLEAN_SWORDS) {
        drawBooleanSwords();

    } else if (mode == FLEXIBLE_SWORDS) {
        drawFlexibleSwords();

    } else if (mode == PHYSICS_SWORDS) {
        drawPhysicsSwords();

    } else if (mode == DYNAMICALLY_CONSTRAINED_SWORDS) {
        drawDynamicallyConstrainedSwords();
    }

    pinHeightMapImage.end();

    // store height map content for graphics computations
    pinHeightMapImage.readToPixels(pinHeightMapContentPixels);

    // record cube footprints in a pixels object for graphics computations
    cubeFootprintsFbo.begin();
    ofBackground(0);
    ofSetColor(255);
    setCubeHeights(cubeHeight);
    cubeFootprintsFbo.end();
    cubeFootprintsFbo.readToPixels(cubeFootprintPixels);

    updateGraphics();
}

// paint pins according to their heights
void HybridTokens::updateGraphics() {
    // some bug in ofPixels causes it to break without this initialization; only necessary once
    if (!initializedInUpdate) {
        pinHeightMapImage.readToPixels(pinGraphicsPixels);
        initializedInUpdate = true;
    }

    // paint pins for 2D display
    int numChannels = pinGraphicsPixels.getNumChannels(); // this should be 4
    for (int i = 0; i < pinGraphicsPixels.size(); i += numChannels) {
        // if a cube sits here, do not paint graphics onto it
        if (cubeFootprintPixels[i]) {
            // do not paint this pixel
            pinGraphicsPixels[i + 0] = 0;
            pinGraphicsPixels[i + 1] = 0;
            pinGraphicsPixels[i + 2] = 0;

            // furthermore, ensure this pixel is reprojected to the location of the cube surface
            pinHeightMapContentPixels[i + 0] += cubeHeight;
            pinHeightMapContentPixels[i + 1] += cubeHeight;
            pinHeightMapContentPixels[i + 2] += cubeHeight;

        // otherwise, calculate this pixel's color
        } else {
            // since the height map rgba pixels are grayscale, the first channel is sufficient
            // to distinguish pin height
            if (pinHeightMapContentPixels[i] > 150) {
                pinGraphicsPixels[i + 0] = pinColorIfHigh[0];
                pinGraphicsPixels[i + 1] = pinColorIfHigh[1];
                pinGraphicsPixels[i + 2] = pinColorIfHigh[2];
            } else if (pinHeightMapContentPixels[i] > 0) {
                pinGraphicsPixels[i + 0] = pinColorIfOn[0];
                pinGraphicsPixels[i + 1] = pinColorIfOn[1];
                pinGraphicsPixels[i + 2] = pinColorIfOn[2];
            } else {
                pinGraphicsPixels[i + 0] = pinColorIfOff[0];
                pinGraphicsPixels[i + 1] = pinColorIfOff[1];
                pinGraphicsPixels[i + 2] = pinColorIfOff[2];
            }
        }
    }

    // paint pins for projection onto 2.5D surface
    reprojectProjectorPixelsForHeightMap(pinGraphicsPixels, pinHeightMapContentPixels, reprojectedPinGraphicsPixels);
}

// lift cubes slightly above neighboring pins to facilitate smooth sliding
void HybridTokens::setCubeHeight(Cube &cube, int height, float edgeLengthMultiplier) {
    ofSetColor(height);

    // draw cube footprint
    glPushMatrix();
    glTranslatef(cube.center.x * lengthScale, cube.center.y * lengthScale, 0.0f);
    glRotatef(-cube.theta, 0.0f, 0.0f, 1.0f);
    float scaledEdgeLength = cubeEdgeLength * lengthScale * edgeLengthMultiplier;
    ofRect(-scaledEdgeLength / 2, -scaledEdgeLength / 2, scaledEdgeLength, scaledEdgeLength);
    glPopMatrix();
}

// lift cubes slightly above neighboring pins to facilitate smooth sliding
void HybridTokens::setCubeHeights(int height, float edgeLengthMultiplier, TouchCondition touchCondition) {
    for (vector<Cube>::iterator cube = kinectTracker->redCubes.begin(); cube < kinectTracker->redCubes.end(); cube++) {
        if (touchCondition == UNDEFINED || (touchCondition == TOUCHED && cube->isTouched) ||
                (touchCondition == NOT_TOUCHED && !cube->isTouched)) {
            setCubeHeight(*cube, height, edgeLengthMultiplier);
        }
    }
}

// height value defaults to cube height. passing a non-negative value to farHeight
// linearly interpolates the sword height
void HybridTokens::drawSword(int height, int farHeight) {
    // if a second height was given, draw a gradient sword
    if (farHeight >= 0) {
        verticalGradientRect(swordRectangle, farHeight, height);

    // else draw a simple sword
    } else {
        ofSetColor(height);
        ofRect(swordRectangle.left, swordRectangle.top, swordRectangle.width, swordRectangle.height);
    }
}

// height value defaults to cube height. passing a non-negative value to farHeight
// linearly interpolates the sword height
void HybridTokens::drawSwordForCube(Cube &cube, int height, int farHeight) {
    // transition to the cube's reference frame
    glPushMatrix();
    glTranslatef(cube.center.x * lengthScale, cube.center.y * lengthScale, 0.0f);
    glRotatef(-cube.theta, 0.0f, 0.0f, 1.0f);

    // draw sword
    drawSword(height, farHeight);

    // reset coordinate system
    glPopMatrix();
}

// calculate the intersection and union drawings of the swords for all cubes
void HybridTokens::getSwordsIntersectionAndUnion(ofPixels &swordsIntersection, ofPixels &swordsUnion) {
    // buffer repository for drawing a single sword into; associated pixels object for manipulating the result
    ofFbo swordBuffer;
    ofPixels swordPixels;
    swordBuffer.allocate(lengthScale, lengthScale, GL_RGBA);

    // pixels objects used to calculate the swords' intersection and union
    swordsIntersection.allocate(lengthScale, lengthScale, 1);
    swordsUnion.allocate(lengthScale, lengthScale, 1);
    swordsIntersection.set(255);
    swordsUnion.set(0);

    // draw each cube's sword and update the swords' intersection and union
    for (int i = 0; i < kinectTracker->redCubes.size(); i++) {
        // draw sword into buffer
        swordBuffer.begin();
        ofBackground(0);
        drawSwordForCube(kinectTracker->redCubes[i]);
        swordBuffer.end();

        // extract sword data as grayscale pixels
        swordBuffer.readToPixels(swordPixels);
        swordPixels.setNumChannels(1);

        for (int j = 0; j < swordPixels.size(); j++) {
            // update intersection pixel
            if (swordsIntersection[j]) {
                swordsIntersection[j] = swordPixels[j];
            }
            // update union pixel
            if (swordPixels[j]) {
                swordsUnion[j] = swordPixels[j];
            }
        }
    }
}

void HybridTokens::drawBooleanSwords() {
    if (!kinectTracker->redCubes.size()) {
        return;
    }

    // calculate intersection and union of swords
    ofPixels swordsIntersection, swordsUnion;
    getSwordsIntersectionAndUnion(swordsIntersection, swordsUnion);

    // calculate appropriate output drawing given the active schema
    ofPixels swordsOutput;
    swordsOutput.allocate(lengthScale, lengthScale, 1);

    // union of swords
    if (booleanSwordsSchema == UNION) {
        swordsOutput = swordsUnion;

    // intersection of swords
    } else if (booleanSwordsSchema == INTERSECTION) {
        swordsOutput = swordsIntersection;

    // sum of swords (add intersection on top of union)
    } else if (booleanSwordsSchema == SUM) {
        swordsOutput = swordsUnion;
        if (kinectTracker->redCubes.size() > 1) {
            for (int j = 0; j < swordsOutput.size(); j++) {
                if (swordsIntersection[j]) {
                    swordsOutput[j] = 255; //swordsOutput[j] += swordsIntersection[j];
                }
            }
        }

    // xor of swords (subtract intersection from union)
    } else if (booleanSwordsSchema == XOR) {
        swordsOutput = swordsUnion;
        if (kinectTracker->redCubes.size() > 1) {
            for (int j = 0; j < swordsOutput.size(); j++) {
                if (swordsIntersection[j]) {
                    swordsOutput[j] = 0;
                }
            }
        }
    }

    // draw the swords
    ofSetColor(255);
    ofImage(swordsOutput).draw(0,0);

    // but don't draw swords under the cubes
    setCubeHeights(0, 1.5);

    // lift touched cubes slightly off the surface for a smooth dragging experience
    setCubeHeights(40, 1.0, TOUCHED);
}

// for now, this function assumes a maximum of two cubes to deal with
void HybridTokens::drawFlexibleSwords(int height) {
    if (!kinectTracker->redCubes.size() || kinectTracker->redCubes.size() > 2) {
        return;
    }

    // if there's just one cube, draw a standard sword
    if (kinectTracker->redCubes.size() == 1) {
        drawBooleanSwords();
        return;
    }

    // construct control points for the bezier curve from the cubes
    ofPoint controlPoints[4];
    controlPoints[0].set(kinectTracker->redCubes[0].center * lengthScale);
    controlPoints[3].set(kinectTracker->redCubes[1].center * lengthScale);
    float angle0 = kinectTracker->redCubes[0].thetaRadians;
    float angle1 = kinectTracker->redCubes[1].thetaRadians;
    int cubeSeparation = controlPoints[0].distance(controlPoints[3]);
    controlPoints[1].set(controlPoints[0] + ofPoint(-sin(angle0), -cos(angle0)) * cubeSeparation * flexibleExtensionSize);
    controlPoints[2].set(controlPoints[3] + ofPoint(-sin(angle1), -cos(angle1)) * cubeSeparation * flexibleExtensionSize);

    // calculate points along the 1D bezier curve and corresponding boundary vertices for the 2D shape
    const int sampleSize = 1000;
    ofPoint interpolationPoints[sampleSize];
    ofPoint boundaryVertices[2 * sampleSize];
    ofPoint *previousPoint = &controlPoints[0];
    for (int i = 0; i < sampleSize; i++) {
        // calculate the cubic bezier interpolation point
        float t = 1.0 * (i + 1) / sampleSize;
        float c0 = (1 - t) * (1 - t) * (1 - t);
        float c1 = (1 - t) * (1 - t) * t * 3;
        float c2 = (1 - t) * t * t * 3;
        float c3 = t * t * t;
        interpolationPoints[i] = c0 * controlPoints[0] + c1 * controlPoints[1] + c2 * controlPoints[2] + c3 * controlPoints[3];

        // find the normalized vectors tangent to the interpolation path at this point
        ofPoint offsetDirection((interpolationPoints[i] - *previousPoint).getNormalized());
        ofPoint leftDirection(offsetDirection.y, -offsetDirection.x);
        ofPoint rightDirection(-offsetDirection.y, offsetDirection.x);

        // add vertex points at the boundaries of a cube tracing the interpolation points
        boundaryVertices[i].set(interpolationPoints[i] + leftDirection * cubeEdgeLength * lengthScale / 2);
        boundaryVertices[2 * sampleSize - 1 - i].set(interpolationPoints[i] + rightDirection * cubeEdgeLength * lengthScale / 2);

        // update the previous point pointer
        previousPoint = &interpolationPoints[i];
    }

    // draw the 2D curve
    ofSetColor(height);
    ofSetLineWidth(0);
    ofFill();
    ofBeginShape();
    ofVertices(vector<ofPoint>(boundaryVertices, boundaryVertices + 2 * sampleSize));
    ofEndShape();

    // but don't draw swords under the cubes
    setCubeHeights(0, 1.5);

    // lift touched cubes slightly off the surface for a smooth dragging experience
    setCubeHeights(40, 1.0, TOUCHED);
}

TiltDirection HybridTokens::getPhysicsSwordTiltDirection(Cube &topCube, Cube &bottomCube) {
    // get the top sword's center point in the bottom cube's coordinates
    ofPoint topSwordCenter(swordRectangle.left + 0.5 *  swordRectangle.width, swordRectangle.top + 0.5 * swordRectangle.height);
    topCube.transformPointFromCubeReferenceFrame(&topSwordCenter, &topSwordCenter, lengthScale);
    bottomCube.transformPointToCubeReferenceFrame(&topSwordCenter, &topSwordCenter, lengthScale);

    // if the top sword's center overlaps the bottom sword, there is no tilt
    if (swordRectangle.containsPoint(topSwordCenter)) {
        return NO_TILT;

    // else, figure out which way the sword tilts
    } else {
        // find the point on the bottom sword nearest the top sword center
        ofPoint nearestPointOnBottomSword;
        swordRectangle.findNearestPointOnPerimeter(topSwordCenter, nearestPointOnBottomSword);
        
        // get the top sword's base point (its bottom-center) in the bottom cube's coordinates
        ofPoint topSwordBase(swordRectangle.left + 0.5 *  swordRectangle.width, swordRectangle.top + swordRectangle.height);
        topCube.transformPointFromCubeReferenceFrame(&topSwordBase, &topSwordBase, lengthScale);
        bottomCube.transformPointToCubeReferenceFrame(&topSwordBase, &topSwordBase, lengthScale);

        // if the top sword's center point forms an obtuse angle with the bottom sword's nearest point
        // and the top sword's base, the cube is tilted back. otherwise, it is tilted forward
        float angle = (nearestPointOnBottomSword - topSwordCenter).angle(topSwordBase - topSwordCenter);
        return angle > 90 ? TILT_BACKWARD : TILT_FORWARD;
    }
}

// for now, this function assumes a maximum of two cubes to deal with
void HybridTokens::drawPhysicsSwords() {
    if (!kinectTracker->redCubes.size() || kinectTracker->redCubes.size() > 2) {
        return;
    }

    // if there's just one cube, draw a standard sword
    if (kinectTracker->redCubes.size() == 1) {
        drawBooleanSwords();
        return;
    }

    // designate the right-side cube as the top one
    Cube *bottomCube, *topCube;
    if (kinectTracker->redCubes[0].center.x < kinectTracker->redCubes[1].center.x) {
        bottomCube = &kinectTracker->redCubes[0];
        topCube = &kinectTracker->redCubes[1];
    } else {
        bottomCube = &kinectTracker->redCubes[1];
        topCube = &kinectTracker->redCubes[0];
    }

    // calculate intersection and union of swords
    ofPixels swordsIntersection, swordsUnion;
    getSwordsIntersectionAndUnion(swordsIntersection, swordsUnion);

    // determine whether the swords intersect
    bool swordsIntersect = false;
    for (int i = 0; i < swordsIntersection.size(); i++) {
        if (swordsIntersection[i]) {
            swordsIntersect = true;
            break;
        }
    }

    // determine the target sword height for the top cube (the bottom cube will just lie flat)
    int targetHeightNearTopCube, targetHeightFarFromTopCube;

    // if the swords don't intersect, the "top" sword should lie on the ground
    if (!swordsIntersect) {
        targetHeightNearTopCube = cubeHeight;
        targetHeightFarFromTopCube = cubeHeight;

    // else, figure out the top sword's tilt-dependent heights
    } else {
        // determine whether the top cube should be falling off, and if so, which way
        TiltDirection topCubeTiltDirection = topCube->isTouched ? NO_TILT :
                getPhysicsSwordTiltDirection(*topCube, *bottomCube);

        // use tilt to determine target sword heights near and far from cube
        if (topCubeTiltDirection == TILT_BACKWARD) {
            targetHeightNearTopCube = cubeHeight;
            targetHeightFarFromTopCube = 255;
        } else if (topCubeTiltDirection == TILT_FORWARD) {
            targetHeightNearTopCube = 255;
            targetHeightFarFromTopCube = cubeHeight;
        } else {
            targetHeightNearTopCube = 255;
            targetHeightFarFromTopCube = 255;
        }
    }

    // interpolate top sword heights towards targets
    int nearHeightAdjustment = targetHeightNearTopCube - physicsTopSwordHeightNearCube;
    int farHeightAdjustment = targetHeightFarFromTopCube - physicsTopSwordHeightFarFromCube;
    int maxChangePerFrame = 255 / ofGetFrameRate();
    nearHeightAdjustment = max(-maxChangePerFrame, min(maxChangePerFrame, nearHeightAdjustment));
    farHeightAdjustment = max(-maxChangePerFrame, min(maxChangePerFrame, farHeightAdjustment));
    physicsTopSwordHeightNearCube += nearHeightAdjustment;
    physicsTopSwordHeightFarFromCube += farHeightAdjustment;

    // allocate a drawing repository
    ofFbo drawBuffer;
    drawBuffer.allocate(lengthScale, lengthScale, GL_RGBA);
    
    // draw swords to buffer
    drawBuffer.begin();
    ofBackground(0);
    
    // bottom cube gets a normal sword
    drawSwordForCube(*bottomCube);
    
    // top cube gets a sword with the calculated heights
    drawSwordForCube(*topCube, physicsTopSwordHeightNearCube, physicsTopSwordHeightFarFromCube);
    
    drawBuffer.end();
    
    // extract sword data as grayscale pixels
    ofPixels swordPixels;
    drawBuffer.readToPixels(swordPixels);
    swordPixels.setNumChannels(1);
    
    // draw cube footprints as depressions into a white background.
    // footprints include clearings and touch-sensitive risers
    drawBuffer.begin();
    ofBackground(255);

    // top cube's footprint
    int topCubeBaseHeight = physicsTopSwordHeightNearCube - cubeHeight;
    if (topCube->isTouched) {
        int clearingHeight = max(0, topCubeBaseHeight - 40);
        setCubeHeight(*topCube, clearingHeight, 1.5);
        setCubeHeight(*topCube, clearingHeight + 40);
    } else {
        setCubeHeight(*topCube, topCubeBaseHeight, 1.5);
    }
    
    // bottom cube's footprint
    setCubeHeight(*bottomCube, 0, 1.5);
    if (bottomCube->isTouched) {
        setCubeHeight(*bottomCube, 40);
    }
    drawBuffer.end();
    
    // extract cube footprint data as grayscale pixels
    ofPixels cubeFootprintPixels;
    drawBuffer.readToPixels(cubeFootprintPixels);
    cubeFootprintPixels.setNumChannels(1);
    
    // cap sword pixel heights at footprint pixel depressions
    for (int i = 0; i < swordPixels.size(); i++) {
        if (swordPixels[i] > cubeFootprintPixels[i]) {
            swordPixels[i] = cubeFootprintPixels[i];
        }
    }
    
    // draw the swords
    ofSetColor(255);
    ofImage(swordPixels).draw(0,0);
}

void HybridTokens::drawDynamicallyConstrainedSwords() {
    if (kinectTracker->redCubes.size() != 1) {
        return;
    }
    
    // for now, assume a ready cube is flat and aligned to the coordinate axes
    
    // dynamic cube
    Cube cube = kinectTracker->redCubes[0];
    
    // for now, hardcode a description of the static block
    ofPoint fixedCenter(0.17, 0.34);
    
    // draw static sword right
    ofSetColor(140);
    int fixedLeft, fixedRight, fixedTop, fixedBottom;
    fixedLeft = (fixedCenter.x + 0.07 + 0.3 * cubeEdgeLength) * lengthScale;
    fixedRight = (fixedCenter.x + 0.07 + 3 * cubeEdgeLength) * lengthScale;
    fixedTop = (fixedCenter.y - 0.07) * lengthScale;
    fixedBottom = (fixedCenter.y + 0.07) * lengthScale;
    ofRect(fixedLeft, fixedTop, fixedRight - fixedLeft, fixedBottom - fixedTop);
    
    // get dynamic sword boundaries
    int left, right, top, bottom;
    left = (cube.center.x - 0.5 * cubeEdgeLength) * lengthScale;
    right = (cube.center.x + 0.5 * cubeEdgeLength) * lengthScale;
    top = (cube.center.y - 0.07 - 3 * cubeEdgeLength) * lengthScale;
    bottom = (cube.center.y - 0.07 - 0.3 * cubeEdgeLength) * lengthScale;

    bool swordMayPass = top > fixedBottom;

    // draw dynamic sword - if it's on a collision course, make sure it doesn't accidentally pass the static sword
    ofSetColor(140);
    if (swordMayPass) {
        ofRect(left, top, right - left, bottom - top);
    } else {
        ofRect(max(left, fixedRight), top, right - left, bottom - top);
    }
    
    // or just do:
    //drawSwordForCube(cube);
    
    // don't draw swords under the dynamic cubes
    setCubeHeights(0, 1.5);
    
    // lift touched cubes slightly off the surface for a smooth dragging experience
    setCubeHeights(40, 1.0, TOUCHED);

    // draw blockade if appropriate
    if (!swordMayPass) {
        ofSetColor(140);
        int closeDistance = 0.07 * lengthScale;
        if (left < fixedRight + closeDistance) {
            int adjWidth = cubeEdgeLength * lengthScale;
            int adjHeight = cubeEdgeLength * lengthScale;
            int cubeBottom = (cube.center.y + 0.07) * lengthScale;
            ofRect(fixedRight - adjWidth / 4, cubeBottom - adjHeight, adjWidth / 4, adjHeight);
        }
    }
}

void HybridTokens::keyPressed(int key) {
    if(key == 'a') {
        booleanSwordsSchema = UNION;
    }

    if(key == 's') {
        booleanSwordsSchema = INTERSECTION;
    }

    if(key == 'd') {
        booleanSwordsSchema = SUM;
    }

    if(key == 'f') {
        booleanSwordsSchema = XOR;
    }

    if(key == '-') {
        flexibleExtensionSize -= 0.1;
    }

    if(key == '=' || key == '+') {
        flexibleExtensionSize += 0.1;
    }
}

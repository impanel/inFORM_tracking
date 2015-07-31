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

    } else if (mode == VERTICAL_DEFORMATION_SWORDS) {
        drawVerticalDeformationSwords();
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
            if (pinHeightMapContentPixels[i] > STANDARD_CUBE_HEIGHT) {
                pinGraphicsPixels[i + 0] = pinColorIfHigh[0];
                pinGraphicsPixels[i + 1] = pinColorIfHigh[1];
                pinGraphicsPixels[i + 2] = pinColorIfHigh[2];
            } else if (pinHeightMapContentPixels[i] == STANDARD_CUBE_HEIGHT - 1) {
                pinGraphicsPixels[i + 0] = pinColorAltIfOn[0];
                pinGraphicsPixels[i + 1] = pinColorAltIfOn[1];
                pinGraphicsPixels[i + 2] = pinColorAltIfOn[2];
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
        verticalLinearGradientRect(swordRectangle, farHeight, height);

    // else draw a simple sword
    } else {
        ofSetColor(height);
        ofRect(swordRectangle.left, swordRectangle.top, swordRectangle.width, swordRectangle.height);
    }
}

// draw sword with a bezier gradient passing through the given interpolation points
void HybridTokens::drawSword(vector<pair<float, float> > &interpolationPoints) {
    // if too few interpolation points are passed in, gracefully fall through to
    // simpler cases. if too many points are passed in, complain and exit.
    if (interpolationPoints.size() != 3) {
        if (interpolationPoints.size() == 0) {
            drawSword();
        } else if (interpolationPoints.size() == 1) {
            drawSword(interpolationPoints[0].second);
        } else if (interpolationPoints.size() == 2) {
            drawSword(interpolationPoints[0].second, interpolationPoints[1].second);
        } else {
            cout << "Error: drawSword does not currently support more than three interpolation points." << endl;
        }
        return;
    }

    // draw sword - use an inverted sword rectangle so that interpolation indices map to
    // distance from cube (e.g. index 0 is the cube, index 1 is the top tip of the sword)
    Rectangle invertedTopAndBottomSword = swordRectangle.withInvertedTopAndBottom();
    verticalBezierSmartInterpolatedGradientRect(invertedTopAndBottomSword, interpolationPoints);
}

// height value defaults to cube height. passing a non-negative value to farHeight
// linearly interpolates the sword height
void HybridTokens::drawSwordForCube(Cube &cube, int height, int farHeight) {
    if (cube.disabled) {
        return;
    }

    // transition to the cube's reference frame
    glPushMatrix();
    glTranslatef(cube.center.x * lengthScale, cube.center.y * lengthScale, 0.0f);
    glRotatef(-cube.theta, 0.0f, 0.0f, 1.0f);

    // draw sword
    drawSword(height, farHeight);

    // if this cube owns any subcubes, draw their swords
    for (int i = 0; i < cube.subCubesCount; i++) {
        drawSwordForCube(*cube.subCubes[i], height, farHeight);
    }

    // reset coordinate system
    glPopMatrix();
}

// draw sword with a bezier gradient passing through the given interpolation points
void HybridTokens::drawSwordForCube(Cube &cube, vector<pair<float, float> > &interpolationPoints) {
    // transition to the cube's reference frame
    glPushMatrix();
    glTranslatef(cube.center.x * lengthScale, cube.center.y * lengthScale, 0.0f);
    glRotatef(-cube.theta, 0.0f, 0.0f, 1.0f);

    // draw sword
    drawSword(interpolationPoints);

    // reset coordinate system
    glPopMatrix();
}

// make it easy to move cubes by clearing space around them and lifting up touched cubes
void HybridTokens::drawStandardClearingsAndRisers() {
    // don't draw swords under the cubes
    setCubeHeights(0, 1.5);

    // lift touched cubes slightly off the surface for a smooth dragging experience
    setCubeHeights(40, 1.0, TOUCHED);
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

// find the intersection point of two swords' primary axes
void HybridTokens::getSwordsAxesIntersectionPoint(Cube &firstCube, Cube &secondCube, ofPoint &dst) {
    // get cube center coordinates
    ofPoint topCubeCenter, bottomCubeCenter;
    topCubeCenter = firstCube.center * lengthScale;
    bottomCubeCenter = secondCube.center * lengthScale;

    // calculate the y = mx + b equations for the swords' central axes:
    //   m = tan(theta)        // note that theta here is cube->theta + 90
    //   b = -m * x_0 + y_0    // x_0 and y_0 can be any intercept; use the cube center
    float m = tan(firstCube.thetaRadians + pi / 2);
    float b = -m * topCubeCenter.x + topCubeCenter.y;
    float m_ = tan(secondCube.thetaRadians + pi / 2);
    float b_ = -m_ * bottomCubeCenter.x + bottomCubeCenter.y;

    // BUT - because the cube coordinate system is left-handed, negate the slopes
    m = -m;
    m_ = -m_;

    // determine the intersection point of these axes:
    //   x = - (b - b') / (m - m')
    //   y = m * x + b
    ofPoint intersectionPoint;
    intersectionPoint.x = -(b - b_) / (m - m_);
    intersectionPoint.y = m * intersectionPoint.x + b;

    dst.set(intersectionPoint);
}

// use the blob finder to find the center point of the swords' intersection
void HybridTokens::getCenterOfImageBlob(ofPixels &thresholdedPixels, ofPoint &dst) {
    // set up blob finder parameters
    blobFinder.bTrackBlobs = true;
    blobFinder.bTrackFingers = false;

    // convert pixels to a cvImage
    int width = thresholdedPixels.getWidth();
    int height = thresholdedPixels.getHeight();
    ofxCvGrayscaleImage cvImage;
    cvImage.allocate(width, height);
    cvImage.setFromPixels(thresholdedPixels);

    // find blob
    blobFinder.findContours(cvImage, 1, width * height, 1, 20.0, false);
    vector<Blob> blobs = blobFinder.blobs;

    // for now, take the simplest exit route when no blobs are found
    if (!blobs.size()) {
        cout << "Error: blob detection failed in getCenterOfImageBlob" << endl;
        return;
    }

    // set dst to the normalized blob centroid
    Blob *intersectionBlob = &blobs[0];
    ofPoint normalizationVector(1.0 / intersectionBlob->widthScale, 1.0 / intersectionBlob->heightScale);
    dst.set(intersectionBlob->centroid * normalizationVector * lengthScale);
}

void HybridTokens::drawSwords() {
    // draw the swords
    ofSetColor(255);
    ofBackground(0);
    for (int i = 0; i < kinectTracker->redCubes.size(); i++) {
        drawSwordForCube(kinectTracker->redCubes[i]);
    }

    // but give cubes room to move
    drawStandardClearingsAndRisers();
}

void HybridTokens::drawBooleanSwords() {
    if (kinectTracker->redCubes.size() < 2) {
        drawSwords();
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
        for (int j = 0; j < swordsOutput.size(); j++) {
            if (swordsIntersection[j]) {
                swordsOutput[j] = 255; //swordsOutput[j] += swordsIntersection[j];
            }
        }

    // xor of swords (subtract intersection from union)
    } else if (booleanSwordsSchema == XOR) {
        swordsOutput = swordsUnion;
        for (int j = 0; j < swordsOutput.size(); j++) {
            if (swordsIntersection[j]) {
                swordsOutput[j] = 0;
            }
        }
    }

    // draw the swords
    ofSetColor(255);
    ofImage(swordsOutput).draw(0,0);

    // but give cubes room to move
    drawStandardClearingsAndRisers();
}

// for now, this function assumes a maximum of two cubes to deal with
void HybridTokens::drawFlexibleSwords(int height) {
    if (kinectTracker->redCubes.size() < 2) {
        drawSwords();
        return;
    }

    // for now, only deal with the two cubes case
    if (kinectTracker->redCubes.size() > 2) {
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

    // but give cubes room to move
    drawStandardClearingsAndRisers();
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
    if (kinectTracker->redCubes.size() < 2) {
        drawSwords();
        return;
    }

    // for now, only deal with the two cubes case
    if (kinectTracker->redCubes.size() > 2) {
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

    // interpolate top sword heights towards targets for smooth frame-to-frame animation
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

void HybridTokens::drawDynamicallyConstrainedSwords(int height) {
    if (kinectTracker->redCubes.size() < 2) {
        drawSwords();
        return;
    }

    // for now, only deal with the two cubes case
    if (kinectTracker->redCubes.size() > 2) {
        return;
    }

    // designate the left-side cube as the fixed one
    Cube *fixedCube, *dynamicCube;
    if (kinectTracker->redCubes[0].center.x < kinectTracker->redCubes[1].center.x) {
        fixedCube = &kinectTracker->redCubes[0];
        dynamicCube = &kinectTracker->redCubes[1];
    } else {
        fixedCube = &kinectTracker->redCubes[1];
        dynamicCube = &kinectTracker->redCubes[0];
    }

    // for now, assume all cubes are aligned to the coordinate axes

    // draw static sword right
    ofSetColor(height + 1); // increase height to receive special high color
    int fixedLeft, fixedRight, fixedTop, fixedBottom;
    fixedLeft = (fixedCube->center.x + 0.5 * cubeEdgeLength) * lengthScale;
    fixedRight = (fixedCube->center.x + (0.5 + 3) * cubeEdgeLength) * lengthScale;
    fixedTop = (fixedCube->center.y - 0.5 * cubeEdgeLength) * lengthScale;
    fixedBottom = (fixedCube->center.y + 0.5 * cubeEdgeLength) * lengthScale;
    ofRect(fixedLeft, fixedTop, fixedRight - fixedLeft, fixedBottom - fixedTop);
    
    // get dynamic sword boundaries
    int left, right, top, bottom;
    left = (dynamicCube->center.x - 0.5 * cubeEdgeLength) * lengthScale;
    right = (dynamicCube->center.x + 0.5 * cubeEdgeLength) * lengthScale;
    top = (dynamicCube->center.y - (0.5 + 3) * cubeEdgeLength) * lengthScale;
    bottom = (dynamicCube->center.y + 0.5 * cubeEdgeLength) * lengthScale;

    bool swordMayPass = top > fixedBottom || bottom < fixedTop;

    // draw dynamic sword up - if it's on a collision course, make sure it doesn't accidentally pass the static sword
    ofSetColor(height);
    if (swordMayPass) {
        ofRect(left, top, right - left, bottom - top);
    } else {
        ofRect(max(left, fixedRight), top, right - left, bottom - top);
    }

    // but give cubes room to move
    drawStandardClearingsAndRisers();

    // draw blockade if appropriate
    if (!swordMayPass) {
        ofSetColor(height - 1); // decrease height to get special alt color
        int closeDistance = 0.5 * cubeEdgeLength * lengthScale;
        if (left < fixedRight + closeDistance) {
            int adjWidth = cubeEdgeLength * lengthScale;
            int adjHeight = cubeEdgeLength * lengthScale;
            int cubeBottom = (dynamicCube->center.y + 0.5 * cubeEdgeLength) * lengthScale;
            ofRect(fixedRight - adjWidth / 4, cubeBottom - adjHeight, adjWidth / 4, adjHeight);
        }
    }
}

// for now, this function assumes a maximum of two cubes to deal with
void HybridTokens::drawVerticalDeformationSwords() {
    if (kinectTracker->redCubes.size() < 2) {
        drawSwords();
        return;
    }

    // for now, only deal with the two cubes case
    if (kinectTracker->redCubes.size() > 2) {
        return;
    }

    // designate the right-side cube as the top one (the deforming one)
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

    // if there's no intersection, just draw regular swords
    if (!swordsIntersect) {
        drawSwords();
        return;
    }

    // find the center of the swords' intersection
    ofPoint intersectionPoint;
    getCenterOfImageBlob(swordsIntersection, intersectionPoint);

    // transform the intersection point into a displacement vector along the top sword's axis
    topCube->transformPointToCubeReferenceFrame(&intersectionPoint, &intersectionPoint, lengthScale);

    // find the displacement vector representing the base of a sword axis
    ofPoint swordAxisBase(0, swordRectangle.top + swordRectangle.height);

    // determine the intersection point's percent displacement along the top sword's central axis
    float intersectionDistance = abs(intersectionPoint.y - swordAxisBase.y) / swordRectangle.height;
    float minimumInterpolationDistance = 0.05;
    intersectionDistance = max(minimumInterpolationDistance, min(1 - minimumInterpolationDistance, intersectionDistance));

    // construct interpolation points through the intersection for the deformed top sword
    float baseInterpolationPoint = cubeHeight * 0.7;
    float tipInterpolationPoint = cubeHeight;
    if (intersectionDistance > 0.8) {
        tipInterpolationPoint = 255;
    } else {
        if (intersectionDistance < 0.2) {
            baseInterpolationPoint = 255;
        }
    }
    vector<pair<float, float> > deformationInterpolationPoints;
    deformationInterpolationPoints.push_back(pair<float, float>(0, baseInterpolationPoint));
    deformationInterpolationPoints.push_back(pair<float, float>(intersectionDistance, 255));
    deformationInterpolationPoints.push_back(pair<float, float>(1, tipInterpolationPoint));

    // calculate the deformed sword cube's matching riser height. use a logistic function
    // that, roughly speaking, lifts the curve from zero to full height as the intersection's
    // distance to the cube's base falls from 0.8 to 0.4
    int topCubeRiserHeight = (255 - cubeHeight) / (1 + exp(10 * (intersectionDistance - 0.6)));

    // allocate a drawing repository
    ofFbo drawBuffer;
    drawBuffer.allocate(lengthScale, lengthScale, GL_RGBA);
    
    // draw swords to buffer
    drawBuffer.begin();
    ofBackground(0);
    
    // bottom cube gets a normal sword
    drawSwordForCube(*bottomCube);
    
    // top cube gets a sword interpolated through the intersection distance
    drawSwordForCube(*topCube, deformationInterpolationPoints);

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
    if (topCube->isTouched) {
        int clearingHeight = max(0, topCubeRiserHeight - 40);
        setCubeHeight(*topCube, clearingHeight, 1.5);
        setCubeHeight(*topCube, clearingHeight + 40);
    } else {
        setCubeHeight(*topCube, topCubeRiserHeight, 1.5);
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

    // merge
    if(key == 'm') {
        if (mode == BOOLEAN_SWORDS && kinectTracker->redCubes.size() == 2) {
            kinectTracker->redCubes[0].addSubCube(&kinectTracker->redCubes[1]);
            kinectTracker->redCubes[1].disabled = true;
        }
    }
}

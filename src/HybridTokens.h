//
//  HybridTokens.h
//  Relief2
//
//  Created by Daniel Windham on 3/18/15.
//
//

#ifndef __Relief2__HybridTokens__
#define __Relief2__HybridTokens__

#include "RenderableObject.h"
#include "KinectTracker.h"
#include "CameraCalibration.h"
#include "Rectangle.h"
#include "DrawingUtils.h"
#include "ofFbo.h"
#include "ofxKCore.h"


enum HybridTokensMode {
    BOOLEAN_SWORDS,
    FLEXIBLE_SWORDS,
    PHYSICS_SWORDS,
    DYNAMICALLY_CONSTRAINED_SWORDS,
    VERTICAL_DEFORMATION_SWORDS
};
enum BooleanSwordsSchema {SUM, XOR, UNION, INTERSECTION};
enum TouchCondition {TOUCHED, NOT_TOUCHED, UNDEFINED};
enum TiltDirection {NO_TILT, TILT_FORWARD, TILT_BACKWARD};

class HybridTokens : public RenderableObject {
public:
    
    HybridTokens(KinectTracker *tracker);
    
    void drawHeightMap();
    void drawGraphics();
    void update(float dt);

    void keyPressed(int key);

    void setMode(HybridTokensMode mode);

    // application mode
    HybridTokensMode mode;

    // boolean swords schema specifier
    BooleanSwordsSchema booleanSwordsSchema;

private:
    KinectTracker *kinectTracker;
    ofFbo pinHeightMapImage;                  // FBO where we render height map
    ofFbo cubeFootprintsFbo;                  // FBO describing space covered by cubes
    ofPixels cubeFootprintPixels;             // pixels object of cube locations, to avoid painting on cubes
    ofPixels pinHeightMapContentPixels;       // pixels object for computing on pin height map content
    ofPixels pinGraphicsPixels;               // pixels object where we render graphics
    ofPixels reprojectedPinGraphicsPixels;    // pixels object whose graphics account for the underlying pin heights
    vector<Cube> storedCubes;

    void updateGraphics();
    void setCubeHeight(Cube &cube, int height, float edgeLengthMultiplier = 1.0);
    void setCubeHeights(int height, float edgeLengthMultiplier = 1.0, TouchCondition touchCondition=UNDEFINED);
    void drawSword(int height=STANDARD_CUBE_HEIGHT, int farHeight=-1);
    void drawSword(vector<pair<float, float> > &interpolationPoints);
    void drawSwordForCube(Cube &cube, int height=STANDARD_CUBE_HEIGHT, int farHeight=-1);
    void drawSwordForCube(Cube &cube, vector<pair<float, float> > &interpolationPoints);
    void drawStandardClearingsAndRisers();
    void getSwordsIntersectionAndUnion(ofPixels &swordsIntersection, ofPixels &swordsUnion, bool useStoredCubes=false);
    void getSwordsAxesIntersectionPoint(Cube &firstCube, Cube &secondCube, ofPoint &dst);
    void getCenterOfImageBlob(ofPixels &thresholdedPixels, ofPoint &dst);
    void drawSwords();
    void drawBooleanSwords();
    void drawFlexibleSwords(int height=STANDARD_CUBE_HEIGHT);
    void drawPhysicsSwords();
    void drawDynamicallyConstrainedSwords(int height=STANDARD_CUBE_HEIGHT);
    void drawVerticalDeformationSwords();
    TiltDirection getPhysicsSwordTiltDirection(Cube &topCube, Cube &bottomCube);

    Rectangle swordRectangle;
    ContourFinder blobFinder;

    char pinColorIfHigh[3] = {255, 135, 0}; // yellow
    char pinColorIfOn[3] = {255, 12, 16}; // red
    char pinColorAltIfOn[3] = {0, 0, 255}; // blue
    char pinColorIfOff[3] = {0, 0, 0}; // black

    const float pinSize = 1.0 / RELIEF_SIZE_X;
    const float cubeEdgeLength = 4 * pinSize;
    const float lengthScale = RELIEF_SIZE_X; // scalar to convert normalized lengths to image coordinate lengths
    const int cubeHeight = STANDARD_CUBE_HEIGHT;

    float flexibleExtensionSize;

    bool initializedInUpdate = false; // hack to fix an ofPixels bug. see .cpp for details

    int physicsTopSwordHeightNearCube = 0;
    int physicsTopSwordHeightFarFromCube = 0;

};

#endif /* defined(__Relief2__HybridTokens__) */
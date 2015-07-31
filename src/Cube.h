//
//  Cube.h
//  Relief2
//
//  Created by Daniel Windham on 3/25/15.
//
//

#include "Constants.h"
#include "utils.h"
#include "CameraCalibration.h"
#include "ofxKCore.h"
#include <vector>


#ifndef __Relief2__Cube__
#define __Relief2__Cube__

//           w                 w    .*1
//  0+---------------+1         . ' | \                        ^ -y
//   |               |      0*'     |  \   . '                 |
// h |       +       |        \     |  .\'   )theta     -x <---+---> +x
//   |       ^center |       --\----+'---\----                 |
//  3+---------------+2       h \   |     \                    v +y
//
// all cube distances are in fractions of a containing unit-square; the image
// frame the cube comes from is interpreted as having units width = height = 1
//
// Note that this coordinate system is left-handed! Be careful when rotating.

const float pinSize = 1.0 / RELIEF_SIZE_X;
const int maxSubCubesCount = 10;

class CubeUpdatesBuffer {
public:
    CubeUpdatesBuffer() {};

    Blob *blob;
    ofPoint rawMarker;
    bool hasMarker;
    ofPoint normalizationVector; // x- and y-direction scaling to normalize blob units
    float width;
    float height;
    ofPoint center;
    ofPoint marker;
    float rawTheta;
    float rawThetaRadians;
    ofPoint rawCorners[4];
    float theta;            // measured counterclockwise
    float thetaRadians;     // theta in radians
    ofPoint corners[4];     // coordinates relative to center

};

class Cube {
public:
    Cube();
    Cube(Blob *_blob, bool _update=true);
    Cube(Blob *_blob, ofPoint _marker, bool _update=true);
    Cube(Cube *cube);
    ~Cube();
    bool isValid();         // test if cube is set up; cube only has meaning when it owns a blob
    void update();
    void setBlob(Blob *_blob, bool _update=true);
    void setMarker(ofPoint _marker, bool _update=true);
    void setBlobAndMarker(Blob *_blob, ofPoint _marker, bool _update=true);
    void clearMarker(bool _update=true);
    Blob *getCandidateBlob();
    void transformPointToCubeReferenceFrame(ofPoint *src, ofPoint *dst, float lengthScale=1.0);
    void transformPointFromCubeReferenceFrame(ofPoint *src, ofPoint *dst, float lengthScale=1.0);
    void transformCubeToCubeReferenceFrame(Cube *cube);
    void addSubCube(Cube *subCube);

    int blobId;
    double timeOfInitialization; // the time at which this object was created
    ofPoint normalizationVector; // x- and y-direction scaling to normalize blob units
    ofPoint marker;
    bool hasMarker;
    bool isTouched = false; // whether someone is touching this cube; cube managers should assign this directly
    double timeWhenLastTouched;
    double timeWhenLastNotTouched;
    float theta;            // measured counterclockwise
    float thetaRadians;     // theta in radians
    float width;
    float height;
    ofPoint center;
    ofPoint corners[4];     // coordinates relative to center
    ofPoint absCorners[4];  // corners in absolute coordinates
    float minX, maxX, minY, maxY; // cube boundary descriptors (absolute coordinates)
    int cubeTrackingId = -1; // cube managers may assign cube ids if desired
    bool disabled = false; // a disabled cube casts a clearing but nothing else
    bool isSubCube = false;
    int subCubesCount = 0;
    Cube *subCubes[maxSubCubesCount];

private:
    CubeUpdatesBuffer candidateUpdates;
    const static int recentThetaCandidatesLength = 5;
    float recentThetaCandidates[recentThetaCandidatesLength];

    void initialize();
    void calculateCandidateUpdates();
    float thetaDistance(float theta1, float theta2);
    float thetaUsingMarkerHysteresis(float thetaCandidate);
    bool candidateUpdatesAreSignificant();

};
#endif /* defined(__Relief2__Cube__) */

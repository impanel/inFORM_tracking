//
//  Cube.cpp
//  Relief2
//
//  Created by Daniel Windham on 3/25/15.
//
//

#include "Cube.h"

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


// Constructors

Cube::Cube() :
    candidateUpdates(*new CubeUpdatesBuffer),
    timeOfInitialization(clockInSeconds())
{
    candidateUpdates.blob = NULL;
    candidateUpdates.hasMarker = false;
    initialize();
}

Cube::Cube(Blob *_blob, bool _update) :
    candidateUpdates(*new CubeUpdatesBuffer),
    timeOfInitialization(clockInSeconds())
{
    candidateUpdates.hasMarker = false;
    setBlob(_blob, _update);
    initialize();
}

Cube::Cube(Blob *_blob, ofPoint _marker, bool _update) :
    candidateUpdates(*new CubeUpdatesBuffer),
    timeOfInitialization(clockInSeconds())
{
    setBlobAndMarker(_blob, _marker, _update);
    initialize();
}

Cube::Cube(Cube *cube) :
    candidateUpdates(*new CubeUpdatesBuffer),
    timeOfInitialization(cube->timeOfInitialization)
{
    candidateUpdates.blob = NULL;
    candidateUpdates.hasMarker = false;
    initialize();

    normalizationVector = cube->normalizationVector;
    theta = cube->theta;
    thetaRadians = cube->thetaRadians;
    width = cube->width;
    height = cube->height;
    center = cube->center;
    for (int i = 0; i < 4; i++) {
        corners[i] = cube->corners[i];
        absCorners[i] = cube->absCorners[i];
    }
    minX = cube->minX;
    maxX = cube->maxX;
    minY = cube->minY;
    maxY = cube->maxY;

    subCubesCount = cube->subCubesCount;
    for (int i = 0; i < subCubesCount; i++) {
        subCubes[i] = new Cube(cube->subCubes[i]);
        subCubes[i]->isSubCube = true;
    }

    for (int i = 0; i < recentThetaCandidatesLength; i++) {
        recentThetaCandidates[i] = cube->recentThetaCandidates[i];
    }
}

void Cube::initialize() {
    // initialize all recent theta values negative (for null)
    for (int i = 0; i < recentThetaCandidatesLength; i++) {
        recentThetaCandidates[i] = -1;
    }
}

Cube::~Cube() {
    for (int i = 0; i < subCubesCount; i++) {
        // for some reason this errors, but it seems like we should want it:
        //delete subCubes[i];
    }
    candidateUpdates.~CubeUpdatesBuffer();
}

bool Cube::isValid() {
    return candidateUpdates.blob != NULL;
}


// Setters

void Cube::setBlob(Blob *_blob, bool _update) {
    candidateUpdates.blob = _blob;
    if (_update) {
        update();
    }
}

void Cube::setMarker(ofPoint _marker, bool _update) {
    candidateUpdates.rawMarker = _marker;
    candidateUpdates.hasMarker = true;
    if (_update) {
        update();
    }
}

void Cube::setBlobAndMarker(Blob *_blob, ofPoint _marker, bool _update) {
    candidateUpdates.blob = _blob;
    candidateUpdates.rawMarker = _marker;
    candidateUpdates.hasMarker = true;
    if (_update) {
        update();
    }
}

void Cube::clearMarker(bool _update) {
    candidateUpdates.hasMarker = false;
    if (_update) {
        update();
    }
}


// Update

void Cube::calculateCandidateUpdates() {
    // locally use a shorthand alias for the candidate updates object
    CubeUpdatesBuffer &cand = candidateUpdates;

    // a blob's angleBoundingRect height and width variables are flipped. furthermore, blob
    // units are scaled by the size of the image they were found in. fix these mistakes.
    cand.normalizationVector.set(1.0 / cand.blob->widthScale, 1.0 / cand.blob->heightScale);
    cand.width = cand.blob->angleBoundingRect.height * cand.normalizationVector.x;
    cand.height = cand.blob->angleBoundingRect.width * cand.normalizationVector.y;
    cand.center.set(cand.blob->angleBoundingRect.x, cand.blob->angleBoundingRect.y);
    cand.center *= cand.normalizationVector;

    // normalized marker position relative to center
    if (cand.hasMarker) {
        cand.marker = cand.rawMarker * cand.normalizationVector - cand.center;
    }

    // the camera sees a cube's front corners on the ground and rear corners in the air, making
    // the raw center an average between grounded corners and corners that need reprojection,
    // i.e. 50% reprojected. it therefore needs only a half reprojection. (do this after
    // calculating the marker's center-relative location, however, since the marker's importance
    // is its relation to the half-reprojected corners.)
    ofPoint doublyCorrectedCenter;
    reprojectColorCameraCoordinateFromHeight(cand.center, doublyCorrectedCenter);
    cand.center = (cand.center + doublyCorrectedCenter) / 2;

    // range is 0 <= rawTheta < 90; raw theta does not take cube orientation into account
    cand.rawTheta = -cand.blob->angle;
    cand.rawThetaRadians = cand.rawTheta * pi / 180;

    // relative corner coordinates using raw theta value
    cand.rawCorners[0].x = -cand.width/2 * cos(cand.rawThetaRadians) - cand.height/2 * sin(cand.rawThetaRadians);
    cand.rawCorners[0].y = cand.width/2 * sin(cand.rawThetaRadians) - cand.height/2 * cos(cand.rawThetaRadians);
    cand.rawCorners[1].x = cand.width/2 * cos(cand.rawThetaRadians) - cand.height/2 * sin(cand.rawThetaRadians);
    cand.rawCorners[1].y = -cand.width/2 * sin(cand.rawThetaRadians) - cand.height/2 * cos(cand.rawThetaRadians);
    cand.rawCorners[2].x = -cand.rawCorners[0].x;
    cand.rawCorners[2].y = -cand.rawCorners[0].y;
    cand.rawCorners[3].x = -cand.rawCorners[1].x;
    cand.rawCorners[3].y = -cand.rawCorners[1].y;

    // if the cube is marked, use this to determine its orientation. cornerA becomes corners[0], cornerB
    // becomes corners[1], and the rest follow. if there is no marker, leave corners as they are.
    int cornerA = 0;
    int cornerB = 1;
    if (cand.hasMarker) {
        // determine which two cube corners the marker is closest to
        vector<pair<float, int> > distances; // (distance, index) tuple vector
        for (int i = 0; i < 4; i++) {
            float distance = cand.marker.squareDistance(cand.rawCorners[i]);
            distances.push_back(pair<float, int>(distance, i));
        }

        // sort by distance value, then extract matching indices
        sort(distances.begin(), distances.end());
        cornerA = min(distances[0].second, distances[1].second);
        cornerB = max(distances[0].second, distances[1].second);
        if (cornerA == 0 && cornerB == 3) {
            cornerA = 3;
            cornerB = 0;
        }
    }

    // adjust the cube angle appropriately (applying a mod-90 angle hysteresis filter for marker noise)
    float thetaCandidate = fmod(cand.rawTheta - 90 * cornerA + 360, 360);
    cand.theta = thetaUsingMarkerHysteresis(thetaCandidate);
    cand.thetaRadians = cand.theta * pi / 180;

    // relative corner coordinates, determined by cycling indices of raw corners
    for (int i = 0; i < 4; i++) {
        cand.corners[i] = cand.rawCorners[(i + cornerA) % 4];
    }
}

// since theta angles are cyclic, with 0 == 360, find the degrees betweewn two angles
float Cube::thetaDistance(float theta1, float theta2) {
    // restrict angles to 0 <= theta < 360
    theta1 = fmod(theta1, 360);
    if (theta1 < 0) {
        theta1 += 360;
    }
    theta2 = fmod(theta2, 360);
    if (theta2 < 0) {
        theta2 += 360;
    }

    float distance = abs(theta1 - theta2);
    return distance < 180 ? distance : 360 - distance;
}

// since marker noise is common, track recent (marker-adjusted) theta values to guard against noise
// and return the theta value that takes this history into account. Assume that sudden angle changes
// of 90, 180, and 270 degrees are unlikely
float Cube::thetaUsingMarkerHysteresis(float thetaCandidate) {
    float selectedTheta;
    
    // enforce 0 <= thetaCandidate < 360
    thetaCandidate = fmod(thetaCandidate, 360);
    if (thetaCandidate < 0) {
        thetaCandidate += 360;
    }

    // if the candidate theta shows no risk of a marker misdetection, accept it
    if (thetaDistance(thetaCandidate, theta) < 70) {
        selectedTheta = thetaCandidate;

    // else, only accept the candidate theta if it matches recent history better than the current theta
    } else {
        int acceptanceRating = 0;
        for (int i = 0; i < recentThetaCandidatesLength; i++) {
            if (recentThetaCandidates[i] >= 0) {
                if (thetaDistance(thetaCandidate, recentThetaCandidates[i]) < thetaDistance(theta, recentThetaCandidates[i])) {
                    acceptanceRating++;
                } else {
                    acceptanceRating--;
                }
            }
        }

        // if the candidate is acceptable, use it
        if (acceptanceRating > 0) {
            selectedTheta = thetaCandidate;

        // else, pick its rotation by 90 that best agrees with the current theta
        } else {
            if (thetaCandidate < theta) {
                int numRotations = ((int) (theta - thetaCandidate) + 45) / 90;
                selectedTheta = fmod(thetaCandidate + 90 * numRotations, 360);
            } else {
                int numRotations = ((int) (thetaCandidate - theta) + 45) / 90;
                selectedTheta = fmod(thetaCandidate - 90 * numRotations + 360, 360);
            }
        }
    }

    // update candidate theta history with uncorrected value
    for (int i = 0; i < recentThetaCandidatesLength - 1; i++) {
        recentThetaCandidates[i + 1] = recentThetaCandidates[i];
    }
    recentThetaCandidates[0] = thetaCandidate;

    return selectedTheta;
}

bool Cube::candidateUpdatesAreSignificant() {
    // locally use a shorthand alias for the candidate updates object
    CubeUpdatesBuffer &cand = candidateUpdates;

    if (hasMarker != cand.hasMarker || blobId != cand.blob->id) {
        return true;
    } else if (center.distance(cand.center) > 0.5 * pinSize) {
        return true;
    } else if (abs(theta - cand.theta) > 10) {
        return true;
    }

    return false;
}

void Cube::update() {
    if (!candidateUpdates.blob) {
        return;
    }

    // to filter out image noise, only update cube values when the blob changes substantially.
    // therefore, calculate updates into a candidate buffer and only propagate them if their
    // difference compared to current values passes a hysteresis threshold
    calculateCandidateUpdates();
    if (!candidateUpdatesAreSignificant()) {
        return;
    }

    // locally use a shorthand alias for the candidate updates object
    CubeUpdatesBuffer &cand = candidateUpdates;

    // hopefully this will return again, but memory management redesign is
    // needed to prevent early deallocation. currently, accessing this later is
    // illegal. using the new/delete paradigm and storing a local copy of the
    // candidate blob should fix this, but I believe the new/delete paradigm
    // needs to be propagated throughout the code base; just injecting it here
    // leads to other bugs.
    //
    // blob = cand.blob;

    blobId = cand.blob->id;
    hasMarker = cand.hasMarker;
    normalizationVector = cand.normalizationVector;
    width = cand.width;
    height = cand.height;
    center = cand.center;
    marker = cand.marker;
    theta = cand.theta;
    thetaRadians = cand.thetaRadians;
    for (int i = 0; i < 4; i++) {
        corners[i] = cand.corners[i];
    }

    // derive additional values for convenience

    // absolute corner coordinates
    absCorners[0] = center + corners[0];
    absCorners[1] = center + corners[1];
    absCorners[2] = center + corners[2];
    absCorners[3] = center + corners[3];

    // cube boundary descriptors
    minX = absCorners[0].x;
    maxX = absCorners[2].x;
    minY = absCorners[1].y;
    maxY = absCorners[3].y;
}

Blob * Cube::getCandidateBlob() {
    return candidateUpdates.blob;
}

// transform a point's coordinates from absolute coordinates into this cube's reference frame.
// if lengthScale is passed in, it is used to define the scale of the coordinate system.
void Cube::transformPointToCubeReferenceFrame(ofPoint *src, ofPoint *dst, float lengthScale) {
    dst->set(*src - (center * lengthScale));
    // rotation is by theta, not -theta, because +y is down
    dst->rotate(theta, ofPoint(0, 0, 1));
}

// transform a point's coordinates from this cube's reference frame into absolute coordinates.
// if lengthScale is passed in, it is used to define the scale of the coordinate system.
void Cube::transformPointFromCubeReferenceFrame(ofPoint *src, ofPoint *dst, float lengthScale) {
    dst->set(*src);
    // rotation is by -theta, not theta, because +y is down
    dst->rotate(-theta, ofPoint(0, 0, 1));
    *dst += (center * lengthScale);
}

void Cube::transformCubeToCubeReferenceFrame(Cube *cube) {
    transformPointToCubeReferenceFrame(&cube->center, &cube->center);

    // absolute corner coordinates
    cube->absCorners[0] = cube->center + cube->corners[0];
    cube->absCorners[1] = cube->center + cube->corners[1];
    cube->absCorners[2] = cube->center + cube->corners[2];
    cube->absCorners[3] = cube->center + cube->corners[3];

    // cube boundary descriptors
    minX = absCorners[0].x;
    maxX = absCorners[2].x;
    minY = absCorners[1].y;
    maxY = absCorners[3].y;

    cube->theta -= theta;
    cube->thetaRadians -= thetaRadians;

    for (int i = 0; i < recentThetaCandidatesLength; i++) {
        cube->recentThetaCandidates[i] -= theta;
    }
}

void Cube::addSubCube(Cube *subCube) {
    if (subCubesCount >= maxSubCubesCount) {
        return;
    }

    Cube *subCubeCopy = new Cube(subCube);
    transformCubeToCubeReferenceFrame(subCubeCopy);
    subCubeCopy->isSubCube = true;
    subCubes[subCubesCount] = subCubeCopy;
    subCubesCount++;
}
//
//  CameraCalibration.cpp
//  Relief2
//
//  Created by Daniel Windham on 4/13/15.
//
//

#include "CameraCalibration.h"


// All calibrations were calculated on inFORM 1. They may need recalibration.
// All functions assume x-y coordinates normalized to [0, 1] and height [0, 255].

// note: for now, assume conversion is always from cube height (140) to ground.
void reprojectColorCameraCoordinateFromHeight(ofPoint &rawXYZPoint, ofPoint &output) {
    output.x = 0.9557 * rawXYZPoint.x + 0.0204;
    output.y = 0.9538 * rawXYZPoint.y + 0.0389;
    output.z = 0;
}

float reprojectProjectorXCoordinateFromHeight(float x, int height) {
    return (1 - 0.0002000 * height) * x + 0.00012 * height;
}

float reprojectProjectorYCoordinateFromHeight(float y, int height) {
    float reprojected = (1 - 0.0001588 * height) * y - 0.00002 * height;
    reprojected -= 0.0333; // short-term calibration hack: note that this breaks the intended structure of the math
    return reprojected;
}

// reproject 2D pixels for projection onto a height map
//
// NOTE: this function approximates all heights as being either 255, 140, or 0.
// An efficient correct implementation of this functionality would be nice to have.
void reprojectProjectorPixelsForHeightMap(ofPixels &pixels2D, ofPixels &heightMap, ofPixels &reprojectedPixels) {
    // get input dimensions
    int width2D = pixels2D.getWidth();
    int height2D = pixels2D.getHeight();
    int numChannels = pixels2D.getNumChannels();

    // for now, reject complicating input images
    if (numChannels < 3) {
        cout << "Error: reprojectProjectorPixelsForHeightMap expects color image inputs" << endl;
        return;
    } else if (heightMap.size() != pixels2D.size()) {
        cout << "Error: reprojectProjectorPixelsForHeightMap inputs must have matching dimension" << endl;
        return;
    }

    // calculate dimensions of reprojected graphics
    ofPoint reprojectedGraphicsBoundaries[2]; // min and max coordinates for reprojected pixels
    reprojectedGraphicsBoundaries[0].x = (int) (RELIEF_PROJECTOR_SIZE_X * projectorMinXCoordinateBoundaryAtCubeHeight);
    reprojectedGraphicsBoundaries[0].y = (int) (RELIEF_PROJECTOR_SIZE_Y * projectorMinYCoordinateBoundaryAtCubeHeight);
    reprojectedGraphicsBoundaries[1].x = (int) (RELIEF_PROJECTOR_SIZE_X * projectorMaxXCoordinateBoundaryAtCubeHeight);
    reprojectedGraphicsBoundaries[1].y = (int) (RELIEF_PROJECTOR_SIZE_Y * projectorMaxYCoordinateBoundaryAtCubeHeight);
    ofPoint reprojectedGraphicsDimensions(reprojectedGraphicsBoundaries[1].x - reprojectedGraphicsBoundaries[0].x,
        reprojectedGraphicsBoundaries[1].y - reprojectedGraphicsBoundaries[0].y);

    // ensure reprojectedPixels object is allocated and has appropriate dimensions
    if (!reprojectedPixels.isAllocated()) {
        reprojectedPixels.allocate(reprojectedGraphicsDimensions.x, reprojectedGraphicsDimensions.y, numChannels);
    } else {
        reprojectedPixels.setNumChannels(numChannels);
        reprojectedPixels.resize(reprojectedGraphicsDimensions.x, reprojectedGraphicsDimensions.y);
    }

    // xy coordinates and pixel array indices for reprojectedPixels object and for various height
    // reprojection mappings of 2D pixel objects
    ofPoint xyRP;
    int xMinRP = reprojectedGraphicsBoundaries[0].x;
    int yMinRP = reprojectedGraphicsBoundaries[0].y;
    int iRP = 0;
    ofPoint xyAt255, xyAt140, xyAt0;
    int indexAt255, indexAt140, indexAt0;
    bool indexInRangeAt255, indexInRangeAt140, indexInRangeAt0;
    
    // reproject 2D pixels for projection onto the height map
    reprojectedPixels.setColor(0);
    for (xyRP.y = yMinRP; xyRP.y < reprojectedGraphicsBoundaries[1].y; xyRP.y += 1) {
        // reproject y coordinate
        xyAt255.y = (int) (height2D * reprojectProjectorYCoordinateFromHeight(1.0 * xyRP.y / RELIEF_PROJECTOR_SIZE_Y, 255));
        xyAt140.y = (int) (height2D * reprojectProjectorYCoordinateFromHeight(1.0 * xyRP.y / RELIEF_PROJECTOR_SIZE_Y, 140));
        xyAt0.y = (int) (height2D * xyRP.y / RELIEF_PROJECTOR_SIZE_Y);

        for (xyRP.x = xMinRP; xyRP.x < reprojectedGraphicsBoundaries[1].x; xyRP.x += 1, iRP += numChannels) {
            // reproject x coordinate
            xyAt255.x = (int) (width2D * reprojectProjectorXCoordinateFromHeight(1.0 * xyRP.x / RELIEF_PROJECTOR_SIZE_X, 255));
            xyAt140.x = (int) (width2D * reprojectProjectorXCoordinateFromHeight(1.0 * xyRP.x / RELIEF_PROJECTOR_SIZE_X, 140));
            xyAt0.x = (int) (width2D * xyRP.x / RELIEF_PROJECTOR_SIZE_X);

            // update 2D pixels indices
            indexAt255 = (xyAt255.x + xyAt255.y * width2D) * numChannels;
            indexAt140 = (xyAt140.x + xyAt140.y * width2D) * numChannels;
            indexAt0 = (xyAt0.x + xyAt0.y * width2D) * numChannels;
            indexInRangeAt255 = 0 <= xyAt255.x && xyAt255.x < width2D && 0 <= xyAt255.y && xyAt255.y < height2D;
            indexInRangeAt140 = 0 <= xyAt140.x && xyAt140.x < width2D && 0 <= xyAt140.y && xyAt140.y < height2D;
            indexInRangeAt0 = 0 <= xyAt0.x && xyAt0.x < width2D && 0 <= xyAt0.y && xyAt0.y < height2D;

            // since the height map pixels are grayscale regardless of image type, the first channel
            // is sufficient to gauge pin height.
            //
            // NOTE: this approximates all heights as being either 255, 140, or 0
            if (indexInRangeAt255 && heightMap[indexAt255] > 200) {
                reprojectedPixels[iRP + 0] = pixels2D[indexAt255 + 0];
                reprojectedPixels[iRP + 1] = pixels2D[indexAt255 + 1];
                reprojectedPixels[iRP + 2] = pixels2D[indexAt255 + 2];
            } else if (indexInRangeAt140) {// && heightMap[indexAt140] > 100) {    // short-term calibration hack
                reprojectedPixels[iRP + 0] = pixels2D[indexAt140 + 0];
                reprojectedPixels[iRP + 1] = pixels2D[indexAt140 + 1];
                reprojectedPixels[iRP + 2] = pixels2D[indexAt140 + 2];
            } else if (indexInRangeAt0) {
                reprojectedPixels[iRP + 0] = pixels2D[indexAt0 + 0];
                reprojectedPixels[iRP + 1] = pixels2D[indexAt0 + 1];
                reprojectedPixels[iRP + 2] = pixels2D[indexAt0 + 2];
            }
        }
    }
}
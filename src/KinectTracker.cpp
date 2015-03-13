//
//  KinectTracker.cpp
//  Relief2
//
//  Created by Sean Follmer on 3/24/13.
//
//

#include "KinectTracker.h"

void KinectTracker::setup(){
    ofSetLogLevel(OF_LOG_VERBOSE);
	// enable depth->video image calibration
	kinect.setRegistration(true);
    
	kinect.init();
	kinect.open();		// opens first available kinect
    
    src[0] = ofPoint(6,4);
    src[1] = ofPoint(188,6);
    src[2] = ofPoint(190, 189);
    src[3] = ofPoint(1,187);
    dst[0] = ofPoint(0, 0);
    dst[1] = ofPoint(190,0);
    dst[2] = ofPoint(190,190);
    dst[3] = ofPoint(0,190);
    
    kinectView.allocate(kinect.width, kinect.height);

    colorImg.allocate(kinect.width, kinect.height);
	depthImg.allocate(kinect.width, kinect.height);
    depthImgBG.allocate(kinect.width, kinect.height);
    depthImgBGPlusSurface.allocate(kinect.width, kinect.height);
    depthImgFiltered.allocate(kinect.width, kinect.height);

    //colorImg.setROI(234, 157, 190, 190);
    scaledColorImg.allocate(190,190);

    hsvImage.allocate(190, 190);
    hue.allocate(190, 190);
    sat.allocate(190, 190);
    bri.allocate(190, 190);
    filtered.allocate(190, 190);

	hueThreshNear.allocate(190, 190);
	hueThreshFar.allocate(190, 190);
	satThresh.allocate(190, 190);
	hueThresh.allocate(190, 190);
    
    pinHeightMapImage.allocate(pinHeightMapWidth, pinHeightMapHeight);

    finger_contourFinder.bTrackBlobs = true;
    finger_contourFinder.bTrackFingers = true;
    ball_contourFinder.bTrackBlobs = true;
    ball_contourFinder.bTrackFingers = false;

    calib.setup(kinect.width, kinect.height, &finger_tracker);
    calib.setup(190, 190, &ball_tracker);
    verdana.loadFont("frabk.ttf", 8, true, true);
    
    loadDepthBackground();
    
    
    // allocate threshold images
    grayThreshNear.allocate(kinect.width, kinect.height);
	grayThreshFar.allocate(kinect.width, kinect.height);
	
	nearThreshold = 255;
	farThreshold = 233;
	bThreshWithOpenCV = true;
    
    depthImageAlpha.allocate(640, 480, OF_IMAGE_COLOR_ALPHA);
    colorImageAlpha.allocate(640, 480, OF_IMAGE_COLOR_ALPHA);
    detectedObjectsImageAlpha.allocate(640, 480, OF_IMAGE_COLOR_ALPHA);
}

void KinectTracker::exit() {
    kinect.setCameraTiltAngle(0); // zero the tilt on exit
	kinect.close();
    
}

void KinectTracker::update(){
    kinect.update();
	
	// there is a new frame and we are connected
	if(kinect.isFrameNew()) {
        
        colorImg.setFromPixels(kinect.getPixels(), kinect.width, kinect.height);
        colorImg.mirror(0,1);
        colorImg.flagImageChanged();
        
		depthImg.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
        depthImg.mirror(1,0);
        depthImg.flagImageChanged();

        // ------------------- begin ROI
        vector<ofPoint> redBalls;
//        findBalls(172, 5, 200, redBalls); // strict red threshold?
        findBalls(172, 205, 100, redBalls); // loose threshold

        size = redBalls.size();
        /*
        if (size == 0) {
            detectedObjectsImageAlpha.getPixelsRef().setColor(ofColor::black);
        } else if (size == 1) {
            detectedObjectsImageAlpha.getPixelsRef().setColor(ofColor::brown);
        } else if (size == 2) {
            detectedObjectsImageAlpha.getPixelsRef().setColor(ofColor::red);
        } else if (size == 3) {
            detectedObjectsImageAlpha.getPixelsRef().setColor(ofColor::orange);
        } else if (size == 4) {
            detectedObjectsImageAlpha.getPixelsRef().setColor(ofColor::yellow);
        } else {
            detectedObjectsImageAlpha.getPixelsRef().setColor(ofColor::green);
        }
        */
        detectedObjectsImageAlpha.getPixelsRef().setFromPixels(colorImg.getPixels(), colorImg.getWidth(), colorImg.getHeight(), colorImg.getPixelsRef().getNumChannels());

        pointsText.str("");
        int width = detectedObjectsImageAlpha.getPixelsRef().getWidth();
        int height = detectedObjectsImageAlpha.getPixelsRef().getHeight();
        for(vector<ofPoint>::iterator itr = redBalls.begin();itr < redBalls.end();itr++) {
            pointsText << '(' << itr->x << ',' << itr->y << ")  ";
            int x = itr->x * width;
            int y = itr->y * height;
            for (int dx = 0; dx < width / 20; dx++) {
                for (int dy = 0; dy < height / 20; dy++) {
                    detectedObjectsImageAlpha.getPixelsRef().setColor(x+dx, y+dy, ofColor::green);
                    /*
                    int i = (width * (y + dy) + x + dx);
                    detectedObjectsImageAlpha.getPixelsRef()[i] = 255;
                    detectedObjectsImageAlpha.getPixelsRef()[i+1] = 0;
                    detectedObjectsImageAlpha.getPixelsRef()[i+2] = 0;
                    detectedObjectsImageAlpha.getPixelsRef()[i+3] = 255;
                    */
                }
            }
        }
        // ------------------- end ROI
        
        if(bThreshWithOpenCV) {
			//grayThreshNear = depthImg;
			grayThreshFar = depthImg;
			//grayThreshNear.threshold(nearThreshold, true);
			grayThreshFar.threshold(farThreshold);
			cvAnd(grayThreshFar.getCvImage(), depthImg.getCvImage(), depthImg.getCvImage(), NULL);
            depthImg.mirror(1, 1);
            depthImg.operator-=(233);
            
            unsigned char * depthPixels = depthImg.getPixels();
            unsigned char * depthAlphaPixels = depthImageAlpha.getPixels();
            
            unsigned char * colorPixels = colorImg.getPixels();
            unsigned char * colorAlphaPixels = colorImageAlpha.getPixels();
            for (int i = 0; i < 640*480; i++) {
                depthPixels[i] *= 10;
                
                int indexRGB = i*3;
                int indexRGBA = i*4;
                
                depthAlphaPixels[indexRGBA] = depthPixels[i];
                depthAlphaPixels[indexRGBA+1] = depthPixels[i];
                depthAlphaPixels[indexRGBA+2] = depthPixels[i];
                depthAlphaPixels[indexRGBA+3] = (depthPixels[i] == 0) ? 0 : 255;
                
                colorAlphaPixels[indexRGBA] = (unsigned char)(colorPixels[indexRGB] * 1.0);
                colorAlphaPixels[indexRGBA+1] = (unsigned char)(colorPixels[indexRGB+1] * 1.0); // 0.7
                colorAlphaPixels[indexRGBA+2] = (unsigned char)(colorPixels[indexRGB+2] * 1.0); // 0.7
                colorAlphaPixels[indexRGBA+3] = (depthPixels[i] == 0) ? 0 : 255;
                
                
            }
            depthImg.flagImageChanged();
            
            depthImageAlpha.update();
            colorImageAlpha.update();
            detectedObjectsImageAlpha.update();
            
		}
    }
}

void KinectTracker::findBalls(int hue_target, int hue_tolerance, int sat_limit, vector<ofPoint>& points){
    scaledColorImg.setFromPixels(colorImg.getPixelsRef()); // was erroring when using .getRoiPixelsRef()
    scaledColorImg.flagImageChanged();

    hsvImage.warpIntoMe(scaledColorImg, dst, dst);
    //hsvImage.setFromPixels(colorImg.getPixelsRef()); // was erroring when using .getRoiPixelsRef()
    hsvImage.convertRgbToHsv();
    hsvImage.convertToGrayscalePlanarImages(hue, sat, bri);
    hsvImage.flagImageChanged();

    hue.erode_3x3();
    hue.dilate_3x3();

    sat.erode_3x3();
    sat.dilate_3x3();

    hueThreshNear = hue;
    hueThreshFar = hue;
    hueThreshNear.threshold(hue_target+hue_tolerance, true);
    hueThreshFar.threshold(hue_target-hue_tolerance);
    
    satThresh = sat;
    satThresh.threshold(sat_limit);

    cvAnd(hueThreshNear.getCvImage(), hueThreshFar.getCvImage(), hueThresh.getCvImage(), NULL);
    cvAnd(hueThresh.getCvImage(), satThresh.getCvImage(), filtered.getCvImage(), NULL);
    filtered.flagImageChanged();
    
    ball_contourFinder.findContours(filtered, 75, (190*190)/2, 20, 20.0, false);
    ball_tracker.track(&ball_contourFinder);
    
    
    
    points.clear();
    for(vector<Blob>::iterator itr = ball_contourFinder.blobs.begin(); itr < ball_contourFinder.blobs.end(); itr++) {
        points.push_back(itr->centroid / 190);
    }
}

void KinectTracker::findFingers(vector<ofPoint> &points) {
    
    int nearThreshold = 255;
	int farThreshold = 200;
    
    unsigned char * pix = depthImg.getPixels();
    unsigned char * filteredPix = depthImgFiltered.getPixels();
    unsigned char * bgPix = depthImgBG.getPixels();
    
    for(int i = 0; i < depthImg.getWidth() * depthImg.getHeight(); i++){
        filteredPix[i] = (pix[i]>(bgPix[i]+1))?pix[i]:0;
        pix[i] = (filteredPix[i] < nearThreshold && filteredPix[i] > farThreshold)?255:0;
        
    }
    depthImgFiltered.flagImageChanged();
    depthImg.flagImageChanged();
    depthImg.erode_3x3();
    depthImg.dilate_3x3();
    depthImg.flagImageChanged();
    
    finger_contourFinder.findContours(depthImg,  (2 * 2) + 1, ((640 * 480) * .4) * (100 * .001), 20, 20.0, false);
    
    finger_tracker.track(&finger_contourFinder);
    
    points.clear();
    
    for(vector<Blob>::iterator itr = finger_contourFinder.fingers.begin(); itr < finger_contourFinder.fingers.end(); itr++){
        ofPoint tempPt = itr->centroid;
        tempPt.z = filteredPix[ int((tempPt.y*kinect.width) + tempPt.x) ] - bgPix[ int((tempPt.y*kinect.width) + tempPt.x) ] -1;
        //cout<<tempPt.x << " " << tempPt.y <<endl;
        
        //Not just any magic numbers! These are magic bean numbers. You put them in the ground and then they grow ; )
        //But serously these numbers convert from kinect space to display space (use them wisely)
        // corners of table are roughly... starting from top left clockwise (236,154), (427,152), (232, 345), (426, 345)
        //I know I suck (sean)
        tempPt.x = ((tempPt.x -232.0)/(427.0-232.0))*900.0;
        tempPt.y = ((tempPt.y - 152.0)/(345-152))*900.0;
        points.push_back(tempPt);
    }
}

void KinectTracker::findFingersAboveSurface(vector<ofPoint> &points) {
    
    int nearThreshold = 255;
	int farThreshold = 200;
    
    
    int roiStartX = 232;
    int roiStartY =  152;
    int roiWidth = 427-232;
    int roiHeight = 345-152;
    
    unsigned char * bgROIPix = depthImgBG.getPixels();
    unsigned char * bgPixAdded = depthImgBGPlusSurface.getPixels();
    
    ofxCvGrayscaleImage tempDepth;
    tempDepth.allocate(roiWidth, roiHeight);
    tempDepth.scaleIntoMe(pinHeightMapImage);
    
    
    unsigned char * tempDepthPix = tempDepth.getPixels();
    
    //cout<<int(tempDepthPix[3])<<endl;
    int counter =0;
    
    for(int i = (232+152*kinect.width); i <  (427+345*kinect.width); i++){
        if(((i%kinect.width)>(roiStartX-1)) && ((i%kinect.width)<(roiStartX+roiWidth)) ){
            bgPixAdded[i] = bgROIPix[i]+ (tempDepthPix[counter])/33; //-1;
            //tempDepthPix[i]=50;
            counter++;
        }
    }
    
    pinHeightMapImage.flagImageChanged();
    
    depthImgBGPlusSurface.flagImageChanged();
    
    
    unsigned char * tempDepthPix2 = pinHeightMapImage.getPixels();
    //cout<<int(tempDepthPix2[3])<<endl;
    
    
    unsigned char * pix = depthImg.getPixels();
    unsigned char * filteredPix = depthImgFiltered.getPixels();
   
    
    
    for(int i = 0; i < depthImg.getWidth() * depthImg.getHeight(); i++){
        filteredPix[i] = (pix[i]>(bgPixAdded[i]+1))?pix[i]:0;
        pix[i] = (filteredPix[i] < nearThreshold && filteredPix[i] > farThreshold)?255:0;
        
    }
    depthImgFiltered.flagImageChanged();
    depthImg.flagImageChanged();
    depthImg.erode_3x3();
    depthImg.dilate_3x3();
    depthImg.flagImageChanged();
    
    
    finger_contourFinder.findContours(depthImg,  (2 * 2) + 1, ((640 * 480) * .4) * (100 * .001), 20, 20.0, false);
    
    finger_tracker.track(&finger_contourFinder);
    
    absFingers.clear();
    points.clear();
    for(vector<Blob>::iterator itr = finger_contourFinder.fingers.begin(); itr < finger_contourFinder.fingers.end(); itr++){
        ofPoint tempPt = itr->centroid;
        int tmpx = tempPt.x;
        int tmpy = tempPt.y;
        //maybe this should be changed from bgPixAdded to bgRoiPix
        tempPt.z = filteredPix[ int((tempPt.y*kinect.width) + tempPt.x) ] - bgPixAdded[ int((tempPt.y*kinect.width) + tempPt.x) ] -1;
        //cout<<tempPt.x << " " << tempPt.y <<endl;
        
        //Not just any magic numbers! These are magic bean numbers. You put them in the ground and then they grow ; )
        //But serously these numbers convert from kinect space to display space (use them wisely)
        // corners of table are roughly... starting from top left clockwise (236,154), (427,152), (232, 345), (426, 345)
        //I know I suck (sean)
        tempPt.x = ((tempPt.x -232.0)/(427.0-232.0))*900.0;
        tempPt.y = ((tempPt.y - 152.0)/(345-152))*900.0;
        points.push_back(tempPt);
        
        ofPoint tempPt2;
        //if there is some error in the finger tracking the problem could be here, that I am modifying tempPt after I pushed it to points vector
        
        tempPt2.x = ((tmpx -232.0)/(427.0-232.0))*900.0;
        tempPt2.y = ((tmpy - 152.0)/(345-152))*900.0;
        tempPt2.z = filteredPix[ int((tmpy*kinect.width) + tmpx) ] - bgROIPix[ int((tmpy*kinect.width) + tmpx) ] -1;
        absFingers.push_back(tempPt2);
    }
}

void KinectTracker::saveDepthImage(){
    
    ofImage tempBG;
    tempBG.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height, OF_IMAGE_GRAYSCALE);
    tempBG.saveImage("background.png");
    
}

void KinectTracker::loadDepthBackground(){
    ofImage tempBG;
    tempBG.loadImage("backgroundGood.png");
    depthImgBG.setFromPixels(tempBG.getPixels(), kinect.width, kinect.height);
    depthImgBGPlusSurface.setFromPixels(tempBG.getPixels(), kinect.width, kinect.height);
}

void KinectTracker::setPinHeightMap(ofPixels & tempPixels){
    pinHeightMapImage.setFromPixels(tempPixels);
    
    pinHeightMapImage.flagImageChanged();
}


//--------------------------------------------------------------
void KinectTracker::draw(int x, int y, int width, int height, int probe_x, int probe_y) {
    
    //ofEnableAlphaBlending();
    ofSetColor(255, 255, 255);
    depthImageAlpha.draw(x,y,width,height);
    //ofDisableAlphaBlending();
    
    
    /*
    filtered.draw(x,y,width,height);
    // draw red circles around balls
    ofSetColor(ofColor::red);
    for(vector<ofPoint>::iterator itr = redBalls.begin();itr < redBalls.end();itr++) {
        ofCircle(x + itr->x / 900.0 * width, y + itr->y / 900.0 * height, 5.0f);
        
        int coord =((int)(itr->y * 190.0 / 900.0) * 190) + (int)(itr->x * 190.0 / 900.0);

        if(0 <= coord && coord < 190 * 190) {
            unsigned char * pix = hue.getPixels();
            pix[coord]= 0;
            hue.flagImageChanged();

            int h = hue.getPixels()[coord];
            int s = sat.getPixels()[coord];
            int v = bri.getPixels()[coord];
            //int f = hue.getPixels()[coord];
            char idStr[1024];
            sprintf(idStr, "id: %f %f %f", itr->x, itr->y, itr->z);
            verdana.drawString(idStr, x + itr->x / 900.0 * width, y + itr->y / 900.0 * height);
        }
    }*/
     

    
    //pinHeightMapImage.draw(x,y,width,height);
    

    /*
    // draw blue circles around fingers
    ofSetColor(ofColor::blue);
    
    for(vector<ofPoint>::iterator itr = fingers.begin();itr < fingers.end();itr++)
        ofCircle(x + itr->x / depthImg.width * width, y + itr->y / depthImg.height * height, 5.0f);
    
    ofSetColor(ofColor::white);

    
    */
    //depthImg.draw(x, y, width, height);
    
    /*
    // pass in a probe x,y coordinate pair to paint that location green for comparison
    kinect.draw(x,y,width,height);
    ofSetColor(ofColor::green);
    ofCircle(probe_x, probe_y, 5.0f);

    int x_c = (probe_x - x) * kinect.width / width;
    int y_c = (probe_y - y) * kinect.height / height;

    if(0 <= x_c && x_c < kinect.width && 0 <= y_c && y_c < kinect.height) {
        char idStr[1024];
        ofVec3f point = kinect.getWorldCoordinateAt(x_c, y_c);
        sprintf(idStr, "%f %f %f", point.x, point.y, point.z);
        verdana.drawString(idStr, probe_x + 10, probe_y);
    }
    */
    
    //ofSetColor(ofColor::white);
   // kinectView.draw(x,y);

}


void KinectTracker::drawColorImage(int x, int y, int width, int height) {
    ofEnableAlphaBlending();
    ofSetColor(255, 255, 255);
    //colorImageAlpha.draw(x,y,width,height);
    colorImg.draw(x,y,width,height);
    ofDisableAlphaBlending();
}


void KinectTracker::drawDetectedObjects(int x, int y, int width, int height) {
    ofEnableAlphaBlending();
    ofSetColor(255, 255, 255);
    detectedObjectsImageAlpha.draw(x,y,width,height);
    ofDisableAlphaBlending();
}
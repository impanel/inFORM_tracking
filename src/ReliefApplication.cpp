#include "ReliefApplication.h"

//--------------------------------------------------------------
void ReliefApplication::setup(){
    if (USE_KINECT) {
        kinectTracker.setup();
    }

    if (SCREEN_IN_USE == 0) {
        //For Acer Screen
        ofSetWindowShape(ACER_SCREEN_RESOLUTION_X+PROJECTOR_RAW_RESOLUTION_X,PROJECTOR_RAW_RESOLUTION_Y);
        projectorOffsetX=ACER_SCREEN_RESOLUTION_X+RELIEF_PROJECTOR_OFFSET_X;
        ofSetWindowPosition(0, 0);
    } else if (SCREEN_IN_USE == 1) {
        //For Sean Screen
        ofSetWindowShape(SEAN_SCREEN_RESOLUTION_X+PROJECTOR_RAW_RESOLUTION_X,PROJECTOR_RAW_RESOLUTION_Y);
        projectorOffsetX=SEAN_SCREEN_RESOLUTION_X+RELIEF_PROJECTOR_OFFSET_X;
        if(CINEMA_DISPLAY){
            ofSetWindowPosition(CINEMA_DISPLAY_OFFSET, 0);
        } else {
            ofSetWindowPosition(0, 0);
        }
    } else {
    //For Daniel Screen
        ofSetWindowShape(DANIEL_SCREEN_RESOLUTION_X+PROJECTOR_RAW_RESOLUTION_X,PROJECTOR_RAW_RESOLUTION_Y);
        projectorOffsetX=DANIEL_SCREEN_RESOLUTION_X+RELIEF_PROJECTOR_OFFSET_X;
        ofSetWindowPosition(0, 0);
    }
    
    ofSetFrameRate(30);
    
	// initialize communication with Relief table
	mIOManager = new ReliefIOManager();
    
    float gain_P = 1.3;
    float gain_I = 0.2;
    int max_I = 60;
    int deadZone = 30;
    int gravityComp = 0;
    int maxSpeed = 220;
    
    mIOManager->sendValueToAllBoards(TERM_ID_GAIN_P, (unsigned char) (gain_P * 25));
    mIOManager->sendValueToAllBoards(TERM_ID_GAIN_I, (unsigned char) (gain_I * 100));
    mIOManager->sendValueToAllBoards(TERM_ID_MAX_I, (unsigned char)max_I);
    mIOManager->sendValueToAllBoards(TERM_ID_DEADZONE, (unsigned char)deadZone);
    mIOManager->sendValueToAllBoards(TERM_ID_GRAVITYCOMP, (unsigned char)gravityComp);
    mIOManager->sendValueToAllBoards(TERM_ID_MAXSPEED, (unsigned char)(maxSpeed/2));
    
    colorInputImage.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);
    depthInputImage.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);
    detectedObjectsImage.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);

    if (DEBUG) {
        debugImage1.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);
        debugImage2.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);
        debugImage3.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);
        debugImage4.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);

        // initialize images to black so unused images aren't distracting
        debugImage1.begin();
        ofBackground(0);
        debugImage1.end();
        debugImage2.begin();
        ofBackground(0);
        debugImage2.end();
        debugImage3.begin();
        ofBackground(0);
        debugImage3.end();
        debugImage4.begin();
        ofBackground(0);
        debugImage4.end();
    }

    pinDisplayImage.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);
    pinHeightMapImageForPins.allocate(RELIEF_SIZE_X, RELIEF_SIZE_Y, GL_RGBA);

    myHybridTokens = new HybridTokens(&kinectTracker);
    myStaticManipulator = new StaticManipulator(&kinectTracker);

    myCurrentRenderedObject = myHybridTokens;
}

//--------------------------------------------------------------
void ReliefApplication::update(){
    if (USE_KINECT) {
        kinectTracker.update();
    }
    
    //app timebase, to send to all animatable objets
	float dt = 1.0f / ofGetFrameRate();

    if (!paused) {
        myCurrentRenderedObject->update(dt);
    }
    
    // render input color image
    colorInputImage.begin();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, 900, 900);
    glOrtho(0.0, 900, 0, 900, -500, 500);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glTranslated(0, 0, -500);
    glPopMatrix();
    
    ofBackground(0);

    kinectTracker.drawColorImage(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
    
    colorInputImage.end();
    
    // render input depth image
    depthInputImage.begin();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, 900, 900);
    glOrtho(0.0, 900, 0, 900, -500, 500);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
    glTranslated(0, 0, -500);
    glPopMatrix();
    
    ofBackground(0);

    kinectTracker.drawDepthImage(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);

    depthInputImage.end();
    
    // render detected objects
    detectedObjectsImage.begin();
    ofBackground(0);
    ofSetColor(255);
    kinectTracker.drawDetectedObjects(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
    detectedObjectsImage.end();

    // debugging images: write to these temporarily to display image data you're Q/A'ing.
    // don't push commits that write to them, however; leave them clean for others
    if (DEBUG) {
        debugImage1.begin();
        ofBackground(0);
        ofSetColor(255);
        // example:
        // kinectTracker.drawDetectedObjects(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
        debugImage1.end();

        debugImage2.begin();
        ofBackground(0);
        ofSetColor(255);
        // example:
        // kinectTracker.drawDetectedObjects(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
        debugImage2.end();

        debugImage3.begin();
        ofBackground(0);
        ofSetColor(255);
        // example:
        // kinectTracker.drawDetectedObjects(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
        debugImage3.end();

        debugImage4.begin();
        ofBackground(0);
        ofSetColor(255);
        // example:
        // kinectTracker.drawDetectedObjects(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
        debugImage4.end();
    }

    // render heightmap
    pinHeightMapImageForPins.begin();
    ofBackground(0);
    ofSetColor(255);
    myCurrentRenderedObject->drawHeightMap();
    pinHeightMapImageForPins.end();

    // render graphics
    pinDisplayImage.begin();
    ofBackground(0);
    ofSetColor(255);
    myCurrentRenderedObject->drawGraphics();
    pinDisplayImage.end();

    sendHeightToRelief();
}

//--------------------------------------------------------------
void ReliefApplication::draw(){
    ofBackground(0,0,0);
    ofSetColor(255);


    // draw image processing images
    ofRect(1, 1, 302, 302);
    colorInputImage.draw(2, 2, 300, 300);

    ofRect(305, 1, 302, 302);
    depthInputImage.draw(306, 2, 300, 300);

    ofRect(609, 1, 302, 302);
    detectedObjectsImage.draw(610, 2, 300, 300);


    // draw output images
    ofRect(913, 1, 302, 302);
    pinHeightMapImageForPins.draw(914, 2, 300, 300);
    
    ofRect(1217, 1, 302, 302);
    pinDisplayImage.draw(1218, 2, 300, 300);


    // draw debug images
    if (DEBUG) {
        ofRect(305, 331, 302, 302);
        debugImage1.draw(306, 332, 300, 300);
        
        ofRect(609, 331, 302, 302);
        debugImage2.draw(610, 332, 300, 300);
        
        ofRect(913, 331, 302, 302);
        debugImage3.draw(914, 332, 300, 300);
        
        ofRect(1217, 331, 302, 302);
        debugImage4.draw(1218, 332, 300, 300);
    }


    // draw general notes
    int menuHeight = 350;
    int menuLeftCoordinate = 21;
    ofDrawBitmapString("Use red cubes with yellow markers for these applications. The marker specifies the", menuLeftCoordinate, menuHeight); menuHeight += 20;
    ofDrawBitmapString("cube's orientation. Only use sideways-pointing cube orientations - these apps don't", menuLeftCoordinate, menuHeight); menuHeight += 20;
    ofDrawBitmapString("understand the cubes' up (center marker) and down (no marker) orientations.", menuLeftCoordinate, menuHeight); menuHeight += 20;

    // draw general instructions
    menuHeight += 30;
    ofDrawBitmapString("General:", menuLeftCoordinate, menuHeight); menuHeight += 30;
    ofDrawBitmapString((string) "   ' ' : " + (paused ? "play application" : "pause application"), menuLeftCoordinate, menuHeight); menuHeight += 20;
    ofDrawBitmapString((string) "   'z' : turn pins " + (drawPins ? "off" : "on"), menuLeftCoordinate, menuHeight); menuHeight += 20;
    ofDrawBitmapString((string) "   'x' : turn graphics " + (paintGraphics ? "off" : "on"), menuLeftCoordinate, menuHeight); menuHeight += 20;

    // draw application selection instructions
    if (myCurrentRenderedObject == myHybridTokens) {
        menuHeight += 30;
        ofDrawBitmapString("Select Application:", menuLeftCoordinate, menuHeight); menuHeight += 30;
        ofDrawBitmapString((string) (myHybridTokens->mode == BOOLEAN_SWORDS ? " * " : "   ") +
                "'q' : boolean swords - intersecting swords do fun things", menuLeftCoordinate, menuHeight); menuHeight += 20;
        ofDrawBitmapString((string) (myHybridTokens->mode == FLEXIBLE_SWORDS ? " * " : "   ") +
                "'w' : flexible swords - use two cubes to control a bezier curve", menuLeftCoordinate, menuHeight); menuHeight += 20;
        ofDrawBitmapString((string) (myHybridTokens->mode == PHYSICS_SWORDS ? " * " : "   ") +
                "'e' : physics swords - stack one sword on the other; unbalanced swords will tip over", menuLeftCoordinate, menuHeight); menuHeight += 20;
        ofDrawBitmapString((string) (myHybridTokens->mode == DYNAMICALLY_CONSTRAINED_SWORDS ? " * " : "   ") +
                "'r' : dynamically constrained swords - swords cannot share the same space", menuLeftCoordinate, menuHeight); menuHeight += 20;
        ofDrawBitmapString((string) (myHybridTokens->mode == VERTICAL_DEFORMATION_SWORDS ? " * " : "   ") +
                "'t' : vertical deformation swords - the top sword is rubbery and will droop", menuLeftCoordinate, menuHeight); menuHeight += 20;

        // draw application-specific instructions
        if (myHybridTokens->mode == BOOLEAN_SWORDS) {
            menuHeight += 30;
            ofDrawBitmapString("Select Sub-Application:", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString((string) (myHybridTokens->booleanSwordsSchema == UNION ? " * " : "   ") +
                    "'a' : union", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString((string) (myHybridTokens->booleanSwordsSchema == INTERSECTION ? " * " : "   ") +
                    "'s' : intersectiion", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString((string) (myHybridTokens->booleanSwordsSchema == SUM ? " * " : "   ") +
                    "'d' : sum", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString((string) (myHybridTokens->booleanSwordsSchema == XOR ? " * " : "   ") +
                    "'f' : xor", menuLeftCoordinate, menuHeight); menuHeight += 20;
        } else if (myHybridTokens->mode == FLEXIBLE_SWORDS) {
            menuHeight += 30;
            ofDrawBitmapString("Select Sub-Application:", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString(" '-' : decrease extension", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString(" '+' : increase extension", menuLeftCoordinate, menuHeight); menuHeight += 20;
        }

        // when drawing asci diagrams, this spacing will make vertical character distances
        // match horizontal character distances for nicely dimensioned drawings
        int asciArtLineHeight = 8;

        // draw application-specific notes
        if (myHybridTokens->mode == PHYSICS_SWORDS) {
            menuHeight += 30;
            ofDrawBitmapString("Notes:", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString("  - the cube on the right is always the top sword", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString("  - stacking swords in this        - stacking swords in this", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("    orientation works well:          orientation is buggy:", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString("              WORKS                     SORT OF WORKS  ", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("               +-+                           +-+       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               | |                           | |       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("        +-+ +--| |--+                     +-------+ +-+", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("        | | |  | |  |                     |       | | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("        +-+ +--| |--+                     +-------+ +-+", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               | |                           | |       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               | |                           | |       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               | |                           | |       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               +-+                           +-+       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                                                       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               +-+                           +-+       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               | |                           | |       ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("               +-+                           +-+       ", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString("    this is because in the second case, graphics get accidentally", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("    projected onto the cube and hinder accurate object detection", menuLeftCoordinate, menuHeight); menuHeight += 20;

        } else if (myHybridTokens->mode == DYNAMICALLY_CONSTRAINED_SWORDS) {
            menuHeight += 30;
            ofDrawBitmapString("Notes:", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString("  - try this starting arrangement:", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("                   +-+", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("    +-+ +-------+  | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("    | | |       |  | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("    +-+ +-------+  | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   +-+", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                      ", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   +-+", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   | |", menuLeftCoordinate, menuHeight); menuHeight += asciArtLineHeight;
            ofDrawBitmapString("                   +-+", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("  - the cube on the left is always aligned to the x axis", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("  - the cube on the right is always aligned to the y axis", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("  - the cube on the right cannot slide left if this would cause sword collision", menuLeftCoordinate, menuHeight); menuHeight += 20;
            ofDrawBitmapString("  - this application does NOT handle illegal scenarios intelligently", menuLeftCoordinate, menuHeight); menuHeight += 20;

        } else if (myHybridTokens->mode == VERTICAL_DEFORMATION_SWORDS) {
            menuHeight += 30;
            ofDrawBitmapString("Notes:", menuLeftCoordinate, menuHeight); menuHeight += 30;
            ofDrawBitmapString("  - the cube on the right is always the top sword", menuLeftCoordinate, menuHeight); menuHeight += 20;
        }
    }


    //Draw Graphics onto projector
    if (paintGraphics) {
        pinDisplayImage.draw(projectorOffsetX, RELIEF_PROJECTOR_OFFSET_Y, RELIEF_PROJECTOR_SCALED_SIZE_X, RELIEF_PROJECTOR_SCALED_SIZE_Y);
    }
}

//--------------------------------------------------------------
void ReliefApplication::keyPressed(int key){

    // keys reserved for rendered objects' use
    if(key == 'a') myCurrentRenderedObject->keyPressed('a');
    if(key == 's') myCurrentRenderedObject->keyPressed('s');
    if(key == 'd') myCurrentRenderedObject->keyPressed('d');
    if(key == 'f') myCurrentRenderedObject->keyPressed('f');
    if(key == 'm') myCurrentRenderedObject->keyPressed('m');
    if(key == '-') myCurrentRenderedObject->keyPressed('-');
    if(key == '=') myCurrentRenderedObject->keyPressed('=');
    if(key == '+') myCurrentRenderedObject->keyPressed('+');
    if(key == KEY_UP) myCurrentRenderedObject->keyPressed(KEY_UP);
    if(key == KEY_DOWN) myCurrentRenderedObject->keyPressed(KEY_DOWN);
    if(key == KEY_LEFT) myCurrentRenderedObject->keyPressed(KEY_LEFT);
    if(key == KEY_RIGHT) myCurrentRenderedObject->keyPressed(KEY_RIGHT);

    if(key == 'q') {
        myHybridTokens->setMode(BOOLEAN_SWORDS);
    }

    if(key == 'w') {
        myHybridTokens->setMode(FLEXIBLE_SWORDS);
    }

    if(key == 'e') {
        myHybridTokens->setMode(PHYSICS_SWORDS);
    }

    if(key == 'r') {
        myHybridTokens->setMode(DYNAMICALLY_CONSTRAINED_SWORDS);
    }

    if(key == 't') {
        myHybridTokens->setMode(VERTICAL_DEFORMATION_SWORDS);
    }

    if(key == ' ') {
        paused = !paused;
    }

    if(key == 'z') {
        drawPins = !drawPins;
    }

    if(key == 'x') {
        paintGraphics = !paintGraphics;
    }

    // other keys
    if(key == 'p') {
        kinectTracker.saveDepthImage();
    }
}

//--------------------------------------------------------------
void ReliefApplication::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ReliefApplication::mouseMoved(int x, int y){
    
}

//--------------------------------------------------------------
void ReliefApplication::mouseDragged(int x, int y, int button){
    
    
}

//--------------------------------------------------------------
void ReliefApplication::mousePressed(int x, int y, int button){
    //test->position.animateTo(ofPoint((x - 2)*3, (y - 2)*3, 50));
    //printf("%f, %f, %f \n",renderableObjects[0].position.getCurrentPosition().x, renderableObjects[0].position.getCurrentPosition().y, renderableObjects[0].position.getCurrentPosition().z);

    //renderableObjects[i].drawGraphics();
}

//--------------------------------------------------------------
void ReliefApplication::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ReliefApplication::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ReliefApplication::gotMessage(ofMessage msg){
    
}

//-----------------------------------------------------------
void ReliefApplication::sendHeightToRelief(){
    
    unsigned char* pixels;
    ofPixels tempPixels;
    pinHeightMapImageForPins.readToPixels(tempPixels);
    //Rotate to align all coordinate systems
    tempPixels.rotate90(1);
    tempPixels.mirror(true, false);
    pixels = tempPixels.getPixels();
    
    for (int j = 0; j < RELIEF_SIZE_Y; j ++) {
        for (int i = 0; i < RELIEF_SIZE_X; i ++) {
            if (drawPins) {
                int y = j;
                int x = RELIEF_SIZE_X - 1 - i;
                int heightMapValue = pixels[((x + (y * RELIEF_SIZE_X)) * 4)];
                mPinHeightToRelief[i][j] = heightMapValue;
            } else {
                mPinHeightToRelief[i][j] = 0;
            }
        }
    }
    
    // send the height map to the hardware interface
	mIOManager->sendPinHeightToRelief(mPinHeightToRelief);
}

//------------------------------------------------------------
void ReliefApplication::exit(){
    if (USE_KINECT) {
        kinectTracker.exit();
    }
    mIOManager->sendValueToAllBoards(TERM_ID_MAXSPEED, (unsigned char) 0);
    ofSleepMillis(1000);
}
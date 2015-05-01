#include "ReliefApplication.h"

//--------------------------------------------------------------
void ReliefApplication::setup(){
    if (USE_KINECT) {
        kinectTracker.setup();
    }

    if(SCREEN_IN_USE == 0){
        //For Acer Screen
        ofSetWindowShape(ACER_SCREEN_RESOLUTION_X+PROJECTOR_RAW_RESOLUTION_X,PROJECTOR_RAW_RESOLUTION_Y);
        projectorOffsetX=ACER_SCREEN_RESOLUTION_X+RELIEF_PROJECTOR_OFFSET_X;
        ofSetWindowPosition(0, 0);
    }
    else if(SCREEN_IN_USE == 1){
        //For Sean Screen
        ofSetWindowShape(SEAN_SCREEN_RESOLUTION_X+PROJECTOR_RAW_RESOLUTION_X,PROJECTOR_RAW_RESOLUTION_Y);
        projectorOffsetX=SEAN_SCREEN_RESOLUTION_X+RELIEF_PROJECTOR_OFFSET_X;
        if(CINEMA_DISPLAY){
            ofSetWindowPosition(CINEMA_DISPLAY_OFFSET, 0);
        }
        else{
            ofSetWindowPosition(0, 0);
            
        }
    }
    else{
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
    }

    pinDisplayImage.allocate(RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X, GL_RGBA);
    pinHeightMapImageForPins.allocate(RELIEF_SIZE_X, RELIEF_SIZE_Y, GL_RGBA);

    myHybridTokens = new HybridTokens(&kinectTracker);
    myStaticManipulator = new StaticManipulator(&kinectTracker);

    myCurrentRenderedObject = myHybridTokens;
    
    bool loadPlane = false;
    
    if (loadPlane) {
        model.loadModel("plane/plane.3ds", 18);
        model.setRotation(0, -5, 1, 0, 0); // plane
        model.setRotation(1, 29, 0, 0, 1); // plane
        model.setPosition(435, 420, -400); // plane
    }
    else {
        model.loadModel("VWBeetle/VWBeetle.3ds", 0.099);
        model.setScale(1,1,1);
        model.setPosition(470, 420, -250);
        model.setRotation(1, 180, 0, 0, 1);
    }
    
    //model.loadModel("plane/plane.3ds", 18); // plant
    
    mHeightMapShader.load("shaders/heightMapShader");
    
    show3DModel = false;
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
    
    //for(int i=0; i < renderableObjects.size(); i++){
    //    renderableObjects[i]->update(dt);
    //}
    
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
    
    glEnable(GL_DEPTH_TEST);
    if(show3DModel) model.draw();
    glDisable(GL_DEPTH_TEST);
    
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

    mHeightMapShader.begin();
    
    glEnable(GL_DEPTH_TEST);
    if(show3DModel) model.draw();
    //glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    mHeightMapShader.end();

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
        // kinectTracker.drawCornerLikelihoods(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
        debugImage1.end();

        debugImage2.begin();
        ofBackground(0);
        ofSetColor(255);
        // example:
        // kinectTracker.drawCornerLikelihoods(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
        debugImage2.end();

        debugImage3.begin();
        ofBackground(0);
        ofSetColor(255);
        // example:
        // kinectTracker.drawCornerLikelihoods(0, 0, RELIEF_PROJECTOR_SIZE_X, RELIEF_PROJECTOR_SIZE_X);
        debugImage3.end();
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

    ofRect(1, 1, 302, 302);
    colorInputImage.draw(2, 2, 300, 300);

    ofRect(305, 1, 302, 302);
    depthInputImage.draw(306, 2, 300, 300);

    ofRect(609, 1, 302, 302);
    detectedObjectsImage.draw(610, 2, 300, 300);

    ofRect(913, 1, 302, 302);
    pinHeightMapImageForPins.draw(914, 2, 300, 300);

    if (DEBUG) {
        ofRect(305, 331, 302, 302);
        debugImage1.draw(306, 332, 300, 300);
        
        ofRect(609, 331, 302, 302);
        debugImage2.draw(610, 332, 300, 300);
        
        ofRect(913, 331, 302, 302);
        debugImage3.draw(914, 332, 300, 300);
    }

    if (myCurrentRenderedObject == myHybridTokens) {
        ofDrawBitmapString("Application:", 1, 350);
        ofDrawBitmapString(" 'b' : boolean swords", 1, 380);
        ofDrawBitmapString(" 'n' : flexible swords", 1, 400);
        ofDrawBitmapString(" 'm' : physics swords", 1, 420);
        ofDrawBitmapString(" 'k' : dynamically constrained swords", 1, 440);

        int subMenuHeight = 440 + 50;
        if (myHybridTokens->mode == BOOLEAN_SWORDS) {
            ofDrawBitmapString("Sub-Application:", 1, subMenuHeight);
            ofDrawBitmapString(" 'a' : union", 1, subMenuHeight + 30);
            ofDrawBitmapString(" 's' : intersect", 1, subMenuHeight + 50);
            ofDrawBitmapString(" 'd' : sum", 1, subMenuHeight + 70);
            ofDrawBitmapString(" 'f' : xor", 1, subMenuHeight + 90);
        } else if (myHybridTokens->mode == FLEXIBLE_SWORDS) {
            ofDrawBitmapString("Sub-Application:", 1, subMenuHeight);
            ofDrawBitmapString(" '-' : decrease extension", 1, subMenuHeight + 30);
            ofDrawBitmapString(" '+' : increase extension", 1, subMenuHeight + 50);
        }
    }


    //Draw Graphics onto projector
    pinDisplayImage.draw(projectorOffsetX, RELIEF_PROJECTOR_OFFSET_Y, RELIEF_PROJECTOR_SCALED_SIZE_X, RELIEF_PROJECTOR_SCALED_SIZE_Y);
    
    
    //kinectTracker.draw(1, 306, 640, 480, 0, 0);
   /* if (kinectTracker.fingers.size()>0) {
        for (int i = 0; i<kinectTracker.fingers.size(); i++){
            cout<< kinectTracker.fingers[i].x << " " << kinectTracker.fingers[i].y << " " << kinectTracker.fingers[i].z << endl;
            //ofCircle(projectorOffsetX+(kinectTracker.fingers[i].x / RELIEF_PROJECTOR_SCALED_SIZE_X), RELIEF_PROJECTOR_OFFSET_Y+ (kinectTracker.fingers[i].y / RELIEF_PROJECTOR_SCALED_SIZE_Y), 10);
        }
    }*/
}

//--------------------------------------------------------------
void ReliefApplication::keyPressed(int key){

    // keys reserved for rendered objects' use
    if(key == 'a') myCurrentRenderedObject->keyPressed('a');
    if(key == 's') myCurrentRenderedObject->keyPressed('s');
    if(key == 'd') myCurrentRenderedObject->keyPressed('d');
    if(key == 'f') myCurrentRenderedObject->keyPressed('f');
    if(key == '-') myCurrentRenderedObject->keyPressed('-');
    if(key == '=') myCurrentRenderedObject->keyPressed('=');
    if(key == '+') myCurrentRenderedObject->keyPressed('+');
    if(key == KEY_UP) myCurrentRenderedObject->keyPressed(KEY_UP);
    if(key == KEY_DOWN) myCurrentRenderedObject->keyPressed(KEY_DOWN);
    if(key == KEY_LEFT) myCurrentRenderedObject->keyPressed(KEY_LEFT);
    if(key == KEY_RIGHT) myCurrentRenderedObject->keyPressed(KEY_RIGHT);

    if(key == 'b') {
        myHybridTokens->setMode(BOOLEAN_SWORDS);
    }

    if(key == 'n') {
        myHybridTokens->setMode(FLEXIBLE_SWORDS);
    }

    if(key == 'm') {
        myHybridTokens->setMode(PHYSICS_SWORDS);
    }

    if(key == 'k') {
        myHybridTokens->setMode(DYNAMICALLY_CONSTRAINED_SWORDS);
    }

    if(key == ' ') {
        paused = !paused;
    }

    // other keys
    if(key == 'p') {
        kinectTracker.saveDepthImage();
    }
    
    if(key == 'r') {
        show3DModel = !show3DModel;
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
            int y = j;
            int x = RELIEF_SIZE_X - 1 - i;
            int heightMapValue = pixels[((x + (y * RELIEF_SIZE_X)) * 4)];
            mPinHeightToRelief[i][j] = heightMapValue;
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
#include "ofApp.h"

int probableGesture;
bool gestureFound = false;
int counter = 0;
float position = 0;
bool gestureDone = false;
float speed = 0;

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofEnableSmoothing();
    ofSetCircleResolution(60);
    
    ofSetWindowTitle("openframeworks gvf visualiser");
    ofSetWindowShape(1024, 768);
    
    ofSetFrameRate(60); // if vertical sync is off, we can go a bit fast... this caps the framerate at 60fps.
    
    drawArea = ofRectangle(ofPoint(0, 0), ofGetWindowWidth(), ofGetWindowHeight());
    isMouseDrawing = false;
    mygvf = new ofxGVF();
    mygvf->loadTemplates("savedGestures.txt");
    if (mygvf->getNumberOfGestureTemplates()>0){
        mygvf->setState(ofxGVF::STATE_FOLLOWING);
        theSample.clear();
        theSample.push_back(0);
        theSample.push_back(0);
        gesture.addObservation(theSample);
    }
    displayParticles = true;
    ofXml xml;
    xml.load("settings_gvf.xml");
    xml.setTo("GVF_OSC");
    oscDestination = xml.getValue("ip");
    oscPort = ofToInt(xml.getValue("port"));
    oscAddress = xml.getValue("address");
    osc.setup(oscDestination, oscPort);
}

//--------------------------------------------------------------
void ofApp::update()
{
    if (isMouseDrawing)
    {
        if (mygvf->getState()==ofxGVF::STATE_FOLLOWING)
        {
            if (gesture.getNumberOfTemplates()>0)
            {
                GVFOutcomes outcomes = mygvf->update(gesture.getLastObservation());
                probableGesture = outcomes.likeliestGesture;
                position = outcomes.alignments[probableGesture];
                if (counter > 10*outcomes.likelihoods.size() && !gestureFound ){
                    gestureFound=true;
                    probableGesture = outcomes.likeliestGesture;
                    
                } else
                    counter++;
                
                int i=0;
                ofVec2f v;
                for (auto d: outcomes.dynamics){
                    for (auto dd:d){
                        if (i==0)
                            v.x=dd;
                        else
                            v.y=dd;
                        i++;
                    }
                }
                speed= v.length();
                gesture.setAutoAdjustRanges(true);
                if (speed>1.1 || position>.985)
                    gestureDone=true;
                if (gestureFound && !gestureDone){
                    ofxOscMessage msg;
                    msg.setAddress(oscAddress);
                    msg.addFloatArg(float(probableGesture)/3.);
                    msg.addFloatArg(position);
                    msg.addFloatArg(speed);
                    osc.sendMessage(msg);
                }
            }
        }
    }
    else{
        gestureFound=false;
        counter = 0;
        gestureDone=false;
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    float templatesScale = 0.5f;
    ofBackgroundGradient(ofColor(2), ofColor(40), OF_GRADIENT_CIRCULAR);
    ofPushMatrix();
    
    string state_string;
    state_string.append("GESTURE VARIATION FOLLOWER (GVF)\n\n'l' to learn a new template\n'c' to clear\n's' to save"
                        "\n"
                        "\nSTATE_LEARINING [");
    if(mygvf->getState() == ofxGVF::STATE_FOLLOWING)
    {
        state_string.append(" ]\nSTATE_FOLLOWING [X]\nSTATE_CLEAR     [ ]");
        if (isMouseDrawing )
        {
            if(displayParticles && !gestureDone)
                displayParticlesOnGesture(gesture);
        }
    }
    else if(mygvf->getState() == ofxGVF::STATE_LEARNING)
    {
        state_string.append("X]\nSTATE_FOLLOWING [ ]\nSTATE_CLEAR     [ ]");
    }
    else
    {
        state_string.append(" ]\nSTATE_FOLLOWING [ ]\nSTATE_CLEAR     [X]");
    }
    
    
    ofPopMatrix();
    ofSetColor(198);
    ofDrawBitmapString(state_string.c_str(), 30, 25);
    ofDrawBitmapString("FPS " + ofToString(ofGetFrameRate(), 0), ofGetWidth() - 200, 25);
    if (gestureFound && !gestureDone){
        ofDrawBitmapString("Gesture: " + ofToString(probableGesture), ofGetWidth() - 200, 100);
        ofDrawBitmapString("Position: " + ofToString(position), ofGetWidth() - 200, 120);
        ofDrawBitmapString("Speed: " + ofToString(speed), ofGetWidth() - 200, 140);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if (key == 'l' || key == 'L')
    {
        if(mygvf->getState() != ofxGVF::STATE_LEARNING && !isMouseDrawing)
        {
            mygvf->setState(ofxGVF::STATE_LEARNING);
        }
    }
    else if(key == 'c' || key == 'C')
    {
        mygvf->clear();
    }
    else if (key == 'f' || key == 'F')
    {
        ofToggleFullscreen();
    }
    else if (key == 's' || key == 'S')
    {
        mygvf->saveTemplates("savedGestures.txt");
    }
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    theSample.clear();
    // if a gesture has already been started, a new point is added to it
    if(isMouseDrawing)
    {
        theSample.push_back(x);
        theSample.push_back(y);
        gesture.addObservation(theSample);
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if (!isMouseDrawing)
    {
        theSample.clear();
        gesture.clear();
    }
    isMouseDrawing = true;
    if (mygvf->getState()==ofxGVF::STATE_FOLLOWING)
        mygvf->restart();
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    if(isMouseDrawing)
    {
        isMouseDrawing = false;
    }
    
    if (mygvf->getState() == ofxGVF::STATE_LEARNING)
    {
        mygvf->addGestureTemplate(gesture);
        mygvf->setState(ofxGVF::STATE_FOLLOWING);
        std::cout << "switch to following" << std::endl;
    }
    else if (mygvf->getState() == ofxGVF::STATE_FOLLOWING)
    {
        mygvf->restart();
    }
    gesture.clear();
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}

//--------------------------------------------------------------
void ofApp::displayParticlesOnGesture(GVFGesture currentGesture)
{
    const std::vector<std::vector<float> > pp = mygvf->getParticlesPositions();
    int ppSize = pp.size();
    float scale = 1;
    if(ppSize > 0)
    {
        vector<float> weights(ppSize,1.0/ppSize);
        float weightAverage = 0.0;
        
        for (int k=0;k<weights.size();k++)
        {
            weightAverage += weights[k]/((float)weights.size());
        }
        
        for(int i = 0; i < ppSize; i++)
        {
            ofPoint initialPoint;
            initialPoint.x = currentGesture.getInitialObservation()[0];
            initialPoint.y = currentGesture.getInitialObservation()[1];
            
            int length = mygvf->getGestureTemplate(pp[i][2]).getTemplate().size();
            int index  = pp[i][0]*mygvf->getGestureTemplate(pp[i][2]).getTemplate().size();
            if (index>=length-1) index=length-1;
            float scale = pp[i][1];
            
            vector<float> initial = currentGesture.getInitialObservation();
            vector<float> obs = mygvf->getGestureTemplate(pp[i][2]).getTemplate()[index];
            
            // each particle position is retrieved
            ofPoint point(obs[0],obs[1]);
            
            // and then scaled and translated in order to be drawn
            float x = point.x * scale + initialPoint.x;
            float y = point.y * scale + initialPoint.y;
            float radius = weights[i]/weightAverage;
            
            ofColor c(50,75,255,50);
            ofSetColor(c);
            ofDrawCircle(x, y, radius * 2.0);
        }
    }
}



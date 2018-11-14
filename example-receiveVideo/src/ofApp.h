#pragma once
#include "ofMain.h"
#include "ofxNDIGrabber3.h"

class ofApp : public ofBaseApp
{
	public:
		ofApp();
		void update();
		void draw();
		void keyPressed(int key);

		ofxNDIGrabber3 _ndiGrabber;
};

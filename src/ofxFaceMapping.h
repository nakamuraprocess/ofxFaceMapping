#pragma once
#include "ofMain.h"
#include "ofxIntelRealSenseFaceTracking.h"

class ofxFaceMapping{
public:
	struct FaceData {
		ofMesh mesh;
		ofVec2f faceRectSize;
		float width;
		float pitch = 0.0;
		float roll = 0.0;
		float yaw = 0.0;
	};

	~ofxFaceMapping();
	void setup(int colorWidth, int colorHeight, int maxFaces, string indexDataPath, string texCoordDataPath);
	void update(ofxIntelRealSenseFaceTracking::personalData* persons);
	void reloadJson();
	void reloadJson(string indexDataPath, string texCoordDataPath);
	void loadTexCoordsJson(string texCoordDataPath, int faceIndex = -1);
	FaceData* getFaceData();

	void drawTest(FaceData* face, ofImage& testPattern);
	void drawWireframe(FaceData* face);
	void captureFace(ofxIntelRealSenseFaceTracking& rsft, int faceIndex, bool bSave, string name = "", bool alphaMask = true);
	void drawCapureFace();

private:
	void saveCapture(ofTexture faceTex, ofxIntelRealSenseFaceTracking& rsft, int faceIndex, string name);
	ofVec2f vec3ToVec2(ofVec3f src) { return ofVec2f(src.x, src.y); }
	ofVec3f vec2ToVec3(ofVec2f src) { return ofVec3f(src.x, src.y, 0); }
	ofVec2f getNearestPoint(ofVec2f p, ofVec2f a, ofVec2f b);
	ofVec2f getSynmetryPoint(ofVec2f p, ofVec2f lineFrom, ofVec2f lineTo, ofVec2f bias);
	ofVec3f getSynmetryPoint(ofVec3f p, ofVec2f lineFrom, ofVec2f lineTo, ofVec2f bias);
	ofVec2f getIntersectionOfLines(ofVec2f p1, ofVec2f p2, ofVec2f p3, ofVec2f p4);

	FaceData* faceData;
	string indexDataPath, texCoordDataPath;
	ofJson indexJson, texCoordJson;

	int maxFaces;
	ofFbo maskFbo, dstFbo;
};
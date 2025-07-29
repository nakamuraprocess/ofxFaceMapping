#include "ofxFaceMapping.h"

ofxFaceMapping::~ofxFaceMapping(){
	delete[] faceData;
}

void ofxFaceMapping::setup(int colorWidth, int colorHeight,  int maxFaces, string indexDataPath, string texCoordDataPath){
	this->indexDataPath = indexDataPath;
	this->texCoordDataPath = texCoordDataPath;

	this->maxFaces = maxFaces;

	faceData = new FaceData[maxFaces];
	reloadJson(indexDataPath, texCoordDataPath);

	maskFbo.allocate(colorWidth, colorHeight, GL_RGBA);
	dstFbo.allocate(colorWidth, colorHeight, GL_RGBA);
}

void ofxFaceMapping::update(ofxIntelRealSenseFaceTracking::personalData* persons){
	for (int i = 0; i < this->maxFaces; ++i) {
		FaceData* face = &faceData[i];

		for (int j = 0; j < _countof(persons[i].landmarkPos); ++j) {
			face->mesh.setVertex(j, persons[i].landmarkPos[j]);
		}

		face->faceRectSize.x = face->width;
		face->faceRectSize.y = (face->mesh.getVertex(27).y - face->mesh.getVertex(26).y) * 12.0;

		ofVec2f p10 = vec3ToVec2(face->mesh.getVertex(10));
		ofVec2f p18 = vec3ToVec2(face->mesh.getVertex(18));

		ofVec2f p14 = vec3ToVec2(face->mesh.getVertex(14));
		ofVec2f p22 = vec3ToVec2(face->mesh.getVertex(22));

		ofVec2f p26 = vec3ToVec2(face->mesh.getVertex(26));
		ofVec2f p53 = vec3ToVec2(face->mesh.getVertex(53));
		ofVec2f p69 = vec3ToVec2(face->mesh.getVertex(69));

		face->width = (p53 - p69).length();

		ofVec3f dist = face->mesh.getVertex(26) - face->mesh.getVertex(61);
		float cosTheta = dist.getNormalized().dot(ofVec3f(1, 0, 0));
		face->roll = -ofRadToDeg(acos(cosTheta)) + 90;

		ofVec2f intersectionPoint = getNearestPoint(p26, p53, p69);
		float lengthP26 = (intersectionPoint - p26).length();
		ofVec2f p26Vec = (intersectionPoint - p26).getNormalized();
		if (p26Vec.y < 0.0) {
			face->pitch = ofMap(lengthP26 / persons[i].faceRect.h, 0.0, 0.25, 0.0, 50.0);
		}
		else {
			face->pitch = ofMap(lengthP26 / persons[i].faceRect.h, 0.0, 0.25, 0.0, -50.0);
		}

		ofVec2f left = vec3ToVec2(face->mesh.getVertex(53) - face->mesh.getVertex(26));
		ofVec2f right = vec3ToVec2(face->mesh.getVertex(69) - face->mesh.getVertex(26));
		float lengthSum = left.length() + right.length();
		if (lengthSum != 0.0) {
			face->yaw = ofMap(left.length() / lengthSum, 0.0, 1.0, -75, 75);
		}
		else {
			face->yaw = 0;
		}

		ofVec2f p78 = getNearestPoint(
			vec3ToVec2(face->mesh.getVertex(53)),
			vec3ToVec2(face->mesh.getVertex(3)), 
			vec3ToVec2(face->mesh.getVertex(8)));

		ofVec2f p90 = getNearestPoint(
			vec3ToVec2(face->mesh.getVertex(69)),
			vec3ToVec2(face->mesh.getVertex(3)),
			vec3ToVec2(face->mesh.getVertex(8)));

		ofVec2f vec = (vec3ToVec2(face->mesh.getVertex(61)) - vec3ToVec2(face->mesh.getVertex(26))).getNormalized() * face->faceRectSize.y * 0.2;
		float vecX1 = 0;
		float vecX2 = 0;
		if (face->pitch < 0) {
			p78.x += ofMap(face->pitch, 0, -50, 0, face->faceRectSize.x * 0.1);
			p90.x += ofMap(face->pitch, 0, -50, 0, -face->faceRectSize.x * 0.1);
		}

		if (face->yaw < 0) {
			ofVec2f v = vec.getNormalized() * ofMap(face->yaw, 0, -75, 0, -face->faceRectSize.y * 0.1);
			p78 += v;
		}else{
			ofVec2f v = vec.getNormalized() * ofMap(face->yaw, 0, 75, 0, -face->faceRectSize.y * 0.1);
			p90 += v;
		}

		face->mesh.setVertex(78, ofVec3f(p78.x, p78.y, -1));
		face->mesh.setVertex(90, ofVec3f(p90.x, p90.y, -1));

		ofPolyline poly;
		poly.addVertex(face->mesh.getVertex(78));
		poly.bezierTo(face->mesh.getVertex(78).x - vec.x + vecX1, face->mesh.getVertex(78).y - vec.y,
			face->mesh.getVertex(90).x - vec.x + vecX2, face->mesh.getVertex(90).y - vec.y,
			face->mesh.getVertex(90).x, face->mesh.getVertex(90).y);

		for (int j = 0; j < 11; j++) {
			float percent = float(j + 1) / 12.0;
			if (face->yaw < 0) {
				percent = pow(percent, ofMap(face->yaw, 0, -75, 1, 2, true));
			}
			else {
				percent = pow(percent, 1.0 / ofMap(face->yaw, 0, 75, 1, 2, true));
			}
			ofVec3f newVertex = poly.getPointAtPercent(percent);
			int index = 79 + j;
			face->mesh.setVertex(index, ofVec3f(newVertex.x, newVertex.y, -1));
		}
	}
}

void ofxFaceMapping::reloadJson(){
	reloadJson(indexDataPath, texCoordDataPath);
}

void ofxFaceMapping::reloadJson(string indexDataPath, string texCoordDataPath){
	this->indexDataPath = indexDataPath;
	this->texCoordDataPath = texCoordDataPath;

	for (int i = 0; i < this->maxFaces; ++i) {
		FaceData* face = &faceData[i];

		face->mesh.clearIndices();
		face->mesh.clearTexCoords();
		face->mesh.getVertices().assign(91, ofVec3f(0, 0, 0));

		ofFile fileIndex(indexDataPath);
		if (fileIndex.exists()) {
			fileIndex >> indexJson;
		}

		ofFile fileTexCoord(texCoordDataPath);
		if (fileTexCoord.exists()) {
			fileTexCoord >> texCoordJson;
		}

		for (int j = 0; j < texCoordJson.size(); j++) {
			face->mesh.addTexCoord(ofVec2f(texCoordJson[ofToString(j)][0], texCoordJson[ofToString(j)][1]));
		}

		for (int j = 0; j < indexJson.size(); j++) {
			face->mesh.addTriangle(indexJson[ofToString(j)][0], indexJson[ofToString(j)][1], indexJson[ofToString(j)][2]);
		}
	}
}

void ofxFaceMapping::loadTexCoordsJson(string texCoordDataPath, int faceIndex){
	if (faceIndex != -1 && faceIndex < this->maxFaces) {
		FaceData* face = &faceData[faceIndex];
		face->mesh.clearTexCoords();

		ofFile fileTexCoord(texCoordDataPath);
		if (fileTexCoord.exists()) {
			fileTexCoord >> texCoordJson;
		}

		for (int i = 0; i < texCoordJson.size(); i++) {
			face->mesh.addTexCoord(ofVec2f(texCoordJson[ofToString(i)][0], texCoordJson[ofToString(i)][1]));
		}
	} else {
		reloadJson(indexDataPath, texCoordDataPath);
	}
}

ofxFaceMapping::FaceData* ofxFaceMapping::getFaceData(){
	return faceData;
}

ofVec2f ofxFaceMapping::getNearestPoint(ofVec2f p, ofVec2f a, ofVec2f b){
	ofVec2f ab, ap;
	ab.x = b.x - a.x;
	ab.y = b.y - a.y;
	ap.x = p.x - a.x;
	ap.y = p.y - a.y;
	ofVec2f nAb = ab.getNormalized();
	float distAx = nAb.dot(ap);
	ofVec2f ret;
	ret.x = a.x + (nAb.x * distAx);
	ret.y = a.y + (nAb.y * distAx);
	return ret;
}

ofVec2f ofxFaceMapping::getSynmetryPoint(ofVec2f p, ofVec2f lineFrom, ofVec2f lineTo, ofVec2f bias){
	ofVec2f nearest = getNearestPoint(p, lineFrom, lineTo);
	ofVec2f synmetryPoint = nearest - (p - nearest) * bias;
	return synmetryPoint;
}

ofVec3f ofxFaceMapping::getSynmetryPoint(ofVec3f p, ofVec2f lineFrom, ofVec2f lineTo, ofVec2f bias){
	ofVec2f pVec2 = vec3ToVec2(p);
	ofVec2f synmetryPoint = getSynmetryPoint(pVec2, lineFrom, lineTo, bias);
	return ofVec3f(synmetryPoint.x, synmetryPoint.y, 0);
}

ofVec2f ofxFaceMapping::getIntersectionOfLines(ofVec2f p1, ofVec2f p2, ofVec2f p3, ofVec2f p4){
	float d = (p2.x - p1.x)*(p4.y - p3.y) - (p2.y - p1.y)*(p4.x - p3.x);
	if (d == 0) return ofVec2f(0, 0);
	float u = ((p3.x - p1.x)*(p4.y - p3.y) - (p3.y - p1.y)*(p4.x - p3.x)) / d;
	float v = ((p3.x - p1.x)*(p2.y - p1.y) - (p3.y - p1.y)*(p2.x - p1.x)) / d;
	ofVec2f intersection;
	intersection.x = p1.x + u * (p2.x - p1.x);
	intersection.y = p1.y + u * (p2.y - p1.y);
	return intersection;
}

void ofxFaceMapping::drawTest(FaceData* face, ofImage& testPattern){
	testPattern.getTexture().bind();
	face->mesh.draw();
	testPattern.getTexture().unbind();
}


void ofxFaceMapping::drawWireframe(FaceData* face){
	ofPushStyle();
	ofSetColor(0, 255, 0);
	face->mesh.drawWireframe();
	//for (int j = 0; j < face->mesh.getNumVertices(); ++j) {
	//	ofDrawBitmapString(ofToString(j), face->mesh.getVertex(j));
	//}
	ofPopStyle();
}

void ofxFaceMapping::drawCapureFace() {
	dstFbo.draw(0, 0);
}

void ofxFaceMapping::captureFace(ofxIntelRealSenseFaceTracking& rsft, int faceIndex, bool bSave, string name, bool alphaMask){
	if (faceIndex < maxFaces) {
		if (alphaMask) {
			ofMesh tmpMesh = faceData[faceIndex].mesh;

			tmpMesh.addTriangle(45, 46, 52);
			tmpMesh.addTriangle(46, 52, 47);
			tmpMesh.addTriangle(52, 47, 51);
			tmpMesh.addTriangle(50, 47, 51);
			tmpMesh.addTriangle(48, 50, 47);
			tmpMesh.addTriangle(49, 48, 50);
			
			maskFbo.begin();
			ofClear(0);
			ofPushStyle();
			ofSetColor(255);
			tmpMesh.drawFaces();
			//faceData[faceIndex].mesh.drawFaces();
			ofPopStyle();
			maskFbo.end();

			ofTexture tex = *rsft.getColor();
			tex.setAlphaMask(maskFbo.getTexture());

			dstFbo.begin();
			ofClear(0);
			tex.draw(0, 0);
			dstFbo.end();

			if (bSave)saveCapture(dstFbo.getTexture(), rsft, faceIndex, name);
		}
		else {
			if (bSave)saveCapture(*rsft.getColor(), rsft, faceIndex, name);
		}
	}
}

void ofxFaceMapping::saveCapture(ofTexture faceTex, ofxIntelRealSenseFaceTracking& rsft, int faceIndex, string name) {
	ofRectangle cropRect = ofRectangle(rsft.getColor()->getWidth(), rsft.getColor()->getHeight(), 0, 0);
	ofVec2f maxPos = ofVec2f::zero();
	for (int i = 0; i < faceData[faceIndex].mesh.getNumVertices(); i++) {
		ofVec3f pos = faceData[faceIndex].mesh.getVertex(i);
		if (pos.x < cropRect.x) cropRect.x = pos.x;
		if (pos.y < cropRect.y) cropRect.y = pos.y;
		if (pos.x > maxPos.x) maxPos.x = pos.x;
		if (pos.y > maxPos.y) maxPos.y = pos.y;
	}
	cropRect.width = maxPos.x - cropRect.x;
	cropRect.height = maxPos.y - cropRect.y;

	ofPixels pix;
	faceTex.readToPixels(pix);
	ofImage img;
	img.setFromPixels(pix);
	img.crop(cropRect.x, cropRect.y, cropRect.width, cropRect.height);
	img.save("capture/" + name + ".png");

	ofJson json;
	for (int i = 0; i < faceData[faceIndex].mesh.getNumVertices(); i++) {
		json[ofToString(i)][0] = int(faceData[faceIndex].mesh.getVertex(i).x - cropRect.x);
		json[ofToString(i)][1] = int(faceData[faceIndex].mesh.getVertex(i).y - cropRect.y);
	}
	ofSaveJson("capture/" + name + ".json", json);
}
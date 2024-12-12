#pragma once

#ifndef MYGLCANVAS_H
#define MYGLCANVAS_H

#include <FL/gl.h>
#include <FL/glut.h>
#include <FL/glu.h>
#include <glm/glm.hpp>
#include <time.h>
#include <iostream>

#include "SceneObject.h"
#include "Camera.h"
#include "Shape.h"
#include "Sphere.h"
#include "Cone.h"
#include "Cube.h"
#include "Cylinder.h"


#define SPLINE_SIZE 100
#define COASTER_SPEED 0.0001

class MyGLCanvas : public Fl_Gl_Window {
public:

	// Length of our spline (i.e how many points do we randomly generate)


	glm::vec3 eyePosition;
	glm::vec3 rotVec;
	glm::vec3 lookatPoint;

	int wireframe;
	int  viewAngle;
	float clipNear;
	float clipFar;
	float camera_near;
	float camera_far;

	int   smooth;
    int   fill;
    int   normal;

	float pixelWidth;
	float pixelHeight;
	float scale;
	OBJ_TYPE objType;

	int isectOnly;
	int maxRecursionDepth;
	Sphere* sphere;
	Cube* cube;
	Cone* cone;
	Cylinder* cylinder;
	Shape* shape;
	int segmentsX, segmentsY;


MyGLCanvas(int x, int y, int w, int h, const char *l = 0);
	~MyGLCanvas();
	void resetScene();
	void setShape(OBJ_TYPE type);
	void drawShape(OBJ_TYPE type);
	void drawScene();
	void drawSphere();
	void drawCube();
	void drawCylinder();
	void drawCone();

private:
	glm::vec3 generateRay(int pixelX, int pixelY);
	glm::vec3 getEyePoint(int pixelX, int pixelY, int screenWidth, int screenHeight);
	glm::vec3 getIsectPointWorldCoord(glm::vec3 eye, glm::vec3 ray, float t);
	double intersect(glm::vec3 eyePointP, glm::vec3 rayV, glm::mat4 transformMatrix);

	void draw();

	void drawAxis();
	void drawGrid();

	int handle(int);
	void resize(int x, int y, int w, int h);
	void updateCamera(int width, int height);

	SceneObject* myObject;
	Camera camera;
	bool castRay;
	bool drag;
	glm::vec3 oldCenter;
	glm::vec3 oldIsectPoint;
	float oldT;

	//// Used for intersection
	glm::vec3 spherePosition;

	int mouseX = 0;
	int mouseY = 0;


};

#endif // !MYGLCANVAS_H
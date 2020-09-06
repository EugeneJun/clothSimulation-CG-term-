#include <time.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <GL/glut.h>
#include "Matrix.h"
#include "BALL.h"
#include "SPRING.h"
#include "SPHEREFORCOLLISIONDETECTION.h"

// 'cameras' stores infomation of 5 cameras.
float cameras[5][9] =
{
	{ 28, 18, 28, 0, 2, 0, 0, 1, 0 },
	{ 28, 18, -28, 0, 2, 0, 0, 1, 0 },
	{ -28, 18, 28, 0, 2, 0, 0, 1, 0 },
	{ -12, 12, 0, 0, 2, 0, 0, 1, 0 },
	{ 0, 100, 0, 0, 0, 0, 1, 0, 0 }
};
int cameraCount = sizeof(cameras) / sizeof(cameras[0]);

int cameraIndex;
vector<Matrix> wld2cam, cam2wld;

unsigned floorTexID;

int frame = 0;
int width, height;

int selectMode;

/*******************************************************************/
int DLSw = -1, PLSw = -1, SLSw = -1, SSw = -1, foward = -1, back = -1;
float camX, camY, camZ; // viewing space 에서의 x축 벡터
time_t currentTime, startTime;

Vector gravity(0.0f, -9.8f, 0.0f);

float mass = 0.01f;
float dampFactor = 0.9f;

const int gridSize = 50;

int numBalls;
BALL * balls1 = NULL;
BALL * balls2 = NULL;
BALL * currentBalls = NULL;
BALL * nextBalls = NULL;

int numSprings;
SPRING * springs = NULL;
float springConstant = 20.0f;
float naturalLength = 0.15f;

SPHERE sphereForCD[8];
int numSpheres = 2;
Vector changedDistance = (0.0f, 0.0f, 0.0f);
/*******************************************************************/

bool shademodel = true;

//------------------------------------------------------------------------------
void munge(int x, float& r, float& g, float& b)
{
	r = (x & 255) / float(255);
	g = ((x >> 8) & 255) / float(255);
	b = ((x >> 16) & 255) / float(255);
}





//------------------------------------------------------------------------------
int unmunge(float r, float g, float b)
{
	return (int(r) + (int(g) << 8) + (int(b) << 16));
}





/*********************************************************************************
* Draw x, y, z axis of current frame on screen.
* x, y, and z are corresponded Red, Green, and Blue, resp.
**********************************************************************************/
void drawAxis(float len)
{
	glDisable(GL_LIGHTING);		// Lighting is not needed for drawing axis.
	glBegin(GL_LINES);			// Start drawing lines.
	glColor3d(1, 0, 0);			// color of x-axis is red.
	glVertex3d(0, 0, 0);
	glVertex3d(len, 0, 0);		// Draw line(x-axis) from (0,0,0) to (len, 0, 0). 
	glColor3d(0, 1, 0);			// color of y-axis is green.
	glVertex3d(0, 0, 0);
	glVertex3d(0, len, 0);		// Draw line(y-axis) from (0,0,0) to (0, len, 0).
	glColor3d(0, 0, 1);			// color of z-axis is  blue.
	glVertex3d(0, 0, 0);
	glVertex3d(0, 0, len);		// Draw line(z-axis) from (0,0,0) - (0, 0, len).
	glEnd();					// End drawing lines.
	glEnable(GL_LIGHTING);
}





void InitCamera() {

	// initialize camera frame transforms.
	for (int i = 0; i < cameraCount; i++)
	{
		float* c = cameras[i];													// 'c' points the coordinate of i-th camera.
		wld2cam.push_back(Matrix());											// Insert {0} matrix to wld2cam vector.
		glPushMatrix();															// Push the current matrix of GL into stack.

		glLoadIdentity();														// Set the GL matrix Identity matrix.
		gluLookAt(c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8]);		// Setting the coordinate of camera.
		glGetFloatv(GL_MODELVIEW_MATRIX, wld2cam[i].matrix());					// Read the world-to-camera matrix computed by gluLookAt.
		cam2wld.push_back(wld2cam[i].inverse());								// Get the camera-to-world matrix.

		glPopMatrix();															// Transfer the matrix that was pushed the stack to GL.
	}
	cameraIndex = 0;
}





void drawCamera()
{
	int i;
	// set viewing transformation.
	glLoadMatrixf(wld2cam[cameraIndex].matrix());

	// draw cameras.
	for (i = 0; i < (int)wld2cam.size(); i++)
	{
		if (i != cameraIndex)
		{
			glPushMatrix();													// Push the current matrix on GL to stack. The matrix is wld2cam[cameraIndex].matrix().
			glMultMatrixf(cam2wld[i].matrix());								// Multiply the matrix to draw i-th camera.
			glPopMatrix();													// Call the matrix on stack. wld2cam[cameraIndex].matrix() in here.
		}
	}
}




/*********************************************************************************
* Draw floor on 3D plane.
**********************************************************************************/
void drawFloor()
{
	if (frame == 0)
	{
		// Initialization part.
		// After making checker-patterned texture, use this repetitively.

		// Insert color into checker[] according to checker pattern.
		const int size = 8;
		unsigned char checker[size*size * 3];
		for (int i = 0; i < size*size; i++)
		{
			if (((i / size) ^ i) & 1)
			{
				checker[3 * i + 0] = 100;
				checker[3 * i + 1] = 100;
				checker[3 * i + 2] = 100;
			}
			else
			{
				checker[3 * i + 0] = 200;
				checker[3 * i + 1] = 200;
				checker[3 * i + 2] = 200;
			}
		}

		// Make texture which is accessible through floorTexID. 
		glGenTextures(1, &floorTexID);
		glBindTexture(GL_TEXTURE_2D, floorTexID);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, checker);
	}

	glDisable(GL_LIGHTING);

	// Set background color.
	if (selectMode == 0)
		glColor3d(0.35, 0.2, 0.1);
	else
	{
		// In backbuffer mode.
		float r, g, b;
		munge(34, r, g, b);
		glColor3f(r, g, b);
	}

	// Draw background rectangle.
	glBegin(GL_POLYGON);
	glVertex3f(-2000, -0.2, -2000);
	glVertex3f(-2000, -0.2, 2000);
	glVertex3f(2000, -0.2, 2000);
	glVertex3f(2000, -0.2, -2000);
	glEnd();


	// Set color of the floor.
	if (selectMode == 0)
	{
		// Assign checker-patterned texture.
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, floorTexID);
	}
	else
	{
		// Assign color on backbuffer mode.
		float r, g, b;
		munge(35, r, g, b);
		glColor3f(r, g, b);
	}

	// Draw the floor. Match the texture's coordinates and the floor's coordinates resp. 
	glBegin(GL_POLYGON);
	glTexCoord2d(0, 0);
	glVertex3d(-12, -0.1, -12);		// Texture's (0,0) is bound to (-12,-0.1,-12).
	glTexCoord2d(0, 1);
	glVertex3d(-12, -0.1, 12);		// Texture's (0,1) is bound to (-12,-0.1,12).
	glTexCoord2d(1, 1);
	glVertex3d(12, -0.1, 12);		// Texture's (1,1) is bound to (12,-0.1,12).
	glTexCoord2d(1, 0);
	glVertex3d(12, -0.1, -12);		// Texture's (1,0) is bound to (12,-0.1,-12).

	glEnd();

	if (selectMode == 0)
	{
		glDisable(GL_TEXTURE_2D);
	}
}



float *return3fv(Vector x) {
	float y[] = { x.x, x.y, x.z };
	return y;
}



void drawCloth() {

	glPushMatrix();
	glEnable(GL_LIGHTING);
	float ambient_front[] = { 0.8f, 0.0f, 1.0f, 0.0f };
	float diffuse_front[] = { 0.8f, 0.0f, 1.0f, 0.0f };
	float ambient_back[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	float diffuse_back[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambient_front);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse_front);
	glMaterialfv(GL_BACK, GL_AMBIENT, ambient_back);
	glMaterialfv(GL_BACK, GL_DIFFUSE, diffuse_back);
	glBegin(GL_TRIANGLES);
	{
		for (int i = 0; i<gridSize - 1; i++)
		{
			for (int j = 0; j<gridSize - 1; j++)
			{
				glColor3f(0.2, 0.5, 0.8);
				glNormal3fv(return3fv(currentBalls[i * gridSize + j].normal));
				glVertex3fv(return3fv(currentBalls[i * gridSize + j].position));
				glNormal3fv(return3fv(currentBalls[i * gridSize + j + 1].normal));
				glVertex3fv(return3fv(currentBalls[i * gridSize + j + 1].position));
				glNormal3fv(return3fv(currentBalls[(i + 1) * gridSize + j].normal));
				glVertex3fv(return3fv(currentBalls[(i + 1) * gridSize + j].position));

				glNormal3fv(return3fv(currentBalls[(i + 1) * gridSize + j].normal));
				glVertex3fv(return3fv(currentBalls[(i + 1) * gridSize + j].position));
				glNormal3fv(return3fv(currentBalls[i * gridSize + j + 1].normal));
				glVertex3fv(return3fv(currentBalls[i * gridSize + j + 1].position));
				glNormal3fv(return3fv(currentBalls[(i + 1) * gridSize + j + 1].normal));
				glVertex3fv(return3fv(currentBalls[(i + 1) * gridSize + j + 1].position));
			}
		}
	}
	glEnd();
	glDisable(GL_LIGHTING); 
	glPopMatrix();
}

void drawSphere() {

	for (int i = 0; i < numSpheres; i++) {
		Vector tmp = sphereForCD[i].position + changedDistance;
		glPushMatrix();
		glTranslatef(tmp.x, tmp.y, tmp.z);
		glutSolidSphere(sphereForCD[i].radius * 1.07f, 20, 20);
		glPopMatrix();
	}
}




void Lighting()
{
	/*******************************************************************/
	glEnable(GL_LIGHTING); //point light를 light1로 정하여 설정, 구를 그려주었다.
	if (PLSw == 1) {
		float light_pos[] = { 1.0f, 10.0f, 1.0f, 1.0f };
		float light_ambient[] = { 0.5f, 0.2f, 0.1f, 1.0f };
		float light_diffuse[] = { 0.4f, 0.3f, 0.9f, 1.0f };
		float light_specular[] = { 0.3f, 1.0f, 0.5f, 1.0f };
		glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, light_pos);
		glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
		glEnable(GL_LIGHT1);
		glPushMatrix();
		glTranslatef(1.0f, 10.0f, 1.0f);
		glColor3d(1.0, 1.0, 1.0);
		glutSolidSphere(0.5, 20, 20);
		glPopMatrix();
		glEnd();
	}
	else {
		glDisable(GL_LIGHT1);
	}
	if (DLSw == 1) { //directional light을 light2로 설정, pos의 마지막 값을 0.0f 로 해주었고 구와 방향벡터를 그려주었다. 
		float light_pos[] = { 10.0f, 10.0f, 0.0f, 0.0f };
		float light_ambient[] = { 0.3f, 0.1f, 0.5f, 1.0f };
		float light_diffuse[] = { 0.4f, 0.9f, 0.2f, 1.0f };
		float light_specular[] = { 1.0f, 0.6f, 0.3f, 1.0f };
		glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT2, GL_POSITION, light_pos);
		glEnable(GL_LIGHT2);
		glPushMatrix();
		glTranslatef(10.0f, 10.0f, 3.0f);
		glutSolidSphere(0.5, 20, 20);
		glPopMatrix();
		glEnd();
		glLineWidth(10.0);
		glBegin(GL_LINES);
		glColor3d(1.0, 1.0, 1.0);
		glVertex3d(10.0f, 10.0f, 3.0f);
		glVertex3d(10.0f / 1.1f, 10.0f / 1.1f, 3.0f / 1.1f);
		glEnd();
		glLineWidth(1.0);
	}
	else {
		glDisable(GL_LIGHT2);
	}
	if (SLSw == 1) {//Spotlight를 light3로 설정, 그려주었다.
		float light_pos[] = { 2.0f, 7.0f, 14.0f, 1.0f };
		float light_dir[] = { 0.0f, 0.0f, -0.5f };
		float light_ambient[] = { 0.9f, 0.4f, 0.2f, 1.0f };
		float light_diffuse[] = { 0.4f, 0.9f, 0.6f, 1.0f };
		float light_specular[] = { 0.7f, 0.9f, 0.8f, 1.0f };
		glLightfv(GL_LIGHT3, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT3, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT3, GL_POSITION, light_pos);
		glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, light_dir);
		glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 80.0f);
		glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 20.0f);
		glEnable(GL_LIGHT3);
		glPushMatrix();
		glTranslatef(2.0f, 7.0f, 14.0f);
		glutSolidSphere(0.5, 20, 20);
		glPopMatrix();
		glEnd();
		glLineWidth(10.0);
		glBegin(GL_LINES);
		glColor3d(1.0, 1.0, 1.0);
		glVertex3d(2.0f, 7.0f, 14.0f);
		glVertex3d(0.0f, 0.0f, -0.5f);
		glEnd();
		glLineWidth(1.0);
	}
	else {
		glDisable(GL_LIGHT3);
	}
	glDisable(GL_LIGHTING);
	//(PA #4) : 다양한 광원을 구현하십시오.
	//1. Point light / Directional light / Spotlight를 서로 다른 색으로 구현하십시오.
	//2. 광원의 위치를 구(sphere)로 표현하십시오.
	//3. Directional light / Spotlight의 경우 빛의 진행방향을 선분으로 표현하십시오.
	/*******************************************************************/
}





/*********************************************************************************
* Call this part whenever display events are needed.
* Display events are called in case of re-rendering by OS. ex) screen movement, screen maximization, etc.
* Or, user can occur the events by using glutPostRedisplay() function directly.
* this part is called in main() function by registering on glutDisplayFunc(display).
**********************************************************************************/
void display()
{
	if (selectMode == 0)
		glClearColor(0, 0.6, 0.8, 1);								// Clear color setting
	else
		glClearColor(0, 0, 0, 1);									// When the backbuffer mode, clear color is set to black

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);				// Clear the screen

	if (shademodel)
		glShadeModel(GL_FLAT);										// Set Flat Shading
	else
		glShadeModel(GL_SMOOTH);

	float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_SPECULAR, white);
	glMaterialf(GL_FRONT, GL_SHININESS, 32.0f);

	drawCamera();													// and draw all of them.
	drawFloor();													// Draw floor.
	drawCloth();

	if (SSw == 1) {
		drawSphere();
	}

	Lighting();

	glFlush();

	// If it is not backbuffer mode, swap the screen. In backbuffer mode, this is not necessary because it is not presented on screen.
	if (selectMode == 0)
		glutSwapBuffers();

	frame++;
}





/*********************************************************************************
* Call this part whenever size of the window is changed.
* This part is called in main() function by registering on glutReshapeFunc(reshape).
**********************************************************************************/
void reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);            // Select The Projection Matrix
	glLoadIdentity();                       // Reset The Projection Matrix
	float aspect = width / float(height);
	gluPerspective(45, aspect, 1, 1024);

	glMatrixMode(GL_MODELVIEW);             // Select The Modelview Matrix
	glLoadIdentity();                       // Reset The Projection Matrix
}



void ResetCloth() {

	for (int i = 0; i < gridSize; i++) {
		for (int j = 0; j < gridSize; j++) {
			balls1[i * gridSize + j].position = Vector(naturalLength * i - naturalLength * gridSize / 2, 10.5f, naturalLength * j - naturalLength * gridSize / 2);
			balls1[i * gridSize + j].velocity = Vector(0.0f, 0.0f, 0.0f);
			balls1[i * gridSize + j].mass = mass;
			balls1[i * gridSize + j].fixed = false;
		}
	}
	balls1[0].fixed = true;
	balls1[gridSize - 1].fixed = true;

	balls1[gridSize * (gridSize - 1)].fixed = true;
	balls1[gridSize * gridSize - 1].fixed = true;

	for (int i = 0; i<numBalls; i++)
		balls2[i] = balls1[i];

	currentBalls = balls1;
	nextBalls = balls2;

	SPRING * currentSpring = &springs[0];

	for (int i = 0; i < gridSize; i++)
	{
		for (int j = 0; j < gridSize - 1; j++)
		{
			currentSpring->ball1 = i * gridSize + j;
			currentSpring->ball2 = i * gridSize + j + 1;

			currentSpring->springConstant = springConstant;
			currentSpring->naturalLength = naturalLength;

			currentSpring++;
		}
	}

	for (int i = 0; i < gridSize - 1; i++)
	{
		for (int j = 0; j < gridSize; j++)
		{
			currentSpring->ball1 = i * gridSize + j;
			currentSpring->ball2 = (i + 1) * gridSize + j;

			currentSpring->springConstant = springConstant;
			currentSpring->naturalLength = naturalLength;

			currentSpring++;
		}
	}

	for (int i = 0; i < gridSize - 1; ++i)
	{
		for (int j = 0; j < gridSize - 1; ++j)
		{
			currentSpring->ball1 = i * gridSize + j;
			currentSpring->ball2 = (i + 1) * gridSize + j + 1;

			currentSpring->springConstant = springConstant;
			currentSpring->naturalLength = naturalLength * sqrt(2.0f);

			currentSpring++;
		}
	}

	for (int i = 0; i < gridSize - 1; i++)
	{
		for (int j = 1; j < gridSize; j++)
		{
			currentSpring->ball1 = i * gridSize + j;
			currentSpring->ball2 = (i + 1) * gridSize + j - 1;

			currentSpring->springConstant = springConstant;
			currentSpring->naturalLength = naturalLength * sqrt(2.0f);

			currentSpring++;
		}
	}

	for (int i = 0; i < gridSize; i++)
	{
		for (int j = 0; j < gridSize - 2; j++)
		{
			currentSpring->ball1 = i * gridSize + j;
			currentSpring->ball2 = i * gridSize + j + 2;

			currentSpring->springConstant = springConstant;
			currentSpring->naturalLength = naturalLength * 2;

			currentSpring++;
		}
	}

	for (int i = 0; i < gridSize - 2; i++)
	{
		for (int j = 0; j < gridSize; j++)
		{
			currentSpring->ball1 = i * gridSize + j;
			currentSpring->ball2 = (i + 2) * gridSize + j;

			currentSpring->springConstant = springConstant;
			currentSpring->naturalLength = naturalLength * 2;

			currentSpring++;
		}
	}
}



void InitCloth() {

	numBalls = gridSize * gridSize;
	numSprings = (gridSize - 1) * gridSize * 2;
	numSprings += (gridSize - 1) * (gridSize - 1) * 2;
	numSprings += (gridSize - 2) * gridSize * 2;

	balls1 = new BALL[numBalls];
	balls2 = new BALL[numBalls];
	springs = new SPRING[numSprings];

	ResetCloth();
}




//------------------------------------------------------------------------------
void initialize()
{
	// Set up OpenGL state
	glEnable(GL_DEPTH_TEST);         // Enables Depth Testing
									 // Initialize the matrix stacks
	reshape(width, height);
	// Define lighting for the scene
	float light_pos[] = { 1.0, 10.0, 1.0, 0.0 };
	float light_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
	float light_diffuse[] = { 0.9, 0.9, 0.9, 1.0 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glEnable(GL_LIGHT0);
	
	InitCamera();
	InitCloth();
}





/*********************************************************************************
* Call this part whenever user types keyboard.
* This part is called in main() function by registering on glutKeyboardFunc(onKeyPress).
**********************************************************************************/
void onKeyPress(unsigned char key, int x, int y)
{
	//카메라 시점 변경
	if ((key >= '1') && (key <= '5'))
		cameraIndex = key - '1';

	/*******************************************************************/
	if ((key == 'w')) {
		sphereForCD[0].position = Vector(0.0f, 3.0f, 0.0f);
		sphereForCD[0].radius = 2.0f;
		//sphereForCD[1].position = Vector(3.0f, 5.0f, 0.0f);
		//sphereForCD[1].radius = 2.0f;
		SSw *= -1;
	}
	else if ((key == 'e')) {
		foward *= -1;
		if (foward == 1) {
			sphereForCD[0].velocity = Vector(-0.02f, 0.0f, 0.0f);
		//	sphereForCD[1].velocity = Vector(-0.02f, 0.0f, 0.0f);
		}
		else {
			sphereForCD[0].velocity = Vector(0.0f, 0.0f, 0.0f);
		//	sphereForCD[1].velocity = Vector(0.0f, 0.0f, 0.0f);
		}
	}
	else if ((key == 'r')) {
		back *= -1;
		if (back == 1) {
			sphereForCD[0].velocity = Vector(+0.02f, 0.0f, 0.0f);
			//sphereForCD[1].velocity = Vector(+0.02f, 0.0f, 0.0f);
		}
		else {
			sphereForCD[0].velocity = Vector(0.0f, 0.0f, 0.0f);
			//sphereForCD[1].velocity = Vector(0.0f, 0.0f, 0.0f);
		}
	}
	else if ((key == 'z')) {
		currentBalls[0].fixed = false;
		currentBalls[gridSize - 1].fixed = false;
	}
	else if ((key == 'x')) {
		currentBalls[gridSize * (gridSize - 1)].fixed = false;
		currentBalls[gridSize * gridSize - 1].fixed = false;
	}
	//(PA #3) : 과제에서 제시하는 대로 키보드 입력에 대한 코드를 추가로 작성하십시오.
	/*******************************************************************/

	if (key == 's') {
		shademodel = !shademodel;
	}

	glutPostRedisplay();
}






void SpecialKey(int key, int x, int y)
{
	/*******************************************************************/
	if (key == GLUT_KEY_F1) { //special key가 들어옴에 따라 각 light switch를 on off 해준다.
		DLSw *= -1;
	}
	else if (key == GLUT_KEY_F2) {
		PLSw *= -1;
	}
	else if (key == GLUT_KEY_F3) {
		SLSw *= -1;
	}
	//(PA #4) : F1 / F2 / F3 버튼에 따라 서로 다른 광원이 On/Off 되도록 구현하십시오.
	/*******************************************************************/
	glutPostRedisplay();
}




void idle() {

	/*******************************************************************/
	float timePassed = 0.01;

	if (foward == 1) {
		changedDistance.x -= 0.02;
	}
	if (back == 1) {
		changedDistance.x += 0.02;
	}

	for (int i = 0; i < numSprings; i++) {
		float springLength = (currentBalls[springs[i].ball1].position - currentBalls[springs[i].ball2].position).Magnitude();
		float extension = springLength - springs[i].naturalLength;

		springs[i].tension = springs[i].springConstant * extension;
		if (springLength > naturalLength * 12) {
			currentBalls[springs[i].ball1].fixed = false;
			currentBalls[springs[i].ball2].fixed = false;
		}
	}

	for (int i = 0; i < numBalls; i++) {
		nextBalls[i].fixed = currentBalls[i].fixed;
		nextBalls[i].mass = currentBalls[i].mass;

		if (currentBalls[i].fixed) {
			nextBalls[i].position = currentBalls[i].position;
			nextBalls[i].velocity = Vector(0.0f, 0.0f, 0.0f);
		}
		else {
			Vector force = currentBalls[i].mass * gravity;

			for (int j = 0; j < numSprings; j++) {
				if (springs[j].ball1 == i) {
					Vector tensionDirection = currentBalls[springs[j].ball2].position - currentBalls[i].position;
					tensionDirection.Normalize();
					force = force + springs[j].tension * tensionDirection;
				}
				if (springs[j].ball2 == i) {
					Vector tensionDirection = currentBalls[springs[j].ball1].position - currentBalls[i].position;
					tensionDirection.Normalize();
					force = force + springs[j].tension * tensionDirection;
				}
			}

			Vector acceleration = force / currentBalls[i].mass;

			nextBalls[i].velocity = currentBalls[i].velocity + (float)timePassed * acceleration;

			nextBalls[i].velocity = dampFactor * nextBalls[i].velocity;

			nextBalls[i].position = currentBalls[i].position + (float)timePassed / 2 * (nextBalls[i].velocity + currentBalls[i].velocity);

			if (SSw == 1) {
				for (int j = 0; j < numSpheres; j++) {
					Vector tmp = sphereForCD[j].position + changedDistance;
					if (pow(nextBalls[i].position.x - tmp.x, 2) + pow(nextBalls[i].position.y - tmp.y, 2) + pow(nextBalls[i].position.z - tmp.z, 2) - sphereForCD[j].radius * sphereForCD[j].radius * 1.08f * 1.08f < 0) {
						Vector alpha = nextBalls[i].position - tmp;
						alpha.Normalize();
						nextBalls[i].position = sphereForCD[j].radius * 1.08f * alpha / alpha.Magnitude() + tmp;
						nextBalls[i].velocity = nextBalls[i].velocity + sphereForCD[j].velocity;
					}
				}
			}
			if (nextBalls[i].position.x < 12 && nextBalls[i].position.x > -12 && nextBalls[i].position.z < 12 && nextBalls[i].position.z > -12) {
				if (nextBalls[i].position.y < 0.0f)
					nextBalls[i].position.y = 0.0f;
			}
		}

		BALL * temp = currentBalls;
		currentBalls = nextBalls;
		nextBalls = currentBalls;
	}
	//(PA #3) : 추가적인 입력이 없을 때의 움직임을 구현하십시오.
	/*******************************************************************/

	glutPostRedisplay();
}






//------------------------------------------------------------------------------
void main(int argc, char* argv[])
{
	width = 800;
	height = 600;
	frame = 0;
	glutInit(&argc, argv);							// Initialize openGL.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);	// Initialize display mode. This project will use float buffer and RGB color.
	glutInitWindowSize(width, height);				// Initialize window size.
	glutInitWindowPosition(100, 100);				// Initialize window coordinate.
	glutCreateWindow("Cloth Simulation");

	glutDisplayFunc(display);						// Register display function to call that when drawing screen event is needed.
	glutReshapeFunc(reshape);						// Register reshape function to call that when size of the window is changed.
	glutKeyboardFunc(onKeyPress);					// Register onKeyPress function to call that when user presses the keyboard.

	glutIdleFunc(idle);

	glutSpecialFunc(SpecialKey);

	initialize();									// Initialize the other thing.
	glutMainLoop();									// Execute the loop which handles events.
}
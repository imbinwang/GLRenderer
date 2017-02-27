#ifndef _GL_RENDERER_H_
#define _GL_RENDERER_H_

// in order to get function prototypes from glext.h, define GL_GLEXT_PROTOTYPES before including glext.h
#define GL_GLEXT_PROTOTYPES

#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdlib>
#include "glext.h"
#include "glInfo.h"                             // glInfo struct
#include "glm.h"
#include <opencv2/opencv.hpp>
#include "cvCamera.h"

class GLRenderer
{
public:
	// GLUT CALLBACK functions ////////////////////////////////////////////////////
	static void displayCB();
	static void reshapeCB(int w, int h);
	static void timerCB(int millisec);
	static void idleCB();
	static void keyboardCB(unsigned char key, int x, int y);

	// CALLBACK function when exit() called ///////////////////////////////////////
	static void exitCB();

	// function declearations /////////////////////////////////////////////////////
	static void init(int argc, char **argv, int width, int height, float nP, float fp, 
		Camera &cam, GLMmodel *mdl);
	static void initGL();
	static int  initGLUT(int argc, char **argv);
	static bool initSharedMem(int width, int height, float nP, float fp, 
		Camera &cam, GLMmodel *mdl);
	static void clearSharedMem();
	static void initLights();
	static void drawBgImg();
	static void drawAxis();
	static void render();
	static bool unproject(float pixel_x, float pixel_y, float &X, float &Y, float &Z);
	static void getRGBABuffer();
	static void getDepthBuffer();

	// FBO utils
	static bool checkFramebufferStatus();
	static void printFramebufferInfo();
	static std::string convertInternalFormatToString(GLenum format);
	static std::string getTextureParameters(GLuint id);
	static std::string getRenderbufferParameters(GLuint id);

	// global variables
	static GLuint fboId;                       // ID of FBO
	static GLuint rboIds[2];                       // ID of Render buffer object
	static int screenWidth;  // just for default win system display
	static int screenHeight; // just for default win system display
	static int renderWidth; // for offscreen rendering
	static int renderHeight; // for offscreen rendering
	static bool fboSupported;
	static bool fboUsed;
	static int drawMode;
	static GLMmodel* model;
	static float modelDimensions[3];
	static GLubyte* rgbaBuffer;
	static GLfloat* depthBuffer;
	static cv::Mat bgrImg;
	static cv::Mat depthMap;
	static Camera camera;
	static GLfloat nearP, farP;

	static GLuint bgImgTextureId;
	static bool bgImgUsed;
	static cv::Mat bgImg;
	static GLubyte* bgImgBuffer;
};

#endif
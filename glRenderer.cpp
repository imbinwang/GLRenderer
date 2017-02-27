#include "glRenderer.h"
#include "glm.h"

using std::stringstream;
using std::string;
using std::cout;
using std::endl;
using std::ends;

// global variables
GLuint GLRenderer::fboId;                       // ID of FBO
GLuint GLRenderer::rboIds[2];                       // ID of Render buffer object
int GLRenderer::screenWidth;
int GLRenderer::screenHeight;
int GLRenderer::renderWidth;
int GLRenderer::renderHeight;
bool GLRenderer::fboSupported;
bool GLRenderer::fboUsed;
int GLRenderer::drawMode;
GLMmodel* GLRenderer::model;
float GLRenderer::modelDimensions[3];
GLubyte* GLRenderer::rgbaBuffer;
GLfloat* GLRenderer::depthBuffer;
cv::Mat GLRenderer::bgrImg;
cv::Mat GLRenderer::depthMap;
Camera GLRenderer::camera;
GLfloat GLRenderer::nearP, GLRenderer::farP;

GLuint GLRenderer::bgImgTextureId;
bool GLRenderer::bgImgUsed;
cv::Mat GLRenderer::bgImg;
GLubyte* GLRenderer::bgImgBuffer;

// function pointers for FBO
// Windows needs to get function pointers from ICD OpenGL drivers,
// because opengl32.dll does not support extensions higher than v1.1.
#ifdef _WIN32
// Framebuffer object
PFNGLGENFRAMEBUFFERSPROC                     pglGenFramebuffers = 0;                      // FBO name generation procedure
PFNGLDELETEFRAMEBUFFERSPROC                  pglDeleteFramebuffers = 0;                   // FBO deletion procedure
PFNGLBINDFRAMEBUFFERPROC                     pglBindFramebuffer = 0;                      // FBO bind procedure
PFNGLCHECKFRAMEBUFFERSTATUSPROC              pglCheckFramebufferStatus = 0;               // FBO completeness test procedure
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC pglGetFramebufferAttachmentParameteriv = 0;  // return various FBO parameters
PFNGLGENERATEMIPMAPPROC                      pglGenerateMipmap = 0;                       // FBO automatic mipmap generation procedure
PFNGLFRAMEBUFFERTEXTURE2DPROC                pglFramebufferTexture2D = 0;                 // FBO texdture attachement procedure
PFNGLFRAMEBUFFERRENDERBUFFERPROC             pglFramebufferRenderbuffer = 0;              // FBO renderbuffer attachement procedure
// Renderbuffer object
PFNGLGENRENDERBUFFERSPROC                    pglGenRenderbuffers = 0;                     // renderbuffer generation procedure
PFNGLDELETERENDERBUFFERSPROC                 pglDeleteRenderbuffers = 0;                  // renderbuffer deletion procedure
PFNGLBINDRENDERBUFFERPROC                    pglBindRenderbuffer = 0;                     // renderbuffer bind procedure
PFNGLRENDERBUFFERSTORAGEPROC                 pglRenderbufferStorage = 0;                  // renderbuffer memory allocation procedure
PFNGLGETRENDERBUFFERPARAMETERIVPROC          pglGetRenderbufferParameteriv = 0;           // return various renderbuffer parameters
PFNGLISRENDERBUFFERPROC                      pglIsRenderbuffer = 0;                       // determine renderbuffer object type

#define glGenFramebuffers                        pglGenFramebuffers
#define glDeleteFramebuffers                     pglDeleteFramebuffers
#define glBindFramebuffer                        pglBindFramebuffer
#define glCheckFramebufferStatus                 pglCheckFramebufferStatus
#define glGetFramebufferAttachmentParameteriv    pglGetFramebufferAttachmentParameteriv
#define glGenerateMipmap                         pglGenerateMipmap
#define glFramebufferTexture2D                   pglFramebufferTexture2D
#define glFramebufferRenderbuffer                pglFramebufferRenderbuffer

#define glGenRenderbuffers                       pglGenRenderbuffers
#define glDeleteRenderbuffers                    pglDeleteRenderbuffers
#define glBindRenderbuffer                       pglBindRenderbuffer
#define glRenderbufferStorage                    pglRenderbufferStorage
#define glGetRenderbufferParameteriv             pglGetRenderbufferParameteriv
#define glIsRenderbuffer                         pglIsRenderbuffer
#endif

// function pointers for WGL_EXT_swap_control
#ifdef _WIN32
typedef BOOL(WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
typedef int (WINAPI * PFNWGLGETSWAPINTERVALEXTPROC) (void);
PFNWGLSWAPINTERVALEXTPROC pwglSwapIntervalEXT = 0;
PFNWGLGETSWAPINTERVALEXTPROC pwglGetSwapIntervalEXT = 0;
#define wglSwapIntervalEXT      pwglSwapIntervalEXT
#define wglGetSwapIntervalEXT   pwglGetSwapIntervalEXT
#endif

void GLRenderer::render()
{
	// the last GLUT call (LOOP)
	// window will be shown and display callback is triggered by events
	// NOTE: this call never return main().
	//glutMainLoop(); /* Start GLUT event-processing loop */

	// the last GLUT call (LOOP)
	// window will be shown and display callback is triggered by events
	// NOTE: this call will return main().
	glutMainLoopEvent();	
	glutPostRedisplay();
}

bool GLRenderer::unproject(float pixel_x, float pixel_y, float &X, float &Y, float &Z)
{
	GLint viewport[4];
	GLdouble modelview[16];
	GLdouble projection[16];
	GLfloat winX, winY, winZ;
	GLdouble posX, posY, posZ;

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	winX = pixel_x;
	winY = viewport[3] - pixel_y;
	winZ = depthBuffer[(int)winY*renderWidth + (int)winX];
	
	GLint status = gluUnProject(winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
	X = float(posX);
	Y = float(posY);
	Z = float(posZ);

	return status;
}

void GLRenderer::getRGBABuffer()
{
	if (fboUsed)
	{
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, renderWidth, renderHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer);
	}
	else
	{
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, renderWidth, renderHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer);
	}

	for (int i = 0; i < renderHeight; ++i)
	{
		cv::Vec3b *rptr = bgrImg.ptr<cv::Vec3b>(renderHeight - i - 1);
		for (int j = 0; j < renderWidth; ++j)
		{
			rptr[j][2] = rgbaBuffer[i*renderWidth * 4 + 4 * j];
			rptr[j][1] = rgbaBuffer[i*renderWidth * 4 + 4 * j + 1];
			rptr[j][0] = rgbaBuffer[i*renderWidth * 4 + 4 * j + 2];
		}
	}
}

void GLRenderer::getDepthBuffer()
{
	if (fboUsed)
	{
		glReadBuffer(GL_DEPTH_ATTACHMENT);
		glReadPixels(0, 0, renderWidth, renderHeight, GL_DEPTH_COMPONENT, GL_FLOAT, depthBuffer);
	}
	else
	{
		glReadBuffer(GL_BACK);
		glReadPixels(0, 0, renderWidth, renderHeight, GL_DEPTH_COMPONENT, GL_FLOAT, depthBuffer);
	}

	for (int i = 0; i < renderHeight; ++i)
	{
		float *rptr = depthMap.ptr<float>(renderHeight - i - 1);
		for (int j = 0; j < renderWidth; ++j)
		{
			rptr[j] = depthBuffer[i*renderWidth + j];
		}
	}

}

void GLRenderer::init(int argc, char **argv, int width, int height, float nP, float fP, 
	Camera &cam, GLMmodel *mdl)
{
	// init global vars
	initSharedMem(width, height, nP, fP, cam, mdl);

	// register exit callback
	atexit(exitCB);

	// init GLUT and GL
	initGLUT(argc, argv);
	initGL();

	// get OpenGL info
	glInfo glInfo;
	glInfo.getInfo();
#ifdef _WIN32
	// check if FBO is supported by your video card
	if (glInfo.isExtensionSupported("GL_ARB_framebuffer_object"))
	{
		// get pointers to GL functions
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
		glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)wglGetProcAddress("glGetFramebufferAttachmentParameteriv");
		glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
		glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
		glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
		glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
		glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
		glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetRenderbufferParameteriv");
		glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)wglGetProcAddress("glIsRenderbuffer");

		// check once again FBO extension
		if (glGenFramebuffers && glDeleteFramebuffers && glBindFramebuffer && glCheckFramebufferStatus &&
			glGetFramebufferAttachmentParameteriv && glGenerateMipmap && glFramebufferTexture2D && glFramebufferRenderbuffer &&
			glGenRenderbuffers && glDeleteRenderbuffers && glBindRenderbuffer && glRenderbufferStorage &&
			glGetRenderbufferParameteriv && glIsRenderbuffer)
		{
			fboSupported = fboUsed = true;
			std::cout << "Video card supports GL_ARB_framebuffer_object." << std::endl;
		}
		else
		{
			fboSupported = fboUsed = false;
			std::cout << "Video card does NOT support GL_ARB_framebuffer_object." << std::endl;
		}
	}

	// check EXT_swap_control is supported
	if (glInfo.isExtensionSupported("WGL_EXT_swap_control"))
	{
		// get pointers to WGL functions
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
		if (wglSwapIntervalEXT && wglGetSwapIntervalEXT)
		{
			// disable v-sync
			wglSwapIntervalEXT(0);
			std::cout << "Video card supports WGL_EXT_swap_control." << std::endl;
		}
	}

#else // for linux, do not need to get function pointers, it is up-to-date
	if (glInfo.isExtensionSupported("GL_ARB_framebuffer_object"))
	{
		fboSupported = fboUsed = true;
		std::cout << "Video card supports GL_ARB_framebuffer_object." << std::endl;
	}
	else
	{
		fboSupported = fboUsed = false;
		std::cout << "Video card does NOT support GL_ARB_framebuffer_object." << std::endl;
	}
#endif

	// create a background texture object
	glGenTextures(1, &bgImgTextureId);
	glBindTexture(GL_TEXTURE_2D, bgImgTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // automatic mipmap generation included in OpenGL v1.4
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderWidth, renderHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (fboSupported)
	{
		// create a framebuffer object, you need to delete them when program exits.
		glGenFramebuffers(1, &fboId);
		glBindFramebuffer(GL_FRAMEBUFFER, fboId);

		// create 2 renderbuffer objects to store depth info and bgr info
		// NOTE: A depth renderable image should be attached the FBO for depth test.
		// If we don't attach a depth renderable image to the FBO, then
		// the rendering output will be corrupted because of missing depth test.
		// If you also need stencil test for your rendering, then you must
		// attach additional image to the stencil attachement point, too.
		glGenRenderbuffers(2, rboIds);
		glBindRenderbuffer(GL_RENDERBUFFER, rboIds[0]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, renderWidth, renderHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, rboIds[1]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, renderWidth, renderHeight);

		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// attach a renderbuffer to FBO color attachment point
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboIds[0]);
		// attach a renderbuffer to FBO depth attachment point
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboIds[1]);

		//@@ disable color buffer if you don't attach any color buffer image,
		//@@ for example, rendering the depth buffer only to a texture.
		//@@ Otherwise, glCheckFramebufferStatus will not be complete.
		//glDrawBuffer(GL_NONE);
		//glReadBuffer(GL_NONE);

		// check FBO status
		printFramebufferInfo();
		checkFramebufferStatus();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

}

int GLRenderer::initGLUT(int argc, char **argv)
{
	// GLUT stuff for windowing
	// initialization openGL window.
	// It must be called before any other GLUT routine.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );   // display mode
	glutInitWindowSize(screenWidth, screenHeight);              // window size
	glutInitWindowPosition(100, 100);                           // window location

	// finally, create a window with openGL context
	// Window will not displayed until glutMainLoop() is called
	// It returns a unique ID.
	int handle = glutCreateWindow(argv[0]);     // param is the title of window

	// register GLUT callback functions
	glutDisplayFunc(displayCB);
	glutIdleFunc(idleCB);                       // redraw whenever system is idle
	glutReshapeFunc(reshapeCB);
	glutKeyboardFunc(keyboardCB);

	return handle;
}

void GLRenderer::initGL()
{
	glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);      // 4-byte pixel alignment
	glPixelStorei(GL_PACK_ALIGNMENT, 1);      // 4-byte pixel alignment

	// enable /disable features
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);

	// track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	glClearColor(0, 0, 0, 0);                   // background color
	glClearDepth(1.0f);                         // 0 is near, 1 is far
	glDepthFunc(GL_LEQUAL);

	initLights();
}

bool GLRenderer::initSharedMem(int width, int height, float nP, float fP,
	Camera &cam, GLMmodel *mdl)
{
	screenWidth = width;
	screenHeight = height;
	renderWidth = width;
	renderHeight = height;
	nearP = nP;
	farP = fP;

	model = mdl;
	glmDimensions(model, modelDimensions);

	camera.copyFrom(cam);

	drawMode = 0; // 0:fill, 1: wireframe, 2:points
	
	fboId = 0;
	rboIds[0] = rboIds[1] = 0;
	fboSupported = fboUsed = false;

	rgbaBuffer = (GLubyte*)malloc(renderWidth * renderHeight * 4);
	depthBuffer = (GLfloat*)malloc(renderWidth * renderHeight * 4);
	bgrImg = cv::Mat::ones(renderHeight, renderWidth, CV_8UC3);
	depthMap = cv::Mat::zeros(renderHeight, renderWidth, CV_32FC1);

	bgImgTextureId = 0;
	bgImgUsed = false;
	bgImg = cv::Mat::zeros(renderHeight, renderWidth, CV_8UC3);
	bgImgBuffer = (GLubyte*)malloc(renderWidth * renderHeight * 3);

	return true;
}

void GLRenderer::clearSharedMem()
{
	glDeleteTextures(1, &bgImgTextureId);
	bgImgTextureId = 0;

	// clean up FBO, RBO
	if (fboSupported)
	{
		glDeleteFramebuffers(1, &fboId);
		fboId = 0;
		glDeleteRenderbuffers(2, rboIds);
		rboIds[0] = rboIds[1] = 0;
	}

	free(rgbaBuffer);
	free(depthBuffer);
	free(bgImgBuffer);	
}

void GLRenderer::initLights()
{
	// set up light colors (ambient, diffuse, specular)
	GLfloat lightKa[] = { .2f, .2f, .2f, 1.0f };  // ambient light
	GLfloat lightKd[] = { .7f, .7f, .7f, 1.0f };  // diffuse light
	GLfloat lightKs[] = { 1, 1, 1, 1 };           // specular light
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightKa);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightKd);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightKs);

	// position the light
	float lightPos[4] = { 0, 0, 50, 1 }; // positional light
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	glEnable(GL_LIGHT0);                        // MUST enable each light source after configuration
}

void GLRenderer::drawBgImg()
{
	for (int i = 0; i < renderHeight; ++i)
	{
		cv::Vec3b *rptr = bgImg.ptr<cv::Vec3b>(renderHeight - i - 1);
		for (int j = 0; j < renderWidth; ++j)
		{
			bgImgBuffer[i*renderWidth * 3 + 3 * j] = rptr[j][2];
			bgImgBuffer[i*renderWidth * 3 + 3 * j + 1] = rptr[j][1];
			bgImgBuffer[i*renderWidth * 3 + 3 * j + 2] = rptr[j][0];
		}
	}

	glBindTexture(GL_TEXTURE_2D, bgImgTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderWidth, renderHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, bgImgBuffer);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(renderWidth, 0);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(renderWidth, renderHeight);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0, renderHeight);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLRenderer::drawAxis()
{
	float diameter = modelDimensions[0];
	diameter = diameter > modelDimensions[1] ? diameter : modelDimensions[1];
	diameter = diameter > modelDimensions[2] ? diameter : modelDimensions[2];
	float radius = diameter*1.2f;   

	// draw axis
	glLineWidth(3);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(radius, 0.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, radius, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, radius);
	glEnd();
	glLineWidth(1);

	//// draw arrows(actually big square dots)
	glPointSize(5);
	glBegin(GL_POINTS);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(radius, 0.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(0.0f, radius, 0.0f);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0.0f, 0.0f, radius);
	glEnd();
	glPointSize(1);
}

bool GLRenderer::checkFramebufferStatus()
{
	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		std::cout << "Framebuffer complete." << std::endl;
		return true;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cout << "[ERROR] Framebuffer incomplete: Attachment is NOT complete." << std::endl;
		return false;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		std::cout << "[ERROR] Framebuffer incomplete: No image is attached to FBO." << std::endl;
		return false;
		/*
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		std::cout << "[ERROR] Framebuffer incomplete: Attached images have different dimensions." << std::endl;
		return false;

		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
		std::cout << "[ERROR] Framebuffer incomplete: Color attached images have different internal formats." << std::endl;
		return false;
		*/
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		std::cout << "[ERROR] Framebuffer incomplete: Draw buffer." << std::endl;
		return false;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		std::cout << "[ERROR] Framebuffer incomplete: Read buffer." << std::endl;
		return false;

	case GL_FRAMEBUFFER_UNSUPPORTED:
		std::cout << "[ERROR] Framebuffer incomplete: Unsupported by FBO implementation." << std::endl;
		return false;

	default:
		std::cout << "[ERROR] Framebuffer incomplete: Unknown error." << std::endl;
		return false;
	}
}

void GLRenderer::printFramebufferInfo()
{
	std::cout << "\n===== FBO STATUS =====\n";

	// print max # of colorbuffers supported by FBO
	int colorBufferCount = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &colorBufferCount);
	std::cout << "Max Number of Color Buffer Attachment Points: " << colorBufferCount << std::endl;

	int objectType;
	int objectId;

	// print info of the colorbuffer attachable image
	for (int i = 0; i < colorBufferCount; ++i)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + i,
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
			&objectType);
		if (objectType != GL_NONE)
		{
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + i,
				GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
				&objectId);

			std::string formatName;

			std::cout << "Color Attachment " << i << ": ";
			if (objectType == GL_TEXTURE)
			{
				std::cout << "GL_TEXTURE, " << getTextureParameters(objectId) << std::endl;
			}
			else if (objectType == GL_RENDERBUFFER)
			{
				std::cout << "GL_RENDERBUFFER, " << getRenderbufferParameters(objectId) << std::endl;
			}
		}
	}

	// print info of the depthbuffer attachable image
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
		&objectType);
	if (objectType != GL_NONE)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
			&objectId);

		std::cout << "Depth Attachment: ";
		switch (objectType)
		{
		case GL_TEXTURE:
			std::cout << "GL_TEXTURE, " << getTextureParameters(objectId) << std::endl;
			break;
		case GL_RENDERBUFFER:
			std::cout << "GL_RENDERBUFFER, " << getRenderbufferParameters(objectId) << std::endl;
			break;
		}
	}

	// print info of the stencilbuffer attachable image
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
		GL_STENCIL_ATTACHMENT,
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
		&objectType);
	if (objectType != GL_NONE)
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,
			GL_STENCIL_ATTACHMENT,
			GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
			&objectId);

		std::cout << "Stencil Attachment: ";
		switch (objectType)
		{
		case GL_TEXTURE:
			std::cout << "GL_TEXTURE, " << getTextureParameters(objectId) << std::endl;
			break;
		case GL_RENDERBUFFER:
			std::cout << "GL_RENDERBUFFER, " << getRenderbufferParameters(objectId) << std::endl;
			break;
		}
	}

	std::cout << std::endl;
}

std::string GLRenderer::getTextureParameters(GLuint id)
{
	if (glIsTexture(id) == GL_FALSE)
		return "Not texture object";

	int width, height, format;
	std::string formatName;
	glBindTexture(GL_TEXTURE_2D, id);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format); // get texture internal format
	glBindTexture(GL_TEXTURE_2D, 0);

	formatName = convertInternalFormatToString(format);

	std::stringstream ss;
	ss << width << "x" << height << ", " << formatName;
	return ss.str();
}

std::string GLRenderer::getRenderbufferParameters(GLuint id)
{
	if (glIsRenderbuffer(id) == GL_FALSE)
		return "Not Renderbuffer object";

	int width, height, format;
	std::string formatName;
	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);    // get renderbuffer width
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);  // get renderbuffer height
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &format); // get renderbuffer internal format
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	formatName = convertInternalFormatToString(format);

	std::stringstream ss;
	ss << width << "x" << height << ", " << formatName;
	return ss.str();
}

std::string GLRenderer::convertInternalFormatToString(GLenum format)
{
	std::string formatName;

	switch (format)
	{
	case GL_STENCIL_INDEX:      // 0x1901
		formatName = "GL_STENCIL_INDEX";
		break;
	case GL_DEPTH_COMPONENT:    // 0x1902
		formatName = "GL_DEPTH_COMPONENT";
		break;
	case GL_ALPHA:              // 0x1906
		formatName = "GL_ALPHA";
		break;
	case GL_RGB:                // 0x1907
		formatName = "GL_RGB";
		break;
	case GL_RGBA:               // 0x1908
		formatName = "GL_RGBA";
		break;
	case GL_LUMINANCE:          // 0x1909
		formatName = "GL_LUMINANCE";
		break;
	case GL_LUMINANCE_ALPHA:    // 0x190A
		formatName = "GL_LUMINANCE_ALPHA";
		break;
	case GL_R3_G3_B2:           // 0x2A10
		formatName = "GL_R3_G3_B2";
		break;
	case GL_ALPHA4:             // 0x803B
		formatName = "GL_ALPHA4";
		break;
	case GL_ALPHA8:             // 0x803C
		formatName = "GL_ALPHA8";
		break;
	case GL_ALPHA12:            // 0x803D
		formatName = "GL_ALPHA12";
		break;
	case GL_ALPHA16:            // 0x803E
		formatName = "GL_ALPHA16";
		break;
	case GL_LUMINANCE4:         // 0x803F
		formatName = "GL_LUMINANCE4";
		break;
	case GL_LUMINANCE8:         // 0x8040
		formatName = "GL_LUMINANCE8";
		break;
	case GL_LUMINANCE12:        // 0x8041
		formatName = "GL_LUMINANCE12";
		break;
	case GL_LUMINANCE16:        // 0x8042
		formatName = "GL_LUMINANCE16";
		break;
	case GL_LUMINANCE4_ALPHA4:  // 0x8043
		formatName = "GL_LUMINANCE4_ALPHA4";
		break;
	case GL_LUMINANCE6_ALPHA2:  // 0x8044
		formatName = "GL_LUMINANCE6_ALPHA2";
		break;
	case GL_LUMINANCE8_ALPHA8:  // 0x8045
		formatName = "GL_LUMINANCE8_ALPHA8";
		break;
	case GL_LUMINANCE12_ALPHA4: // 0x8046
		formatName = "GL_LUMINANCE12_ALPHA4";
		break;
	case GL_LUMINANCE12_ALPHA12:// 0x8047
		formatName = "GL_LUMINANCE12_ALPHA12";
		break;
	case GL_LUMINANCE16_ALPHA16:// 0x8048
		formatName = "GL_LUMINANCE16_ALPHA16";
		break;
	case GL_INTENSITY:          // 0x8049
		formatName = "GL_INTENSITY";
		break;
	case GL_INTENSITY4:         // 0x804A
		formatName = "GL_INTENSITY4";
		break;
	case GL_INTENSITY8:         // 0x804B
		formatName = "GL_INTENSITY8";
		break;
	case GL_INTENSITY12:        // 0x804C
		formatName = "GL_INTENSITY12";
		break;
	case GL_INTENSITY16:        // 0x804D
		formatName = "GL_INTENSITY16";
		break;
	case GL_RGB4:               // 0x804F
		formatName = "GL_RGB4";
		break;
	case GL_RGB5:               // 0x8050
		formatName = "GL_RGB5";
		break;
	case GL_RGB8:               // 0x8051
		formatName = "GL_RGB8";
		break;
	case GL_RGB10:              // 0x8052
		formatName = "GL_RGB10";
		break;
	case GL_RGB12:              // 0x8053
		formatName = "GL_RGB12";
		break;
	case GL_RGB16:              // 0x8054
		formatName = "GL_RGB16";
		break;
	case GL_RGBA2:              // 0x8055
		formatName = "GL_RGBA2";
		break;
	case GL_RGBA4:              // 0x8056
		formatName = "GL_RGBA4";
		break;
	case GL_RGB5_A1:            // 0x8057
		formatName = "GL_RGB5_A1";
		break;
	case GL_RGBA8:              // 0x8058
		formatName = "GL_RGBA8";
		break;
	case GL_RGB10_A2:           // 0x8059
		formatName = "GL_RGB10_A2";
		break;
	case GL_RGBA12:             // 0x805A
		formatName = "GL_RGBA12";
		break;
	case GL_RGBA16:             // 0x805B
		formatName = "GL_RGBA16";
		break;
	case GL_DEPTH_COMPONENT16:  // 0x81A5
		formatName = "GL_DEPTH_COMPONENT16";
		break;
	case GL_DEPTH_COMPONENT24:  // 0x81A6
		formatName = "GL_DEPTH_COMPONENT24";
		break;
	case GL_DEPTH_COMPONENT32:  // 0x81A7
		formatName = "GL_DEPTH_COMPONENT32";
		break;
	case GL_DEPTH_STENCIL:      // 0x84F9
		formatName = "GL_DEPTH_STENCIL";
		break;
	case GL_RGBA32F:            // 0x8814
		formatName = "GL_RGBA32F";
		break;
	case GL_RGB32F:             // 0x8815
		formatName = "GL_RGB32F";
		break;
	case GL_RGBA16F:            // 0x881A
		formatName = "GL_RGBA16F";
		break;
	case GL_RGB16F:             // 0x881B
		formatName = "GL_RGB16F";
		break;
	case GL_DEPTH24_STENCIL8:   // 0x88F0
		formatName = "GL_DEPTH24_STENCIL8";
		break;
	default:
		stringstream ss;
		ss << "Unknown Format(0x" << std::hex << format << ")" << std::ends;
		formatName = ss.str();
	}

	return formatName;
}



//=============================================================================
// CALLBACKS
//=============================================================================

void GLRenderer::displayCB()
{
	// with FBO
	// render directly to a texture
	if (fboUsed)
	{
		// set FBO as the rendering destination
		glBindFramebuffer(GL_FRAMEBUFFER, fboId);

		// clear buffer
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw background image
		if (bgImgUsed)
		{
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDisable(GL_LIGHTING);	
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, renderWidth, 0, renderHeight);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glPushMatrix();
			drawBgImg();
			glPopMatrix();

			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glEnable(GL_LIGHTING);

			// reset the draw mode
			if (drawMode == 0)        // fill mode
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glEnable(GL_CULL_FACE);
			}
			else if (drawMode == 1)  // wireframe mode
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDisable(GL_CULL_FACE);
			}
			else                    // point mode
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				glDisable(GL_CULL_FACE);
			}
		}

		// draw the model
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(camera.getProjectionIntrinsic(renderWidth, renderHeight, nearP, farP));

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLoadMatrixf(camera.getModelviewExtrinsic());

		glPushMatrix();
#if 0
		// draw object coordinate system
		if (drawMode == 1)  //wireframe mode
		{
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			drawAxis();

			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
		}
#endif

		// draw object
		glmDraw(model, GLM_MATERIAL | GLM_SMOOTH);

		glPopMatrix();

		getRGBABuffer();
		getDepthBuffer();

		// unset FBO
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// without FBO
	// render to the backbuffer and copy the backbuffer to a texture
	else
	{
		// clear buffer
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_PIXEL_MODE_BIT); // for GL_DRAW_BUFFER and GL_READ_BUFFER
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);

		if (bgImgUsed)
		{
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glDisable(GL_LIGHTING);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, renderWidth, 0, renderHeight);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		
			glPushMatrix();
			drawBgImg();
			glPopMatrix();

			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glEnable(GL_LIGHTING);

			// reset the draw mode
			if (drawMode == 0)        // fill mode
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glEnable(GL_CULL_FACE);
			}
			else if (drawMode == 1)  // wireframe mode
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDisable(GL_CULL_FACE);
			}
			else                    // point mode
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
				glDisable(GL_CULL_FACE);
			}
		}

		// draw the model
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(camera.getProjectionIntrinsic(renderWidth, renderHeight, nearP, farP));

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLoadMatrixf(camera.getModelviewExtrinsic());

		glPushMatrix();
#if 0
		// draw object coordinate system
		if (drawMode == 1)  //wireframe mode
		{
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			drawAxis();

			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
	}
#endif

		// draw object
		glmDraw(model, GLM_MATERIAL | GLM_SMOOTH);
		glPopMatrix();
		glPopAttrib(); // GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
	
		getRGBABuffer();
		getDepthBuffer();
	}

	// draw
	glutSwapBuffers();
}

void GLRenderer::reshapeCB(int width, int height)
{
	screenWidth = width;
	screenHeight = height;

	// set viewport to be the entire window
	glViewport(0, 0, (GLsizei)renderWidth, (GLsizei)renderHeight);

	// set perspective viewing frustum
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(camera.getProjectionIntrinsic(renderWidth, renderHeight, nearP, farP));

	// switch to modelview matrix in order to set scene
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GLRenderer::timerCB(int millisec)
{
	glutTimerFunc(millisec, timerCB, millisec);
	glutPostRedisplay();
}

void GLRenderer::idleCB()
{
	glutPostRedisplay();
}

void GLRenderer::keyboardCB(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: // ESCAPE
		exit(0);
		break;

	case ' ':
		if (fboSupported)
			fboUsed = !fboUsed;
		std::cout << "FBO mode: " << (fboUsed ? "on" : "off") << std::endl;
		break;

	case 'd': // switch rendering modes (fill -> wire -> point)
	case 'D':
		drawMode = ++drawMode % 3;
		if (drawMode == 0)        // fill mode
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
		}
		else if (drawMode == 1)  // wireframe mode
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
		}
		else                    // point mode
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
		}
		break;

	default:
		;
	}
}

void GLRenderer::exitCB()
{
	clearSharedMem();
}
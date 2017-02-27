#include "glRenderer.h"
#include "glm.h"
#include "cvCamera.h"
#include "markerDetector.h"
#include "timer.h"

int main(int argc, char **argv)
{
	// offline calibrated camera parameters
	float fxy = 832.560809f;
	float cx = 319.5f;
	float cy = 239.5f;
	float distortions[5] = { 0.066338f, -1.51067f, 0.f, 0.f, 7.626152f };
	Camera cam(fxy, fxy, cx, cy, distortions);

	// marker configuration (real size)
	cv::Size2f marker9x9 = cv::Size2f(9.0f, 9.0f);

	// initialize a marker detector
	MarkerDetector markerDetector(cam, marker9x9);

	// load mesh model
	GLMmodel *bmdl = glmReadOBJ("./data/lego.obj");
	glmFacetNormals(bmdl);
	glmVertexNormals(bmdl, 90.0f);
	
	// open the usb camera
	cv::VideoCapture vc;
	vc.open(0);
	if (!vc.isOpened())
	{
		printf("Cannot open camera\n");
		return -1;
	}
	int frameWidth = (int)vc.get(CV_CAP_PROP_FRAME_WIDTH);
	int frameHeight = (int)vc.get(CV_CAP_PROP_FRAME_HEIGHT);
	double frameFPS = 30.0;

	// initialize a renderer
	float nearPlane = 1.0f, farPlane = 1000.0f;
	GLRenderer renderer;
	renderer.init(argc, argv, frameWidth, frameHeight, nearPlane, farPlane, cam, bmdl);

	// process each frame
	uchar key = 0;
	cv::Mat frame, frameDrawing, depth32, depth8;
	Timer t;
	while (vc.read(frame))
	{
		frameDrawing = frame.clone();
		depth8 = cv::Mat::zeros(frame.size(), CV_8UC1);
		t.start();
		markerDetector.processFrame(frameDrawing);
		t.stop();
		printf("marker detection:%f\n", t.getElapsedTimeInMilliSec());

		const std::vector<cv::Mat> &markerTrans = markerDetector.getTransformations();
		
		t.start();
		if (markerTrans.size()>0)
		{
			renderer.camera.setExtrinsic(markerTrans[0]);
			renderer.bgImg = frameDrawing;
			renderer.bgImgUsed = true;
			renderer.render();
			frameDrawing = renderer.bgrImg;
			depth32 = renderer.depthMap;
			cv::normalize(depth32, depth8, 0, 255, cv::NORM_MINMAX, CV_8UC1);
		}
		t.stop();
		printf("rendering:%f\n", t.getElapsedTimeInMilliSec());

		cv::imshow("Show Marker", frameDrawing);
		cv::imshow("d", depth8);
		key = cv::waitKey(1);
		if (key == 27) break;
	}

	vc.release();
	glmDelete(bmdl);
	return 0;
}
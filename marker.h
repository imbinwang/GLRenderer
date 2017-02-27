/*****************************************************************************************
*   Marker class
*		reprensents a marker (bar code)
*	Each marker has an internal code given by 5 words of 5 bits each. The codification
*	employed is a slight modification of the hamming code. In total, each word has only
*	2 bits of information out of the 5 bits employed. The other 3 are employed for error
*	detection. As a consequence, we can have up to 1024 different IDs.
*
*	Marker cells are 7x7, iner 5x5 codes the information
*******************************************************************************************/

#ifndef _MARKER_H_
#define _MARKER_H_

////////////////////////////////////////////////////////////////////
// Standard includes:
#include <vector>
#include <iostream>
#include <opencv2/opencv.hpp>

/**
* This class represents a marker
*/
class Marker
{
public:
	Marker();

	friend bool operator<(const Marker &M1, const Marker&M2);
	friend std::ostream & operator<<(std::ostream &str, const Marker &M);

	static cv::Mat rotate(cv::Mat  in);
	static int hammDistMarker(cv::Mat bits);
	static int mat2id(const cv::Mat &bits);
	static int getMarkerId(cv::Mat &in, int &nRotations);

	/*
	** Use case:
	int mc[] = {1,0,0,0,0,
	0,1,1,1,0,
	1,0,1,1,1,
	1,0,1,1,1,
	1,0,1,1,1};
	std::vector<int> markerCode(mc, mc+25);

	cv::Mat markerImg;
	Marker::generateMarkerImg(50, markerCode, markerImg);

	cv::imwrite("marker.png",markerImg);
	*/
	static void generateMarkerImg(const int cellSize, const std::vector<int> markerCode,
		cv::Mat &markerImg);

public:

	// Id of  the marker
	int m_id;

	// Marker transformation with regards to the camera
	cv::Mat m_transformation;

	std::vector<cv::Point2f> m_points;

	// Helper function to draw the marker contour over the image
	void drawContour(cv::Mat& image, cv::Scalar color = CV_RGB(0, 250, 0)) const;
};

#endif
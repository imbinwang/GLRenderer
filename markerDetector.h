#ifndef _MARKER_DETECTOR_H_
#define _MARKER_DETECTOR_H_

////////////////////////////////////////////////////////////////////
// Standard includes:
#include <vector>
#include <opencv2/opencv.hpp>

////////////////////////////////////////////////////////////////////
// Forward declaration:
class Marker;
class Camera;

/**
* A top-level class that encapsulate marker detector algorithm
*/

class MarkerDetector
{
public:
	typedef std::vector<cv::Point>    PointsVector;
	typedef std::vector<PointsVector> ContoursVector;

	/**
	* Initialize a new instance of marker detector object
	* @calibration[in] - Camera calibration (intrinsic and distortion components) necessary for pose estimation.
	*/
	MarkerDetector(const Camera &calibration, const cv::Size2f &markerRealSize);

	//! Searches for markers and fills the list of transformation for found markers
	void processFrame(const cv::Mat& frame);

	const std::vector<cv::Mat>& getTransformations() const;

protected:

	//! Main marker detection routine
	bool findMarkers(const cv::Mat& frame, std::vector<Marker>& detectedMarkers);

	//! Converts image to grayscale
	void prepareImage(const cv::Mat& bgraMat, cv::Mat& grayscale);

	//! Performs binary threshold
	void performThreshold(const cv::Mat& grayscale, cv::Mat& thresholdImg);

	//! Detects appropriate contours
	void findContours(cv::Mat& thresholdImg, ContoursVector& contours, int minContourPointsAllowed);

	//! Finds marker candidates among all contours
	void findCandidates(const ContoursVector& contours, std::vector<Marker>& detectedMarkers);

	//! Tries to recognize markers by detecting marker code 
	void recognizeMarkers(const cv::Mat& grayscale, std::vector<Marker>& detectedMarkers);

	//! Calculates marker poses in 3D
	void estimatePosition(std::vector<Marker>& detectedMarkers);

private:
	float m_minContourLengthAllowed;

	cv::Size markerSize;
	cv::Mat camMatrix;
	cv::Mat distCoeff;
	std::vector<cv::Mat> m_transformations;

	cv::Mat m_grayscaleImage;
	cv::Mat m_thresholdImg;
	cv::Mat canonicalMarkerImage;

	ContoursVector           m_contours;
	std::vector<cv::Point3f> m_markerCorners3d;
	std::vector<cv::Point2f> m_markerCorners2d;
};

#endif

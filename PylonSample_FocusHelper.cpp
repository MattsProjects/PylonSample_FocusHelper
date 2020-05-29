// PylonSample_FocusHelper.cpp
/*
Note: Before getting started, Basler recommends reading the Programmer's Guide topic
in the pylon C++ API documentation that gets installed with pylon.
If you are upgrading to a higher major version of pylon, Basler also
strongly recommends reading the Migration topic in the pylon C++ API documentation.

This sample shows how to use OpenCV's Laplacian to measure the focus of the current image.

*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include <opencv2\opencv.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <Windows.h>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 10000;

double GetFocusLevel(CPylonImage &rawImage, int offsetX, int offsetY, int roiWidth, int roiHeight)
{
	// choose the focus roi of the image
	CPylonImage roiImage;
	roiImage = rawImage.GetAoi(offsetX, offsetY, roiWidth, roiHeight);

	// display the roi image
	Pylon::DisplayImage(1, roiImage);

	// Create an OpenCV image from the Pylon Image
	Pylon::CImageFormatConverter converter;
	converter.OutputPixelFormat.SetValue(Pylon::EPixelType::PixelType_Mono8);
	Pylon::CPylonImage convertedImage;
	converter.Convert(convertedImage, roiImage);
	cv::Mat matImage;
	matImage = cv::Mat(convertedImage.GetHeight(), convertedImage.GetWidth(), CV_8UC1, convertedImage.GetBuffer());

	// use cvLaplace to measure focus
	cv::Mat lapMat = cv::Mat(cvSize(convertedImage.GetWidth(), convertedImage.GetHeight()), CV_8UC1, 1);
	cv::Laplacian(matImage, lapMat, CV_8UC1, 1, 1, 0, 4);
	cv::Scalar mean, stddev;
	meanStdDev(lapMat, mean, stddev, cv::Mat());

	// higher variance = sharper image.
	double variance = stddev.val[0] * stddev.val[0];

	// cleanup
	convertedImage.Release();
	matImage.release();
	lapMat.release();

	// return the "focus level"
	return variance;
}

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
	// is initialized during the lifetime of this object.
	Pylon::PylonAutoInitTerm autoInitTerm;

	try
	{
		// Create an instant camera object with the camera device found first.
		CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

		// open the camera so we can configure the physical device itself.
		camera.Open();

		// set original pixel format from camera
		GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("PixelFormat"))->FromString("Mono8");
		GenApi::CFloatPtr(camera.GetNodeMap().GetNode("ExposureTime"))->SetValue(10000);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		double bestFocusSoFar = 0;

		// Start the grabbing of c_countOfImagesToGrab images.
		camera.StartGrabbing(c_countOfImagesToGrab);

		while (camera.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			// Camera.StopGrabbing() is called automatically by RetrieveResult() when c_countOfImagesToGrab have been grabbed.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded())
			{
				CPylonImage image;
				image.AttachGrabResultBuffer(ptrGrabResult);
				Pylon::DisplayImage(0, image);

				int roiOffsetX = ptrGrabResult->GetWidth() / 2;
				int roiOffsetY = ptrGrabResult->GetHeight() / 2;
				int roiWidth = 300;
				int roiHeight = 300;

				double FocusLevel = 0;
				FocusLevel = GetFocusLevel(image, roiOffsetX, roiOffsetY, roiWidth, roiHeight);

				if (FocusLevel > bestFocusSoFar)
					bestFocusSoFar = FocusLevel;

				cout << "Current Focus: " << std::setw(16) << FocusLevel << " Best Focus So Far: " << std::setw(16) << bestFocusSoFar << endl;
				
			}
			else
			{
				cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}
	}
	catch (GenICam::GenericException &e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	return exitCode;
}

// std
#include <iostream>
#include <exception>

// Boost
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

// sfl
#include <sfl/sequence_face_landmarks.h>
#include <sfl/utilities.h>

// vsal
#include <vsal/VideoStreamFactory.h>
#include <vsal/VideoStreamOpenCV.h>

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::runtime_error;
using namespace boost::program_options;
using namespace boost::filesystem;


int main(int argc, char* argv[])
{
	// Parse command line arguments
	string inputPath, landmarksPath;
	bool draw_ind;
	try {
		options_description desc("Allowed options");
		desc.add_options()
			("help", "display the help message")
			("input,i", value<string>(&inputPath)->required(), "path to video sequence")
			("landmarks,l", value<string>(&landmarksPath), "path to landmarks file (.pb)")
			("draw_ind,d", value<bool>(&draw_ind)->default_value(false)->implicit_value(true),
				"draw landmark indices")
			;
		variables_map vm;
		store(command_line_parser(argc, argv).options(desc).
			positional(positional_options_description().add("input", -1)).run(), vm);
		if (vm.count("help")) {
			cout << "Usage: cache_face_landmarks [options]" << endl;
			cout << desc << endl;
			exit(0);
		}
		notify(vm);
	}
	catch (const error& e) {
		cout << "Error while parsing command-line arguments: " << e.what() << endl;
		cout << "Use --help to display a list of options." << endl;
		exit(1);
	}

	try
	{
		// Initialize Sequence Face Landmarks
		std::shared_ptr<sfl::SequenceFaceLandmarks> sfl =
			sfl::SequenceFaceLandmarks::create();

		// Set landmarks path
		if (landmarksPath.empty())
		{
			path input = path(inputPath);
			landmarksPath = 
				(input.parent_path() / (input.stem() += "_landmarks.pb")).string();
			if (!is_regular_file(landmarksPath)) 
				throw runtime_error("Couldn't find landmarks file!");
		}

		// Load landmarks from file
		cout << "Loading landmarks from \"" << landmarksPath << "\"" << endl;
		sfl->load(landmarksPath);

		// Create video source
		vsal::VideoStreamFactory& vsf = vsal::VideoStreamFactory::getInstance();
		std::unique_ptr<vsal::VideoStreamOpenCV> vs(
			(vsal::VideoStreamOpenCV*)vsf.create(inputPath));
		if (vs == nullptr) throw runtime_error("No video source specified!");

		// Open video source
		if (!vs->open()) throw runtime_error("Failed to open video source!");

		// Preview loop
		cv::Mat frame;
		int frameCounter = 0, faceCounter = 0;
		const std::list<std::unique_ptr<sfl::Frame>>& sfl_frames = sfl->getSequence();
		std::list<std::unique_ptr<sfl::Frame>>::const_iterator it = sfl_frames.begin();
		while (vs->read())
		{
			if (!vs->isUpdated()) continue;

			frame = vs->getFrame();
			const std::unique_ptr<sfl::Frame>& sfl_frame = *it++;
			faceCounter += sfl_frame->faces.size();

			// Render landmarks
			sfl::render(frame, *sfl_frame, draw_ind);

			// Show overlay
			string msg = "Frame count: " + std::to_string(++frameCounter);
			cv::putText(frame, msg, cv::Point(15, 15),
				cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 102, 255), 1, CV_AA);
			msg = "Face count: " + std::to_string(faceCounter);
			cv::putText(frame, msg, cv::Point(15, 40),
				cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 102, 255), 1, CV_AA);
			cv::putText(frame, "press any key to stop", cv::Point(10, frame.rows - 20),
				cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 102, 255), 1, CV_AA);
			
			// Show frame
			cv::imshow("face_landmarks_viewer", frame);
			int key = cv::waitKey(45);
			if (key >= 0) break;
		}
	}
	catch (std::exception& e)
	{
		cerr << e.what() << endl;
		return 1;
	}

	return 0;
}


/**
 * @file april_tags.cpp
 * @brief Example application for April tags library
 * @author: Michael Kaess
 *
 * Opens the first available camera (typically a built in camera in a
 * laptop) and continuously detects April tags in the incoming
 * images. Detections are both visualized in the live image and shown
 * in the text console. Optionally allows selecting of a specific
 * camera in case multiple ones are present and specifying image
 * resolution as long as supported by the camera. Also includes the
 * option to send tag detections via a serial port, for example when
 * running on a Raspberry Pi that is connected to an Arduino.
 */

using namespace std;

#include <iostream>
#include <cstring>
#include <list>
#include <sys/time.h>
#include <fstream>


const string usage = "\n"
  "Usage:\n"
  "  apriltag_image_detector [OPTION...] [IMG1 [IMG2...]]\n"
  "\n"
  "Options:\n"
  "  -h  -?          Show help options\n"
  "  -d              Disable graphics\n"
  "  -b              Bumblebee camera\n"
  "  -l              Left hand camera\n"
  "  -t              Timing of tag extraction\n"
  "  -C <bbxhh>      Tag family (default 36h11)\n"
  "  -F <fx>         Focal length in pixels\n"
  "  -S <size>       Tag size (square black frame) in meters\n"
  "  -E <exposure>   Manually set camera exposure (default auto; range 0-10000)\n"
  "  -G <gain>       Manually set camera gain (default auto; range 0-255)\n"
  "  -B <brightness> Manually set the camera brightness (default 128; range 0-255)\n"
  "\n";

const string intro = "\n"
    "April tags test code\n"
    "(C) 2012-2014 Massachusetts Institute of Technology\n"
    "Michael Kaess\n"
    "\n";



#ifndef __APPLE__
#define EXPOSURE_CONTROL // only works in Linux
#endif

#ifdef EXPOSURE_CONTROL
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <errno.h>
#endif

// OpenCV library for easy access to USB camera and drawing of images
// on screen
#include "opencv2/opencv.hpp"

// April tags detector and various families that can be selected by command line option
#include "AprilTags/TagDetector.h"
#include "AprilTags/Tag16h5.h"
#include "AprilTags/Tag25h7.h"
#include "AprilTags/Tag25h9.h"
#include "AprilTags/Tag36h9.h"
#include "AprilTags/Tag36h11.h"

// Needed for getopt / command line options processing
#include <unistd.h>
extern int optind;
extern char *optarg;


const char* windowName = "apriltags_demo";


// utility function to provide current system time (used below in
// determining frame rate at which images are being processed)
double tic() {
  struct timeval t;
  gettimeofday(&t, NULL);
  return ((double)t.tv_sec + ((double)t.tv_usec)/1000000.);
}


#include <cmath>

#ifndef PI
const double PI = 3.14159265358979323846;
#endif
const double TWOPI = 2.0*PI;

/**
 * Normalize angle to be within the interval [-pi,pi].
 */
inline double standardRad(double t) {
  if (t >= 0.) {
    t = fmod(t+PI, TWOPI) - PI;
  } else {
    t = fmod(t-PI, -TWOPI) + PI;
  }
  return t;
}

/**
 * Convert rotation matrix to Euler angles
 */
void wRo_to_euler(const Eigen::Matrix3d& wRo, double& yaw, double& pitch, double& roll) {
    yaw = standardRad(atan2(wRo(1,0), wRo(0,0)));
    double c = cos(yaw);
    double s = sin(yaw);
    pitch = standardRad(atan2(-wRo(2,0), wRo(0,0)*c + wRo(1,0)*s));
    roll  = standardRad(atan2(wRo(0,2)*s - wRo(1,2)*c, -wRo(0,1)*s + wRo(1,1)*c));
}


class ImageProcessor {
  AprilTags::TagDetector* m_tagDetector;
  AprilTags::TagCodes m_tagCodes;

  bool m_draw; // draw image and April tag detections?
  bool m_timing; // print timing information for each tag extraction call

  int m_width; // image size in pixels
  int m_height;
  double m_tagSize; // April tag side length in meters of square black frame
  double m_fx; // camera focal length in pixels
  double m_fy;
  double m_px; // camera principal point
  double m_py;

  list<string> m_imgNames;
  string m_camera;

  int m_exposure;
  int m_gain;
  int m_brightness;

public:
  // default constructor
  ImageProcessor() :
    // default settings, most can be modified through command line options (see below)
    m_tagDetector(NULL),
    m_tagCodes(AprilTags::tagCodes36h11),

    m_draw(false),
    m_timing(false),
    m_camera("NONE"),

    m_width(640),
    m_height(480),
    m_tagSize(0.163),
    m_fx(600),
    m_fy(600),
    m_px(m_width/2),
    m_py(m_height/2),
    m_exposure(-1),
    m_gain(-1),
    m_brightness(-1)
  {}

  // changing the tag family
  void setTagCodes(string s) {
    if (s=="16h5") {
      m_tagCodes = AprilTags::tagCodes16h5;
    } else if (s=="25h7") {
      m_tagCodes = AprilTags::tagCodes25h7;
    } else if (s=="25h9") {
      m_tagCodes = AprilTags::tagCodes25h9;
    } else if (s=="36h9") {
      m_tagCodes = AprilTags::tagCodes36h9;
    } else if (s=="36h11") {
      m_tagCodes = AprilTags::tagCodes36h11;
    } else {
      cout << "Invalid tag family specified" << endl;
      exit(1);
    }
  }

  // parse command line options to change default behavior
  void parseOptions(int argc, char* argv[]) {
    int c;
    while ((c = getopt(argc, argv, ":h?dbltC:F:H:S:W:E:G:B:")) != -1) {
      // Each option character has to be in the string in getopt();
      // the first colon changes the error character from '?' to ':';
      // a colon after an option means that there is an extra
      // parameter to this option; 'W' is a reserved character
      switch (c) {
      case 'h':
      case '?':
        cout << intro;
        cout << usage;
        exit(0);
        break;
      // Draw the detections
      case 'd':
        m_draw = true;
        break;
      // Using the bumblebee camera
      case 'b':
        m_fx = 811.857711829874;
        m_fy = 811.857711829874;
        m_px = 515.7504920959473;
        m_py = 403.7249565124512;
        m_camera = "bumblebee";
        break;
      // Using the left hand camera
      case 'l':
        m_fx = 406.91154295;
        m_fy = 406.91154295;
        m_px = 639.265369663;
        m_py = 393.084656577;
        m_camera = "leftHandCamera";
        break;
      // Display the time of extractions
      case 't':
        m_timing = true;
        break;
      case 'C':
        setTagCodes(optarg);
        break;
      case 'F':
        m_fx = atof(optarg);
        m_fy = m_fx;
        break;
      case 'S':
        m_tagSize = atof(optarg);
        break;
      case 'W':
        m_width = atoi(optarg);
        m_px = m_width/2;
        break;
      case 'E':
#ifndef EXPOSURE_CONTROL
        cout << "Error: Exposure option (-E) not available" << endl;
        exit(1);
#endif
        m_exposure = atoi(optarg);
        break;
      case 'G':
#ifndef EXPOSURE_CONTROL
        cout << "Error: Gain option (-G) not available" << endl;
        exit(1);
#endif
        m_gain = atoi(optarg);
        break;
      case 'B':
#ifndef EXPOSURE_CONTROL
        cout << "Error: Brightness option (-B) not available" << endl;
        exit(1);
#endif
        m_brightness = atoi(optarg);
        break;
      case ':': // unknown option, from getopt
        cout << intro;
        cout << usage;
        exit(1);
        break;
      }
    }

    if (argc > optind) {
      for (int i=0; i<argc-optind; i++) {
        m_imgNames.push_back(argv[optind+i]);
      }
    }
  }

  void setup() {
    m_tagDetector = new AprilTags::TagDetector(m_tagCodes);
  }

  void write_transform(ofstream& file, AprilTags::TagDetection& detection, double dt) const {
    // recovering the relative pose of a tag:

    // NOTE: for this to be accurate, it is necessary to use the
    // actual camera parameters here as well as the actual tag size
    // (m_fx, m_fy, m_px, m_py, m_tagSize)

    Eigen::Matrix4d transformation;
    if (detection.id == 21) {
        cout << m_fx << endl;
        cout << m_fy << endl;
        cout << m_px << endl;
        cout << m_py << endl;
        transformation = detection.getRelativeTransform(.08, m_fx, m_fy, m_px, m_py);
    } else {
        cout << m_fx << endl;
        cout << m_fy << endl;
        cout << m_px << endl;
        cout << m_py << endl;
        cout << m_tagSize << endl;
        transformation = detection.getRelativeTransform(m_tagSize, m_fx, m_fy, m_px, m_py);
    }
    for (int i = 0; i < transformation.size(); i++) {
            file << *(transformation.data() + i) << endl;
    }
  }

  void processImage(ofstream& file, cv::Mat& image, cv::Mat& image_gray, string imageName) {
    // detect April tags (requires a gray scale image)
    cv::cvtColor(image, image_gray, CV_BGR2GRAY);

    double dt;
    vector<AprilTags::TagDetection> detections;

    double t0;
    if (m_timing) {
      t0 = tic();
    }
    detections = m_tagDetector->extractTags(image_gray);
    if (m_timing) {
        dt = tic() - t0;
        cout << imageName << " time: " << dt << endl;
    }

    for (int i=0; i<detections.size(); i++) {
        cout << "Writing TagID: " << detections[i].id << endl;
        write_transform(file, detections[i], dt);
    }

    // show the current image including any detections
    if (m_draw) {
        for (int i=0; i<detections.size(); i++) {
        // also highlight in the image
          detections[i].draw(image);
        }
        imshow(windowName, image); // OpenCV call
    }
  }

  // Load and process a single image
  void loadImages() {
    cv::Mat image;
    cv::Mat image_gray;

    string fileName = m_camera + ".txt";
    ofstream file(fileName.c_str());
    for (list<string>::iterator it=m_imgNames.begin(); it!=m_imgNames.end(); it++) {
        image = cv::imread(*it); // load image with opencv
        processImage(file, image, image_gray, *it);
        if (m_draw) {
            while (cv::waitKey(100) == -1) {}
        }
    }
    file.close();
  }

}; // ImageProcessor


// here is where everything begins
int main(int argc, char* argv[]) {
  ImageProcessor imageProcessor;
  imageProcessor.parseOptions(argc, argv);
  imageProcessor.setup();
  imageProcessor.loadImages();
  return 0;
}

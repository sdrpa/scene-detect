#include <iostream>
#include <string>
#include <unistd.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>

#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>

void print_usage(char *name) {
   fprintf(stderr, "Usage: %s [-t threshold] [-n nth_second] FILE\n", name);
   std::cout << "Extract frames from a video file (looking for changes in content)" << std::endl;
   std::cout << "" << std::endl;
   std::cout << "   -t difference threshold in range 0...1 (default 0.3)" << std::endl;
   std::cout << "   -n every nth second (default 1, every second)" << std::endl;
   std::cout << "" << std::endl;
}

int main(int argc, char * argv[]) {
   int opt;
   std::string input = "";
   int nth_sec = 1;
   float diff_threshold = 0.3;

   // Retrieve the (non-option) argument:
   if ((argc <= 1) || (argv[argc-1] == NULL) || (argv[argc-1][0] == '-')) {  // NO input...
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
   }
   else {
      input = argv[argc-1];
   }
   // Shut GetOpt error messages down (return '?'): 
   opterr = 0;

   // Retrieve the options:
   while ((opt = getopt(argc, argv, "n:t:")) != -1) {
      switch (opt) {
         case 'n':
            nth_sec = atoi(optarg);
            break;
         case 't':
            diff_threshold = atof(optarg);
            break;
         default: /* '?' */
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
      }
   }
   
   std::cout << "OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << std::endl;   
   cv::VideoCapture cap(input);

   double fps = cap.get(cv::CAP_PROP_FPS);
   uint64_t frame_count = cap.get(cv::CAP_PROP_FRAME_COUNT);

   std::cout << "Filename: " << input << std::endl;
   std::cout << "Frame count: " << frame_count << ". FPS: " << fps << std::endl;
   std::cout << "Nth second: " << nth_sec << std::endl;
   std::cout << "Diff threshold: " << diff_threshold << std::endl;

   if (!cap.isOpened()) {
      std::cout << "Cannot open the video file.\n";
      return -1;
   }

   cv::Mat previous;
   cap.set(cv::CAP_PROP_POS_FRAMES, 0);
   if (!cap.read(previous)) {
      std::cout << "Failed to extract the first frame.\n";
      return -1;
   }

   float progress = 0.0;
   int bar_width = 70;
   
   int nth_frame = (uint8_t)fps * nth_sec;
   for (int fi = 1; fi < frame_count; fi++) { // fi = frame index; cap.get(cv::CAP_PROP_FRAME_COUNT)
      // Progress
      std::cout << "[";
      int pos = bar_width * progress;
      for (int i = 0; i < bar_width; ++i) {
         if (i < pos) std::cout << "=";
         else if (i == pos) std::cout << ">";
         else std::cout << " ";
      }
      std::cout << "] " << int(progress * 100.0) << " %\r";
      std::cout.flush();
      progress = (float)fi / (float)frame_count;

      if (fi % nth_frame) {
         continue;
      }
      //std::cout << fi << std::endl;
      
      cap.set(cv::CAP_PROP_POS_FRAMES, fi);
      cv::Mat current;
      if (!cap.read(current)) {
         std::cout << "Failed to extract a frame.\n";
         return -1;
      }

      // Step 1: Detect the keypoints using SURF Detector, compute the descriptors
      int minHessian = 400;
      cv::Ptr<cv::xfeatures2d::SURF> detector = cv::xfeatures2d::SURF::create(minHessian);
      std::vector<cv::KeyPoint> keypoints1, keypoints2;
      cv::Mat descriptors1, descriptors2;
      detector->detectAndCompute(previous, cv::noArray(), keypoints1, descriptors1);
      detector->detectAndCompute(current, cv::noArray(), keypoints2, descriptors2);
      
      // Step 2: Matching descriptor vectors with a FLANN based matcher
      // Since SURF is a floating-point descriptor NORM_L2 is used
      cv::Ptr<cv::DescriptorMatcher> matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
      std::vector< std::vector<cv::DMatch> > knn_matches;
      try {
         matcher->knnMatch(descriptors1, descriptors2, knn_matches, 2);
      } catch (cv::Exception e ) {
         continue;
      }

      // Filter matches using the Lowe's ratio test
      const float ratio_thresh = 0.7f;
      std::vector<cv::DMatch> good_matches;
      for (size_t i = 0; i < knn_matches.size(); i++) {
         if (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance) {
            good_matches.push_back(knn_matches[i][0]);
         }
      }
      // std::cout << good_matches.size() << std::endl;

      // How good it's the match
      size_t num_keypoints = keypoints1.size() > keypoints2.size() ? keypoints1.size() : keypoints2.size();
      double percent = (double)good_matches.size() / (double)num_keypoints;
      //std::cout << fi << ": " << (percent * 100.0) << "%" << std::endl;

      if (percent < diff_threshold) {
         std::string image_path = "frames/" + std::to_string(fi) + ".jpg"; 
         cv::imwrite(image_path, current);
      }
      
      previous = current;
   }
   std::cout << std::endl;

   return 0;
}

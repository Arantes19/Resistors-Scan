#include <iostream>
#include <string>
#include <chrono>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}

void vc_timer(void) {
    static bool running = false;
    static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

    if (!running) {
        running = true;
    }
    else {
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        std::chrono::steady_clock::duration elapsedTime = currentTime - previousTime;

        // Time in seconds.
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(elapsedTime);
        double nseconds = time_span.count();

        std::cout << "Elapsed time: " << nseconds << " seconds" << std::endl;
        std::cout << "Press any key to continue...\n";
        std::cin.get();
    }
}

int main(void) {
    // Video
    char videofile[20] = "video_resistors.mp4";
    cv::VideoCapture capture;
    struct
    {
        int width, height;
        int ntotalframes;
        int fps;
        int nframe;
    } video;

    int counter = 0;
    // Others
    std::string str;
    int key = 0;

    /* Reading video from a file */
    /* IMPORTANT NOTE:
    The video.avi file should be located in the same directory as the source code file.
    */
    capture.open(videofile);

    /* Alternatively, open video capture from Webcam #0 */
    //capture.open(0, cv::CAP_DSHOW); // You can also use just capture.open(0);

    /* Check if it was possible to open the video file */
    if (!capture.isOpened()) {
        std::cerr << "Error opening video file!\n";
        return 1;
    }

    /* Total number of frames in the video */
    video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    /* Video frame rate */
    video.fps = (int)capture.get(cv::CAP_PROP_FPS);
    /* Video resolution */
    video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

    /* Create a window to display the video */
    cv::namedWindow("VC - VIDEO", cv::WINDOW_AUTOSIZE);
    //cv::namedWindow("Segmented", cv::WINDOW_AUTOSIZE);

    /* Start the timer */
    vc_timer();

    cv::Mat frame;
    while (key != 'q') {
        /* Read a frame from the video */
        capture.read(frame);

        /* Check if the frame was read successfully */
        if (frame.empty()) break;

        cv::GaussianBlur(frame, frame, cv::Size(9, 9), 0);

        /* Frame number being processed */
        video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

        /* Example of inserting text into the frame */
        str = std::string("Resolution: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
        cv::putText(frame, str, cv::Point(20, 900), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 900), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("Total Frames: ").append(std::to_string(video.ntotalframes));
        cv::putText(frame, str, cv::Point(20, 925), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 925), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("Frame rate: ").append(std::to_string(video.fps));
        cv::putText(frame, str, cv::Point(20, 950), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 950), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        str = std::string("Frame Number: ").append(std::to_string(video.nframe));
        cv::putText(frame, str, cv::Point(20, 975), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 975), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        // Create images
        IVC* image0 = vc_image_new(video.width, video.height, 3, 255);

        // Copy image data from the cv::Mat structure to an IVC structure
        memcpy(image0->data, frame.data, video.width * video.height * 3);

        vc_convert_bgr_to_rgb(image0);

        IVC* image1 = vc_image_new(video.width, video.height, 3, 255);
        vc_rgb_to_hsv(image0, image1);

        IVC* image2 = vc_image_new(video.width, video.height, 1, 255);
        vc_hsv_segmentation(image1, image2); // resistor color here

        IVC* image3 = vc_image_new(video.width, video.height, 1, 255);
        vc_binary_close(image2, image3, 3, 3);

        OVC* blobs = nullptr;
        int nblobs = 0;
        int count;
        int min_x1, max_x1, min_y1, max_y1;
        int min_x2, max_x2, min_y2, max_y2;
        int min_x3, max_x3, min_y3, max_y3;

        IVC* image4 = vc_image_new(video.width, video.height, 1, 255);

        blobs = vc_binary_blob_labelling(image3, image4, &nblobs);

        vc_binary_blob_info(image4, blobs, nblobs);

        /* Three drawings were made to avoid creating a border between two resistors */
        count = draw_resistor_box_1(image4, image0, blobs, nblobs, video.width, video.height, &min_x1, &max_x1, &min_y1, &max_y1); // Top part of the image, <=30% of the height 
        draw_resistor_box_2(image4, image0, blobs, nblobs, video.width, video.height, &min_x2, &max_x2, &min_y2, &max_y2, count); // Middle part of the image, >= 30% & <= 70% of the height
        draw_resistor_box_3(image4, image0, blobs, nblobs, video.width, video.height, &min_x3, &max_x3, &min_y3, &max_y3, count); // Bottom part of the image, >= 70% of the height

        IVC* image5 = vc_image_new(video.width, video.height, 1, 255);
        vc_color_segmentation(image1, image5, max_y1, min_y1, max_x1, min_x1);
        vc_color_segmentation(image1, image5, max_y2, min_y2, max_x2, min_x2);
        vc_color_segmentation(image1, image5, max_y3, min_y3, max_x3, min_x3);

        OVC* colorblobs = nullptr;
        int ncolorblobs = 0;
        int result;

        IVC* image6 = vc_image_new(video.width, video.height, 1, 255);
        colorblobs = vc_binary_blob_labelling(image5, image6, &ncolorblobs);

        IVC* image7 = vc_image_new(video.width, video.height, 1, 255);

        result = vc_color_calculator(image5, image7, &ncolorblobs, &min_x1, &max_x1, &min_y1, &max_y1, count);

        /*cv::Mat seg_frame(video.height, video.width, CV_8UC1, image5->data);

        cv::Mat color_seg_frame;
        cv::cvtColor(seg_frame, color_seg_frame, cv::COLOR_GRAY2BGR);*/

        rgb_to_bgr(image0);
        memcpy(frame.data, image0->data, video.width * video.height * 3);

        if (result != 0) {
            std::string str = "Resistance Value " + std::to_string(count) + ": " + std::to_string(result) + " Ohms";
            cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
            cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);
        }
        str = std::string("Number of Resistors: ").append(std::to_string(count));
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
        cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

        // Release the memory of the created IVC images
        vc_image_free(image0);
        vc_image_free(image1);
        vc_image_free(image2);
        vc_image_free(image3);
        vc_image_free(image4);
        vc_image_free(image5);
        vc_image_free(image6);
        vc_image_free(image7);

        // Free the memory of the labeled resistor
        free(blobs);

        // Display the original frame
        cv::imshow("VC - VIDEO", frame);
        //cv::imshow("Segmented", color_seg_frame);

        /* Exit the application if the user presses the 'q' key */
        key = cv::waitKey(1);
    }

    /* Stop the timer and display the elapsed time */
    vc_timer();

    // Close the windows
    cv::destroyWindow("VC - VIDEO");
    //cv::destroyWindow("Segmented");
    /* Close the video file */
    capture.release();

    return 0;
}

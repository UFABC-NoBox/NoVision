#include "bDetect.hpp"
#include <chrono>

using ms = std::chrono::milliseconds;
using tick = std::chrono::steady_clock;

int main(int argc, char *argv[]){
  //loading and testing json file
  Json::Value root;
  std::ifstream Config("../../../inc/configs.json");
  Config >> root;
  std::cout << "teste" << std::endl;
  cv::Size fieldSize(root.get("fieldSize", 0)[0].asInt(),
                     root.get("fieldSize", 0)[1].asInt() );
  cv::Size fieldSizeMM(root.get("fieldSizeMM", 0)[0].asInt(),
                              root.get("fieldSizeMM", 0)[1].asInt() );

  int circleWarpSize = root.get("circleWarpSize", 0).asInt();
  // Initialize Static Masks
  // This mask is used to exclude outer area of circles detected in HoughCircles
  //
  cv::Mat circleMask(circleWarpSize,circleWarpSize,CV_8UC1,cv::Scalar(1,1,1));
  int halfMaskSize = circleWarpSize / 2;
  cv::circle(circleMask,
          //center
          cv::Point(circleWarpSize / 2, circleWarpSize / 2),
          //radius
          (int)(circleWarpSize / 2.5),
          cv::Scalar(255,255,255), -1, 8 , 0 );

  // Constant that multiplied by a unit in mm get the size in pc
  float FIELD_MM = (float)(fieldSize.width) / (float)(fieldSizeMM.width);

  // Plotting (DEBUG)
  int plotWidth = int(fieldSize.width * 2);
  int plotHeigth = int(fieldSize.height * 2);
  std::vector<int> plots;
  //plotsLg = None;
  //plot = np.zeros((plotHeigth, plotWidth, 3), np.uint8)
  //cv2.resizeWindow("plots", plotWidth, plotHeigth)

  changeCameraProp("focus_auto", "0", root);
  changeCameraProp("exposure_auto_priority", "0", root);
  changeCameraProp("exposure_auto", "1", root);
  changeCameraProp("exposure_absolute", "75", root);
  createTrackBars();

  std::string camera = root.get("camera", 1).asString();
  char capNumber = camera.at(10) - 48;
  cv::VideoCapture cap(capNumber); // open the default camera
  if (!cap.isOpened())  // check if we succeeded
    return -1;

  cap.set(3, root.get("cameraRes",0)[0].asInt());
  cap.set(4, root.get("cameraRes",0)[1].asInt());
  cv::Size frameSize(cap.get(3), cap.get(4) );



  cv::Scalar color_rgb(0,0,0);
  fieldCorners[0] = cv::Point2f(root.get("fieldCorners",0)[0][0].asFloat(),
                                      root.get("fieldCorners",0)[0][1].asFloat() );
  fieldCorners[1] = cv::Point2f(root.get("fieldCorners",0)[1][0].asFloat(),
                                      root.get("fieldCorners",0)[1][1].asFloat() );
  fieldCorners[2] = cv::Point2f(root.get("fieldCorners",0)[2][0].asFloat(),
                                      root.get("fieldCorners",0)[2][1].asFloat() );
  fieldCorners[3] = cv::Point2f(root.get("fieldCorners",0)[3][0].asFloat(),
                                      root.get("fieldCorners",0)[3][1].asFloat() );

  cv::Scalar colors[6] = {cv::Scalar( root.get("colors",0)["blue"][0].asInt() ,
                    root.get("colors",0)["blue"][1].asInt() ,
                    root.get("colors",0)["blue"][2].asInt() ),
       cv::Scalar( root.get("colors",0)["green"][0].asInt() ,
                    root.get("colors",0)["green"][1].asInt() ,
                    root.get("colors",0)["green"][2].asInt() ),
       cv::Scalar( root.get("colors",0)["orange"][0].asInt() ,
                    root.get("colors",0)["orange"][1].asInt() ,
                    root.get("colors",0)["orange"][2].asInt() ),
       cv::Scalar( root.get("colors",0)["purple"][0].asInt() ,
                    root.get("colors",0)["purple"][1].asInt() ,
                    root.get("colors",0)["purple"][2].asInt() ),
       cv::Scalar( root.get("colors",0)["red"][0].asInt() ,
                    root.get("colors",0)["red"][1].asInt() ,
                    root.get("colors",0)["red"][2].asInt() ),
       cv::Scalar( root.get("colors",0)["yellow"][0].asInt() ,
                    root.get("colors",0)["yellow"][1].asInt() ,
                    root.get("colors",0)["yellow"][2].asInt()),
                  };

  cv::Mat warpMatrix =  fieldCornersUpdated(fieldCorners, frameSize);
  std::vector<double> times(10);
  cv::Mat bin, hsv, tgt;
  //Create a window
  cv::namedWindow("Frame", CV_WINDOW_AUTOSIZE);
  //set the callback function for any mouse event
  cv::setMouseCallback("Frame", CallBackFunc, NULL);

  for(;;){
    auto start_total = tick::now();
    /*
    if(k == ord('s')):
      if(SHOW_DISPLAY is True):
        SHOW_DISPLAY = False;
      else:
        SHOW_DISPLAY = True;

    #
    # Check for "calibrate ball" action
    #
    if (k == ord("b")):
      actionCalibrateBall();

    #
    # Check for calibrate color action
    #
    if(k == ord('c')):
      actionConfigureColors();

    t = Timing();
    plots = [];
    plotsLg = None;

  #############################################################################################################

  #############################################################################################################


    # List where all objects get stored
    objects = [];

    # Capture frame
    t.start("capture")
    ret, frame = cap.read()
    height, width = frame.shape[:2];
    # plots.append(frame);
    out = frame;
    t.end();

    #
    # Generate Perspective Transform
    #
    t.start("wrap");
    field = cv2.warpPerspective(out, warpMatrix, tuple(fieldSize));
    plots.append(field);
    out = field;
    t.end();
    */
    //===============================================================================
    auto start = tick::now();
    cap >> Gframe; // get a new frame from camera
    cv::Mat warpedFrame = Gframe.clone();//trocar pelo tamanho
    cv::warpPerspective(Gframe,warpedFrame,warpMatrix,Gframe.size(),cv::INTER_NEAREST,
                        cv::BORDER_CONSTANT, cv::Scalar() );


    Gframe = warpedFrame;
    warpedFrame.release();
    auto finish = tick::now();
    auto diff = finish - start;
    times[0] = (double)std::chrono::duration_cast<ms>(diff).count();

    //===============================================================================
    start = tick::now();
    colorDetection(Gframe, bin,hsv,tgt, colors , 3);
    std::vector<std::vector<cv::Point> > contours;
    std::vector<cv::Vec4i> hierarchy;
    //std::vector<cv::Vec3f> circles;

    findPos(bin, Gframe, contours, hierarchy, root, FIELD_MM);
    finish = tick::now();
    diff = finish - start;
    times[1] = (double)std::chrono::duration_cast<ms>(diff).count();
    //===============================================================================
    start = tick::now();
    cv::Mat robots;
    std::vector<cv::Vec3f> circles;
    detectCircles(Gframe,robots, circles, root,FIELD_MM);
    finish = tick::now();
    diff = finish - start;
    times[2] = (double)std::chrono::duration_cast<ms>(diff).count();
    //===============================================================================


    cv::imshow("Frame",Gframe);

    start = tick::now();
    int k = cv::waitKey(1);
    //int k = 0;
    finish = tick::now();
    diff = finish - start;
    times[3] = (double)std::chrono::duration_cast<ms>(diff).count();
    if (k == 27 || k == 'q')
      break;
    else if(k == 'f'){
      state = 'f';
      actionPickCorners(cap, root);
      warpMatrix = fieldCornersUpdated(fieldCorners, frameSize);
      cv::warpPerspective(Gframe,warpedFrame,warpMatrix,fieldSize,cv::INTER_LINEAR,
                          cv::BORDER_CONSTANT, cv::Scalar() );
    }else if(k == 'c'){
      state = 'c';
      actionConfigureColors(cap,root);
    }else if(k == 's') {
      //função de escrever no json
      saveInJson(root);
    }

    cv::imshow("Bola", tgt);
    cv::imshow("HSV", hsv);

    auto finish_total = tick::now();
    diff = finish_total - start_total;
    double time_total = (double)std::chrono::duration_cast<ms>(diff).count();

    std::cout <<"Tempo total: " << time_total << "\tLendo Frame: " << times[0] << "\tDetectando a bolinha: " <<
    times[1] << "\tDetectando os robos: " << times[2] << "\tRestante: " <<
    times[3] << std::endl;

  }
  return 0;
}

#include <dlfcn.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "opencv2/opencv.hpp"
#include "decoder.h"

unsigned int gframeIndex = 0;
Decoder *loadPlugin(const char *path)
{
    void *handle = dlopen(path,RTLD_LAZY);
    if(!handle){
        std::cout << dlerror() << std::endl;
        return nullptr;
    }

    dlerror();
    typedef Decoder*(*CreatePlugin)();
    CreatePlugin createFun = dlsym(handle,"createDecoder");

    char *error = nullptr;
    if((error = dlerror()) != nullptr){
        std::cout << error << std::endl;
        return nullptr;
    }
    return createFun();
}

void onFrameDecoded(AVPixelFormat format,unsigned char *ptr,int w,int h)
{
    cv::Mat frame;
    switch (format) {
    case AV_PIX_FMT_NV12:
        frame = cv::Mat(3 * h / 2, w, CV_8UC1, ptr);
        cv::cvtColor(frame, frame, CV_YUV2BGR_NV12);
        break;
    case AV_PIX_FMT_YUV420P:
        frame = cv::Mat(3 * h / 2, w, CV_8UC1, ptr);
        cv::cvtColor(frame, frame, CV_YUV2BGR_I420);
        break;
    default:
        break;
    }

    cv::imwrite("images/" + std::to_string(gframeIndex++) + ".jpg",frame);
    cv::imshow("rtsVieo",frame);
    cv::waitKey(1);
}

int main(int argc, char **argv)
{
    if(argc != 3){
        std::cout << "UseAge: " << argv[0] << " decoder(fmgDecoder or nvDecoder) rtsp" << std::endl;
        return -1;
    }
    std::string pluginPath("decoder_plugins/");
    if(!::strcmp(argv[1],"fmgDecoder")){
        pluginPath += "libFmgDecoder.so";
    }else if(!::strcmp(argv[1],"nvDecoder")){
        pluginPath += "libNvidiaDecoder.so";
    }
    Decoder *rtspDecoder = loadPlugin(pluginPath.data());
    if(!rtspDecoder){
        return -1;
    }
    bool isOk = rtspDecoder->initsize();
    if(!isOk)return -1; //rtsp://admin:123ABCabc@192.168.2.242/h264/ch1/main/av_stream

    while(true){
        std::string errorStr;
        isOk = rtspDecoder->decode(argv[2],errorStr,onFrameDecoded);

        if(!isOk){
            std::cout << errorStr << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    return 0;
}

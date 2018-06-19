#include "opencv2/opencv.hpp"
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace cv;


int main(int argc, char** argv)
{


    
    //--------------------------------------------------------
    //networking stuff: socket , connect
    //--------------------------------------------------------
    int         sokt;
    char*       serverIP;
    int         serverPort;

    int         Wd=1920;
    int         Hd=1080;

    if (argc != 4) {
           std::cerr << "Usage: cv_video_client <serverIP> <serverPort> <PathToCascade>" << std::endl;
    }
    
    std::string fn_bus =  std::string(argv[3]);
    CascadeClassifier bus_cascade;
    bus_cascade.load(fn_bus);
    //Получить IP из аргумента.
    serverIP   = argv[1]; // Просто адрес
    hostent *record = gethostbyname(argv[1]); // или имя

    serverPort = atoi(argv[2]);

    struct  sockaddr_in serverAddr;
            socklen_t   addrLen = sizeof(struct sockaddr_in);

    //Получаем дескриптор сокета типа SOCK_STREAM (TCP сервис или потоковый сокет) в семействе протоколов PF_INET (Сетевой протокол IPv4)
    if ((sokt = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket() failed" << std::endl;
    }

    serverAddr.sin_family = PF_INET;
    //serverAddr.sin_addr.s_addr = inet_addr(serverIP); //Если в аргументе адрес
    serverAddr.sin_addr.s_addr = ((in_addr*)record->h_addr_list[0])->s_addr; //Если в аргументе имя
    serverAddr.sin_port = htons(serverPort);

    if (connect(sokt, (sockaddr*)&serverAddr, addrLen) < 0) {
        std::cerr << "connect() failed! " << inet_addr(serverIP) << std::endl;
        exit(1);
    }



    //----------------------------------------------------------
    //OpenCV Code
    //----------------------------------------------------------

    Mat img;
    img = Mat::zeros(Hd , Wd, CV_8UC3);    
    int imgSize = img.total() * img.elemSize();
    uchar *iptr = img.data;
    int bytes = 0;
    int key;
    char answer[2];
    strcpy(answer,"OK");

    //make img continuos
    if ( ! img.isContinuous() ) { 
          img = img.clone();
    }

    //namedWindow("CV Video Client",1);
    //std::cerr << "recv failed, received bytes = " << bytes << std::endl;
    while (true) {
        int failCounter = 0;
        std::cout << "Image Size:" << imgSize << std::endl;
        if ((bytes = recv(sokt, iptr, imgSize , MSG_WAITALL)) == -1) {
            std::cerr << "recv failed, repeat in " << 2 * failCounter << " seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(2000*failCounter));
            if (failCounter < 5) {
                failCounter++;
            }
            continue;
        }
        failCounter = 0;
        
//        Mat gray;
        Mat dst;
        Point2f pc(img.cols/2., img.rows/2.);
        Mat r = cv::getRotationMatrix2D(pc, -25, 1.0);
        warpAffine(img, dst, r, img.size()); // what size I should use?
        
        Rect region_of_interest = Rect(4,212,1534,314);
        Mat image_roi = dst(region_of_interest);


//        cvtColor(image_roi, gray, COLOR_BGR2GRAY);
//        imwrite("rotated_im.png", gray);
        Mat r = cv::getRotationMatrix2D(pc, 25, 1.0);
        warpAffine(img, dst, r, img.size());
        std::vector< Rect_<int> > bus;


        bus_cascade.detectMultiScale(image_roi, bus);
        
        for(int i = 0; i < bus.size(); i++) {
            Rect bus_i = bus[i];
            //Mat one_bus = gray(bus_i);
            rectangle(dst, bus_i, CV_RGB(0, 255,0), 1);
        }
//                Эмуляция обработки
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        std::time_t t = std::time(0);
        //std::string(t)
        std::stringstream ss;
        ss << "test" << t << ".jpg";
        imwrite(ss.str(), dst);
 /*       
        cv::Mat dst;
        cv::Point2f pc(img.cols/2., img.rows/2.);
        cv::Mat r = cv::getRotationMatrix2D(pc, -25, 1.0);
        cv::warpAffine(img, dst, r, img.size()); // what size I should use?
        cv::imwrite("rotated_im.png", dst);
*/    
        send(sokt, answer, 2, 0);
    }   

    close(sokt);

    return 0;
}  

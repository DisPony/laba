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

    if ( ! img.isContinuous() ) { 
          img = img.clone();
    }

    while (true) {
        ofstream fout;
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
        std::time_t t = std::time(0);
        std::vector< Rect_<int> > bus;
        bus_cascade.detectMultiScale(img, bus);

        fout.open("buses", ios_base::app);
        for(int i = 0; i < bus.size(); i++) {
            std::stringstream ss;
            Rect bus_i = bus[i];
            ss << t << " " << bus_i.x << " " << bus_i.y << " " <<bus_i.height << " " << bus_i.width << std::endl;
        }
        fout << ss.str();
        fout.close();
        ss << "test" << t << ".jpg";
        imwrite(ss.str(), img);
        send(sokt, answer, 2, 0);
    }   

    close(sokt);

    return 0;
}  

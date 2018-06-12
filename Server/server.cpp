//
// Created by Никита on 10.06.2018.
//

#include "opencv2/opencv.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
//#include <termios.h>
#include <time.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>
#include <future>
#include <functional>



using namespace cv;
using namespace std;
using namespace std::placeholders;

void *display(void *);
void *getvideo(void *);
string type2str(int type);

void removeThread(std::thread::id id);


VideoCapture camera(0);
Mat frame, image;


//Многопоточность
std::condition_variable m_condVar;
std::mutex m_mutex;
#define THREAD_MAX  8
std::vector<std::thread> threads(THREAD_MAX);
std::map<std::thread::id, int> Locks;











int main(int argc, char** argv)
{

    //--------------------------------------------------------
    //networking stuff: socket, bind, listen
    //--------------------------------------------------------
    int     localSocket,
            remoteSocket,
            port = 4097;

    struct  sockaddr_in localAddr,
            remoteAddr;

    pthread_t thread_id, thread_getvideo;



    int addrLen = sizeof(struct sockaddr_in);


//Start listen
    if ( (argc > 1) && (strcmp(argv[1],"-h") == 0) ) {
        std::cerr << "usage: ./cv_video_srv [port] [capture device]\n" <<
                  "port           : socket port (4097 default)\n" <<
                  "capture device : (0 default)\n" << std::endl;

        exit(1);
    }

    if (argc == 2) port = atoi(argv[1]);

    localSocket = socket(AF_INET , SOCK_STREAM , 0);
    if (localSocket == -1){
        perror("socket() call failed!!");
    }

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons( port );

    if( bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
        perror("Can't bind() socket");
        exit(1);
    }

    //Listening
    listen(localSocket , 3);

    std::cout <<  "Waiting for connections...\n"
              <<  "Server Port:" << port << std::endl;
//Start capture serial
//              pthread_create(&thread_getvideo,NULL,getvideo,NULL);
    std::thread t_getvideo(getvideo, &remoteSocket);




    //accept connection from an incoming client
    while(1){

        remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr,  (socklen_t*)&addrLen);
        std::cout << remoteSocket<< "32"<< std::endl;
        if(Locks.size()>THREAD_MAX) {
            std::cout << "max_thread error > "<< THREAD_MAX<< std::endl;
            close(remoteSocket);
        }

        if (remoteSocket < 0) {
            //       perror("accept failed!");
            //       exit(1);
        }
        else{
            std::cout << "Connection accepted" << std::endl;

            threads.push_back(std::thread (display, &remoteSocket));
        }

    }
    std::cout << "End" << std::endl;
    return 0;
}

void *display(void *ptr){

//Функция отдает в сокет картинку
    
char answer[2];

    int socket = *(int *)ptr;
    //Помещаем в map id нового потока
    Locks.insert(std::pair<std::thread::id, int>(std::this_thread::get_id(),0));

    std::unique_lock<std::mutex> mlock(m_mutex);
    //OpenCV Code
    //----------------------------------------------------------



    while(1) {
    int imgSize = frame.total() * frame.elemSize();
    int bytes = 0;
std::cerr << "Send sock "<< std::this_thread::get_id() << std::endl;
std::cerr << "Send size "<< imgSize << std::endl;
if( imgSize == 6220800) { //Если кадр не равен ожидаемому, то пропускаем цикл
        //send processed image
        if ((bytes = send(socket, frame.data, imgSize, 0)) < 0 || (bytes != imgSize)){
            //   что-то пошло не так и мы не смогли записать в сокет. закрываем поток и чистим за собой.
            threads.push_back(
                    std::thread([]() {
                        std::async(removeThread, std::this_thread::get_id());
                    })
            );
            Locks.erase(Locks.find(std::this_thread::get_id()));
               std::cerr << "Break "<< std::this_thread::get_id() << std::endl;
            break;
        }
     
        
        //Ждем подтверждение обработки
        if ((bytes = recv(socket, answer, 2 , MSG_WAITALL)) == -1) {
            std::cerr << "recv failed, received bytes = " << bytes << std::endl;
        }
}        
        std::cerr << "Send  frame size "<< image.size() << std::endl;
        // Главный поток должен нам сказать, что есть новые данные.
        m_condVar.wait(mlock, [](){return Locks[std::this_thread::get_id()] == 1;});
        Locks[std::this_thread::get_id()] = 0;

    }

}

void *getvideo(void *ptr) {

    while(true) {
        //Крутим цикл, пока не откроем камеру
            while(!camera.isOpened()) {
            camera.open("http://video1.belrts.ru:9786/cameras/4/streaming/main.flv?authorization=Basic%20d2ViOndlYg%3D%3D");            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
           // std::cerr << "Camera not open " << std::endl;
            }
            
    
            camera >> frame;
            //Если получили пустой фрейм, то прерываем цикл и по новой подключаемся к камере
				if(frame.empty()) {
					break;
				}
                
                //Даем знать клиентам, что есть новые данные
                if(Locks.size()>0) { 
                    image = frame.clone();
                            std::cerr << "Send new frame to "<< Locks.size() << " clients" << std::endl;
                            std::cerr << "Img format "<< type2str(image.type()) << std::endl;
                            //std::cerr << "Img size "<< type2str(image.type)  std::endl;
                                    for (std::pair<std::thread::id, int> element : Locks) {
                                        Locks[element.first] = 1;
                                    }
                            

                                m_condVar.notify_all();

                }	
    }
}

void removeThread(std::thread::id id)
{
    // std::lock_guard<std::mutex> lock(threadMutex);
    auto iter = std::find_if(threads.begin(), threads.end(), [=](std::thread &t) { return (t.get_id() == id); });
    if (iter != threads.end())
    {
        iter->detach();
        threads.erase(iter);
    }
}

string type2str(int type) {
  string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

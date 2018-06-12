FROM debian
RUN apt-get update && \
apt-get install -y build-essential cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev python-dev python-numpy  libjpeg-dev libpng-dev libtiff5-dev  libdc1394-22-dev && \
git clone git://source.ffmpeg.org/ffmpeg.git ffmpeg && \
cd ffmpeg/ && \
./configure --enable-nonfree --enable-pic --enable-shared && \
make -j4 && \
make install && \
echo -n /usr/local/lib >> /etc/ld.so.conf && \
ldconfig && \
cd .. && \
git clone https://github.com/opencv/opencv.git && \
cd opencv && \
mkdir build && \
cd build && \
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local .. && \
make -j4 &&\
make install && \
rm -rf /opencv && \
rm -rf /ffmpeg && \
apt-get clean

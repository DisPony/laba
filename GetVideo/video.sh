#!/bin/sh
filename=$(date +%Y-%m-%d_%H-%M-%S)
echo $filename
while [ 1 ]; do
ffmpeg -i http://video1.belrts.ru:9786/cameras/4/streaming/main.flv?authorization=Basic%20d2ViOndlYg%3D%3D  \
-filter:v fps=fps=1/10 \
/data/new/$filename-%010d.png
done

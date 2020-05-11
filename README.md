Looks at the difference between each pair of adjacent (or n-th) frames, extracting a frame as an image when the difference exceeds a threshold value
(using OpenCV FLANN based matcher).

Compile:
```
g++ `pkg-config --cflags opencv` main.cpp `pkg-config --libs opencv` -o scene-detect
```

Usage: 
```
./scene-detect -t 0.4 -n 3 movie.mp4

scene-detect [-t threshold] [-n nth_second] FILE

  -t difference threshold in range 0...1 (default 0.3)
  -n every nth second (default 1, every second)
```

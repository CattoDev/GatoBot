# FFmpeg
GatoBot uses FFmpeg to render videos.

## Build process
### Windows version has been configured with the following flags:
```
./configure --arch=x86_64 --target-os=mingw32 --cross-prefix=x86_64-w64-mingw32- --disable-static --enable-shared --disable-stripping --enable-libx264 --extra-ldflags="-static-libgcc -static-libstdc++" --enable-gpl
```
Built using MinGW on Windows 10 x64

### Android version has been configured with [ffmpeg-android-maker](https://github.com/Javernaut/ffmpeg-android-maker) with the following flags:
```
./ffmpeg-android-maker
```
Built using WSL with Ubuntu 22.04.3 on Windows 10 x64

Official compilation guide [here](https://trac.ffmpeg.org/wiki/CompilationGuide).
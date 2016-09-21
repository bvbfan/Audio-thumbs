# Audio-thumbs
Preview of embed album art in audio file for Dolphin 5

## Requirements
* Plasma 5
* Qt 5.3+
* TagLib
* Extra CMake Modules (only for building)

### More Specifically for Ubuntu 16.04 Xenial
```
sudo apt-get install g++ cmake extra-cmake-modules qtbase5-dev kio-dev libtag1-dev
```

### More Specifically for OpenSuse
```
sudo zypper install extra-cmake-modules libQt5Core-devel kio-devel libtag-devel
```

## Compile and install
```
git clone https://github.com/bvbfan/Audio-thumbs
cd Audio-thumbs
mkdir build
cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIB_INSTALL_DIR=lib \
    -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
make
sudo make install
Configure Dolphin - General - Preview - Audio files


Special thanks to Vytautas Mickus who made Dolphin 4 version.

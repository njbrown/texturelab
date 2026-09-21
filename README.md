<h1 align="center">
  TextureLab
</h1>

<p align="center">
  <img src="https://github.com/njbrown/texturelab/workflows/Build/badge.svg" />
  <img src="https://img.shields.io/badge/License-GPLv3-blue.svg" />
  <a href="https://discord.gg/975NdQPsSc">
    <img src="https://img.shields.io/discord/769312171266932786.svg?logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2" alt="Discord invite" />
  </a><br/>
  Free, Cross-Platform, GPU-Accelerated Procedural Texture Generator.<br/>
  <a href="https://njbrown.itch.io/texturelab">DOWNLOAD AT ITCH.IO</a> | <a href="https://discord.gg/975NdQPsSc" >JOIN OUR DISCORD SERVER</a>
</p>

![Screenshot](https://user-images.githubusercontent.com/1708550/123368911-4ceb9f00-d542-11eb-87b5-b0fc3ea3cc3d.png)

# NOTE!

Texturelab will soon be converted to a qt project. All issues will be addressed after the conversion is complete.

## Building

Prerequisites

```
install Qt 6 and required dependencies

Note: Linux needs libmesa:
https://doc.qt.io/qt-6/linux.html

sudo apt install build-essential libgl1-mesa-dev libxkbcommon-dev libvulkan-dev libcurl4-openssl-dev

Note: libcurl4-openssl-dev is required by sentry-native (crash reporting).
Without it, CMake fails with "CURL: Required feature AsynchDNS is not found".

```

Building is done with `yarn`. Install it [here](https://classic.yarnpkg.com/en/docs/install) if you havent already.

```
git clone https://github.com/njbrown/texturelab.git
cd texturelab
git submodule update --init --recursive
```

Ensure Qt6 is added to your CMAKE_PREFIX_PATH env

generate build files:

```
cmake -G "Unix Makefiles"
```

build

```
make texturelab
```

## Windows Setup
If you dont want to or have MSVC installed, you can use g++ via MSYS2
```
winget install MSYS2.MSYS2
```
Then in MSYS2 UCRT64 terminal:
```
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake
```
Add `C:\msys64\ucrt64\bin` to your PATH

## Feedback

Got ideas, suggestions or feedback? Reach out to me on [twitter](https://twitter.com/njbrown92)

## Built Using

- **[Vue.js](https://vuejs.org)**
- **[THREE.js](https://threejs.org/)**
- **[Golden Layout](https://golden-layout.com/)** via **[vue-golden-layout](https://github.com/emedware/vue-golden-layout)**
- **[Electron](https://electronjs.org)**

## Licence

[GPLv3](https://github.com/njbrown/texturelab/blob/master/LICENSE)

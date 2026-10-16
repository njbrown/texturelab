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
  <a href="https://github.com/njbrown/texturelab/releases">DOWNLOAD</a> | <a href="https://njbrown.itch.io/texturelab">ITCH.IO</a> | <a href="https://discord.gg/975NdQPsSc" >JOIN OUR DISCORD SERVER</a>
</p>

![Screenshot](https://user-images.githubusercontent.com/1708550/123368911-4ceb9f00-d542-11eb-87b5-b0fc3ea3cc3d.png)

## Building

TextureLab is a C++17 / Qt 6 application built with CMake.

### Prerequisites

- **Qt 6** (6.6 or newer; CI uses 6.7.3) with the `qtshadertools` module. Install via the
  [Qt online installer](https://www.qt.io/download-qt-installer) or `aqtinstall`.
- **CMake** 3.16+ and a C++17 compiler (GCC, Clang, or MSVC 2022).
- **Linux** also needs:

  ```
  sudo apt install build-essential cmake libgl1-mesa-dev libxkbcommon-dev libvulkan-dev \
      libxcb-cursor0 libxcb-shape0-dev libcurl4-openssl-dev
  ```

  `libcurl4-openssl-dev` is required by sentry-native (crash reporting). Without it,
  CMake fails with "CURL: Required feature AsynchDNS is not found".

### Build

```
git clone https://github.com/njbrown/texturelab.git
cd texturelab
git submodule update --init --recursive

cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build build --target texturelab --parallel
./build/src/texturelab/texturelab
```

On Windows, use the Visual Studio generator (`-G "Visual Studio 17 2022" -A x64`) and point
`CMAKE_PREFIX_PATH` at e.g. `C:\Qt\6.x.x\msvc2019_64`. On macOS the app is built as
`build/src/texturelab/texturelab.app`.

Useful flags: `--no-launcher` opens straight into the editor; Debug builds enable theme
hot-reload (`--no-dev-theme` to turn it off).

Local builds don't send crash reports. To test Sentry, configure with
`-DTEXTURELAB_SENTRY_DSN=<dsn>`.

### Tests

```
cmake --build build --parallel
ctest --test-dir build
```

### Packaging

`scripts/package-linux.sh`, `scripts/package-macos.sh` and `scripts/package-windows.ps1`
produce the AppImage, DMG and zip that `.github/workflows/release.yml` publishes.

## Feedback

Got ideas, suggestions or feedback? Reach out to me on [twitter](https://twitter.com/njbrown92)

## Built Using

- **[Qt 6](https://www.qt.io)**
- **[Qt Advanced Docking System](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System)**
- **[sentry-native](https://github.com/getsentry/sentry-native)** with Crashpad
- **[TinyGLTF](https://github.com/syoyo/tinygltf)**

## Licence

[GPLv3](https://github.com/njbrown/texturelab/blob/master/LICENSE). Third-party components and
their licences are listed in [packaging/licenses](packaging/licenses/THIRD_PARTY_NOTICES.txt).

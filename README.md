# vulkanPbr


## Prerequisites
* [CMake](https://cmake.org/download/)
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) ([Installation guide](https://vulkan.lunarg.com/doc/sdk/latest/windows/getting_started.html))

### Optional
(Install these if you want to run `format.py` (runs clang-format on all source files))
* [Python](https://www.python.org/downloads/)
* [Clang](https://releases.llvm.org/download.html)


## Clone
Clone with `--recursive`
```
git clone --recursive https://github.com/amitshky/vulkanPbr.git
```


## Build and Run
* Configure and build the project with `-DVKPBR_USE_PRE_BUILT_LIB=OFF` to build the static libraries. This will also copy the libraries into the `binaries/` directory. You can then build with `-DVKPBR_USE_PRE_BUILT_LIB=ON` to use those libraries while building instead of building them from scratch.
```
cmake -B build -S . -DVKPBR_USE_PRE_BUILT_LIB=OFF
cmake --build build
```
* Then navigate to the output file and run it (run it from the root directory of the repo). For example,
```
./build/<path_to_executable>
```

OR (in VSCode)

* Start debugging (Press F5) (Currently configured for Clang with Ninja and MSVC for Windows) (NOTE: This will build with `-DVKPBR_USE_PRE_BUILT_LIB=ON`)

OR (using bat scripts from `scripts` folder)

* Run them from the root directory of the repo (NOTE: This will build with `-DVKPBR_USE_PRE_BUILT_LIB=ON`).
```
./scripts/config-clang-rel.bat
./scripts/build-clang.bat
./scripts/run-clang.bat
```

* To format all the source files according to `.clang-format` styles,
```
python format.py
```


## Usage
* WASD to move the camera forward, left, back, and right respectively.
* E and Q to move the camera up and down.
* R to reset the camera
* Ctrl+Q to close the window
* Left click and drag the mouse to move the camera


## Screenshots



## References
* [Vulkan tutorial](https://vulkan-tutorial.com/)
* [learnopengl.com](https://learnopengl.com/)
* [Vulkan Specification](https://registry.khronos.org/vulkan/specs/1.3-extensions/pdf/vkspec.pdf)
* [Sascha Willems examples](https://github.com/SaschaWillems/Vulkan)
* [vkguide.dev](https://vkguide.dev/)

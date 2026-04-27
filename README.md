基于WzComparerR2的C#代码转译重构版本,使用[LLVM18](https://github.com/mstorsjo/llvm-mingw/releases/tag/20240619),[Ninja](https://github.com/ninja-build/ninja/releases),[Cmake3.28](https://cmake.org/download/)编译

编译前先配置好环境变量

拉取项目
```
git clone --recurse-submodules https://github.com/lin8631/SDL3MapleStory2.git --depth 1
```

新建build目录.
```
  cmake -S . -B build -DWZLIB_USE_INTERNAL_AES=ON
  cmake --build build -j4
  ./build/MapViewer_EnTT /home/ltj/MapleStory/072/Data 100000000
```
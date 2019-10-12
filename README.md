[![Build Status](https://tankijong.visualstudio.com/Urban/_apis/build/status/tankiJong.urban?branchName=master)](https://tankijong.visualstudio.com/Urban/_build/latest?definitionId=1&branchName=master)
# urban 
Urban is a personal rendering framework based on D3D12(for now). It's still very primitive and relatively low level.

# structure
```
├───engine              // framework 
│   ├───engine             // source code
│   ├───resource           // resource files, will copy to ${BuildPath}/engine
│   └───external           // third party libs
└───projects            // different projects
    ├───es-console
    ├───model-viewer
    └───path-tracer

```

# Build
## prerequisite
- DXR compatible Windows 10(2019 fall update)
- DXR compatible GPU and driver
- Win SDK 10.17763
- visual studio 2019

## Build steps
- `git clone https://github.com/tankiJong/urban.git`
- open `.sln` file
- run projects

# Notice
* Be aware that the `.vcproj` is manually edited with several customized build steps, so the best way to create a new project for now is to copy the project structure and manually rename it with a code editor and remove all source files.

# Third Libraries
* Dear ImGui [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui)
* stb image [http://nothings.org/stb](http://nothings.org/stb)
* assimp [http://www.assimp.org/](http://www.assimp.org/)
* yaml-cpp [https://github.com/jbeder/yaml-cpp](https://github.com/jbeder/yaml-cpp)
* v8 [https://v8.dev/](https://v8.dev/)
* easy profiler[https://github.com/yse/easy_profiler](https://github.com/yse/easy_profiler)
* fmt[https://github.com/fmtlib/fmt](https://github.com/fmtlib/fmt)
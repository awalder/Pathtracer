
## Table of Contents
+ [Description](#Description)
+ [Building](#Building)
+ [Current state](#Currentstate)

## <a name="Description"></a> Description
Purpose of this project is to create pathtracer taking advantage of the RT-cores present
in (currently) latest generation NVIDIA graphics cards. This naturally limits support only to RTX-20xx series.
NVIDIA drivers 425.31 begun supporting VK_NVX_raytracing extensions on Pascal graphics cards but I have not tested this.

## <a name="Building"></a> Building
This reposity contains everything required to build the project. So far only VS2017 have been tested.
```
git clone https://github.com/awalder/Pathtracer.git
```
Use cmake to build Visual studio project files
```
cmake -G "Visual Studio 15 Win64"
```

## <a name="Currentstate"></a> Current state
This is still work on progress. Currently can load scene, render it using rasterizing pipeline or raytrace using RT-cores.
Two modes implemented, pathtracing and ambient occlusion.

### Implemented features / TODO list
- [ ] Bidirectiona pathtracer
- [ ] Multiple importance sampling
- [x] Trowbridge-Reitz microfacet surface model

There are many smaller tasks to be done and issues needing fixing but these point the direction for this project.

## External libraries used
- [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
- [GLFW window manager](https://www.glfw.org/)
- [OpenGL mathematics (GLM)](https://glm.g-truc.net/)
- [spdlog logging library](https://github.com/gabime/spdlog)
- [Assimp asset importer](http://www.assimp.org/)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [STB image](https://github.com/nothings/stb)
- [Tiny object loader](https://github.com/syoyo/tinyobjloader)


## Useful resources used for this project;
### Vulkan
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Sascha Willems Vulkan examples](https://github.com/SaschaWillems/Vulkan)

### Raytracing
- [Physically based rendering book](http://www.pbr-book.org/)
- [NVIDIA Vulkan Ray Tracing Tutorial](https://developer.nvidia.com/rtx/raytracing/vkray)
- [Importance Sampling techniques for GGX with Smith Masking-Shadowing: Part 2](https://schuttejoe.github.io/post/ggximportancesamplingpart2/)



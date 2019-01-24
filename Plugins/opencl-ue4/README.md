OpenCL Plugin for Unreal Engine 4
=============

[![GitHub release](https://img.shields.io/github/release/getnamo/opencl-ue4.svg)](https://github.com/getnamo/opencl-ue4/releases)
[![Github All Releases](https://img.shields.io/github/downloads/getnamo/opencl-ue4/total.svg)](https://github.com/getnamo/opencl-ue4/releases)

Enables using OpenCL kernel from UE4. Supports both blueprint and C++.

Forked from original work by [kwonoh](https://github.com/kwonoh/OpenCL.uplugin).




Installation
----------------------

### Windows

1. Install NVIDIA, AMD Graphics Driver, or [Intel OpenCL runtime](https://software.intel.com/en-us/articles/opencl-drivers).
2. Browse to your project folder (typically found at Documents/Unreal Project/{Your Project Root})
3. Copy Plugins folder into your Project root.

### OS X

1. Browse to your project folder
2. Copy Plugins folder into your Project root.

### C++ extra steps
3. Add "OpenCL" under "PublicDependencyModuleNames" in your *.Build.cs file to include header files from your project.
4. Include "OpenCL.h" in your source code.

How to use
----------------------

Add an ```Open CL Component``` to an actor of your choice.

![add component](https://i.imgur.com/zoU4PXM.png)

Optionally enumerate and add to or remove from your Device Group. For v0.3 the plugin will only run kernels on the first device in the Device Group array (index 0)

![enumerate devices](https://i.imgur.com/4mrW2tf.png)

Run a kernel either directly from string source

![run inline](https://i.imgur.com/JjbHOEw.png)

or from a file, default expected location is *{project root}/Content/Kernels*

![run from file](https://i.imgur.com/6uqXev6.png)

then subscribe to your component's ```On Result``` callback

![subscribe](https://i.imgur.com/pJMQVUv.png)

for example print out the result string

![print results](https://i.imgur.com/lBOn1xr.png)

Note that kernel compile errors will be logged to your Output log, which should make development easy.

![kernel compile](https://i.imgur.com/xrBCO3s.png)

To improve the development loop further the plugin contains a convenience ```OpenCLDevActor``` where you can specify a filename and kernel function name and upon play it will watch that file in *{project root}/Content/Kernels*. This means you can open your favorite text editor

![own editor](https://i.imgur.com/nuehUry.png)

and it will hotreload whenever that file has changed.

![hotreload](https://i.imgur.com/oUPImfk.gif)

That's more or less it for blueprint use for v0.3. You have access to the whole OpenCL library so if you want to dig down to C++ level you can bind much more complicated workflows. See https://github.com/getnamo/opencl-ue4/blob/master/Source/OpenCL/Private/OpenCLPlugin.cpp#L131 for code examples.

Supported Platform
----------------------

Tested on windows UE 4.18.

Older version Tested on Unreal Engine 4.7.6 in OS X, Windows on Intel CPU/GPU, NVIDIA GPU.
I've tested on OS X 10.10.3 (Intel Ivy Bridge, GT 650M, GTX 980) and Windows 8.1.
All suggestions for other platforms are welcome.

Known Issues
----------------------

* If you get an access error when calling ```clGetPlatformIDs``` during Debug, it is likely the intel platform causing problems. Solution: [https://software.intel.com/en-us/forums/opencl/topic/705036](https://software.intel.com/en-us/forums/opencl/topic/705036)

Screenshots
----------------------

### Windows
![Log Screenshots on Windows](https://raw.githubusercontent.com/kwonoh/OpenCL-UE4Plugin/gh-pages/images/opencl-ue4plugin-log-win.png)

### OS X
![Log Screenshots on OS X](https://raw.githubusercontent.com/kwonoh/OpenCL-UE4Plugin/gh-pages/images/opencl-ue4plugin-log-osx.png)

Legal info
----------------------

Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere. Unreal® Engine, Copyright 1998 – Current, Epic Games, Inc. All rights reserved.

Plugin is completely free and available under [MIT open-source license](LICENSE).

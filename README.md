# V-SENSE Tango AR

This repository is the implementation of the methods described in the paper "Dynamic Environment Mapping for Augmented Reality Applications on Mobile Devices", presented at VMV 2018.

## Getting started

The code included in the repository is mainly written in C++, with the exception of the required Java portions of code for the Android application. The same libraries used in the Android application can be compiled independently of the Android compilation environment.

## Prerequisites

This repository has been tested using the Lenovo Phab2 Pro phone and its code compiled under Windows 10. Two different IDEs were used:
* Android Studio 3.1.2
  -All libraries (vsense-libs) and main Android application.
* MSVC 2017
  -All libraries (vsense-libs) and applications to test and calculate Spherical Harmonics for meshes.
	
All the libraries and applications are linked together using CMake 3.9.

Dependencies for the Android application have been kept to a minimum:
* Google Tango Hopka
  -Since it's now hard to find this library, it was included in the repository (app/src/main/jniLibs), and you linked by CMake.
* GLM 0.9.8.5

All the MSVC applications make use of the Qt5 framework and the dependencies are:
* GLM 0.9.8.5
* Qt 5.9.1
* OpenCV 3.4.1 (Only for vsense_sh_mesh_app)
* Boost 1.64.0 (Only for vsense_sh_mesh_app)
* CGAL 4.11 (Only for vsense_sh_mesh_app)

As an alternative to defining environment variables for the dependencies, you can modify the file *cmake/SetVariables.cmake*, see comments for instructions.

**Important Note!**
The mesh files and their SH coefficients files are stored separately due to their relative large size, you can download them [here](https://drive.google.com/open?id=1Vu_Yx2yQWYRcdOdLyC6cG8yoQY15g3mZ). They have to be transferred to the 
mobile device you're intending to use to test the main application, such that extracted file creates this structure:

- sdcard: Root folder in the phone
  - TCD
    - data: Folder where recorded files will be stored
    - map: Precomputed files used when projecting to the EM 
    - mesh: OBJ files
    - out: Convenient folder to transfer manually the files from data. After creation, files in data won't be visible on Windows (not sure if that's always the case), I transfer manually the files to this folder and they become visible on Windows then
    - screenshot: Folder where screenshots will be saved
    - sh: Folder with the precomputed SH coefficients for the meshes

The test applications are intended to use the output data captured using the Tango phone. A simple description of the application's GUI can be seen [here](https://drive.google.com/open?id=1Pqgy5e96AZ5Mj__-kAs-jnjnijqmDmXG).

If you're not using a Lenovo Phab2 Pro, it's very likely the mapping files I'm using will need to be recalculated. The ptMap.bin and random.bin files are generated using the MATLAB code found here *matlab/runmeToRegenerateMapFiles.m*. Pay attention to the comments to modify it accordingly.

## Author

* [Rafael Monroy](http://www.rmonroy.com)

## License

This work is licensed under the GNU GPL-3.0 License

## Contact
If you have problems compiling or using the repository, you can contat me at monroyrr[at]tcd.ie
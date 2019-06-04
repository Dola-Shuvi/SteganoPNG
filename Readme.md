SteganoPNG
=================

C++ program based on stegonographical methods to hide files in PNG images using the Least Significant Bit technique.

SteganoPNG uses the most basic method which is the least significant bit. A pixel in a PNG image is composed of a red, green, blue and alpha channel encoded in one byte each.
 The least significant bit was chosen to visually modify the image as little as possible.


Information
-----------

SteganoPNG uses LodePNG to hide data in PNG files. It uses the first bit of each color channel, including the alpha channel, of a pixel. 
The program can hide all of the data if there is enough space in the image. **You need roughly twice as many pixels as bytes you want to hide**.

> *Only PNG files are supported* to obtain maximum storage capacity within the image.

Installation
------------

Download a precompiled binary on the releases tab or compile them yourself.

Building
-----

Building on unix systems is greatly simplified by using a Makefile.  Simply run `make` inside the project folder. Building on unix requires the C++ compiler `g++` version 8 to be installed.

Building on windows is as simple as cloning the repository into Visual Studio 2019 and building.


Usage
-----

```
SteganoPNG

Syntax: SteganoPNG <command> <image.png> [data.xxx]

Commands:
        a       Hide provided file in image
        x       Extract file hidden in image
        t       Verify if the image contains enough pixels for storage
        h       Show this help screen

Examples:
        SteganoPNG a image.png FileYouWantToHide.xyz  to hide "FileYouWantToHide.xyz" inside image.png
        SteganoPNG x image.png                        to extract the file hidden in image.png
        SteganoPNG v image.png FileYouWantToHide.xyz  to verify that the image contains enough pixels for storage
```

Dependencies
-------

This program depends on [LodePNG](https://github.com/lvandeve/lodepng).
LodePNG is licensed under the zlib license.

License
-------

This software is MIT-Licensed.

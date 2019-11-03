# fontcvt
This is just a simple font converter.  
It converts ttf (and other standard) fonts in a format suitable to be used by an MCU GUI.  
It provides glyph bitmaps with antialiasing upto 8 bpp and kerning informations. If you don't know about font kerning i suggest you to google for it. It can realy improve the beauty of the text on your emebedded divice.


## compile
This program rely on the FreeType 2 library https://www.freetype.org/. So you have to install it.  
On ubuntu:
```
sudo apt install libfreetype6-dev
git clone https://github.com/singds/fontcvt.git
cd fontcvt
make
```
After that you have a `../fontcvt/build` directory with the executable **fontcvt**.


## usage
To see all the available options run:
```
./build/fontcvt -h
```
The program produces the output files on the cwd. So you need to cd on the directory you want the output and from there run **fontcvt**.
This is an example supposing you have copied the `arial.ttf` font inside the fontcvt directory:
```
./build/fontcvt arial.ttf  -b4 -s14 -r32-255 -o arial
```
This will produce the files `arial.c arial.h`.

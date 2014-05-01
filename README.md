kano-screenshot
===============

Helper tool to take screenshots in Kanux.

It is based on the tool raspi2png from Andrew:

  https://github.com/AndrewFromMelbourne/raspi2png

It can take screenshots of applications drawing on different layers
of the RaspberryPI GPU surfaces, where other tools like scrot cannot.

This is the case with applications like Minecraft.

It also is capable of cropping areas from the screenshot by coordinates and by application name.
Here's a practical usage transcript to better understand how to use it.

```
albert@kano ~ $ kano-screenshot -h
kano-screenshot: option requires an argument -- 'h'
Usage: kano-screenshot [-p pngname] [-v] [-w <width>] [-h <height>] [-t <type>] [-d <delay>] [-c <x,y,width,height>]
 [-a <application>]
    -p - name of png file to create (default is kano-screenshot.png)
    -v - verbose
    -h - image height (default is screen height)
    -w - image width (default is screen width)
    -c - crop area off the screenshot by given coordinates (default is full screen)
    -a - crop area off the screenshot occupied by X11 application window name (as reported by xwininfo)
    -l - list of all X11 application window names to help using the -a option
    -p - override output image filename (default is kano-screenshot-timestamp.png
    -t - type of image captured
         can be one of the following: RGB565 RGB888 RGBA16 RGBA32
    -d - delay in seconds (default 0)

albert@kano ~ $ kano-screenshot -l
list of X11 windows from which a screen shot can be taken
use the -a flag along with the complete window name

Window id 0x1400004 (make-minecraft)
Window id 0x1400001 (make-minecraft)
Window id 0xa0b090 (lxpanel)
Window id 0xa01589 (lxpanel)
Window id 0xa0157b (lxpanel)
Window id 0x1200001 (lxterminal)
Window id 0xe00005 (KdeskControlWindow)
Window id 0xa00021 (lxpanel)
Window id 0xa00003 (lxpanel)
Window id 0xa00001 (lxpanel)
Window id 0xa0001e (panel)
Window id 0x1400024 (Make Minecraft)
Window id 0x1200004 (albert@kano: ~)
Window id 0x1600002 (Minecraft - Pi edition)

albert@kano ~ $ kano-screenshot -a "Minecraft - Pi edition" -p /tmp/minecraft.png
Cropping application name 'Minecraft - Pi edition' (x=0, y=0, width=1440, height=428)
Cropping screenshot area @0x0 size 1440x428
```

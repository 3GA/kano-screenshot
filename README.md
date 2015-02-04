kano-screenshot
===============

The RaspberryPI graphical processing unit on the chip comes with powerful features 
which allow applications to draw on the display using multiple "layers".

Most applications on KanoOS will use the XServer, which occupies only one of the many graphical layers available on the chip.
Other applications will use different graphical layers, which are invisible to the XServer applications and most times 
will sit on top of it. Minecraft, OMXPlayer, and XBMC are some examples.

This is why most X11 screenshot programs fail to capture these applications, and here is where kano-screenshot is useful.
It will capture everything that all combined layers are currently displaying on your screen.

Kano screenshot is based on the tool raspi2png from Andrew:

  https://github.com/AndrewFromMelbourne/raspi2png

It also is capable of cropping areas from the screen, and capture areas occupied by X11 applications given by name.

When invoked without parameters it defaults to taking a complete screenshot and will be saved in the file ```kano-screenshot.png```
on the current directory.

Here's a practical usage transcript to better understand how to use it.

```
albert@kano ~ $ kano-screenshot -?
Usage:
 kano-screenshot [-?] [-p pngname] [-f folder] [-v] [-w <width>] [-h <height>] [-t <type>]
                 [-d <delay>] [-c <x,y,width,height>] [-a <application>] [-l]

    -? - this help screen
    -p - name of png file to save (default is kano-screenshot-timespamp.png)
    -f - folder name, directory to save the screenshot file (can be combined with -p)
    -v - verbose, explain the steps being done
    -w - image width (default is screen width)
    -h - image height (default is screen height)
    -t - type of image to capture, can be one of: RGB565 RGB888 RGBA16 RGBA32
    -d - delay in seconds before taking the screenshot (default 0)
    -c - crop area off the screenshot by given coordinates (default is full screen)
    -a - crop area off the screenshot occupied by X11 application window name (as reported by xwininfo)
    -l - list of all X11 applications that can be captured using the "-a" option

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

albert@kano ~ $ kano-screenshot -c 120,150,450,600 -p /tmp/desktop-area.png
Cropping screenshot area @120x150 size 450x600

```

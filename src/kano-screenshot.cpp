//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2013 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

//
// kano-screenshot.cpp
//
// Copyright (C) 2014, 2015 Computing Ltd.
// License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
//
// Application that takes screenshots on the RaspberryPI from all GPU layers
//

#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "bcm_host.h"
#include "xwindows.h"

// A printf macro sensitive to the -v (verbose) flag
// Use kprintf for regular stdout messages instead of printf or cout
bool verbose=false; // Mute by default
#define kprintf(fmt, ...) ( ((verbose==false) ? 1 : printf(fmt, ##__VA_ARGS__) ))


#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

const char* program = NULL;

typedef struct
{
    const char* name;
    VC_IMAGE_TYPE_T type;
    int bytesPerPixel;
}
ImageInfo;

ImageInfo imageInfo[] =
{
    { "RGB565", VC_IMAGE_RGB565, 2 },
    { "RGB888", VC_IMAGE_RGB888, 3 },
    { "RGBA16", VC_IMAGE_RGBA16, 2 },
    { "RGBA32", VC_IMAGE_RGBA32, 4 }
};

static size_t imageEntries = sizeof(imageInfo)/sizeof(imageInfo[0]);

void pngWriteImageRGB565(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int rowLength = 3 * width;
    uint8_t *imageRow = (uint8_t *) calloc(rowLength, 1);

    if (imageRow == NULL)
    {
      kprintf("%s: unable to allocated row buffer\n", program);
      exit(EXIT_FAILURE);
    }

    int y = 0;
    for (y = 0; y < height; y++)
    {
        int x = 0;
        for (x = 0; x < width; x++)
        {
            uint16_t pixels = *(uint16_t*)((uint16_t*)buffer + (y * pitch) + (x * 2));
            int index = x * 3;

            uint8_t r5 = (pixels >> 11) & 0x1F;
            uint8_t g6 = (pixels >> 5) & 0x3F;
            uint8_t b5 = (pixels) & 0x1F;

            imageRow[index] =  (r5 << 3) | (r5 >> 2);
            imageRow[index + 1] =  (g6 << 2) | (g6 >> 4);
            imageRow[index + 2] =  (b5 << 3) | (b5 >> 2);
        }
        png_write_row(pngPtr, imageRow);

    }

    free(imageRow);
}

//-----------------------------------------------------------------------

void
pngWriteImageRGB888(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int y = 0;
    for (y = 0; y < height; y++)
    {
        png_write_row(pngPtr, (png_bytep) buffer + (pitch * y));
    }
}

//-----------------------------------------------------------------------

void
pngWriteImageRGBA16(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int rowLength = 4 * width;
    uint8_t *imageRow = (uint8_t *) calloc(rowLength, 1);

    if (imageRow == NULL)
    {
        kprintf("%s: unable to allocated row buffer\n", program);
        exit(EXIT_FAILURE);
    }

    int y = 0;
    for (y = 0; y < height; y++)
    {
        int x = 0;
        for (x = 0; x < width; x++)
        {
            uint16_t pixels = *(uint16_t*)((uint16_t*)buffer + (y * pitch) + (x * 2));
            int index = x * 4;

            uint8_t r4 = (pixels >> 12) & 0xF;
            uint8_t g4 = (pixels >> 8) & 0xF;
            uint8_t b4 = (pixels >> 4) & 0xF;
            uint8_t a4 = pixels & 0xF;

            imageRow[index] =  (r4 << 4) | r4;
            imageRow[index + 1] =  (g4 << 4) | g4;
            imageRow[index + 2] =  (b4 << 4) | b4;
            imageRow[index + 3] =  (a4 << 4) | a4;
        }
        png_write_row(pngPtr, imageRow);

    }

    free(imageRow);
}

//-----------------------------------------------------------------------

void
pngWriteImageRGBA32(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int y = 0;
    for (y = 0; y < height; y++)
    {
        png_write_row(pngPtr, (png_bytep) buffer + (pitch * y));
    }
}


/*
 *  crop() extracts an area from one plain image (source) into another one (target) using bpp bytes per pixel
 *
 */
int crop (png_bytep source, png_bytep target, int sourcew, int sourceh, int cropx, int cropy, int cropw, int croph, int bpp)
{
  int y;
  for (y=0; y < croph; y++)
    {
        // TODO: Error control for out-of-range cropping coordinates
        //
        memcpy ( target + (y * (cropw * bpp)),
                 source + ((y + cropy) * sourcew * bpp + (cropx * bpp)), 
                 cropw * bpp);
    }

  return 0;
}


/*
 * rotate_image_180() rotates the provided image 180 degrees, returning a newly allocated image buffer.
 * you should free the previous buffer if this function returns non NULL.
 * call is_screen_flipped() to find if your screen is flipped.
 */
unsigned char *rotate_image_180 (png_bytep image, unsigned int width, unsigned int height, unsigned int pitch, int bpp)
{
  png_bytep rotated_image = (png_bytep) calloc(height, pitch);
  if (!rotated_image) {
    return NULL;
  }

  // Rotate the image 180 degrees into the new buffer
  for(unsigned int y=0; y < height; y++)
    {
      for (unsigned int x=0; x < width; x++)
	{
	  // byte displacement into the next pixel
	  unsigned int byte_offset = x * bpp + y * pitch;

	  // copy a pixel (bpp bytes long) from the end of the source
	  // image into the start of the target image.
	  png_bytep psource = image + (height * pitch) - byte_offset;
	  png_bytep ptarget = rotated_image + byte_offset;
	  memcpy(ptarget, psource, bpp);
	}
    }

  return rotated_image;
}


/*
 * is_screen_flipped() returns true if the screen is rotated 180 degrees
 * by querying if "display_rotate" is set to "2" in /boot/config.txt file.
 */
bool is_screen_flipped(void)
{
  const char *command="vcgencmd get_config display_rotate";
  bool flipped=false;
  int flag=0;  // the actual config.txt value for display_rotate
  char buff[512];
  FILE *f;

  if (!(f=popen(command, "r"))) {
    return false;
  }

  if (fgets(buff, sizeof(buff), f)) {
    sscanf(buff, "display_rotate=%d\n", &flag);
    if (flag==2) {
      flipped=true;
    }
  }

  pclose(f);
  return flipped;
}


char *buildScreenshotFilename(char *directory, char *filename, int size)
{
  struct tm *p;
  time_t t;
  char pchTime[128];
  char *prefix=(char *) "kano-screenshot";
  int needed_size=strlen (prefix) + 32;
  char *base_filename=(char *) "kano-screenshot";

  if (directory) {
    needed_size += strlen(directory) + sizeof(char);
  }

  if (size < needed_size) {
    return NULL;
  }

  t = time(NULL);
  p = localtime(&t);

  // week day, month name, month day, hour, minute, seconds
  strftime (pchTime, sizeof(pchTime), "-%a-%b-%d-%H-%M-%S", p);

  if (directory) {
    strcpy (filename, directory);
    strcat (filename, "/");
    strcat (filename, base_filename);
  }
  else {
    strcpy (filename, base_filename);
  }

  strcat (filename, pchTime);
  strcat (filename, ".png");
  return filename;
}


//-----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int opt = 0;

    char defaultPngName[256];
    char *pngName=defaultPngName;
    char *image_filename=NULL;
    char *directory_name=NULL;

    int requestedWidth = 0;
    int requestedHeight = 0;
    int delay = 0, retry = 0;

    // -c parameter variables
    bool cropping = false;
    bool appfound = false;
    int cropx=0, cropy=0, cropwidth=0, cropheight=0;

    // -a parameter variables
    char *appname=NULL;

    const char* imageTypeName = "RGB888";
    VC_IMAGE_TYPE_T imageType = VC_IMAGE_MIN;
    int bytesPerPixel  = 0;

    int result = 0;

    program = basename(argv[0]);

    //-------------------------------------------------------------------

    while ((opt = getopt(argc, argv, "d:h:p:f:t:vw:c:a:l?")) != -1)
    {
        switch (opt)
        {
        case 'v':
            verbose = true;
            break;

        case 'd':
            delay = atoi(optarg);
            break;

        case 'h':
            requestedHeight = atoi(optarg);
            if (!requestedHeight) {
              kprintf ("Invalid Height requested: %s\n", optarg);
              exit (1);
            }
            break;

        case 'p':
            image_filename = optarg;
            break;

        case 'f':
            directory_name = optarg;
            break;

        case 't':
            imageTypeName = optarg;
            break;

        case 'w':
            requestedWidth = atoi(optarg);
            if (!requestedWidth) {
              kprintf ("Invalid Width requested: %s\n", optarg);
              exit (1);
            }
            break;

	case 'l':
	  // list all X11 window names that kano-screenshot can find
	  printf ("list of X11 windows from which a screen shot can be taken\n");
	  printf ("use the -a flag along with the complete window name\n\n");
	  appfound = findWindowCoordinatesByName (NULL, true, NULL, NULL, NULL, NULL);
	  exit (0);

	case 'a':
	  // cropping based on a X11 app window name
	  retry=delay;
	  appname = optarg;
	  appfound = findWindowCoordinatesByName (appname, verbose, &cropx, &cropy, &cropwidth, &cropheight);
	  while (retry > 0 && appfound == false) {
	    kprintf ("Application window not found, waiting and retrying...\n");
	    sleep (1);
	    appfound = findWindowCoordinatesByName (appname, verbose, &cropx, &cropy, &cropwidth, &cropheight);
	    if (appfound == true) {
	      cropping = true;
	      break;
	    }
	    else {
	      retry--;
	    }
	  }

	  if (appfound == false) {
	    kprintf ("Could not find application name: '%s'\n", appname);
	    exit (EXIT_FAILURE);
	  }
	  else {
	    kprintf ("Application window '%s' found\n", appname);
	    cropping = true;
	  }
	  break;

	case 'c':
	  // cropping a specific area off the screenshot
	  sscanf (optarg, "%d,%d,%d,%d", &cropx, &cropy, &cropwidth, &cropheight);
	  
	  // minimum validation
	  if (cropwidth && cropheight) {
	    cropping = true;
	    if (verbose == true) {
	      kprintf ("Cropping area: x=%d, y=%d, width=%d, height=%d\n",
		      cropx, cropy, cropwidth, cropheight);
	    }
	  }
	  else {
	    cropping = false;
	    kprintf ("Error parsing the cropping area: %s\n", optarg);
	    exit(EXIT_FAILURE);
	  }
	  break;

	case '?':
        default:

            printf("Usage:\n %s [-?] [-p pngname] [-f folder] [-v] ", program);
            printf("[-w <width>] [-h <height>] [-t <type>]\n");
            printf("\t\t [-d <delay>] [-c <x,y,width,height>] [-a <application>] [-l]\n\n");

            printf( "    -? - this help screen\n");
            printf( "    -p - name of png file to save ");
            printf( "(default is kano-screenshot-timespamp.png)\n");
            printf( "    -f - folder name, directory to save the screenshot file (can be combined with -p)\n");
            printf( "    -v - verbose, explain the steps being done\n");
            printf( "    -w - image width (default is screen width)\n");
            printf( "    -h - image height (default is screen height)\n");
            printf( "    -t - type of image to capture, can be one of:");

            size_t entry = 0;
            for (entry = 0; entry < imageEntries; entry++)
            {
              printf(" %s", imageInfo[entry].name);
            }

            printf( "\n    -d - delay in seconds before taking the screenshot (default 0)\n");
            printf( "    -c - crop area off the screenshot by given coordinates (default is full screen)\n");
            printf( "    -a - crop area off the screenshot occupied by X11 application window name (as reported by xwininfo)\n");

            printf( "    -l - list of all X11 applications that can be captured using the \"-a\" option\n");

            printf("\n");
            printf("\n");

            exit(EXIT_FAILURE);
            break;
        }
    }


    //-------------------------------------------------------------------

    // Build screenshot complete directory and filename
    if (!image_filename) {
      if (!buildScreenshotFilename (directory_name, (char *) defaultPngName, sizeof(defaultPngName))) {
        strcpy (defaultPngName, "kano-screenshot.png");
        kprintf ("Warning: Could not build file pathname, saving image to: %s\n", defaultPngName);
      }
    }
    else {
      if (directory_name) {
        strcat (defaultPngName, directory_name);
        strcat (defaultPngName, "/");
        strcat (defaultPngName, image_filename);
      }
      else {
        strcpy (defaultPngName, image_filename);
      }
    }

    //-------------------------------------------------------------------

    size_t entry = 0;
    for (entry = 0; entry < imageEntries; entry++)
    {
        if (strcasecmp(imageTypeName, imageInfo[entry].name) == 0)
        {
            imageType = imageInfo[entry].type;
            bytesPerPixel =  imageInfo[entry].bytesPerPixel;
            break;
        }
    }

    if (imageType == VC_IMAGE_MIN)
    {
      kprintf(
                "%s: unknown image type %s\n",
                program,
                imageTypeName);

        exit(EXIT_FAILURE);
    }

    //-------------------------------------------------------------------

    if (delay)
    {
        if (verbose)
        {
            kprintf("sleeping for %d seconds ...\n", delay);
        }

        sleep(delay);
    }


    //-------------------------------------------------------------------

    bcm_host_init();

    DISPMANX_DISPLAY_HANDLE_T displayHandle = vc_dispmanx_display_open(0);

    DISPMANX_MODEINFO_T modeInfo;
    result = vc_dispmanx_display_get_info(displayHandle, &modeInfo);

    if (result != 0)
    {
        kprintf("%s: unable to get display information\n", program);
	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    int width = modeInfo.width;
    int height = modeInfo.height;


    if (requestedWidth > 0)
      {
        // avoid larger widths than the screen fits
        requestedWidth = (requestedWidth > width ? width : requestedWidth);
	
        if (requestedHeight == 0)
	  {
            double numerator = height * requestedWidth;
            double denominator = width;
            height = (int)ceil(numerator / denominator);
	  }

        width = requestedWidth;
        kprintf ("Rescaling width: new width=%d, height=%d\n", width, height);
      }

    if (requestedHeight > 0)
      {
        // avoid larger heights than the screen fits
        requestedHeight = (requestedHeight > height ? height : requestedHeight);
	
        if (requestedWidth == 0)
	  {
	    double numerator = width * requestedHeight;
            double denominator = height;
            width = (int)ceil(numerator / denominator);
	  }

        height = requestedHeight;
        kprintf ("Rescaling height: new width=%d, height=%d\n", width, height);
      }


    int pitch = bytesPerPixel * ALIGN_TO_16(width);


    kprintf("screen width = %d\n", modeInfo.width);
    kprintf("screen height = %d\n", modeInfo.height);
    kprintf("requested width = %d\n", requestedWidth);
    kprintf("requested height = %d\n", requestedHeight);
    kprintf("image width = %d\n", width);
    kprintf("image height = %d\n", height);
    kprintf("image type = %s\n", imageTypeName);
    kprintf("bytes per pixel = %d\n", bytesPerPixel);
    kprintf("pitch = %d\n", pitch);



    void *dmxImagePtr = malloc(pitch * height);

    if (dmxImagePtr == NULL)
    {
        kprintf("%s: unable to allocated image buffer\n", program);
	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    uint32_t vcImagePtr = 0;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    resourceHandle = vc_dispmanx_resource_create(imageType,
                                                 width,
                                                 height,
                                                 &vcImagePtr);

    // TODO: Unfortunately at this time, DISPMANX_NO_ROTATE seems to be
    // the only implemented option at this API level (see rotate_image_180 function).
    result = vc_dispmanx_snapshot(displayHandle,
                                  resourceHandle,
                                  DISPMANX_NO_ROTATE);

    if (verbose)
    {
        kprintf("vc_dispmanx_snapshot() returned %d\n", result);
    }

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        kprintf("%s: vc_dispmanx_snapshot() failed\n", program);
	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    VC_RECT_T rect;

    result = vc_dispmanx_rect_set(&rect, 0, 0, width, height);
    if (verbose)
    {
        printf("vc_dispmanx_rect_set() returned %d\n", result);
        kprintf("rect = { %d, %d, %d, %d }\n",
               rect.x,
               rect.y,
               rect.width,
               rect.height);
    }

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        kprintf("%s: vc_dispmanx_rect_set() failed\n", program);
	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }


    result = vc_dispmanx_resource_read_data(resourceHandle,
                                            &rect,
                                            dmxImagePtr,
                                            pitch);

    if (result != 0)
    {
        vc_dispmanx_resource_delete(resourceHandle);
        vc_dispmanx_display_close(displayHandle);

        kprintf("%s: vc_dispmanx_resource_read_data() failed\n",
                program);

	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    if (verbose)
    {
        kprintf("vc_dispmanx_resource_read_data() returned %d\n", result);
    }

    result = vc_dispmanx_resource_delete(resourceHandle);

    if (verbose)
    {
        kprintf("vc_dispmanx_resource_delete() returned %d\n", result);
    }

    result = vc_dispmanx_display_close(displayHandle);

    if (verbose)
    {
        kprintf("vc_dispmanx_display_close() returned %d\n", result);
    }

    // Do the screenshot cropping if requested.
    if (cropping == true) {

      void *dmxCroppedImagePtr = calloc (cropheight, pitch);
      if (!dmxCroppedImagePtr) {
	kprintf("%s: unable to allocated cropping buffer\n", program);
	exit(EXIT_FAILURE);
      }
      else {
	// Extract the requested area from the complete screenshot
	kprintf ("Cropping screenshot area @%dx%d size %dx%d\n", cropx, cropy, cropwidth, cropheight);

	// Crop image off screenshot, then swap the image buffers so that png saves the cropped image instead
	crop ((png_bytep)dmxImagePtr, (png_bytep)dmxCroppedImagePtr, 
              ALIGN_TO_16(width), height, cropx, cropy, ALIGN_TO_16(cropwidth), cropheight, bytesPerPixel);

	free (dmxImagePtr);
	dmxImagePtr = dmxCroppedImagePtr;

	// Provide the new image size details
	width = cropwidth;
	height = cropheight;
      }

      pitch = bytesPerPixel * ALIGN_TO_16(width);
    }

    if ((requestedWidth > 0 || requestedHeight > 0) && cropping == true)
      {
	// TODO: A crop followed by a resize needs the image buffer be rescaled, so not implemented yet
	kprintf ("Crop followed by resize is not implemented yet\n");
	exit(EXIT_FAILURE);
      }

    //-------------------------------------------------------------------

    // rotate the image if necessary
    if (is_screen_flipped())
      {
	kprintf("flipping image due to rotated screen...\n");
	void *dmxRotatedImagePtr=rotate_image_180((unsigned char *)dmxImagePtr, ALIGN_TO_16(width), height, pitch, bytesPerPixel);
	if (dmxRotatedImagePtr) {
	  free(dmxImagePtr);
	  dmxImagePtr = dmxRotatedImagePtr;
	}
	else {
	  kprintf("could not flip image... out of memory error\n");
	}
      }

    //-------------------------------------------------------------------

    png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                 NULL,
                                                 NULL,
                                                 NULL);

    if (pngPtr == NULL)
    {
        kprintf("%s: unable to allocated PNG write structure\n",
                program);

	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);

    if (infoPtr == NULL)
    {
        kprintf("%s: unable to allocated PNG info structure\n",
                program);

	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        kprintf("%s: unable to create PNG\n", program);
	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    FILE *pngfp = fopen(pngName, "wb");

    if (pngfp == NULL)
    {
        kprintf("%s: unable to create %s - %s\n",
                program,
                pngName,
                strerror(errno));

	bcm_host_deinit();
        exit(EXIT_FAILURE);
    }

    png_init_io(pngPtr, pngfp);

    int png_color_type = PNG_COLOR_TYPE_RGB;

    if ((imageType == VC_IMAGE_RGBA16) || (imageType == VC_IMAGE_RGBA32))
    {
      png_color_type = PNG_COLOR_TYPE_RGBA;
    }

    png_set_IHDR(
        pngPtr,
        infoPtr,
        width,
        height,
        8,
        png_color_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE);

    png_write_info(pngPtr, infoPtr);

    switch(imageType)
    {
    case VC_IMAGE_RGB565:

      pngWriteImageRGB565(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    case VC_IMAGE_RGB888:

        pngWriteImageRGB888(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    case VC_IMAGE_RGBA16:

        pngWriteImageRGBA16(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    case VC_IMAGE_RGBA32:

        pngWriteImageRGBA32(width,
                            height,
                            pitch,
                            dmxImagePtr,
                            pngPtr,
                            infoPtr);
        break;

    default:

        break;
    }


    


    png_write_end(pngPtr, NULL);
    png_destroy_write_struct(&pngPtr, &infoPtr);
    fclose(pngfp);

    //-------------------------------------------------------------------

    free(dmxImagePtr);
    bcm_host_deinit();
    return 0;
}


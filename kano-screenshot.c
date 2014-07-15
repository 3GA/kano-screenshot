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

#define _GNU_SOURCE

#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bcm_host.h"

#include "xwindows.h"


// A printf macro sensitive to the -v (verbose) flag
// Use kprintf for regular stdout messages instead of printf or cout
bool verbose=false; // Mute by default
#define kprintf(fmt, ...) ( ((verbose==false) ? 1 : printf(fmt, ##__VA_ARGS__) ))


//-----------------------------------------------------------------------

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

//-----------------------------------------------------------------------

const char* program = NULL;

//-----------------------------------------------------------------------

typedef struct
{
    const char* name;
    VC_IMAGE_TYPE_T type;
    int bytesPerPixel;
}
ImageInfo;

#define IMAGE_INFO_ENTRY(t, b) \
    { .name=(#t), .type=(VC_IMAGE_ ## t), .bytesPerPixel=(b) }

ImageInfo imageInfo[] =
{
  IMAGE_INFO_ENTRY(RGB565, 2),
  IMAGE_INFO_ENTRY(RGB888, 3),
  IMAGE_INFO_ENTRY(RGBA16, 2),
  IMAGE_INFO_ENTRY(RGBA32, 4)
};

static size_t imageEntries = sizeof(imageInfo)/sizeof(imageInfo[0]);

//-----------------------------------------------------------------------

void
pngWriteImageRGB565(
    int width,
    int height,
    int pitch,
    void* buffer,
    png_structp pngPtr,
    png_infop infoPtr)
{
    int rowLength = 3 * width;
    uint8_t *imageRow = malloc(rowLength);

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
            uint16_t pixels = *(uint16_t*)(buffer + (y * pitch) + (x * 2));
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
        png_write_row(pngPtr, buffer + (pitch * y));
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
    uint8_t *imageRow = malloc(rowLength);

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
            uint16_t pixels = *(uint16_t*)(buffer + (y * pitch) + (x * 2));
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
        png_write_row(pngPtr, buffer + (pitch * y));
    }
}


/*
 *  crop() extracts an area from one plain image (source) into another one (target) using bpp bytes per pixel
 *
 */
int crop (unsigned char *source, unsigned char *target, int sourcew, int sourceh, int cropx, int cropy, int cropw, int croph, int bpp)
{
  int y;
  for (y=0; y < croph; y++)
    {
      // TODO: Error control for out-of-range cropping coordinates
      //
      memcpy ( (void*) target + (y * (cropw * bpp)),
	       (void*) source + ((y + cropy) * sourcew * bpp + (cropx * bpp)), 
	       cropw * bpp);
    }

  return 0;
}

//-----------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int opt = 0;

    char *pngName = "kano-screenshot.png";

    int requestedWidth = 0;
    int requestedHeight = 0;
    int delay = 0;

    // -c parameter variables
    bool cropping = false;
    int cropx=0, cropy=0, cropwidth=0, cropheight=0;

    // -a parameter variables
    char *appname=NULL;

    const char* imageTypeName = "RGB888";
    VC_IMAGE_TYPE_T imageType = VC_IMAGE_MIN;
    int bytesPerPixel  = 0;

    int result = 0;

    program = basename(argv[0]);

    //-------------------------------------------------------------------

    while ((opt = getopt(argc, argv, "d:h:p:t:vw:c:a:l?")) != -1)
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
            break;

        case 'p':

            pngName = optarg;
            break;

        case 't':

            imageTypeName = optarg;
            break;

        case 'w':

            requestedWidth = atoi(optarg);
            break;

	case 'l':
	  // list all X11 window names that kano-screenshot can find
	  printf ("list of X11 windows from which a screen shot can be taken\n");
	  printf ("use the -a flag along with the complete window name\n\n");
	  bool appfound = findWindowCoordinatesByName (NULL, true, NULL, NULL, NULL, NULL);
	  exit (0);

	case 'a':
	  // cropping based on a X11 app window name
	  appname = optarg;
	  appfound = findWindowCoordinatesByName (appname, verbose, &cropx, &cropy, &cropwidth, &cropheight);
	  if (appfound == true) {
	    cropping = true;
	    kprintf ("Cropping application name '%s' (x=%d, y=%d, width=%d, height=%d)\n",
		    appname, cropx, cropy, cropwidth, cropheight);
	  }
	  else {
	    kprintf ("Could not find coordinates of X11 application name: %s\n", appname);
	    exit(EXIT_FAILURE);
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

            printf("Usage: %s [-p pngname] [-v]", program);
            printf(" [-w <width>] [-h <height>] [-t <type>]");
            printf(" [-d <delay>] [-c <x,y,width,height>]\n");
	    printf(" [-a <application>]\n");

            printf("    -p - name of png file to create ");
            printf("(default is %s)\n", pngName);
            printf("    -v - verbose\n");

            printf(
                    "    -h - image height (default is screen height)\n");
            printf(
                    "    -w - image width (default is screen width)\n");
            printf(
                    "    -c - crop area off the screenshot by given coordinates (default is full screen)\n");
            printf(
                    "    -a - crop area off the screenshot occupied by X11 application window name (as reported by xwininfo)\n");
            printf(
                    "    -l - list of all X11 application window names to help using the -a option\n");
            printf(
                    "    -p - override output image filename (default is kano-screenshot-timestamp.png\n");
            printf("    -t - type of image captured\n");
            printf("         can be one of the following:");

            size_t entry = 0;
            for (entry = 0; entry < imageEntries; entry++)
            {
                kprintf(" %s", imageInfo[entry].name);
            }
            printf("\n");
            printf("    -d - delay in seconds (default 0)\n");
            printf("\n");

            exit(EXIT_FAILURE);
            break;
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
    if (cropping) {

      void *dmxCroppedImagePtr = calloc (cropheight, pitch);
      if (!dmxCroppedImagePtr) {
	kprintf("%s: unable to allocated cropping buffer\n", program);
	exit(EXIT_FAILURE);
      }
      else {
	// Extract the requested area from the complete screenshot
	kprintf ("Cropping screenshot area @%dx%d size %dx%d\n", cropx, cropy, cropwidth, cropheight);

	// Crop image off screenshot, then swap the image buffers so that png saves the cropped image instead
	crop (dmxImagePtr, dmxCroppedImagePtr, ALIGN_TO_16(width), height, cropx, cropy, ALIGN_TO_16(cropwidth), cropheight, bytesPerPixel);
	free (dmxImagePtr);
	dmxImagePtr = dmxCroppedImagePtr;

	// Provide the new image size details
	width = cropwidth;
	height = cropheight;
      }
    }


    if ((requestedWidth > 0 || requestedHeight > 0) && cropping == true)
      {
	// TODO: A crop followed by a resize needs the image buffer be rescaled, so not implemented yet
	kprintf ("Crop followed by resize is not implemented yet\n");
	exit(EXIT_FAILURE);
      }

    if (requestedWidth > 0)
      {
        width = (requestedWidth > width ? width : requestedWidth);
	
        if (requestedHeight == 0)
	  {
            double numerator = height * requestedWidth;
            double denominator = width;
	    
            height = (int)ceil(numerator / denominator);
	  }

	if (verbose == true) {
	  kprintf ("Rescaling width: new width=%d, height=%d\n", width, height);
	}
      }

    if (requestedHeight > 0)
      {
	height = (requestedHeight > height ? height : requestedHeight);
	
        if (requestedWidth == 0)
	  {
	    double numerator = width * requestedHeight;
            double denominator = height;
	    
            width = (int)ceil(numerator / denominator);
	  }

	if (verbose == true) {
	  kprintf ("Rescaling height: new width=%d, height=%d\n", width, height);
	}
      }

    pitch = bytesPerPixel * ALIGN_TO_16(width);

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


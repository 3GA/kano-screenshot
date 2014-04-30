// 
// haw.c - this is a transitional tool POC to be able to crop
// an area of the screenshot.
//
// This module will disappear after the code moves into kano-screenshot.
//
// outputs 2 files: screenshot.ppm and screenshot-cropped.ppm
// change variables photoXXX to define the cropping area.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "bcm_host.h"

#ifndef ALIGN_TO_16
#define ALIGN_TO_16(x)  ((x + 15) & ~15)
#endif

int crop (unsigned char *source, unsigned char *target, int sourcew, int sourceh, int cropx, int cropy, int cropw, int croph, int bpp);

int main(void)
{
  DISPMANX_DISPLAY_HANDLE_T   display;
  DISPMANX_MODEINFO_T         info;
  DISPMANX_RESOURCE_HANDLE_T  resource;
  VC_IMAGE_TYPE_T type = VC_IMAGE_RGB888;
  VC_IMAGE_TRANSFORM_T        transform = 0;
  VC_RECT_T                   rect;
  void                       *image;

  unsigned char *photo;

  // Sample cropping area coordinates
  //int photox=50, photoy=50, photow=300, photoh=100;
  int photox=120, photoy=50, photow=400, photoh=600;

  uint32_t                    vc_image_ptr;
  int                   ret;
  uint32_t        screen = 0;

  bcm_host_init();

  printf("Open display[%i]...\n", screen );
  display = vc_dispmanx_display_open( screen );

  ret = vc_dispmanx_display_get_info(display, &info);
  assert(ret == 0);
  printf( "Display is %d x %d\n", info.width, info.height );

  // bpp will be provided by kano-screenshot imageInfo[entry].bytesPerPixel
  int bitdepth=3;

  // this is the original screenshot
  image = calloc( 1, info.width * bitdepth * info.height );

  // this is the cropped screenshot
  photo = calloc (1, photow * photoh * bitdepth);

  assert (image);
  assert (photo);

  resource = vc_dispmanx_resource_create( type,
					  info.width,
					  info.height,
					  &vc_image_ptr );

  vc_dispmanx_snapshot(display, resource, transform);
  vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
  vc_dispmanx_resource_read_data(resource, &rect, image, info.width*bitdepth);

  // save the complete screenshot
  FILE *fp = fopen("screenshot.ppm", "wb");
  fprintf(fp, "P6\n%d %d\n255\n", info.width, info.height);
  fwrite(image, info.width*bitdepth*info.height, 1, fp);
  fclose(fp);

  // the cropping happens here, from <image> into <photo>
  crop (image, photo, info.width, info.height, photox, photoy, photow, photoh, bitdepth);  

  // save the cropped image
  fp = fopen("screenshot-cropped.ppm", "wb");
  fprintf(fp, "P6\n%d %d\n255\n", photow, photoh);
  fwrite(photo, photow * photoh * 3, 1, fp);
  fclose(fp);

  // byebye
  ret = vc_dispmanx_resource_delete( resource );
  assert( ret == 0 );
  ret = vc_dispmanx_display_close(display );
  assert( ret == 0 );
  return 0;
}

int crop (unsigned char *source, unsigned char *target, int sourcew, int sourceh, int cropx, int cropy, int cropw, int croph, int bpp)
{
  int y;
  for (y=0; y < croph; y++)
    {
      // that's it!
      memcpy ( (void*) target + (y * cropw * bpp), (void*) source + ( (y + cropy) * sourcew * bpp + (cropx * bpp)), cropw * bpp);
    }
  return 0;
}

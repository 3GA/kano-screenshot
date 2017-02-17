#ifndef ENCODE_H
#define ENCODE_H

extern "C" {
#include "bcm_host.h"
#include "interface/vmcs_host/vcgencmd.h"
}

typedef struct {
  void *dmxImagePtr;
  DISPMANX_DISPLAY_HANDLE_T displayHandle;
  DISPMANX_RESOURCE_HANDLE_T resourceHandle;
  int resourceHandleValid:1;

}resources;


typedef struct {
  VC_RECT_T rect;
  int pitch;
  resources *r;
  int width, height, frames, height16;

}frameData;

int getFrame(frameData *fdat);
int video_encode(char *outputfilename, frameData *fdat);


#endif // ENCODE_H


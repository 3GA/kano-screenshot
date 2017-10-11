#ifndef DISPMANX_GRABBER_H
#define DISPMANX_GRABBER_H
#include "bcm_host.h"

typedef struct
{
    const char* name;
    VC_IMAGE_TYPE_T type;
    int bytesPerPixel;
}
DispManxImageInfo;

typedef int (*DispmanxGrabber_logPrintf)(const char *  format, ...);


typedef struct
{
    int width;
    int height;
    int bytesPerPixel;
    int pitch;
    VC_IMAGE_TYPE_T imageType;
    int frame_size;
}DispmanxGrabberFrameInfo;    

typedef struct
{
    int requestedWidth;
    int requestedHeight;
    int alignLog2;
    const char * imageTypeName;
    bool verbose;
    DispmanxGrabber_logPrintf logPrintf;
}DispmanxGrabberConfig;

typedef struct
{
    DISPMANX_DISPLAY_HANDLE_T displayHandle;
    DISPMANX_MODEINFO_T modeInfo;
    uint32_t vcImagePtr;
    DISPMANX_RESOURCE_HANDLE_T resourceHandle;
    DispmanxGrabberFrameInfo frameInfo;
    DispmanxGrabberConfig cfg;
} DispmanxGrabberState;


typedef enum { DISPMANX_GRABBER_OK=0,
       DISPMANX_GRABBER_INVALID_IMAGE_NAME,
       DISPMANX_GRABBER_DISP_FAIL,
       DISPMANX_GRABBER_MODE_FAIL,
       DISPMANX_GRABBER_MALLOC_FAIL,
       DISPMANX_GRABBER_SNAPSHOT_FAIL,
       DISPMANX_GRABBER_SETRECT_FAIL,
       DISPMANX_GRABBER_RESREAD_FAIL
} DispmanxGrabberErrcode;
       
    


int dispmanx_grabber_init(DispmanxGrabberState *st,
                          DispmanxGrabberConfig cfg);

DispmanxGrabberFrameInfo dispmanx_grabber_frameinfo(DispmanxGrabberState *st);

int dispmanx_grabber_close(DispmanxGrabberState *st);

void dispmanx_grabber_print_names(void);

int dispmanx_grabber_grab(DispmanxGrabberState *st, void *imagePtr);

#endif //DISPMANX_GRABBER_H

#include "dispmanx_grabber.h"
#include <stdio.h>
#include <math.h>

#define kprintf if(st->cfg.verbose && st->cfg.logPrintf) st->cfg.logPrintf

static DispManxImageInfo imageInfo[] =
{
    { "RGB565", VC_IMAGE_RGB565, 2 },
    { "RGB888", VC_IMAGE_RGB888, 3 },
    { "BGR888", VC_IMAGE_BGR888, 3 },
    { "RGBA16", VC_IMAGE_RGBA16, 2 },
    { "RGBA32", VC_IMAGE_RGBA32, 4 }
};
static size_t imageEntries = sizeof(imageInfo)/sizeof(imageInfo[0]);

void dispmanx_grabber_print_names(void)
{
    size_t entry = 0;
    for (entry = 0; entry < imageEntries; entry++)
    {
        printf(" %s", imageInfo[entry].name);
    }
}



int align(int value, int alignLog2)
{
    int alignBits = (1 << alignLog2) -1;
    return (value + alignBits) & ~alignBits;
}


int dispmanx_grabber_init(DispmanxGrabberState *st,
                          DispmanxGrabberConfig cfg)
                          
{
    int result;
    memset(st, 0, sizeof(DispmanxGrabberState));

    size_t entry = 0;
    for (entry = 0; entry < imageEntries; entry++)
    {
        if (strcasecmp(cfg.imageTypeName, imageInfo[entry].name) == 0)
        {
            st->frameInfo.imageType = imageInfo[entry].type;
            st->frameInfo.bytesPerPixel =  imageInfo[entry].bytesPerPixel;
            break;
        }
    }

    if (st->frameInfo.imageType == VC_IMAGE_MIN)
    {

      return DISPMANX_GRABBER_INVALID_IMAGE_NAME;
    }
    
    st->cfg = cfg;
    
    bcm_host_init();
    st->displayHandle = vc_dispmanx_display_open(0);
    if(!st->displayHandle)
    {
        return DISPMANX_GRABBER_DISP_FAIL;
    }
    
    result = vc_dispmanx_display_get_info(st->displayHandle, &st->modeInfo);
    if (result != 0)
    {
        return DISPMANX_GRABBER_MODE_FAIL;
        
    }
    int width = st->modeInfo.width;
    int height = st->modeInfo.height;

    if (cfg.requestedWidth > 0)
      {
        // avoid larger widths than the screen fits
        cfg.requestedWidth = (cfg.requestedWidth > width ? width : cfg.requestedWidth);
	
        if (cfg.requestedHeight == 0)
	  {
            double numerator = height * cfg.requestedWidth;
            double denominator = width;
            height = (int)ceil(numerator / denominator);
	  }

        width = cfg.requestedWidth;
        kprintf ("Rescaling width: new width=%d, height=%d\n", width, height);
      }

    if (cfg.requestedHeight > 0)
      {
        // avoid larger heights than the screen fits
        cfg.requestedHeight = (cfg.requestedHeight > height ? height : cfg.requestedHeight);
	
        if (cfg.requestedWidth == 0)
	  {
	    double numerator = width * cfg.requestedHeight;
            double denominator = height;
            width = (int)ceil(numerator / denominator);
	  }

        height = cfg.requestedHeight;
        kprintf ("Rescaling height: new width=%d, height=%d\n", width, height);
      }


    int pitch = st->frameInfo.bytesPerPixel * align(width, cfg.alignLog2);
    

    st->vcImagePtr = 0;
    st->resourceHandle = vc_dispmanx_resource_create(st->frameInfo.imageType,
                                                     width,
                                                 height,
                                                 &st->vcImagePtr);
    st->frameInfo.width = width;
    st->frameInfo.height = height;
    st->frameInfo.pitch = pitch;
    st->frameInfo.frame_size = pitch * height;


    return DISPMANX_GRABBER_OK;

}



int dispmanx_grabber_close(DispmanxGrabberState *st)
{
    if(st->resourceHandle)
    {
        int result = vc_dispmanx_resource_delete(st->resourceHandle);
        st->resourceHandle = 0;
        kprintf("vc_dispmanx_resource_delete() returned %d\n", result);
    }
    if(st->displayHandle)
    {
        int result = vc_dispmanx_display_close(st->displayHandle);
        st->displayHandle = 0;
        kprintf("vc_dispmanx_display_close() returned %d\n", result);
    }
    bcm_host_deinit();
    return DISPMANX_GRABBER_OK;
}

int dispmanx_grabber_grab(DispmanxGrabberState *st, void *imagePtr)
{
    // TODO: Unfortunately at this time, DISPMANX_NO_ROTATE seems to be
    // the only implemented option at this API level (see rotate_image_180 function).
    int result = vc_dispmanx_snapshot(st->displayHandle,
                                      st->resourceHandle,
                                  DISPMANX_NO_ROTATE);
    
    if (result != 0)
    {
        
        return DISPMANX_GRABBER_SNAPSHOT_FAIL;
    }
    
    
    
    VC_RECT_T rect;
    
    result = vc_dispmanx_rect_set(&rect, 0, 0, st->frameInfo.width, st->frameInfo.height);
    if (result != 0)
    {
        return DISPMANX_GRABBER_SETRECT_FAIL;
    }
    
    kprintf("vc_dispmanx_rect_set() returned %d\n", result);
    kprintf("rect = { %d, %d, %d, %d }\n",
            rect.x,
            rect.y,
            rect.width,
            rect.height);
    
    result = vc_dispmanx_resource_read_data(st->resourceHandle,
                                            &rect,
                                            imagePtr,
                                            st->frameInfo.pitch);
    
    if (result != 0)
    {
        return DISPMANX_GRABBER_RESREAD_FAIL;
    }
    return DISPMANX_GRABBER_OK;
}

DispmanxGrabberFrameInfo dispmanx_grabber_frameinfo(DispmanxGrabberState *st)
{
    return st->frameInfo;
}

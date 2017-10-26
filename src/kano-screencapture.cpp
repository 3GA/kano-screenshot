/*
Copyright (c) 2012, Broadcom Europe Ltd
Copyright (c) 2012, Kalle Vahlman <zuh@iki>
                    Tuomas Kulve <tuomas@kulve.fi>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Video encode demo using OpenMAX IL though the ilcient helper library

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <math.h>   

#include "bcm_host.h"
#include "ilclient.h"
}

#include "dispmanx_grabber.h"

// variables used by signal handers
static bool caughtINT = false;

void sigINTHandler(int)
{
    caughtINT = true;
}
void sigHUPHandler(int)
{
    // don't need to do anything - just interrupt wait
}


static void
print_def(OMX_PARAM_PORTDEFINITIONTYPE def)
{
   printf("Port %u: %s %u/%u %u %u %s,%s,%s %ux%u %ux%u @%u %u\n",
	  def.nPortIndex,
	  def.eDir == OMX_DirInput ? "in" : "out",
	  def.nBufferCountActual,
	  def.nBufferCountMin,
	  def.nBufferSize,
	  def.nBufferAlignment,
	  def.bEnabled ? "enabled" : "disabled",
	  def.bPopulated ? "populated" : "not pop.",
	  def.bBuffersContiguous ? "contig." : "not cont.",
	  def.format.video.nFrameWidth,
	  def.format.video.nFrameHeight,
	  def.format.video.nStride,
	  def.format.video.nSliceHeight,
	  def.format.video.xFramerate, def.format.video.eColorFormat);
}

static int
video_encode_test(char *outputfilename,
                  int requestedWidth,
                  int requestedHeight,
                  float rate,
                  int frames,
                  bool stopmotion)

{
   OMX_VIDEO_PARAM_PORTFORMATTYPE format;
   OMX_PARAM_PORTDEFINITIONTYPE def;
   COMPONENT_T *video_encode = NULL;
   COMPONENT_T *list[5];
   OMX_BUFFERHEADERTYPE *buf;
   OMX_BUFFERHEADERTYPE *out;
   OMX_ERRORTYPE r;
   ILCLIENT_T *client;
   int status = 0;
   int framenumber = 0;
   FILE *outf;
   uint64_t us_rate = floor(1000000.0/rate);
   if(stopmotion)
       us_rate = 24*60*60*1000000;

   //const char* imageTypeName = "RGB565";
   //const char* imageTypeName = "RGBA32";
   const char* imageTypeName = "BGR888";
   
   DispmanxGrabberState grabState;
   DispmanxGrabberConfig grabCfg = {
       .requestedWidth = requestedWidth,
       .requestedHeight = requestedHeight,
       .alignLog2 = 5,
       .imageTypeName = imageTypeName, 
       .verbose = true,
       .logPrintf = &printf
   };
   
   int result = dispmanx_grabber_init(&grabState, grabCfg);
   if(result) {
       printf(" dispmanx_grabber_init_failed: %d \n",  result);
       dispmanx_grabber_close(&grabState);
       exit(1);
   }

   DispmanxGrabberFrameInfo frameInfo = dispmanx_grabber_frameinfo(&grabState);

   memset(list, 0, sizeof(list));

   if ((client = ilclient_init()) == NULL) {
      return -3;
   }

   if (OMX_Init() != OMX_ErrorNone) {
      ilclient_destroy(client);
      return -4;
   }

   // create video_encode
   r = ilclient_create_component(client, &video_encode, "video_encode",
				 ILCLIENT_DISABLE_ALL_PORTS |
				 ILCLIENT_ENABLE_INPUT_BUFFERS |
				 ILCLIENT_ENABLE_OUTPUT_BUFFERS);
   if (r != 0) {
      printf
	 ("ilclient_create_component() for video_encode failed with %x!\n",
	  r);
      exit(1);
   }
   list[0] = video_encode;

   // get current settings of video_encode component from port 200
   memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
   def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
   def.nVersion.nVersion = OMX_VERSION;
   def.nPortIndex = 200;

   if (OMX_GetParameter
       (ILC_GET_HANDLE(video_encode), OMX_IndexParamPortDefinition,
	&def) != OMX_ErrorNone) {
      printf("%s:%d: OMX_GetParameter() for video_encode port 200 failed!\n",
	     __FUNCTION__, __LINE__);
      exit(1);
   }

   print_def(def);

   // Port 200: in 1/1 115200 16 enabled,not pop.,not cont. 320x240 320x240 @1966080 20
   def.format.video.nFrameWidth = frameInfo.width;
   def.format.video.nFrameHeight = frameInfo.height;
   def.format.video.xFramerate = 30 << 16;
   def.format.video.nSliceHeight = def.format.video.nFrameHeight;
   def.format.video.nStride = def.format.video.nFrameWidth;
//   def.format.video.eColorFormat = OMX_COLOR_Format16bitRGB565;
   def.format.video.eColorFormat = OMX_COLOR_Format24bitBGR888;
   //def.format.video.eColorFormat = OMX_COLOR_Format32bitARGB8888;
   print_def(def);

   r = OMX_SetParameter(ILC_GET_HANDLE(video_encode),
			OMX_IndexParamPortDefinition, &def);
   if (r != OMX_ErrorNone) {
      printf
	 ("%s:%d: OMX_SetParameter() for video_encode port 200 failed with %x!\n",
	  __FUNCTION__, __LINE__, r);
      exit(1);
   }

   memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   format.nVersion.nVersion = OMX_VERSION;
   format.nPortIndex = 201;
   format.eCompressionFormat = OMX_VIDEO_CodingAVC;

   printf("OMX_SetParameter for video_encode:201...\n");
   r = OMX_SetParameter(ILC_GET_HANDLE(video_encode),
			OMX_IndexParamVideoPortFormat, &format);
   if (r != OMX_ErrorNone) {
      printf
	 ("%s:%d: OMX_SetParameter() for video_encode port 201 failed with %x!\n",
	  __FUNCTION__, __LINE__, r);
      exit(1);
   }

   OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
   // set current bitrate to 1Mbit
   memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
   bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
   bitrateType.nVersion.nVersion = OMX_VERSION;
   bitrateType.eControlRate = OMX_Video_ControlRateVariable;
   bitrateType.nTargetBitrate = 3000000;
   bitrateType.nPortIndex = 201;
   r = OMX_SetParameter(ILC_GET_HANDLE(video_encode),
                       OMX_IndexParamVideoBitrate, &bitrateType);
   if (r != OMX_ErrorNone) {
      printf
        ("%s:%d: OMX_SetParameter() for bitrate for video_encode port 201 failed with %x!\n",
         __FUNCTION__, __LINE__, r);
      exit(1);
   }


   // get current bitrate
   memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
   bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_BITRATETYPE);
   bitrateType.nVersion.nVersion = OMX_VERSION;
   bitrateType.nPortIndex = 201;

   if (OMX_GetParameter
       (ILC_GET_HANDLE(video_encode), OMX_IndexParamVideoBitrate,
       &bitrateType) != OMX_ErrorNone) {
      printf("%s:%d: OMX_GetParameter() for video_encode for bitrate port 201 failed!\n",
            __FUNCTION__, __LINE__);
      exit(1);
   }
   printf("Current Bitrate=%u\n",bitrateType.nTargetBitrate);



   printf("encode to idle...\n");
   if (ilclient_change_component_state(video_encode, OMX_StateIdle) == -1) {
      printf
	 ("%s:%d: ilclient_change_component_state(video_encode, OMX_StateIdle) failed",
	  __FUNCTION__, __LINE__);
   }

   printf("enabling port buffers for 200...\n");
   if (ilclient_enable_port_buffers(video_encode, 200, NULL, NULL, NULL) != 0) {
      printf("enabling port buffers for 200 failed!\n");
      exit(1);
   }

   printf("enabling port buffers for 201...\n");
   if (ilclient_enable_port_buffers(video_encode, 201, NULL, NULL, NULL) != 0) {
      printf("enabling port buffers for 201 failed!\n");
      exit(1);
   }

   printf("encode to executing...\n");
   ilclient_change_component_state(video_encode, OMX_StateExecuting);

   outf = fopen(outputfilename, "w");
   if (outf == NULL) {
      printf("Failed to open '%s' for writing video\n", outputfilename);
      exit(1);
   }

   printf("looping for buffers...\n");
   do {
      buf = ilclient_get_input_buffer(video_encode, 200, 1);
      if (buf == NULL) {
	 printf("Doh, no buffers for me!\n");
      }
      else {
          /* fill it */
          struct timeval tv, tv_next;
          gettimeofday(&tv, NULL);
          uint64_t us = tv.tv_usec + tv.tv_sec * 1000000ull;
          result = dispmanx_grabber_grab(&grabState, buf->pBuffer);
          buf->nFilledLen = frameInfo.frame_size;
          framenumber++;
          
          
          OMX_TICKS time;
          time.nLowPart = us & 0xffffffff;
          time.nHighPart = us >> 32;
          buf->nTimeStamp = time;
          if(framenumber==0){
              buf->nFlags |=OMX_BUFFERFLAG_STARTTIME;
          }

	 if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_encode), buf) !=
	     OMX_ErrorNone) {
	    printf("Error emptying buffer!\n");
	 }

	 out = ilclient_get_output_buffer(video_encode, 201, 1);

	 r = OMX_FillThisBuffer(ILC_GET_HANDLE(video_encode), out);
	 if (r != OMX_ErrorNone) {
	    printf("Error filling buffer: %x\n", r);
	 }

	 if (out != NULL) {
	    if (out->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
	       int i;
	       for (i = 0; i < out->nFilledLen; i++)
		  printf("%x ", out->pBuffer[i]);
	       printf("\n");
	    }
            printf("output timestamp %d",out->nTimeStamp);

	    r = fwrite(out->pBuffer, 1, out->nFilledLen, outf);
	    if (r != out->nFilledLen) {
	       printf("fwrite: Error emptying buffer: %d!\n", r);
	    }
	    else {
                printf("Writing frame %d/%d len %d\n", framenumber, frames,  out->nFilledLen);
	    }
	    out->nFilledLen = 0;
	 }
	 else {
	    printf("Not getting it :(\n");
	 }

         gettimeofday(&tv_next, NULL);
         uint64_t us_next = tv_next.tv_usec + tv_next.tv_sec * 1000000ull;
         uint64_t us_diff = us_next-us;
         uint64_t us_wait = us_rate - us_diff;
         struct timespec ts_wait = {.tv_sec = us_wait/1000000,
                                    .tv_nsec = (us_wait %1000000) * 1000};
         nanosleep(&ts_wait, NULL);
                                    
      }
   }
   while (!caughtINT && framenumber < frames);

   fclose(outf);

   printf("Teardown.\n");

   printf("disabling port buffers for 200 and 201...\n");
   ilclient_disable_port_buffers(video_encode, 200, NULL, NULL, NULL);
   ilclient_disable_port_buffers(video_encode, 201, NULL, NULL, NULL);

   ilclient_state_transition(list, OMX_StateIdle);
   ilclient_state_transition(list, OMX_StateLoaded);

   ilclient_cleanup_components(list);

   OMX_Deinit();

   ilclient_destroy(client);
   return status;
}
void usage(void)
{
    printf("Usage: kano-screencapture [OPTIONS]\n" );
    printf("Options:\n");
    printf("\t[-h <height>] :    output height (default 480)\n");
    printf("\t[-w <width>]  :    output width (default 640)\n");
    printf("\t[-f <frames>] :    number of frames (default 300) \n");
    printf("\t[-r <rate>]   :    frame rate (default 25.0) \n");
    printf("\t[-o <outfile>]:    output file (default kano-screencapture.h264) \n");
    printf("\t[-s]          :    stopmotion mode \n");

    exit(1);
}

int
main(int argc, char **argv)
{
    int requestedWidth = 640;
    int requestedHeight = 480;
    float rate = 25;
    int frames = 300;
    bool stopmotion = false;
    int opt = 0;
    char *outfile = (char *)"kano-screencapture.h264";
    if (argc < 2) {
        usage();
    }
    while ((opt = getopt(argc, argv, "h:w:f:r:o:s")) != -1)
    {
        switch (opt)
        {
        case 'h':
            requestedHeight = atoi(optarg);
            break;
        case 'w':
            requestedWidth = atoi(optarg);
            break;
        case 'f':
            frames = atoi(optarg);
            break;
        case 'r':
            rate = atof(optarg);
            break;
        case 'o':
            outfile = optarg;
            break;
        case 's':
            stopmotion = true;
            break;
        default:
            usage();
        }          
    }

    bcm_host_init();


    // TODO, not sure if encoder will like signals, maybe use some other IPC mechanism
    signal(SIGINT, sigINTHandler); 
    signal(SIGHUP, sigHUPHandler);
    
    return video_encode_test(outfile,
                             requestedWidth,
                             requestedHeight,
                             rate,
                             frames,
                             stopmotion
                            );
}

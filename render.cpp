//cd /root/Bela/projects/Software
//mkdir lib
//cd lib
//gcc -c -D__BELA__ -I/root/Bela/include/ /root/Bela/projects/Software/Robot_Communication_Framework/*.c /root/Bela/projects/Software/Robot_Communication_Framework/*.cpp /root/Bela/projects/Software/Beat-and-Tempo-Tracking/src/*.c /root/Bela/projects/Software/Main/Rhythm_Generators/*.c /root/Bela/projects/Software/Main/*.c
//ar rcs libmylib.a *.o

//in IDE -> settings -> Make Parameters
//CPPFLAGS+=-D__BELA__; LDFLAGS=-L/root/Bela/projects/Software/lib; LDLIBS=-lmylib


#ifdef __BELA__

#include <Bela.h>
#include "Main/Microphone.h"

/*--------------------------------------------------------------------*/
Microphone* mic;
BTT* btt;

/*--------------------------------------------------------------------*/
bool setup(BelaContext *context, void *userData)
{
  Microphone* mic = mic_new();
  if(mic == NULL) {perror("Unable to create microphone object"); exit(-1);}
  BTT* btt = mic_get_btt(mic);
  
  return true;
}

/*--------------------------------------------------------------------*/
void render(BelaContext *context, void *userData)
{
    mic_audio_callback(mic, (float*)context->audioIn, context->audioFrames, context->analogInChannels);
}

/*--------------------------------------------------------------------*/
void cleanup(BelaContext *context, void *userData)
{
  //mic = mic->destroy(mic);
}

#endif// __BELA__

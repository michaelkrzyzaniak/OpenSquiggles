#include <Bela.h>
#include <Midi.h>
#include <stdlib.h>
#include <cmath>

Midi midi;
const char* gMidiPort0 = "hw:1,0,0";


float peakValue = 0;
float thresholdToTrigger = 0.001;
float amountBelowPeak = 0.001;
float rolloffRate = 0.00005;
int triggered = 0;


int gSampleCount = 44100; // how often to send out a control change

/*
 * This callback is called every time a new input Midi message is available
 *
 * Note that this is called in a different thread than the audio processing one.
 *
 */
void midiMessageCallback(MidiChannelMessage message, void* arg){
  if(arg != NULL){
    rt_printf("Message from midi port %s ", (const char*) arg);
  }
  message.prettyPrint();
  if(message.getType() == kmmNoteOn){
    rt_printf("Note On Rec'd\n");
  }
}

bool setup(BelaContext *context, void *userData)
{
  midi.readFrom(gMidiPort0);
  midi.writeTo(gMidiPort0);
  midi.enableParser(true);
  midi.setParserCallback(midiMessageCallback, (void*) gMidiPort0);
  
  /*------------------------*/
  if(context->analogSampleRate > context->audioSampleRate)
  {
    fprintf(stderr, "Error: for this project the sampling rate of the analog inputs has to be <= the audio sample rate\n");
    return false;
  }
  
  
  
  return true;
}

void render(BelaContext *context, void *userData)
{
  for(unsigned int n = 0; n < context->audioFrames; n+=context->analogInChannels)
    {
      if(context->audioIn[n] > 0.75)
        {
        midi_byte_t bytes[3] = {0x90, 64, 127};
        midi.writeOutput(bytes, 3); // send a control change message
        }
    //for(unsigned int ch = 0; ch < context->analogInChannels; ch++)
      //{
      
    //}
  }
  
  // using MIDI control changes
  /*
  for(unsigned int n = 0; n < context->audioFrames; n++){
    static int count = 0;
    if(count % gSampleCount == 0){
      midi_byte_t bytes[3] = {0x90, 64, 127};
      midi.writeOutput(bytes, 3); // send a control change message
    }
    ++count;
  }
  */
}


void cleanup(BelaContext *context, void *userData)
{

}

//_________________________________________________________________________________________________________________________nFO
// "snd_osx.m" - MacOS X Sound driver.
//
// Written by:	Axel 'awe' Wefers			[mailto:awe@fruitz-of-dojo.de].
//				©2001-2006 Fruitz Of Dojo 	[http://www.fruitz-of-dojo.de].
//
// Quakeª is copyrighted by id software		[http://www.idsoftware.com].
//
// Version History:
// v1.0.9: Added function for reserving the buffersize [required to be called before any QuickTime media is loaded].
// v1.0.0: Initial release.
//____________________________________________________________________________________________________________________iNCLUDES

#pragma mark =Includes=

#include <Foundation/Foundation.h>
#include <CoreAudio/AudioHardware.h>
#include "quakedef.h"

#pragma mark -

//_____________________________________________________________________________________________________________________dEFINES

#pragma mark =Defines=

#define OUTPUT_BUFFER_SIZE	(2 * 1024)

#pragma mark -

//___________________________________________________________________________________________________________________vARIABLES

#pragma mark =Variables=

static AudioDeviceID 			gSndDeviceID;
static unsigned char 			gSndBuffer[64*1024];
static volatile BOOL			gSndIOProcIsInstalled = NO;
static UInt32					gSndBufferPosition,
								gSndBufferByteCount;

#pragma mark -

//_________________________________________________________________________________________________________fUNCTION_pROTOTYPES

#pragma mark =Function Prototypes=

static OSStatus SNDDMA_AudioIOProc (AudioDeviceID, const AudioTimeStamp *, const AudioBufferList *,
                                   const AudioTimeStamp *, AudioBufferList *, const AudioTimeStamp *,
                                   void *);

#pragma mark -

//________________________________________________________________________________________________________SNDDMA_AudioIOProc()

OSStatus	SNDDMA_AudioIOProc (AudioDeviceID inDevice,
								const AudioTimeStamp *inNow,
								const AudioBufferList *inInputData,
								const AudioTimeStamp *inInputTime,
								AudioBufferList *outOutputData, 
								const AudioTimeStamp *inOutputTime,
								void *inClientData)
{
    // fixes a rare crash on app exit (race condition in CoreAudio?):
    if (gSndIOProcIsInstalled == YES)
    {
		short *	myDMA		= ((short *) gSndBuffer) + gSndBufferPosition / (shm->samplebits >> 3);
		float *	myOutBuffer = (float *) outOutputData->mBuffers[0].mData;
		UInt16	i			= 0;
	
		// convert the buffer to float, required by CoreAudio:
		for (; i < gSndBufferByteCount; i++)
		{
			*myOutBuffer++	= (*myDMA) * (1.0f / 32768.0f);
			*myDMA++		= 0x0000;
		}
		
		// increase the bufferposition:
		gSndBufferPosition += gSndBufferByteCount * (shm->samplebits >> 3);
		
		if (gSndBufferPosition >= sizeof (gSndBuffer))
		{
			gSndBufferPosition = 0;
		}
	}
	
    // return 0 = no error:
    return 0;
}

//__________________________________________________________________________________________________SNDDMA_ReserveBufferSize()

BOOL	SNDDMA_ReserveBufferSize (void)
{
    OSStatus		myError;
    AudioDeviceID	myAudioDevice;
    UInt32			myPropertySize;
    
    // this function has to be called before any QuickTime movie data is loaded, so that the QuickTime handler
    // knows about our custom buffersize!
    myPropertySize = sizeof (AudioDeviceID);
    myError = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice,
                                        &myPropertySize,&myAudioDevice);
    
    if (!myError && myAudioDevice != kAudioDeviceUnknown)
    {
        UInt32		myBufferByteCount = OUTPUT_BUFFER_SIZE * sizeof (float);

        myPropertySize = sizeof (myBufferByteCount);

        // set the buffersize for the audio device:
        myError = AudioDeviceSetProperty (myAudioDevice, NULL, 0, NO, kAudioDevicePropertyBufferSize,
                                          myPropertySize, &myBufferByteCount);
        
        if (!myError)
        {
            return (YES);
        }
    }
    
    return NO;
}

//_______________________________________________________________________________________________________________SNDDMA_Init()

qboolean SNDDMA_Init (void)
{
    AudioStreamBasicDescription	myBasicDescription;
    UInt32						myPropertySize;

    Con_Printf ("Initializing CoreAudio...\n");
    myPropertySize = sizeof (gSndDeviceID);
    
    // find a suitable audio device:
    if (AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice, &myPropertySize, &gSndDeviceID))
    {
        Con_Printf ("Audio init fails: Can\t get audio device.\n");
        return 0;
    }
    
    // is the device valid?
    if (gSndDeviceID == kAudioDeviceUnknown)
    {
        Con_Printf ("Audio init fails: Unsupported audio device.\n");
        return 0;
    }
    
    // get the buffersize of the audio device [must previously be set via "SNDDMA_ReserveBufferSize ()"]:
    myPropertySize = sizeof (gSndBufferByteCount);
    if (AudioDeviceGetProperty (gSndDeviceID, 0, NO, kAudioDevicePropertyBufferSize,
                                &myPropertySize, &gSndBufferByteCount) || gSndBufferByteCount == 0)
    {
        Con_Printf ("Audio init fails: Can't get audiobuffer.\n");
        return 0;
    }
    
    //check the buffersize:
    gSndBufferByteCount /= sizeof (float);
    if (gSndBufferByteCount != OUTPUT_BUFFER_SIZE)
    {
        Con_Printf ("Audio init: Audiobuffer size is not sufficient for optimized playback!\n");
    }
    if (sizeof (gSndBuffer) % gSndBufferByteCount != 0 ||
        sizeof (gSndBuffer) / gSndBufferByteCount < 2)
    {
        Con_Printf ("Audio init: Bad audiobuffer size!\n");
        return 0;
    }
    
    // get the audiostream format:
    myPropertySize = sizeof (myBasicDescription);
    if (AudioDeviceGetProperty (gSndDeviceID, 0, NO, kAudioDevicePropertyStreamFormat,
                                &myPropertySize, &myBasicDescription))
    {
        Con_Printf ("Audio init fails.\n");
        return 0;
    }
    
    // is the format LinearPCM?
    if (myBasicDescription.mFormatID != kAudioFormatLinearPCM)
    {
        Con_Printf ("Default Audio Device doesn't support Linear PCM!\n");
        return 0;
    }
    
    // is sound ouput suppressed?
    if (!COM_CheckParm ("-nosound"))
    {
		gSndIOProcIsInstalled = YES;
		
        // add the sound FX IO:
        if (AudioDeviceAddIOProc (gSndDeviceID, SNDDMA_AudioIOProc, NULL))
        {
			gSndIOProcIsInstalled = NO;
            Con_Printf ("Audio init fails: Can\'t install IOProc.\n");
            return 0;
        }
        
        // start the sound FX:
        if (AudioDeviceStart (gSndDeviceID, SNDDMA_AudioIOProc))
        {
			gSndIOProcIsInstalled = NO;
            Con_Printf ("Audio init fails: Can\'t start audio.\n");
            return 0;
        }
        gSndIOProcIsInstalled = YES;
    }
    else
    {
        gSndIOProcIsInstalled = NO;
    }
    
    // setup Quake sound variables:
    shm = (void *) Hunk_AllocName (sizeof (*shm), "shm");
    shm->splitbuffer = 0;
    shm->samplebits = 16;
    shm->speed = myBasicDescription.mSampleRate;
    shm->channels = myBasicDescription.mChannelsPerFrame;
    shm->samples = sizeof (gSndBuffer) / (shm->samplebits >> 3);
    shm->samplepos = 0;
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = gSndBufferByteCount;
    shm->buffer = gSndBuffer;
    gSndBufferPosition = 0;
    
    // output a description of the sound format:
    if (!COM_CheckParm ("-nosound"))
    {
        Con_Printf ("Sound Channels: %d\n", shm->channels);
        Con_Printf ("Sound sample bits: %d\n", shm->samplebits);
    }
    
    return 1;
}

//___________________________________________________________________________________________________________SNDDMA_Shutdown()

void	SNDDMA_Shutdown (void)
{
    // shut everything down:
    if (gSndIOProcIsInstalled == YES)
    {
		gSndIOProcIsInstalled = NO;
		
        AudioDeviceStop (gSndDeviceID, SNDDMA_AudioIOProc);
        AudioDeviceRemoveIOProc (gSndDeviceID, SNDDMA_AudioIOProc);
    }
}

//_____________________________________________________________________________________________________________SNDDMA_Submit()

void	SNDDMA_Submit (void)
{
}

//__________________________________________________________________________________________________________SNDDMA_GetDMAPos()

int	SNDDMA_GetDMAPos (void)
{
    if (gSndIOProcIsInstalled == NO)
    {
        return 0;
    }
	
    return gSndBufferPosition / (shm->samplebits >> 3);
}

//_________________________________________________________________________________________________________________________eOF

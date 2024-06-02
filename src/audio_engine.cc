#include "audio_engine.h"

#include <string.h>

#include <SDL.h>

namespace fallout {

#define AUDIO_ENGINE_SOUND_BUFFERS 8

struct AudioEngineSoundBuffer {
    bool active;
    unsigned int size;
    int bitsPerSample;
    int channels;
    int rate;
    void* data;
    int volume;
    bool playing;
    bool looping;
    unsigned int pos;
    SDL_AudioStream* stream;
    SDL_mutex* mutex;
};

extern bool GNW95_isActive;

static bool soundBufferIsValid(int soundBufferIndex);
static void audioEngineMixin(void* userData, Uint8* stream, int length);

static SDL_AudioSpec gAudioEngineSpec;
static SDL_AudioDeviceID gAudioEngineDeviceId = -1;
static AudioEngineSoundBuffer gAudioEngineSoundBuffers[AUDIO_ENGINE_SOUND_BUFFERS];

static bool audioEngineIsInitialized()
{
    return gAudioEngineDeviceId != -1;
}

static bool soundBufferIsValid(int soundBufferIndex)
{
    return soundBufferIndex >= 0 && soundBufferIndex < AUDIO_ENGINE_SOUND_BUFFERS;
}

static void audioEngineMixin(void* userData, Uint8* stream, int length)
{
    memset(stream, gAudioEngineSpec.silence, length);

    if (!GNW95_isActive) {
        return;
    }

    for (int index = 0; index < AUDIO_ENGINE_SOUND_BUFFERS; index++) {
        AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[index]);
        // for some strange reason std::lock_guard isn't locking the mutex properly on Vita
        // which results in audioEngineSoundBufferRelease call during this loop and subsequent crash
        // replacing the std::recursive_mutex with SDL_mutex is fixing this issue
        SDL_LockMutex(soundBuffer->mutex);

        if (soundBuffer->active && soundBuffer->playing) {
            int srcFrameSize = soundBuffer->bitsPerSample / 8 * soundBuffer->channels;

            unsigned char buffer[1024];
            int pos = 0;
            while (pos < length) {
                int remaining = length - pos;
                if (remaining > sizeof(buffer)) {
                    remaining = sizeof(buffer);
                }

                // TODO: Make something better than frame-by-frame convertion.
                SDL_AudioStreamPut(soundBuffer->stream, (unsigned char*)soundBuffer->data + soundBuffer->pos, srcFrameSize);
                soundBuffer->pos += srcFrameSize;

                int bytesRead = SDL_AudioStreamGet(soundBuffer->stream, buffer, remaining);
                if (bytesRead == -1) {
                    break;
                }

                SDL_MixAudioFormat(stream + pos, buffer, gAudioEngineSpec.format, bytesRead, soundBuffer->volume);

                if (soundBuffer->pos >= soundBuffer->size) {
                    if (soundBuffer->looping) {
                        soundBuffer->pos %= soundBuffer->size;
                    } else {
                        soundBuffer->playing = false;
                        break;
                    }
                }

                pos += bytesRead;
            }
        }

        SDL_UnlockMutex(soundBuffer->mutex);
    }
}

bool audioEngineInit()
{
    for (int index = 0; index < AUDIO_ENGINE_SOUND_BUFFERS; index++) {
        AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[index]);
        soundBuffer->mutex = SDL_CreateMutex();
    }

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
        return false;
    }

    SDL_AudioSpec desiredSpec;
    desiredSpec.freq = 22050;
    desiredSpec.format = AUDIO_S16;
    desiredSpec.channels = 2;
    desiredSpec.samples = 1024;
    desiredSpec.callback = audioEngineMixin;

    gAudioEngineDeviceId = SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &gAudioEngineSpec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (gAudioEngineDeviceId == -1) {
        return false;
    }

    SDL_PauseAudioDevice(gAudioEngineDeviceId, 0);

    return true;
}

void audioEngineExit()
{
    if (audioEngineIsInitialized()) {
        SDL_CloseAudioDevice(gAudioEngineDeviceId);
        gAudioEngineDeviceId = -1;
    }

    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    for (int index = 0; index < AUDIO_ENGINE_SOUND_BUFFERS; index++) {
        AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[index]);
        SDL_DestroyMutex(soundBuffer->mutex);
    }
}

void audioEnginePause()
{
    if (audioEngineIsInitialized()) {
        SDL_PauseAudioDevice(gAudioEngineDeviceId, 1);
    }
}

void audioEngineResume()
{
    if (audioEngineIsInitialized()) {
        SDL_PauseAudioDevice(gAudioEngineDeviceId, 0);
    }
}

int audioEngineCreateSoundBuffer(unsigned int size, int bitsPerSample, int channels, int rate)
{
    if (!audioEngineIsInitialized()) {
        return -1;
    }

    for (int index = 0; index < AUDIO_ENGINE_SOUND_BUFFERS; index++) {
        AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[index]);
        SDL_LockMutex(soundBuffer->mutex);

        if (!soundBuffer->active) {
            soundBuffer->active = true;
            soundBuffer->size = size;
            soundBuffer->bitsPerSample = bitsPerSample;
            soundBuffer->channels = channels;
            soundBuffer->rate = rate;
            soundBuffer->volume = SDL_MIX_MAXVOLUME;
            soundBuffer->playing = false;
            soundBuffer->looping = false;
            soundBuffer->pos = 0;
            soundBuffer->data = malloc(size);
            soundBuffer->stream = SDL_NewAudioStream(bitsPerSample == 16 ? AUDIO_S16 : AUDIO_S8, channels, rate, gAudioEngineSpec.format, gAudioEngineSpec.channels, gAudioEngineSpec.freq);
            SDL_UnlockMutex(soundBuffer->mutex);
            return index;
        }

        SDL_UnlockMutex(soundBuffer->mutex);
    }

    return -1;
}

bool audioEngineSoundBufferRelease(int soundBufferIndex)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    soundBuffer->active = false;

    free(soundBuffer->data);
    soundBuffer->data = NULL;

    SDL_FreeAudioStream(soundBuffer->stream);
    soundBuffer->stream = NULL;

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferSetVolume(int soundBufferIndex, int volume)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    soundBuffer->volume = volume;

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferGetVolume(int soundBufferIndex, int* volumePtr)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    *volumePtr = soundBuffer->volume;

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferSetPan(int soundBufferIndex, int pan)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    // NOTE: Audio engine does not support sound panning. I'm not sure it's
    // even needed. For now this value is silently ignored.

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferPlay(int soundBufferIndex, unsigned int flags)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    soundBuffer->playing = true;

    if ((flags & AUDIO_ENGINE_SOUND_BUFFER_PLAY_LOOPING) != 0) {
        soundBuffer->looping = true;
    }

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferStop(int soundBufferIndex)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    soundBuffer->playing = false;

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferGetCurrentPosition(int soundBufferIndex, unsigned int* readPosPtr, unsigned int* writePosPtr)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    if (readPosPtr != NULL) {
        *readPosPtr = soundBuffer->pos;
    }

    if (writePosPtr != NULL) {
        *writePosPtr = soundBuffer->pos;

        if (soundBuffer->playing) {
            // 15 ms lead
            // See: https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mt708925(v=vs.85)#remarks
            *writePosPtr += soundBuffer->rate / 150;
            *writePosPtr %= soundBuffer->size;
        }
    }

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferSetCurrentPosition(int soundBufferIndex, unsigned int pos)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    soundBuffer->pos = pos % soundBuffer->size;

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferLock(int soundBufferIndex, unsigned int writePos, unsigned int writeBytes, void** audioPtr1, unsigned int* audioBytes1, void** audioPtr2, unsigned int* audioBytes2, unsigned int flags)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    if (audioBytes1 == NULL) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    if ((flags & AUDIO_ENGINE_SOUND_BUFFER_LOCK_FROM_WRITE_POS) != 0) {
        if (!audioEngineSoundBufferGetCurrentPosition(soundBufferIndex, NULL, &writePos)) {
            SDL_UnlockMutex(soundBuffer->mutex);
            return false;
        }
    }

    if ((flags & AUDIO_ENGINE_SOUND_BUFFER_LOCK_ENTIRE_BUFFER) != 0) {
        writeBytes = soundBuffer->size;
    }

    if (writePos + writeBytes <= soundBuffer->size) {
        *(unsigned char**)audioPtr1 = (unsigned char*)soundBuffer->data + writePos;
        *audioBytes1 = writeBytes;

        if (audioPtr2 != NULL) {
            *audioPtr2 = NULL;
        }

        if (audioBytes2 != NULL) {
            *audioBytes2 = 0;
        }
    } else {
        unsigned int remainder = writePos + writeBytes - soundBuffer->size;
        *(unsigned char**)audioPtr1 = (unsigned char*)soundBuffer->data + writePos;
        *audioBytes1 = soundBuffer->size - writePos;

        if (audioPtr2 != NULL) {
            *(unsigned char**)audioPtr2 = (unsigned char*)soundBuffer->data;
        }

        if (audioBytes2 != NULL) {
            *audioBytes2 = writeBytes - (soundBuffer->size - writePos);
        }
    }

    // TODO: Mark range as locked.

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferUnlock(int soundBufferIndex, void* audioPtr1, unsigned int audioBytes1, void* audioPtr2, unsigned int audioBytes2)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    // TODO: Mark range as unlocked.

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

bool audioEngineSoundBufferGetStatus(int soundBufferIndex, unsigned int* statusPtr)
{
    if (!audioEngineIsInitialized()) {
        return false;
    }

    if (!soundBufferIsValid(soundBufferIndex)) {
        return false;
    }

    AudioEngineSoundBuffer* soundBuffer = &(gAudioEngineSoundBuffers[soundBufferIndex]);
    SDL_LockMutex(soundBuffer->mutex);

    if (!soundBuffer->active) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    if (statusPtr == NULL) {
        SDL_UnlockMutex(soundBuffer->mutex);
        return false;
    }

    *statusPtr = 0;

    if (soundBuffer->playing) {
        *statusPtr |= AUDIO_ENGINE_SOUND_BUFFER_STATUS_PLAYING;

        if (soundBuffer->looping) {
            *statusPtr |= AUDIO_ENGINE_SOUND_BUFFER_STATUS_LOOPING;
        }
    }

    SDL_UnlockMutex(soundBuffer->mutex);
    return true;
}

} // namespace fallout

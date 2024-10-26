#include <iostream>
#include <ostream>
#include <stdexcept>
#include <cassert>
#include "sound_system.hpp"
#include "load_sound_file.hpp"

#include "AL/alc.h"

SoundSystem::SoundSystem() { initialize_openal(); }
SoundSystem::SoundSystem(int num_sources) {
    initialize_openal();
    init_sound_buffers();
    init_sound_sources(num_sources);
}

SoundSystem::~SoundSystem() { deinitialize_openal(); }

void SoundSystem::initialize_openal() {
    const ALCchar *name;
    ALCdevice *device;
    ALCcontext *ctx;

    /* Open and initialize a device */

    device = alcOpenDevice(NULL); // open the preferred device

    if (!device) {
        fprintf(stderr, "Could not open a device!\n");
        throw std::runtime_error("could not open a device");
    }

    ctx = alcCreateContext(device, NULL);
    if (ctx == NULL || alcMakeContextCurrent(ctx) == ALC_FALSE) {
        if (ctx != NULL)
            alcDestroyContext(ctx);
        alcCloseDevice(device);
        fprintf(stderr, "Could not set a context!\n");
        throw std::runtime_error("Could not set a context!\n");
    }

    name = NULL;
    if (alcIsExtensionPresent(device, "ALC_ENUMERATE_ALL_EXT"))
        name = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
    if (!name || alcGetError(device) != AL_NO_ERROR)
        name = alcGetString(device, ALC_DEVICE_SPECIFIER);

    printf("Opened \"%s\"\n", name);
}

void SoundSystem::deinitialize_openal() {

    /* All done. Delete resources, and close down OpenAL. */
    for (auto const &[sound_name, buffer_id] : sound_name_to_loaded_buffer) {
        alDeleteBuffers(1, &buffer_id);
    }

    for (auto const &[source_name, source_id] : source_name_to_source_id) {
        alDeleteSources(1, &source_id);
    }

    // NEW
    for (ALuint source : sound_sources) {
        alDeleteSources(1, &source);
    }
    // NEW

    ALCdevice *device;
    ALCcontext *ctx;

    ctx = alcGetCurrentContext();
    if (ctx == NULL)
        return;

    device = alcGetContextsDevice(ctx);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
}

void SoundSystem::create_sound_source(const std::string &source_name) {

    bool source_name_available = source_name_to_source_id.count(source_name) == 0;
    if (!source_name_available) {
        throw std::runtime_error("a source with the same name was already created.");
    }

    /* Create the source to play the sound with. */
    ALuint source_id = 0;
    alGenSources(1, &source_id);
    assert(alGetError() == AL_NO_ERROR && "Failed to setup sound source");

    source_name_to_source_id[source_name] = source_id;
}

/**
 * \bug if you try to play a sound which is already playing this fails, need to enqueue the sound for playback or
 * somehow play two at once? or just overwrite the last sound playing.
 */
void SoundSystem::play_sound(const std::string &source_name, const std::string &sound_name) {
    bool source_exists = source_name_to_source_id.count(source_name) == 1;
    bool sound_exists = sound_name_to_loaded_buffer.count(sound_name) == 1;

    if (!sound_exists) {
        throw std::runtime_error("You tried to play a sound which doesn't exist.");
    }
    if (!source_exists) {
        throw std::runtime_error("You tried to play a sound from a source which doesn't exist.");
    }

    ALuint loaded_sound_buffer_id = sound_name_to_loaded_buffer[sound_name];
    if (loaded_sound_buffer_id == 0) {
        std::cerr << "Loaded sound buffer ID is invalid!" << std::endl;
        return;
    }

    ALuint source_id = source_name_to_source_id[source_name];

    // Check the state of the source
    ALint state;
    alGetSourcei(source_id, AL_SOURCE_STATE, &state);
    if (state == AL_PLAYING) {
        alSourceStop(source_id);
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cerr << "OpenAL error stopping source: " << alGetString(error) << std::endl;
            return;
        }
    }

    // Now it's safe to set the buffer
    alSourcei(source_id, AL_BUFFER, (ALint)loaded_sound_buffer_id);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL error setting buffer: " << alGetString(error) << std::endl;
        return;
    }

    // Play the source
    alSourcePlay(source_id);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL error playing source: " << alGetString(error) << std::endl;
    }
}

void SoundSystem::load_sound_into_system_for_playback(const std::string &sound_name, const char *filename) {
    bool sound_name_available = sound_name_to_loaded_buffer.count(sound_name) == 0;
    if (!sound_name_available) {
        throw std::runtime_error("a sound with the same name was already loaded.");
    }

    ALuint sound_buffer = load_sound_and_generate_openal_buffer(filename);

    if (!sound_buffer) {
        deinitialize_openal();
        throw std::runtime_error("failed to generate sound buffer");
    }

    sound_name_to_loaded_buffer[sound_name] = sound_buffer;
}

void SoundSystem::set_listener_position(float x, float y, float z) {
    ALfloat listener_pos[] = {x, y, z};
    alListenerfv(AL_POSITION, listener_pos);
    assert(alGetError() == AL_NO_ERROR && "Failed to setup sound source");
}

void SoundSystem::set_source_gain(const std::string &source_name, float gain) {

    assert(0 <= gain && gain <= 1);

    bool source_exists = source_name_to_source_id.count(source_name) == 1;
    if (!source_exists) {
        throw std::runtime_error("you tried to play a sound from a source which doesn't exist");
    }

    ALuint source_id = source_name_to_source_id[source_name];

    alGetError(); // clear error state
    alSourcef(source_id, AL_GAIN, gain);
    assert(alGetError() == AL_NO_ERROR && "Failed to set gain");

    //    if ((error = alGetError()) != AL_NO_ERROR)
    //        DisplayALError("alSourcef 0 AL_GAIN : \n", error);
}

void SoundSystem::set_source_looping_option(const std::string &source_name, bool looping) {

    bool source_exists = source_name_to_source_id.count(source_name) == 1;
    if (!source_exists) {
        throw std::runtime_error("you tried to play a sound from a source which doesn't exist");
    }

    ALuint source_id = source_name_to_source_id[source_name];

    ALboolean looping_status = looping ? AL_TRUE : AL_FALSE;

    alSourcei(source_id, AL_LOOPING, looping_status);
    assert(alGetError() == AL_NO_ERROR && "Failed to set looping option");
    //    if ((error = alGetError()) != AL_NO_ERROR)
    //        DisplayALError("alSourcei 0 AL_LOOPING true: \n", error);
}

// NEW
void SoundSystem::init_sound_buffers() {
    // this needs to be generalized soon
    sound_buffers[SoundType::SOUND_1] = load_sound_and_generate_openal_buffer("assets/sounds/BD2575.WAV");
    sound_buffers[SoundType::SOUND_2] = load_sound_and_generate_openal_buffer("assets/sounds/CH.WAV");
    sound_buffers[SoundType::SOUND_3] = load_sound_and_generate_openal_buffer("assets/sounds/MC50.WAV");
}

void SoundSystem::init_sound_sources(int num_sources) {
    for (int i = 0; i < num_sources; i++) {
        ALuint source;
        alGenSources(1, &source);
        sound_sources.push_back(source);
    }
}

void SoundSystem::queue_sound(SoundType type, glm::vec3 position) { sound_to_play_queue.push({type, position}); }

void SoundSystem::play_all_sounds() {
    while (!sound_to_play_queue.empty()) {
        QueuedSound queued_sound = sound_to_play_queue.front();
        sound_to_play_queue.pop();

        ALuint source = get_available_source();
        if (source != 0) {
            ALuint buffer = sound_buffers[queued_sound.type];
            alSourcei(source, AL_BUFFER, buffer);
            alSource3f(source, AL_POSITION, queued_sound.position.x, queued_sound.position.y, queued_sound.position.z);
            alSourcePlay(source);
        } else {
            std::cout << "bad source" << std::endl;
        }
    }
}

ALuint SoundSystem::get_available_source() {
    for (ALuint source : sound_sources) {
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            return source;
        }
    }
    return 0; // no available source
}
// NEW

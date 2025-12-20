#include <iostream>
#include <ostream>
#include <stdexcept>
#include <cassert>
#include "sound_system.hpp"
#include "load_sound_file.hpp"
#include "openal_utils/openal_utils.hpp"

#include <AL/alc.h>

SoundSystem::SoundSystem(int num_sources, const std::unordered_map<SoundType, std::string> &sound_type_to_file) {
    GlobalLogSection _("sound system constructor");
    initialize_openal();
    init_sound_buffers(sound_type_to_file);
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

    global_logger->info("Just initialized openal with: {}", name);
}

void SoundSystem::deinitialize_openal() {

    /* All done. Delete resources, and close down OpenAL. */
    for (auto const &[sound_name, buffer_id] : sound_name_to_loaded_buffer_id) {
        openal_utils::delete_buffer(buffer_id);
    }

    for (auto const &[source_name, source_id] : source_name_to_source_id) {
        openal_utils::delete_source(source_id);
    }

    // NEW
    for (ALuint source : sound_sources) {
        openal_utils::delete_source(source);
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

    ALuint source_id = openal_utils::create_source();
    source_name_to_source_id[source_name] = source_id;
}

/**
 * I think this is deprecated
 * \bug if you try to play a sound which is already playing this fails, need to enqueue the sound for playback or
 * somehow play two at once? or just overwrite the last sound playing.
 */
void SoundSystem::play_sound(const std::string &source_name, const std::string &sound_name) {
    bool source_exists = source_name_to_source_id.count(source_name) == 1;
    bool sound_exists = sound_name_to_loaded_buffer_id.count(sound_name) == 1;

    if (!sound_exists) {
        throw std::runtime_error("You tried to play a sound which doesn't exist.");
    }
    if (!source_exists) {
        throw std::runtime_error("You tried to play a sound from a source which doesn't exist.");
    }

    ALuint loaded_sound_buffer_id = sound_name_to_loaded_buffer_id[sound_name];
    if (loaded_sound_buffer_id == 0) {
        std::cerr << "Loaded sound buffer ID is invalid!" << std::endl;
        return;
    }

    ALuint source_id = source_name_to_source_id[source_name];
    if (openal_utils::get_source_state(source_id) == AL_PLAYING) {
        openal_utils::stop_source(source_id);
    }

    openal_utils::set_source_buffer(source_id, loaded_sound_buffer_id);
    openal_utils::play_source(source_id);
}

void SoundSystem::load_sound_into_system_for_playback(const std::string &sound_name, const char *filename) {
    bool sound_name_available = sound_name_to_loaded_buffer_id.count(sound_name) == 0;
    if (!sound_name_available) {
        throw std::runtime_error("a sound with the same name was already loaded.");
    }

    ALuint sound_buffer = load_sound_and_generate_openal_buffer(filename);

    if (!sound_buffer) {
        deinitialize_openal();
        throw std::runtime_error("failed to generate sound buffer");
    }

    sound_name_to_loaded_buffer_id[sound_name] = sound_buffer;
}

void SoundSystem::set_listener_position(float x, float y, float z) {
    ALfloat listener_pos[] = {x, y, z};
    alListenerfv(AL_POSITION, listener_pos);
    assert(alGetError() == AL_NO_ERROR && "Failed to setup sound source");
}

/*
Think of "AT" as a string attached to your nose, and think of "UP" as a string attached to the top of your head.

Without the string attached to the top of your head, you could tilt your head clockwise/counterclockwise and still be
facing "AT". But since you can tilt your head, there's no way for the computer to be sure whether something to the
canonical "right" should sound in your right ear (the top of your head faces "upwards") or your left ear (the top of
your head faces "downwards" because you're upside down). The "AT" and "UP" vectors pin the listener's "head" such that
there's no ambiguity for which way it's facing, and which way it's oriented.

There are actually 3 vectors you need to set: Position, "AT", and "UP". Position 0,0,0 means the head is at the center
of the universe. "AT" 0,0,-1 means the head is looking into the screen, and "UP" is usually 0,1,0, such that the top of
the "head" is pointing up. With this setup, anything the user sees on the left side of the screen will sound in his left
ear. The only time you'd choose something different is in a first-person style game where the player moves around in a
virtual 3d world. The vectors don't have to be normalized, actually, so you could use 0,42,0 for "UP" and it would do
the same thing as 0,1,0.

If you do change "AT" and "UP" from their canonical values, the vectors MUST be perpendicular.
*/
void SoundSystem::set_listener_orientation(const glm::vec3 &forward, const glm::vec3 &up) {
    ALfloat listener_orientation[] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};
    alListenerfv(AL_ORIENTATION, listener_orientation);
    assert(alGetError() == AL_NO_ERROR && "Failed to set listener orientation");
}

void SoundSystem::set_source_gain_by_name(const std::string &source_name, float gain) {

    assert(0 <= gain && gain <= 1);

    bool source_exists = source_name_to_source_id.count(source_name) == 1;
    if (!source_exists) {
        throw std::runtime_error("you tried to play a sound from a source which doesn't exist");
    }

    openal_utils::set_source_gain(source_name_to_source_id[source_name], gain);
}

void SoundSystem::set_source_looping_by_name(const std::string &source_name, bool looping) {

    bool source_exists = source_name_to_source_id.count(source_name) == 1;
    if (!source_exists) {
        throw std::runtime_error("you tried to play a sound from a source which doesn't exist");
    }

    ALuint source_id = source_name_to_source_id[source_name];
    openal_utils::set_source_looping(source_id, looping);
}

// NEW
//
void SoundSystem::init_sound_buffers(const std::unordered_map<SoundType, std::string> &sound_type_to_file) {
    GlobalLogSection _("init_sound_buffers");
    for (auto &pair : sound_type_to_file) {
        SoundType sound_type = pair.first;
        std::string file_path = pair.second;
        global_logger->debug("about to initialize sound buffer for: {}", file_path);
        sound_type_to_buffer_id[sound_type] = load_sound_and_generate_openal_buffer(file_path.c_str());
    }
}

void SoundSystem::init_sound_sources(int num_sources) {
    for (int i = 0; i < num_sources; i++) {
        ALuint source;
        alGenSources(1, &source);
        sound_sources.push_back(source);
    }
}

void SoundSystem::queue_sound(SoundType type, glm::vec3 position, float gain) {
    sound_to_play_queue.push({type, position, gain});
}

// NOTE: I called this queue looping sound to match the naming of the other, one and because eventually I do want
// it to use a queuing method, but right now it will start the sound instantly
[[nodiscard]] unsigned int SoundSystem::queue_looping_sound(SoundType type, glm::vec3 position, float gain) {
    ALuint source_id = get_available_source_id();
    if (source_id != 0) {
        ALuint buffer_id = sound_type_to_buffer_id[type];
        openal_utils::set_source_looping(source_id, true);
        openal_utils::set_source_buffer(source_id, buffer_id);
        openal_utils::set_source_position(source_id, position);
        openal_utils::set_source_gain(source_id, gain);
        openal_utils::play_source(source_id);
        return source_id;
    } else {
        // TODO: return an optional or something, this is bad... just putting it here so the return type is valid
        return 999;
        std::cout << "bad source" << std::endl;
    }
}

void SoundSystem::stop_looping_sound(const unsigned int &source_id) {
    openal_utils::set_source_looping(source_id, false);

    ALint state = openal_utils::get_source_state(source_id);
    if (state == AL_PLAYING || state == AL_PAUSED) {
        openal_utils::stop_source(source_id);
    } else {
        std::cout << "Warning: Source was not playing or paused." << std::endl;
    }

    openal_utils::detach_source_buffer(source_id);
}

void SoundSystem::play_all_sounds() {
    while (!sound_to_play_queue.empty()) {
        QueuedSound queued_sound = sound_to_play_queue.front();
        sound_to_play_queue.pop();

        ALuint source = get_available_source_id();

        if (source != 0) {
            ALuint buffer = sound_type_to_buffer_id[queued_sound.type];
            // NOTE: the source becomes dirty, ie we've modified its openal state (ie, gain pitch position, buffer), but
            // that's not a big deal because every time a sound is played it will override those properties, we use
            // default parameters so that they're all defined, not sure if this is a good long term approach yet
            openal_utils::set_source_buffer(source, buffer);
            openal_utils::set_source_position(source, queued_sound.position);
            openal_utils::set_source_gain(source, queued_sound.gain);

            openal_utils::play_source(source);
        } else {
            std::cout << "Error: No available audio source." << std::endl;
        }
    }
}

ALuint SoundSystem::get_available_source_id() {
    for (ALuint source : sound_sources) {
        if (openal_utils::get_source_state(source) != AL_PLAYING) {
            return source;
        }
    }
    return 0; // no available source
}
// NEW

#ifndef SOUND_SYSTEM_HPP
#define SOUND_SYSTEM_HPP

#include <AL/al.h>
#include <glm/fwd.hpp>
#include <map>
#include <queue>
#include <string>
#include <glm/glm.hpp>
#include <unordered_map>

#include "sbpt_generated_includes.hpp"

/* Sound System:
 * - play a sound right now, at a position without caring about any of the implementation details
 * - play a sound right now, have it looping forever until I turn it off
 * - have a deeper understanding of the openal api, and allow the user to create their own sound sources
 *   which do not interfere with any of the above
 * - given a sound source we can queue up a sequence of sounds to be played sequentially over time.
 *
 */

struct QueuedSound {
    SoundType type;
    glm::vec3 position;
};

class SoundSystem {
  public:
    ConsoleLogger logger = ConsoleLogger("sound_system");

    // TODO: in the future I specifying the num sources should be optional, otherwise we get new ones as needed.
    SoundSystem(int num_sources, std::unordered_map<SoundType, std::string> &sound_type_to_file);
    void queue_sound(SoundType type, glm::vec3 position = glm::vec3(0));
    // returns the id of the source that will be playing that looping sound
    [[nodiscard]] unsigned int queue_looping_sound(SoundType type, glm::vec3 position);
    // pass the id of the source that's playing the sound here to turn it off
    void stop_looping_sound(const unsigned int &source_id);

    // plays all queued sounds
    void play_all_sounds();

    void set_listener_position(float x, float y, float z);
    // NEW

    // TODO: delete this soon
    SoundSystem();

    ~SoundSystem();

    void load_sound_into_system_for_playback(const std::string &sound_name, const char *filename);
    void create_sound_source(const std::string &source_name);
    void set_source_gain_by_name(const std::string &source_name, float gain);

    void set_source_looping_by_name(const std::string &source_name, bool looping);

    // deprecated
    void play_sound(const std::string &source_name, const std::string &sound_name);
    void set_listener_orientation(const glm::vec3 &forward, const glm::vec3 &up);

  private:
    std::map<std::string, ALuint> sound_name_to_loaded_buffer_id;
    std::map<std::string, ALuint> source_name_to_source_id;

    // NEW
    std::vector<ALuint> sound_sources;                             // Pool of sound sources
    std::unordered_map<SoundType, ALuint> sound_type_to_buffer_id; // Map of sound buffers
    std::queue<QueuedSound> sound_to_play_queue;                   // Queue of sounds to play
                                                                   // NEW

    // Helper functions
    ALuint get_available_source_id();
    void init_sound_buffers(std::unordered_map<SoundType, std::string> &sound_type_to_file);
    void init_sound_sources(int num_sources);

    void initialize_openal();
    void deinitialize_openal();
};

#endif // SOUND_SYSTEM_HPP

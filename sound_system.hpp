#ifndef SOUND_SYSTEM_HPP
#define SOUND_SYSTEM_HPP

#include <AL/al.h>
#include <map>
#include <queue>
#include <string>
#include <glm/glm.hpp>
#include <unordered_map>

#include "sbpt_generated_includes.hpp"

// structure representing a sound to be queued
struct QueuedSound {
    SoundType type;
    glm::vec3 position;
};

class SoundSystem {
  public:
    // NEW
    SoundSystem(int num_sources, std::unordered_map<SoundType, std::string> &sound_type_to_file);
    void queue_sound(SoundType type, glm::vec3 position);
    [[nodiscard]] unsigned int queue_looping_sound(SoundType type, glm::vec3 position);
    void stop_looping_sound(const unsigned int &source_id);
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

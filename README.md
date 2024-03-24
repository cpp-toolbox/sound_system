# Info

sound system which wraps openal

# Dependencies
- [openal-soft](https://github.com/kcat/openal-soft)
- [libsndfile](https://github.com/libsndfile/libsndfile)

# CMake

```
...

# openal-soft: for sound

set(LIBTYPE STATIC)

add_subdirectory(external_libraries/openal-soft)
include_directories(external_libraries/openal-soft/include)

# libsndfile: for loading sound files

set(ENABLE_EXTERNAL_LIBS OFF)
# list(APPEND CMAKE_MODULE_PATH libsndfile/cmake/FindOgg.cmake libsndfile/cmake/FindVorbis.cmake libsndfile/cmake/FindFLAC.cmake libsndfile/cmake/FindOpus.cmake)
#list(APPEND CMAKE_MODULE_PATH libsndfile/cmake/FindOgg.cmake)
#list(APPEND CMAKE_MODULE_PATH libsndfile/cmake/FindVorbis.cmake)
#list(APPEND CMAKE_MODULE_PATH libsndfile/cmake/FindFLAC.cmake)
#list(APPEND CMAKE_MODULE_PATH libsndfile/cmake/FindOpus.cmake)
add_subdirectory(external_libraries/libsndfile)
include_directories(external_libraries/libsndfile/include)

... 

target_link_libraries(your_project_name ... OpenAL SndFile::sndfile)
```

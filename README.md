# Info

sound system which wraps openal 

# warning 
You must define a subproject called sound_types where you have an enum like this:

`sound_types.hpp`
```cpp
#ifndef SOUND_TYPES_HPP
#define SOUND_TYPES_HPP
// Enum representing different sound types
enum class SoundType {
    SOUND_1,
    SOUND_2,
    SOUND_3,
};
#endif //SOUND_TYPES_HPP
```

also give it a `sbpt.ini` file:
```
[subproject]
export = sound_types.hpp
```

and be sure to run `sbpt.py` so that the sound system will know where to find that.

# Dependencies
- openal-soft/1.23.1 
- libsndfile/1.2.2


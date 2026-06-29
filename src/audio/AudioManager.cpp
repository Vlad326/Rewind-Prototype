// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Kolmogorov Vlad
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "audio/AudioManager.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

AudioManager::AudioManager() : device(nullptr), context(nullptr), masterVolume(1.0f) {}

AudioManager::~AudioManager() {
    for (ALuint source : activeSources) {
        alDeleteSources(1, &source);
    }
    activeSources.clear();

    for (auto& pair : soundCache) {
        alDeleteBuffers(1, &pair.second);
    }
    soundCache.clear();

    if (context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        context = nullptr;
    }
    if (device) {
        alcCloseDevice(device);
        device = nullptr;
    }
}

bool AudioManager::init() {
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "[AudioManager] Failed to open OpenAL device!" << std::endl;
        return false;
    }

    context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "[AudioManager] Failed to create OpenAL context!" << std::endl;
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }

    if (!alcMakeContextCurrent(context)) {
        std::cerr << "[AudioManager] Failed to make OpenAL context current!" << std::endl;
        alcDestroyContext(context);
        context = nullptr;
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "[AudioManager] OpenAL initialization error: " << alGetString(error) << std::endl;
        return false;
    }

    ALint distanceModel;
    alGetIntegerv(AL_DISTANCE_MODEL, &distanceModel);
    std::cout << "[AudioManager] Active distance model: 0x" << std::hex << distanceModel;
    if (distanceModel == 0xD002) std::cout << " (AL_INVERSE_DISTANCE_CLAMPED)";
    else if (distanceModel == 0xD001) std::cout << " (AL_INVERSE_DISTANCE)";
    else if (distanceModel == 0xD000) std::cout << " (AL_NONE)";
    else std::cout << " (unknown)";
    std::cout << std::dec << std::endl;

    std::cout << "[AudioManager] OpenAL initialized successfully." << std::endl;
    return true;
}

void AudioManager::setListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    float orientation[6] = {
        forward.x, forward.y, forward.z,
        up.x, up.y, up.z
    };
    alListenerfv(AL_ORIENTATION, orientation);
    alListenerf(AL_GAIN, masterVolume);
}

ALuint AudioManager::loadSound(const std::string& filepath) {
    auto it = soundCache.find(filepath);
    if (it != soundCache.end()) {
        return it->second;
    }

    ALuint buffer = loadWAV(filepath);
    if (buffer != 0) {
        soundCache[filepath] = buffer;
    }
    return buffer;
}

ALuint AudioManager::loadWAV(const std::string& filepath) {
    drwav wav;
    if (!drwav_init_file(&wav, filepath.c_str(), nullptr)) {
        std::cerr << "[AudioManager] Failed to load WAV: " << filepath << std::endl;
        return 0;
    }

    uint64_t totalFrameCount = wav.totalPCMFrameCount;
    uint64_t totalSampleCount = totalFrameCount * wav.channels;
    std::vector<int16_t> samples(totalSampleCount);
    size_t samplesRead = drwav_read_pcm_frames_s16(&wav, totalFrameCount, samples.data());
    
    if (samplesRead == 0) {
        std::cerr << "[AudioManager] No samples read from WAV: " << filepath << std::endl;
        drwav_uninit(&wav);
        return 0;
    }

    if (wav.channels > 1) {
        std::cout << "[AudioManager] Converting stereo to mono: " << filepath << std::endl;
        std::vector<int16_t> monoSamples(totalFrameCount);
        for (uint64_t i = 0; i < totalFrameCount; ++i) {
            int32_t sum = 0;
            for (int ch = 0; ch < wav.channels; ++ch) {
                sum += samples[i * wav.channels + ch];
            }
            monoSamples[i] = static_cast<int16_t>(sum / wav.channels);
        }
        samples = std::move(monoSamples);
        wav.channels = 1;
    }

    ALenum format = AL_FORMAT_MONO16;

    ALuint buffer;
    alGenBuffers(1, &buffer);
    alBufferData(buffer, format, samples.data(),
                 static_cast<ALsizei>(samples.size() * sizeof(int16_t)),
                 wav.sampleRate);
    drwav_uninit(&wav);

    if (alGetError() != AL_NO_ERROR) {
        std::cerr << "[AudioManager] OpenAL error creating buffer for: " << filepath << std::endl;
        alDeleteBuffers(1, &buffer);
        return 0;
    }

    std::cout << "[AudioManager] Loaded mono buffer: " << filepath 
              << " (" << samples.size() << " samples, " 
              << wav.sampleRate << " Hz)" << std::endl;
    return buffer;
}

ALuint AudioManager::createSource(ALuint buffer,
                                  const glm::vec3& position,
                                  float volume,
                                  bool looping,
                                  float referenceDist,
                                  float maxDist,
                                  float rolloff) {
    ALuint source;
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        std::cerr << "[AudioManager] Failed to generate source!" << std::endl;
        return 0;
    }

    alSourcei(source, AL_BUFFER, buffer);
    alSource3f(source, AL_POSITION, position.x, position.y, position.z);
    alSourcef(source, AL_GAIN, volume);
    alSourcei(source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    alSourcef(source, AL_REFERENCE_DISTANCE, referenceDist);
    alSourcef(source, AL_MAX_DISTANCE, maxDist);
    alSourcef(source, AL_ROLLOFF_FACTOR, rolloff);

    activeSources.push_back(source);
    return source;
}

ALuint AudioManager::create2DSource(ALuint buffer,
                                    float volume,
                                    bool looping) {
    ALuint source;
    alGenSources(1, &source);
    if (alGetError() != AL_NO_ERROR) {
        std::cerr << "[AudioManager] Failed to generate 2D source!" << std::endl;
        return 0;
    }

    alSourcei(source, AL_BUFFER, buffer);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSourcef(source, AL_GAIN, volume);
    alSourcei(source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);

    activeSources.push_back(source);
    return source;
}

void AudioManager::playSource(ALuint source) {
    alSourcePlay(source);
}

void AudioManager::stopSource(ALuint source) {
    alSourceStop(source);
}

void AudioManager::pauseSource(ALuint source) {
    alSourcePause(source);
}

void AudioManager::setSourcePosition(ALuint source, const glm::vec3& position) {
    alSource3f(source, AL_POSITION, position.x, position.y, position.z);
}

void AudioManager::setSourceVolume(ALuint source, float volume) {
    alSourcef(source, AL_GAIN, volume);
}

void AudioManager::setSourceLooping(ALuint source, bool loop) {
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
}

bool AudioManager::isSourcePlaying(ALuint source) const {
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

void AudioManager::deleteSource(ALuint source) {
    alDeleteSources(1, &source);
    auto it = std::find(activeSources.begin(), activeSources.end(), source);
    if (it != activeSources.end()) {
        activeSources.erase(it);
    }
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume = volume;
    alListenerf(AL_GAIN, masterVolume);
}
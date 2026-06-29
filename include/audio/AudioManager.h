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

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool init();
    void setListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    ALuint loadSound(const std::string& filepath);

    ALuint createSource(ALuint buffer,
                        const glm::vec3& position = glm::vec3(0.0f),
                        float volume = 1.0f,
                        bool looping = false,
                        float referenceDist = 1.0f,
                        float maxDist = 50.0f,
                        float rolloff = 1.0f);

    ALuint create2DSource(ALuint buffer,
                          float volume = 1.0f,
                          bool looping = false);

    void playSource(ALuint source);
    void stopSource(ALuint source);
    void pauseSource(ALuint source);
    void setSourcePosition(ALuint source, const glm::vec3& position);
    void setSourceVolume(ALuint source, float volume);
    void setSourceLooping(ALuint source, bool loop);
    bool isSourcePlaying(ALuint source) const;
    void deleteSource(ALuint source);

    void setMasterVolume(float volume);

private:
    ALCdevice* device = nullptr;
    ALCcontext* context = nullptr;
    float masterVolume = 1.0f;

    std::unordered_map<std::string, ALuint> soundCache;
    std::vector<ALuint> activeSources;

    ALuint loadWAV(const std::string& filepath);
};

#endif
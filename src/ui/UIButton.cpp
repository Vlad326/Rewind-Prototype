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

#include "ui/UIButton.h"
#include "ui/UIText.h"
#include "ui/FontManager.h"
#include "audio/AudioManager.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

AudioManager* UIButton::globalAudioManager = nullptr;
std::string UIButton::globalHoverSoundPath;
float UIButton::globalHoverVolume = 1.0f;

UIButton::UIButton(FontManager& fm, int x, int y, int width, int height,
                   const std::string& text,
                   const std::string& fontPath, int fontSize)
    : UIWidget(x, y, width, height), fontManager(&fm)
{
    uiText = std::make_unique<UIText>(fm, fontPath, fontSize);
    uiText->setText(text);
    uiText->setColor(textColor);
    uiText->setScreenSize(screenW, screenH);
    buttonText = text;
    initDefaultSound();
}

UIButton::~UIButton() {
    if (hoverSource && audioManager) {
        audioManager->deleteSource(hoverSource);
    }
}

void UIButton::setFillColor(const glm::vec4& color)    { fillColor = color; }
void UIButton::setOutlineColor(const glm::vec4& color) { outlineColor = color; }
void UIButton::setOutlineThickness(int thickness)      { outlineThickness = thickness; }
void UIButton::setCornerLength(float fraction)         { cornerFraction = fraction; }
void UIButton::setTextColor(const glm::vec3& color)    { textColor = color; }
void UIButton::setText(const std::string& text)        { buttonText = text; uiText->setText(text); }
void UIButton::setFontSize(int size)                   { uiText->setFontSize(size); }

void UIButton::setAlpha(float a) {
    UIWidget::setAlpha(a);
    applyAlphaToText();
}

void UIButton::setScreenSize(int width, int height) {
    UIWidget::setScreenSize(width, height);
    if (uiText) uiText->setScreenSize(width, height);
}

void UIButton::applyAlphaToText() {
    if (uiText) uiText->setAlpha(alpha);
}

void UIButton::setGlobalHoverSound(AudioManager* am, const std::string& path, float volume) {
    globalAudioManager = am;
    globalHoverSoundPath = path;
    globalHoverVolume = volume;
}

void UIButton::clearGlobalHoverSound() {
    globalAudioManager = nullptr;
    globalHoverSoundPath.clear();
    globalHoverVolume = 1.0f;
}

void UIButton::initDefaultSound() {
    if (globalAudioManager && !globalHoverSoundPath.empty()) {
        setAudioManager(globalAudioManager);
        setHoverSound(globalHoverSoundPath, globalHoverVolume);
    }
}

void UIButton::setAudioManager(AudioManager* am) { audioManager = am; }

void UIButton::setHoverSound(const std::string& soundPath, float volume) {
    if (!audioManager) {
        std::cerr << "[UIButton] AudioManager not set!\n";
        return;
    }
    if (hoverSource) {
        audioManager->deleteSource(hoverSource);
        hoverSource = 0;
    }
    ALuint buf = audioManager->loadSound(soundPath);
    if (buf) {
        hoverSource = audioManager->create2DSource(buf, volume, false);
        hoverVolume = volume;
    }
}

void UIButton::clearHoverSound() {
    if (hoverSource && audioManager) {
        audioManager->deleteSource(hoverSource);
        hoverSource = 0;
    }
    hoverVolume = 1.0f;
}

void UIButton::update(int mouseX, int mouseY, bool mouseDown) {
    bool hover = isMouseInside(mouseX, mouseY);

    if (hover && !prevHovered) {
        if (hoverSource && audioManager) {
            audioManager->stopSource(hoverSource);
            audioManager->playSource(hoverSource);
        }
    }
    prevHovered = hover;
    hoveredState = hover;

    if (mouseDown && !prevMouseDown) {
        if (hover) {
            clickInitiated = true;
            pressedState = true;
            if (trigger == ButtonTrigger::OnPress) {
                onClick();
            }
        }
    } else if (!mouseDown && prevMouseDown) {
        if (clickInitiated && hover && trigger == ButtonTrigger::OnRelease) {
            onClick();
        }
        clickInitiated = false;
        pressedState = false;
    }

    if (!hover && mouseDown) {
        clickInitiated = false;
    }

    prevMouseDown = mouseDown;
}

void UIButton::onClick() {
    if (onClickCallback) {
        onClickCallback();
    }
}

void UIButton::render() {
    if (w <= 0 || h <= 0) return;

    GLboolean blend = glIsEnabled(GL_BLEND);
    GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cull  = glIsEnabled(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glm::vec4 bgColor = fillColor;
    if (pressedState) {
        bgColor = fillColor * 0.7f;
        bgColor.a = fillColor.a;
    } else if (hoveredState) {
        bgColor = fillColor * 1.3f;
        bgColor.a = fillColor.a;
    }
    drawRect(posX, posY, w, h, bgColor);
    drawOutline(posX, posY, w, h, outlineThickness, cornerFraction, outlineColor);

    if (!buttonText.empty()) {
        int textW = uiText->getTextWidth();
        int textH = uiText->getTextHeight();
        int textX = posX + (w - textW) / 2;
        int textY = posY + (h - textH) / 2;
        uiText->setPosition(textX, textY);
        uiText->setColor(textColor);
        uiText->render();
    }

    if (!blend) glDisable(GL_BLEND);
    if (depth)  glEnable(GL_DEPTH_TEST);
    if (cull)   glEnable(GL_CULL_FACE);
}
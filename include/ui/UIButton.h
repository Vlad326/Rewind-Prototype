#ifndef UIBUTTON_H
#define UIBUTTON_H

#include "ui/UIWidget.h"
#include <string>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <AL/al.h>

class UIText;
class AudioManager;
class FontManager;

enum class ButtonTrigger {
    OnPress,
    OnRelease
};

class UIButton : public UIWidget {
public:
    UIButton(FontManager& fontManager,
             int x, int y, int width, int height,
             const std::string& text,
             const std::string& fontPath = "../models/Arial.ttf",
             int fontSize = 24);
    virtual ~UIButton();

    void setFillColor(const glm::vec4& color);
    void setOutlineColor(const glm::vec4& color);
    void setOutlineThickness(int thickness);
    void setCornerLength(float fraction);
    void setTextColor(const glm::vec3& color);
    void setText(const std::string& text);
    virtual void setFontSize(int size);

    void setTrigger(ButtonTrigger t) { trigger = t; }
    void setOnClick(std::function<void()> callback) { onClickCallback = callback; }

    void setAudioManager(AudioManager* am);
    void setHoverSound(const std::string& soundPath, float volume = 1.0f);
    void clearHoverSound();

    static void setGlobalHoverSound(AudioManager* am,
                                    const std::string& path,
                                    float volume = 1.0f);
    static void clearGlobalHoverSound();

    bool isHovered() const { return hoveredState; }
    bool isPressed() const { return pressedState; }

    void setAlpha(float a) override;
    void setScreenSize(int width, int height) override;       void update(int mouseX, int mouseY, bool mouseDown) override;
    void render() override;

protected:
    FontManager* fontManager = nullptr;
    std::unique_ptr<UIText> uiText;
    std::string buttonText;

    glm::vec4 fillColor    = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 outlineColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 textColor    = glm::vec3(1.0f);
    int outlineThickness = 3;
    float cornerFraction = 0.0f;

    bool hoveredState = false;
    bool pressedState = false;
    bool prevHovered  = false;

    AudioManager* audioManager = nullptr;
    ALuint hoverSource = 0;
    float hoverVolume = 1.0f;

    ButtonTrigger trigger = ButtonTrigger::OnRelease;
    bool prevMouseDown = false;
    bool clickInitiated = false;
    std::function<void()> onClickCallback;

    virtual void onClick();
    void initDefaultSound();
    void applyAlphaToText();

private:
    static AudioManager* globalAudioManager;
    static std::string globalHoverSoundPath;
    static float globalHoverVolume;
};

#endif
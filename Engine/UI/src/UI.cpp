#include "UI.hpp"
#include <algorithm>

namespace MapleEngine {

MiniMapUI::MiniMapUI() {
    width_ = 200;
    height_ = 200;
    x_ = 10;
    y_ = 10;
}

void MiniMapUI::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_FRect rect = {static_cast<float>(x_), static_cast<float>(y_), 
                      static_cast<float>(width_), static_cast<float>(height_)};
    SDL_RenderFillRect(renderer, &rect);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &rect);
    
    if (camera_) {
        auto clipRect = camera_->getClipRect();
        float scaleX = static_cast<float>(width_) / (clipRect.w > 0 ? clipRect.w : 1);
        float scaleY = static_cast<float>(height_) / (clipRect.h > 0 ? clipRect.h : 1);
        float scale = std::min(scaleX, scaleY);
        
        int centerX = static_cast<int>(camera_->getCenter().x);
        int centerY = static_cast<int>(camera_->getCenter().y);
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect viewRect = {
            static_cast<float>(x_ + width_ / 2 - 5),
            static_cast<float>(y_ + height_ / 2 - 5),
            10.0f,
            10.0f
        };
        SDL_RenderRect(renderer, &viewRect);
    }
}

void MiniMapUI::update(float deltaTime) {
}

TooltipUI::TooltipUI() {
    padding_ = 8;
    fontSize_ = 14;
    width_ = 100;
    height_ = 30;
}

void TooltipUI::setText(const std::string& text) {
    text_ = text;
    width_ = text_.length() * fontSize_ / 2 + padding_ * 2;
    height_ = fontSize_ + padding_ * 2;
}

void TooltipUI::setPosition(int x, int y) {
    x_ = x;
    y_ = y;
}

void TooltipUI::render(SDL_Renderer* renderer) {
    if (text_.empty()) return;
    
    SDL_SetRenderDrawColor(renderer, 32, 32, 32, 230);
    SDL_FRect rect = {static_cast<float>(x_), static_cast<float>(y_),
                      static_cast<float>(width_), static_cast<float>(height_)};
    SDL_RenderFillRect(renderer, &rect);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderRect(renderer, &rect);
}

void TooltipUI::update(float deltaTime) {
}

TopBarUI::TopBarUI() {
    x_ = 0;
    y_ = 0;
    width_ = 1280;
    height_ = 40;
}

void TopBarUI::render(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect rect = {static_cast<float>(x_), static_cast<float>(y_),
                      static_cast<float>(width_), static_cast<float>(height_)};
    SDL_RenderFillRect(renderer, &rect);
}

UIManager& UIManager::getInstance() {
    static UIManager instance;
    return instance;
}

void UIManager::addElement(std::shared_ptr<UIElement> element) {
    elements_.push_back(element);
}

void UIManager::removeElement(UIElement* element) {
    elements_.erase(
        std::remove_if(elements_.begin(), elements_.end(),
            [element](const std::shared_ptr<UIElement>& e) { return e.get() == element; }),
        elements_.end()
    );
}

void UIManager::update(float deltaTime) {
    for (auto& element : elements_) {
        if (element->isVisible()) {
            element->update(deltaTime);
        }
    }
}

void UIManager::render(SDL_Renderer* renderer) {
    for (auto& element : elements_) {
        if (element->isVisible()) {
            element->render(renderer);
        }
    }
}

bool UIManager::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        int mouseX = static_cast<int>(event.motion.x);
        int mouseY = static_cast<int>(event.motion.y);
        
        for (auto& element : elements_) {
            if (element->isVisible() && element->containsPoint(mouseX, mouseY)) {
                return element->handleEvent(event);
            }
        }
    }
    return false;
}

void UIManager::setViewport(int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
}

} // namespace MapleEngine

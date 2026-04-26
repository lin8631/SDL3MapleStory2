#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace MapleEngine {

struct Frame {
    int textureId = -1;
    int x = 0, y = 0;
    int width = 0, height = 0;
    int originX = 0, originY = 0;
    int delay = 100;
    float a0 = 1.0f;
    int atlasRect = 0;
};

class AnimationData {
public:
    std::vector<Frame> frames;
    bool repeat = true;
    int frameCount() const { return static_cast<int>(frames.size()); }
};

class FrameAnimator {
public:
    FrameAnimator() = default;
    explicit FrameAnimator(std::shared_ptr<AnimationData> data) : data_(data) {}
    
    void setData(std::shared_ptr<AnimationData> data) { data_ = data; }
    std::shared_ptr<AnimationData> getData() const { return data_; }
    
    int getCurrentFrameIndex() const { return currentFrame_; }
    Frame* getCurrentFrame() {
        if (!data_ || data_->frames.empty()) return nullptr;
        return &data_->frames[currentFrame_];
    }
    
    void update(uint32_t deltaTime) {
        if (!data_ || data_->frames.empty()) return;
        
        time_ += deltaTime;
        const Frame& frame = data_->frames[currentFrame_];
        
        while (time_ >= frame.delay) {
            time_ -= frame.delay;
            currentFrame_++;
            
            if (currentFrame_ >= data_->frameCount()) {
                if (data_->repeat) {
                    currentFrame_ = 0;
                } else {
                    currentFrame_ = data_->frameCount() - 1;
                }
            }
        }
    }
    
    void reset() {
        currentFrame_ = 0;
        time_ = 0;
    }
    
private:
    std::shared_ptr<AnimationData> data_;
    int currentFrame_ = 0;
    uint32_t time_ = 0;
};

struct StateInfo {
    std::string name;
    std::shared_ptr<AnimationData> animation;
    int nextState = -1;
    int priority = 0;
};

class StateMachineAnimator {
public:
    StateMachineAnimator() = default;
    explicit StateMachineAnimator(const std::vector<StateInfo>& states) : states_(states) {
        if (!states.empty()) {
            currentState_ = 0;
        }
    }
    
    void addState(const StateInfo& state) {
        states_.push_back(state);
    }
    
    void setAnimation(int stateIndex) {
        if (stateIndex >= 0 && stateIndex < static_cast<int>(states_.size())) {
            currentState_ = stateIndex;
            time_ = 0;
        }
    }
    
    void setAnimation(const std::string& stateName) {
        for (int i = 0; i < static_cast<int>(states_.size()); ++i) {
            if (states_[i].name == stateName) {
                setAnimation(i);
                return;
            }
        }
    }
    
    int getCurrentState() const { return currentState_; }
    
    StateInfo* getCurrentStateInfo() {
        if (currentState_ >= 0 && currentState_ < static_cast<int>(states_.size())) {
            return &states_[currentState_];
        }
        return nullptr;
    }
    
    FrameAnimator* getAnimator() {
        auto* state = getCurrentStateInfo();
        if (state && state->animation) {
            return &frameAnimator_;
        }
        return nullptr;
    }
    
    void update(uint32_t deltaTime) {
        auto* state = getCurrentStateInfo();
        if (state && state->animation) {
            frameAnimator_.setData(state->animation);
            frameAnimator_.update(deltaTime);
        }
    }
    
    void reset() {
        currentState_ = 0;
        time_ = 0;
        frameAnimator_.reset();
    }
    
private:
    std::vector<StateInfo> states_;
    int currentState_ = -1;
    uint32_t time_ = 0;
    FrameAnimator frameAnimator_;
};

} // namespace MapleEngine

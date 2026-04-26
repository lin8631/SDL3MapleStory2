#include "Wz_Node.hpp"
#include "Wz_Image.hpp"
#include "Wz_File.hpp"
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <typeinfo>
#include <climits>

namespace WzLibCpp {

static bool TryParseToInt(const std::string& s, int& result) {
    if (s.empty()) return false;
    try {
        size_t pos;
        result = std::stoi(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

static bool TryParseToInt64(const std::string& s, int64_t& result) {
    if (s.empty()) return false;
    try {
        size_t pos;
        result = std::stoll(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

static bool TryParseToFloat(const std::string& s, float& result) {
    if (s.empty()) return false;
    try {
        size_t pos;
        result = std::stof(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

static bool TryParseToDouble(const std::string& s, double& result) {
    if (s.empty()) return false;
    try {
        size_t pos;
        result = std::stod(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

static bool TryParseToBool(const std::string& s, bool& result) {
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) {
        if (c >= 'A' && c <= 'Z') lower += c + ('a' - 'A');
        else if (c == 't' || c == 'f' || c == '1' || c == '0') lower += c;
    }
    if (lower == "true" || lower == "1") {
        result = true;
        return true;
    }
    if (lower == "false" || lower == "0") {
        result = false;
        return true;
    }
    return false;
}

Wz_Node::Wz_Node() {
    nodes = std::make_shared<WzNodeCollection>();
    nodes->setOwner(weak_from_this());
}

Wz_Node::Wz_Node(const std::string& nodeText)
    : Wz_Node() {
    text = nodeText;
}

Wz_Node::~Wz_Node() = default;

std::shared_ptr<void> Wz_Node::getValue() const {
    return value;
}

void Wz_Node::setValue(std::shared_ptr<void> val) {
    value = val;
}

const std::string& Wz_Node::getText() const {
    return text;
}

void Wz_Node::setText(const std::string& txt) {
    text = txt;
}

std::string Wz_Node::getFullPath() const {
    std::stack<std::string> path;
    Wz_NodePtr node = std::const_pointer_cast<Wz_Node>(shared_from_this());
    while (node != nullptr) {
        path.push(node->text);
        node = node->parentNode.lock();
    }
    std::string result;
    while (!path.empty()) {
        result += path.top();
        path.pop();
        if (!path.empty()) result += "\\";
    }
    return result;
}

std::string Wz_Node::getFullPathToFile() const {
    return getFullPath();
}

std::shared_ptr<WzNodeCollection> Wz_Node::getNodes() const {
    return nodes;
}

void Wz_Node::setNodes(std::shared_ptr<WzNodeCollection> coll) {
    nodes = coll;
}

Wz_NodeWeakPtr Wz_Node::getParentNode() const {
    return parentNode;
}

void Wz_Node::setParentNode(Wz_NodeWeakPtr parent) {
    parentNode = parent;
}

std::string Wz_Node::toString() const {
    std::ostringstream oss;
    oss << text << " - " << (nodes ? nodes->getCount() : 0);
    return oss.str();
}

std::shared_ptr<Wz_Value> Wz_Node::getWzValue() const {
    if (!value) return nullptr;
    
    auto wzVal = std::static_pointer_cast<Wz_Value>(value);
    if (wzVal && !wzVal->isNull()) {
        return wzVal;
    }
    
    return nullptr;
}

std::shared_ptr<Wz_Image> Wz_Node::getWzImage() const {
    if (!value) return nullptr;
    return std::static_pointer_cast<Wz_Image>(value);
}

std::shared_ptr<Wz_Value> Wz_Node::extractValue() const {
    return getWzValue();
}

void Wz_Node::setWzValue(std::shared_ptr<Wz_Value> val) {
    value = val;
}

int Wz_Node::getInt(int defaultVal) const {
    auto wzVal = getWzValue();
    if (wzVal && !wzVal->isNull()) {
        if (wzVal->isString()) {
            std::string strVal = wzVal->getString("");
            int intVal;
            if (TryParseToInt(strVal, intVal)) {
                return intVal;
            }
            return defaultVal;
        }
        return wzVal->getInt(defaultVal);
    }
    return defaultVal;
}

int32_t Wz_Node::getInt32(int32_t defaultVal) const {
    auto wzVal = getWzValue();
    return wzVal ? wzVal->getInt32(defaultVal) : defaultVal;
}

int64_t Wz_Node::getInt64(int64_t defaultVal) const {
    auto wzVal = getWzValue();
    return wzVal ? wzVal->getInt64(defaultVal) : defaultVal;
}

float Wz_Node::getFloat(float defaultVal) const {
    auto wzVal = getWzValue();
    return wzVal ? wzVal->getFloat(defaultVal) : defaultVal;
}

double Wz_Node::getDouble(double defaultVal) const {
    auto wzVal = getWzValue();
    return wzVal ? wzVal->getDouble(defaultVal) : defaultVal;
}

std::string Wz_Node::getString(const std::string& defaultVal) const {
    auto wzVal = getWzValue();
    if (wzVal && !wzVal->isNull()) {
        return wzVal->getString(defaultVal);
    }
    return defaultVal;
}

bool Wz_Node::getBool(bool defaultVal) const {
    auto wzVal = getWzValue();
    return wzVal ? wzVal->getBool(defaultVal) : defaultVal;
}

Wz_NodePtr Wz_Node::findNodeByPath(const std::string& fullPath) const {
    return findNodeByPath(fullPath, false);
}

Wz_NodePtr Wz_Node::findNodeByPath(const std::string& fullPath, bool extractImage) const {
    std::vector<std::string> components;
    std::string item;
    
    for (size_t i = 0; i < fullPath.size(); ++i) {
        char c = fullPath[i];
        if (c == '\\' || c == '/') {
            if (!item.empty()) {
                components.push_back(item);
                item.clear();
            }
        } else {
            item += c;
        }
    }
    if (!item.empty()) {
        components.push_back(item);
    }
    
    return findNodeByPath(extractImage, false, components);
}

Wz_NodePtr Wz_Node::findNodeByPath(bool extractImage, bool ignoreCase, 
                                     const std::vector<std::string>& pathComponents) const {
    Wz_NodePtr node = std::const_pointer_cast<Wz_Node>(shared_from_this());

    if (extractImage) {
        auto img = getValue<Wz_Image>();
        if (img) {
            if (img->getOffset() == 0) {
                auto wzFile = img->getWzFile();
                auto wzFilePtr = std::dynamic_pointer_cast<Wz_File>(wzFile);
                if (wzFilePtr) {
                    int64_t calculatedOffset = wzFilePtr->calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset());
                    img->setOffset(calculatedOffset);
                }
            }
            if (img->tryExtract()) {
                auto imgNode = img->getNode();
                if (imgNode) node = imgNode;
            }
        }
    }

    for (const auto& txt : pathComponents) {
        if (!node || !node->nodes) return nullptr;
        
        Wz_NodePtr foundNode = nullptr;
        
        if (ignoreCase) {
            for (const auto& subNode : *node->nodes) {
                if (subNode && subNode->text == txt) {
                    foundNode = subNode;
                    break;
                }
            }
        } else {
            auto it = node->nodes->find(txt);
            if (it != node->nodes->end()) foundNode = *it;
        }
        
        if (!foundNode) return nullptr;
        node = foundNode;
        
        if (extractImage) {
            auto img = node->getValue<Wz_Image>();
            if (img) {
                if (img->getOffset() == 0) {
                    auto wzFile = img->getWzFile();
                    auto wzFilePtr = std::dynamic_pointer_cast<Wz_File>(wzFile);
                    if (wzFilePtr) {
                        int64_t calculatedOffset = wzFilePtr->calcOffset(img->getHashedOffsetPosition(), img->getHashedOffset());
                        img->setOffset(calculatedOffset);
                    }
                }
                if (img->tryExtract()) {
                    auto imgNode = img->getNode();
                    if (imgNode) node = imgNode;
                }
            }
        }
    }
    return node;
}

Wz_NodePtr Wz_Node::clone() const {
    auto newNode = std::make_shared<Wz_Node>(text);
    newNode->value = value;
    if (nodes) {
        for (const auto& child : *nodes) {
            auto clonedChild = child->clone();
            newNode->nodes->add(clonedChild);
        }
    }
    return newNode;
}

bool Wz_Node::operator<(const Wz_Node& other) const {
    return text < other.text;
}

bool Wz_Node::operator==(const Wz_Node& other) const {
    return text == other.text;
}

WzNodeCollection::WzNodeCollection() {}

void WzNodeCollection::setOwner(Wz_NodeWeakPtr owner) {
    ownerNode = owner;
}

Wz_NodePtr WzNodeCollection::operator[](size_t index) {
    if (index < items.size()) return items[index];
    return nullptr;
}

Wz_NodePtr WzNodeCollection::operator[](const std::string& key) {
    auto it = dictionary.find(key);
    if (it != dictionary.end()) return it->second;
    return nullptr;
}

Wz_NodePtr WzNodeCollection::add(const std::string& nodeText) {
    auto newNode = std::make_shared<Wz_Node>(nodeText);
    add(newNode);
    return newNode;
}

void WzNodeCollection::add(Wz_NodePtr item) {
    if (!item) return;
    items.push_back(item);
    dictionary[item->getText()] = item;
    item->setParentNode(ownerNode);
}

void WzNodeCollection::remove(Wz_NodePtr item) {
    if (!item) return;
    items.erase(std::remove(items.begin(), items.end(), item), items.end());
    dictionary.erase(item->getText());
}

size_t WzNodeCollection::getCount() const {
    return items.size();
}

void WzNodeCollection::sort() {
    std::sort(items.begin(), items.end(),
             [](const Wz_NodePtr& a, const Wz_NodePtr& b) {
                 return *a < *b;
             });
}

void WzNodeCollection::trim() {
    items.shrink_to_fit();
}

void WzNodeCollection::clear() {
    items.clear();
    dictionary.clear();
}

WzNodeCollection::iterator WzNodeCollection::begin() { return items.begin(); }
WzNodeCollection::iterator WzNodeCollection::end() { return items.end(); }
WzNodeCollection::const_iterator WzNodeCollection::begin() const { return items.begin(); }
WzNodeCollection::const_iterator WzNodeCollection::end() const { return items.end(); }

typename std::vector<Wz_NodePtr>::const_iterator WzNodeCollection::find(const std::string& key) const {
    auto it = dictionary.find(key);
    if (it != dictionary.end()) {
        for (auto vit = items.begin(); vit != items.end(); ++vit) {
            if (*vit && (*vit)->getText() == key) return vit;
        }
    }
    return items.end();
}

std::string Wz_Node::extractStringFromNode(std::shared_ptr<Wz_Node> node, const std::string& defaultValue) {
    if (!node) return defaultValue;
    
    // 方式1: 直接获取字符串
    std::string result = node->getString("");
    if (!result.empty()) return result;
    
    // 方式2: 如果是Wz_Image类型，提取后获取
    auto wzImg = node->getValue<Wz_Image>();
    if (wzImg) {
        try {
            if (wzImg->getOffset() == 0) {
                auto wzFile = std::dynamic_pointer_cast<Wz_File>(wzImg->getWzFile());
                if (wzFile) {
                    wzImg->setOffset(wzFile->calcOffset(wzImg->getHashedOffsetPosition(), wzImg->getHashedOffset()));
                }
            }
            if (wzImg->tryExtract()) {
                auto extracted = wzImg->getNode();
                if (extracted && extracted->getNodes() && extracted->getNodes()->getCount() > 0) {
                    auto first = (*extracted->getNodes())[0];
                    if (first) {
                        result = first->getText();
                        if (!result.empty()) return result;
                    }
                }
            }
        } catch (...) {}
    }
    
    return defaultValue;
}

int Wz_Node::extractIntFromNode(std::shared_ptr<Wz_Node> node, int defaultValue) {
    if (!node) return defaultValue;
    
    // 方式1: 直接获取整数值
    int result = node->getInt(INT_MIN);
    if (result != INT_MIN) return result;
    
    // 方式2: 如果是Wz_Image类型
    auto wzImg = node->getValue<Wz_Image>();
    if (wzImg) {
        auto wzFile = std::dynamic_pointer_cast<Wz_File>(wzImg->getWzFile());
        if (!wzFile) return defaultValue;
        
        int64_t currentOffset = wzImg->getOffset();
        
        // 重新计算偏移
        if (currentOffset == 0 || currentOffset > 100000000000LL) {
            auto newOffset = wzFile->calcOffset(wzImg->getHashedOffsetPosition(), wzImg->getHashedOffset());
            wzImg->setOffset(newOffset);
        }
        
        // 提取并获取值
        if (wzImg->getOffset() < 1000000000) {
            try {
                if (wzImg->tryExtract()) {
                    auto extracted = wzImg->getNode();
                    if (extracted && extracted->getNodes() && extracted->getNodes()->getCount() > 0) {
                        auto first = (*extracted->getNodes())[0];
                        if (first) {
                            result = first->getInt(INT_MIN);
                            if (result != INT_MIN) return result;
                        }
                    }
                }
            } catch (...) {}
        }
    }
    
    return defaultValue;
}

} // namespace WzLibCpp

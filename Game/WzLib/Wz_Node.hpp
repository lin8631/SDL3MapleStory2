#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <stack>
#include <algorithm>
#include <unordered_map>
#include "Wz_Value.hpp"

namespace WzLibCpp {

// Forward declarations
class Wz_File;
class Wz_Image;
class Wz_ImageNode;
class WzNodeCollection;

class Wz_Node;
using Wz_NodePtr = std::shared_ptr<Wz_Node>;
using Wz_NodeWeakPtr = std::weak_ptr<Wz_Node>;

class Wz_Node : public std::enable_shared_from_this<Wz_Node> {
public:
    Wz_Node();
    explicit Wz_Node(const std::string& nodeText);
    virtual ~Wz_Node();

    std::shared_ptr<void> getValue() const;
    void setValue(std::shared_ptr<void> val);

    const std::string& getText() const;
    void setText(const std::string& txt);

    std::string getFullPath() const;
    std::string getFullPathToFile() const;

    std::shared_ptr<WzNodeCollection> getNodes() const;
    void setNodes(std::shared_ptr<WzNodeCollection> coll);

    Wz_NodeWeakPtr getParentNode() const;
    void setParentNode(Wz_NodeWeakPtr parent);

    virtual std::string toString() const;

    Wz_NodePtr findNodeByPath(const std::string& fullPath) const;
    Wz_NodePtr findNodeByPath(const std::string& fullPath, bool extractImage) const;
    Wz_NodePtr findNodeByPath(bool extractImage, bool ignoreCase, 
                              const std::vector<std::string>& pathComponents) const;

    template<typename T>
    std::shared_ptr<T> getValue() const {
        if (!value) return nullptr;
        return std::static_pointer_cast<T>(value);
    }

    // Wz_Value 便捷方法
    std::shared_ptr<Wz_Value> getWzValue() const;
    std::shared_ptr<Wz_Image> getWzImage() const;
    void setWzValue(std::shared_ptr<Wz_Value> val);
    
    // 尝试从 Wz_Image 中提取值（用于 v072 格式）
    std::shared_ptr<Wz_Value> extractValue() const;
    
    // 属性读取方法
    int getInt(int defaultVal = 0) const;
    int32_t getInt32(int32_t defaultVal = 0) const;
    int64_t getInt64(int64_t defaultVal = 0) const;
    float getFloat(float defaultVal = 0.0f) const;
    double getDouble(double defaultVal = 0.0) const;
    std::string getString(const std::string& defaultVal = "") const;
    bool getBool(bool defaultVal = false) const;
    
    // 静态辅助函数：安全提取节点值
    static std::string extractStringFromNode(std::shared_ptr<Wz_Node> node, const std::string& defaultValue = "");
    static int extractIntFromNode(std::shared_ptr<Wz_Node> node, int defaultValue = 0);

    virtual Wz_NodePtr clone() const;

    bool operator<(const Wz_Node& other) const;
    bool operator==(const Wz_Node& other) const;

protected:
    std::shared_ptr<void> value;
    std::string text;
    std::shared_ptr<WzNodeCollection> nodes;
    Wz_NodeWeakPtr parentNode;
};

class WzNodeCollection {
public:
    WzNodeCollection();

    void setOwner(Wz_NodeWeakPtr owner);

    Wz_NodePtr operator[](size_t index);
    Wz_NodePtr operator[](const std::string& key);

    Wz_NodePtr add(const std::string& nodeText);
    void add(Wz_NodePtr item);
    void remove(Wz_NodePtr item);

    size_t getCount() const;
    void sort();
    void trim();
    void clear();

    using iterator = typename std::vector<Wz_NodePtr>::iterator;
    using const_iterator = typename std::vector<Wz_NodePtr>::const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    typename std::vector<Wz_NodePtr>::const_iterator find(const std::string& key) const;

private:
    Wz_NodeWeakPtr ownerNode;
    std::vector<Wz_NodePtr> items;
    std::unordered_map<std::string, Wz_NodePtr> dictionary;
};

} // namespace WzLibCpp
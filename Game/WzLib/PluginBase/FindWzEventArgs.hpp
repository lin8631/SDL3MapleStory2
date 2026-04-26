#pragma once
#include <string>
#include <memory>
#include <functional>
#include "Wz_Type.hpp"

namespace WzLibCpp {
    class Wz_File;
    class Wz_Node;
}

namespace WzLibCpp::PluginBase {

/**
 * 查找 Wz 节点的事件参数
 * 对应 C# 代码中的 FindWzEventArgs 类
 */
class FindWzEventArgs {
public:
    FindWzEventArgs() = default;
    
    FindWzEventArgs(Wz_Type type)
        : wzType(type) {}

    /**
     * 获取或设置要查找 wz 节点的完全名称，用于输入参数
     * 支持 '/' 或 '\' 作为分隔符，如 "Mob/8144006.img/die1/6"
     */
    const std::string& getFullPath() const { return fullPath; }
    void setFullPath(const std::string& path) { fullPath = path; }

    /**
     * 获取或设置要查找 wz 节点的 Wz_Type，用于输入参数
     */
    Wz_Type getWzType() const { return wzType; }
    void setWzType(Wz_Type type) { wzType = type; }

    /**
     * 获取或设置要查找 wz 节点的所属 Wz_File，用于输入和输出参数
     */
    std::shared_ptr<Wz_File> getWzFile() const { return wzFile; }
    void setWzFile(std::shared_ptr<Wz_File> file) { wzFile = file; }

    /**
     * 获取或设置要查找 wz 节点的 Wz_Node，用于输出参数
     */
    std::shared_ptr<Wz_Node> getWzNode() const { return wzNode; }
    void setWzNode(std::shared_ptr<Wz_Node> node) { wzNode = node; }

private:
    std::string fullPath;
    Wz_Type wzType = Wz_Type::Unknown;
    std::shared_ptr<Wz_File> wzFile;
    std::shared_ptr<Wz_Node> wzNode;
};

/**
 * WzFileFinding 事件处理函数类型
 */
using FindWzEventHandler = std::function<void(std::shared_ptr<void> sender, FindWzEventArgs& e)>;

} // namespace WzLibCpp::PluginBase

#include "WzBrowserWindow.hpp"
#include "imgui.h"

#include <filesystem>

#include "Wz_Node.hpp"
#include "Wz_Structure.hpp"
#include "Wz_File.hpp"
#include "Wz_Header.hpp"
#include "Wz_Image.hpp"

using namespace WzLibCpp;

namespace {

constexpr int MAX_DEPTH = 6;
constexpr int MAX_ITEMS_DEPTH_0_1 = 100;
constexpr int MAX_ITEMS_DEPTH_2_3 = 50;
constexpr int MAX_ITEMS_DEPTH_4_6 = 20;
constexpr size_t MAX_FILES = 20;

int getMaxItemsForDepth(int depth) {
    if (depth < 2) return MAX_ITEMS_DEPTH_0_1;
    if (depth < 4) return MAX_ITEMS_DEPTH_2_3;
    return MAX_ITEMS_DEPTH_4_6;
}

}

WzBrowserWindow::WzBrowserWindow() : visible(true) {}

WzBrowserWindow::~WzBrowserWindow() = default;

void WzBrowserWindow::render(
    std::shared_ptr<Wz_Structure> structure,
    std::shared_ptr<Wz_Node> mapNode,
    const std::vector<std::shared_ptr<Wz_File>>& wzFiles,
    float zoom,
    int cameraX,
    int cameraY,
    bool showFoothold,
    bool showPortal,
    bool showLife,
    bool showBack,
    bool showTile,
    bool showObj,
    size_t backCount,
    size_t tileCount,
    size_t objCount
) {
    if (!visible) return;

    if (!ImGui::Begin("WZ Resource Browser", &visible)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("trees", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoKeepColumnsVisible)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 250.0f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 250.0f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 250.0f);

        // 第一列：WZ文件列表
        ImGui::TableNextColumn();
        if (ImGui::BeginChild("##col1")) {
            // 使用普通 lambda 代替 std::function 提高性能
            auto renderWzNode = [&](auto&& self, std::shared_ptr<Wz_Node> node, int depth, const std::string& path) -> void {
                if (!node || depth > MAX_DEPTH) return;
                auto nodes = node->getNodes();
                if (nodes && nodes->getCount() > 0) {
                    int maxItems = getMaxItemsForDepth(depth);
                    size_t totalCount = nodes->getCount();
                    bool truncated = totalCount > (size_t)maxItems;
                    for (size_t i = 0; i < totalCount; i++) {
                        if (i >= (size_t)maxItems) break;
                        auto child = (*nodes)[i];
                        if (!child) continue;
                        
                        std::string text = child->getText();
                        std::string childPath = path + "/" + text;
                        auto childNodes = child->getNodes();
                        auto wzImg = child->getWzImage();
                        
                        bool hasChildNodes = childNodes && childNodes->getCount() > 0;
                        bool isWzImage = wzImg != nullptr;
                        
                        ImGui::PushID((void*)child.get());
                        
                        // 只有用户主动点击 TreeNode 的标签部分才更新选中状态
                        if (hasChildNodes) {
                            if (ImGui::TreeNode((void*)child.get(), "%s", text.c_str())) {
                                // 使用 IsItemClicked 判断用户是否点击了节点标签（而非展开箭头）
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    selectedNode = child;
                                    selectedNodeTitle = childPath;
                                }
                                self(self, child, depth + 1, childPath);
                                ImGui::TreePop();
                            }
                            // 双击展开节点
                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                            }
                        } else if (isWzImage) {
                            bool isSelected = (selectedNode == child);
                            if (ImGui::Selectable(text.c_str(), isSelected, ImGuiSelectableFlags_None)) {
                                selectedNode = child;
                                selectedNodeTitle = childPath;
                                if (wzImg) wzImg->tryExtract();
                            }
                        } else {
                            ImGui::Text("%s", text.c_str());
                        }
                        
                        ImGui::PopID();
                    }
                    if (truncated) {
                        ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "... +%zu more", totalCount - maxItems);
                    }
                }
            };
            
            if (!wzFiles.empty()) {
                auto baseNode = structure->getWzNode();
                if (baseNode && baseNode->getNodes() && baseNode->getNodes()->getCount() > 0) {
                    std::string rootName = "base.wz";
                    ImGui::PushID((void*)baseNode.get());
                    if (ImGui::TreeNode((void*)baseNode.get(), "%s", rootName.c_str())) {
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                            selectedNode = baseNode;
                            selectedNodeTitle = rootName;
                        }
                        renderWzNode(renderWzNode, baseNode, 0, rootName);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                } else {
                    size_t totalFiles = wzFiles.size();
                    size_t shown = 0;
                    for (size_t i = 0; i < totalFiles && shown < (size_t)MAX_FILES; i++) {
                        if (!wzFiles[i] || !wzFiles[i]->getHeader()) continue;
                        if (wzFiles[i]->getIsSubDir()) continue;
                        
                        std::string fullPath = wzFiles[i]->getHeader()->getFileName();
                        std::string fileName = std::filesystem::path(fullPath).filename().string();
                        
                        ImGui::PushID((void*)wzFiles[i].get());
                        auto wzNode = wzFiles[i]->getNode();
                        bool hasChildren = wzNode && wzNode->getNodes() && wzNode->getNodes()->getCount() > 0;
                        if (hasChildren) {
                            if (ImGui::TreeNode((void*)wzFiles[i].get(), "%s", fileName.c_str())) {
                                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                    selectedNode = wzNode;
                                    selectedNodeTitle = fileName;
                                }
                                renderWzNode(renderWzNode, wzNode, 0, fileName);
                                ImGui::TreePop();
                            }
                        } else {
                            ImGui::Text("%s", fileName.c_str());
                        }
                        ImGui::PopID();
                        shown++;
                    }
                    if (totalFiles > shown) {
                        ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "... +%zu more files", totalFiles - shown);
                    }
                }
            } else {
                ImGui::Text("No WZ files loaded");
            }
        }
        ImGui::EndChild();

        // 第二列：选中节点的内容
        ImGui::TableNextColumn();
        if (ImGui::BeginChild("##col2")) {
            auto displayNode = selectedNode ? selectedNode : mapNode;
            if (displayNode && displayNode->getNodes()) {
                auto renderWzNode2 = [&](auto&& self, std::shared_ptr<Wz_Node> node, int depth) -> void {
                    if (!node || depth > MAX_DEPTH) return;
                    auto nodes = node->getNodes();
                    if (nodes && nodes->getCount() > 0) {
                        int maxItems = getMaxItemsForDepth(depth);
                        size_t totalCount = nodes->getCount();
                        bool truncated = totalCount > (size_t)maxItems;
                        for (size_t i = 0; i < totalCount; i++) {
                            if (i >= (size_t)maxItems) break;
                            auto child = (*nodes)[i];
                            if (!child) continue;
                            
                            std::string text = child->getText();
                            auto childNodes = child->getNodes();
                            auto wzImg = child->getWzImage();
                            
                            bool hasChildNodes = childNodes && childNodes->getCount() > 0;
                            bool isWzImage = wzImg != nullptr;
                            
                            ImGui::PushID((void*)child.get());
                            // 只显示有子节点的非img目录（只读，不展开）
                            if (hasChildNodes && !isWzImage) {
                                if (ImGui::TreeNode((void*)child.get(), "%s", text.c_str())) {
                                    self(self, child, depth + 1);
                                    ImGui::TreePop();
                                }
                            } else {
                                // img节点和叶子节点都只读显示
                                ImGui::Text("%s", text.c_str());
                            }
                            ImGui::PopID();
                        }
                        if (truncated) {
                            ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "... +%zu more", totalCount - maxItems);
                        }
                    }
                };
                renderWzNode2(renderWzNode2, displayNode, 0);
            }
        }
        ImGui::EndChild();

        // 第三列：渲染信息（添加独立滚动区域）
        ImGui::TableNextColumn();
        if (ImGui::BeginChild("##col3")) {
            ImGui::Text("Zoom: %.2f", zoom);
            ImGui::Text("Camera: (%d, %d)", cameraX, cameraY);
            ImGui::Separator();
            ImGui::Text("Render Options:");
            ImGui::Text("  Foothold: %s", showFoothold ? "Yes" : "No");
            ImGui::Text("  Portal: %s", showPortal ? "Yes" : "No");
            ImGui::Text("  Life: %s", showLife ? "Yes" : "No");
            ImGui::Text("  Back: %s", showBack ? "Yes" : "No");
            ImGui::Text("  Tile: %s", showTile ? "Yes" : "No");
            ImGui::Text("  Obj: %s", showObj ? "Yes" : "No");
            ImGui::Separator();
            ImGui::Text("Counts:");
            ImGui::Text("  Back: %zu", backCount);
            ImGui::Text("  Tile: %zu", tileCount);
            ImGui::Text("  Obj: %zu", objCount);
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }
    ImGui::End();
}
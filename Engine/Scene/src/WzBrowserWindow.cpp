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

void WzBrowserWindow::render(const WzBrowserState& state) {
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
                        
                        // 综合判断：优先使用 getWzImage()，备用地使用名称判断
                        bool wzImgExists = wzImg != nullptr;
                        bool textEndsWithImg = (text.length() > 4 && text.substr(text.length() - 4) == ".img");
                        
                        // img节点：wzImg存在 OR 名称以.img结尾
                        bool isImgNode = wzImgExists || textEndsWithImg;
                        
                        // 只有非img且有子节点才能展开
                        bool hasChildren = childNodes && childNodes->getCount() > 0;
                        bool canExpand = !isImgNode && hasChildren;
                        
                        ImGui::PushID((void*)child.get());
                        
                        // img节点始终显示为Selectable，不展开
                        if (isImgNode) {
                            // 使用外壳节点用于高亮判断
                            bool isSelected = (selectedImgNode == child);
                            if (ImGui::Selectable(text.c_str(), isSelected, ImGuiSelectableFlags_None)) {
                                selectedNodeTitle = childPath;
                                selectedNode = nullptr; // 清空目录高亮
                                selectedImgNode = child; // 记录外壳节点用于高亮
                                if (wzImg && wzImg->tryExtract()) {
                                    displayNode = wzImg->getNode(); // 第二列显示解压后的内容
                                } else {
                                    displayNode = nullptr; // 解压失败时清空
                                }
                            }
                        } else if (canExpand) {
                            // 只有非img目录且有子节点时才能展开为TreeNode
                            bool nodeOpen = ImGui::TreeNode((void*)child.get(), "%s", text.c_str());
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                selectedNode = child;
                                selectedNodeTitle = childPath;
                                displayNode = nullptr; // 清空第二列显示
                                selectedImgNode = nullptr; // 清空img高亮
                            }
                            if (nodeOpen) {
                                self(self, child, depth + 1, childPath);
                                ImGui::TreePop();
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
            
            const auto& wzFiles = state.wzFiles;
            if (!wzFiles.empty()) {
                auto baseNode = state.structure->getWzNode();
                if (baseNode && baseNode->getNodes() && baseNode->getNodes()->getCount() > 0) {
                    std::string rootName = "base.wz";
                    ImGui::PushID((void*)baseNode.get());
                    bool nodeOpen = ImGui::TreeNode((void*)baseNode.get(), "%s", rootName.c_str());
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                        selectedNode = baseNode;
                        selectedNodeTitle = rootName;
                        displayNode = nullptr;
                        selectedImgNode = nullptr;
                    }
                    if (nodeOpen) {
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
                            bool nodeOpen = ImGui::TreeNode((void*)wzFiles[i].get(), "%s", fileName.c_str());
                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                selectedNode = wzNode;
                                selectedNodeTitle = fileName;
                                displayNode = nullptr;
                                selectedImgNode = nullptr;
                            }
                            if (nodeOpen) {
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
            auto col2Node = displayNode ? displayNode : state.mapNode;
            if (col2Node && col2Node->getNodes()) {
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
                            // 第二列逻辑：有子节点且非img -> 可以展开的TreeNode
                            // img节点或其他情况 -> 只读文本
                            if (hasChildNodes && !isWzImage) {
                                if (ImGui::TreeNode((void*)child.get(), "%s", text.c_str())) {
                                    self(self, child, depth + 1);
                                    ImGui::TreePop();
                                }
                            } else if (isWzImage) {
                                // img节点：只读显示文本（不解压不点击）
                                ImGui::Text("%s", text.c_str());
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
                renderWzNode2(renderWzNode2, col2Node, 0);
            }
        }
        ImGui::EndChild();

        // 第三列：渲染信息（添加独立滚动区域）
        ImGui::TableNextColumn();
        if (ImGui::BeginChild("##col3")) {
            ImGui::Text("Selected:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "%s", 
                selectedNodeTitle.empty() ? "(none)" : selectedNodeTitle.c_str());
            ImGui::Separator();
            ImGui::Text("Zoom: %.2f", state.zoom);
            ImGui::Text("Camera: (%d, %d)", state.cameraX, state.cameraY);
            ImGui::Separator();
            ImGui::Text("Render Options:");
            ImGui::Text("  Foothold: %s", state.showFoothold ? "Yes" : "No");
            ImGui::Text("  Portal: %s", state.showPortal ? "Yes" : "No");
            ImGui::Text("  Life: %s", state.showLife ? "Yes" : "No");
            ImGui::Text("  Back: %s", state.showBack ? "Yes" : "No");
            ImGui::Text("  Tile: %s", state.showTile ? "Yes" : "No");
            ImGui::Text("  Obj: %s", state.showObj ? "Yes" : "No");
            ImGui::Separator();
            ImGui::Text("Counts:");
            ImGui::Text("  Back: %zu", state.backCount);
            ImGui::Text("  Tile: %zu", state.tileCount);
            ImGui::Text("  Obj: %zu", state.objCount);
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }
    ImGui::End();
}
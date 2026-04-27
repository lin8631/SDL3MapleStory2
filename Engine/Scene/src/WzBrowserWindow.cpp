#include "WzBrowserWindow.hpp"
#include "imgui.h"

#include <filesystem>

#include "Wz_Node.hpp"
#include "Wz_Structure.hpp"
#include "Wz_File.hpp"
#include "Wz_Header.hpp"
#include "Wz_Image.hpp"

using namespace WzLibCpp;

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
            std::function<void(std::shared_ptr<Wz_Node>, int, std::string)> renderWzNode = 
            [&](std::shared_ptr<Wz_Node> node, int depth, std::string path) {
                if (!node || depth > 6) return;
                auto nodes = node->getNodes();
                if (nodes && nodes->getCount() > 0) {
                    int maxItems = (depth < 2) ? 100 : (depth < 4) ? 50 : 20;
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
                        
                        if (!selectedNodeTitle.empty() && selectedNodeTitle == childPath) {
                            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                        }
                        
                        if (hasChildNodes) {
                            if (ImGui::TreeNode((void*)child.get(), "%s", text.c_str())) {
                                if (selectedNode != child) {
                                    selectedNode = child;
                                    selectedNodeTitle = childPath;
                                }
                                renderWzNode(child, depth + 1, childPath);
                                ImGui::TreePop();
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
                        renderWzNode(baseNode, 0, rootName);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                } else {
                    size_t totalFiles = wzFiles.size();
                    size_t maxFiles = 20;
                    int shown = 0;
                    for (size_t i = 0; i < totalFiles && shown < (int)maxFiles; i++) {
                        if (!wzFiles[i] || !wzFiles[i]->getHeader()) continue;
                        if (wzFiles[i]->getIsSubDir()) continue;
                        
                        std::string fullPath = wzFiles[i]->getHeader()->getFileName();
                        std::string fileName = std::filesystem::path(fullPath).filename().string();
                        
                        ImGui::PushID((void*)wzFiles[i].get());
                        auto wzNode = wzFiles[i]->getNode();
                        bool hasChildren = wzNode && wzNode->getNodes() && wzNode->getNodes()->getCount() > 0;
                        if (hasChildren) {
                            if (ImGui::TreeNode((void*)wzFiles[i].get(), "%s", fileName.c_str())) {
                                renderWzNode(wzNode, 0, fileName);
                                ImGui::TreePop();
                            }
                        } else {
                            ImGui::Text("%s", fileName.c_str());
                        }
                        ImGui::PopID();
                        shown++;
                    }
                    if (totalFiles > (size_t)shown) {
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
                std::function<void(std::shared_ptr<Wz_Node>, int)> renderWzNode2 = 
                    [&](std::shared_ptr<Wz_Node> node, int depth) {
                        if (!node || depth > 6) return;
                        auto nodes = node->getNodes();
                        if (nodes && nodes->getCount() > 0) {
                            int maxItems = (depth < 2) ? 100 : (depth < 4) ? 50 : 20;
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
                                if (hasChildNodes && !isWzImage) {
                                    if (ImGui::TreeNode((void*)child.get(), "%s", text.c_str())) {
                                        renderWzNode2(child, depth + 1);
                                        ImGui::TreePop();
                                    }
                                } else if (isWzImage) {
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
                renderWzNode2(displayNode, 0);
            }
        }
        ImGui::EndChild();

        // 第三列：渲染信息
        ImGui::TableNextColumn();
        ImGui::Text("Zoom: %.2f", zoom);
        ImGui::Text("Camera: (%d, %d)", cameraX, cameraY);
        ImGui::Text("Show Foothold: %s", showFoothold ? "Yes" : "No");
        ImGui::Text("Show Portal: %s", showPortal ? "Yes" : "No");
        ImGui::Text("Show Life: %s", showLife ? "Yes" : "No");
        ImGui::Text("Show Back: %s", showBack ? "Yes" : "No");
        ImGui::Text("Show Tile: %s", showTile ? "Yes" : "No");
        ImGui::Text("Show Obj: %s", showObj ? "Yes" : "No");
        ImGui::Text("Back: %zu, Tile: %zu, Obj: %zu", backCount, tileCount, objCount);

        ImGui::EndTable();
    }
    ImGui::End();
}
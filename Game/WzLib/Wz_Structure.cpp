#include "Wz_Structure.hpp"
#include "Wz_Crypto.hpp"
#include "Wz_File.hpp"
#include "Wz_Node.hpp"
#include "Wz_Header.hpp"
#include <filesystem>
#include <stdexcept>

namespace WzLibCpp {

Wz_Structure::Wz_Structure() {
    wz_files = std::vector<std::shared_ptr<Wz_File>>();
    ms_files = std::vector<std::shared_ptr<IMapleStoryFile>>();
    encryption = std::make_shared<Wz_Crypto>();
    WzNode = nullptr;
    img_number = 0;
    has_basewz = false;
    sorted = false;
    autoDetectExtFiles = true;
    imgCheckDisabled = false;
    wzVersionVerifyMode = WzVersionVerifyMode::Detect;
}

void Wz_Structure::clear() {
    for (auto& f : wz_files) {
        if (f) f->close();
    }
    wz_files.clear();
    for (auto& f : ms_files) {
        if (f) f->close();
    }
    ms_files.clear();
    if (encryption) encryption->reset();
    img_number = 0;
    has_basewz = false;
    WzNode = nullptr;
    sorted = false;
}

void Wz_Structure::calculate_img_count() {
    img_number = 0;
    for (auto& f : wz_files) {
        if (f) {
            img_number += f->getImageCount();
        }
    }
}

std::shared_ptr<Wz_File> Wz_Structure::loadFile(const std::string& fileName, std::shared_ptr<Wz_Node> node, 
                                                  bool useBaseWz, bool loadWzAsFolder) {
    std::shared_ptr<Wz_File> file = nullptr;

    try {
        file = std::make_shared<Wz_File>(fileName, shared_from_this());
        if (!file->isLoaded()) {
            throw std::runtime_error("The file is not a valid wz file.");
        }
        wz_files.push_back(file);
        node->setValue(file);
        file->setNode(node);
        
        // 设置加密类型
        if (!encryption->encryption_detected) {
            encryption->detectEncryption(file.get());
        }
        
        // 加载目录树（必须在版本检测之前，因为 CalcOffset 需要 Wz_Image 数据）
        file->getDirTree(node, useBaseWz, loadWzAsFolder);
        
        // 记录目录树结束位置（必须在版本验证之前）
        auto header = file->getHeader();
        if (header) {
            header->setDirEndPosition(file->getFileStreamPosition());
        }
        
        // 检测 wz 版本（在 getDirTree 之后，因为 CalcOffset 需要 Wz_Image）
        file->detectWzVersion(wzVersionVerifyMode);
        
        // 检测 wz 类型
        file->detectWzType();
        
        return file;
    } catch (...) {
        if (file) {
            file->close();
            auto it = std::find(wz_files.begin(), wz_files.end(), file);
            if (it != wz_files.end()) {
                wz_files.erase(it);
            }
        }
        throw;
    }
}

void Wz_Structure::load(const std::string& fileName, bool useBaseWz) {
    namespace fs = std::filesystem;
    
    std::string fileBasename = fs::path(fileName).filename().string();
    std::string dirPath = fs::path(fileName).parent_path().string();
    
    if (fileBasename == "List.wz" || fileBasename == "list.wz") {
        encryption->loadListWz(dirPath);
        WzNode = std::make_shared<Wz_Node>("List.wz");
        for (const auto& list : encryption->getList()) {
            WzNode->getNodes()->add(list);
        }
    } else {
        WzNode = std::make_shared<Wz_Node>(fileBasename);
        loadFile(fileName, WzNode, useBaseWz);
    }
    calculate_img_count();
}

void Wz_Structure::loadImg(const std::string& fileName) {
    namespace fs = std::filesystem;
    std::string basename = fs::path(fileName).filename().string();
    auto node = std::make_shared<Wz_Node>(basename);
    loadImg(fileName, node);
}

void Wz_Structure::loadImg(const std::string& fileName, std::shared_ptr<Wz_Node> node) {
    namespace fs = std::filesystem;
    
    std::shared_ptr<Wz_File> file = nullptr;
    try {
        file = std::make_shared<Wz_File>(fileName, shared_from_this());
        node->setValue(file);
        file->setNode(node);
        
        auto imgNode = std::make_shared<Wz_Node>(node->getText());
        imgNode->setValue(nullptr); // Wz_Image 稍后创建
        node->getNodes()->add(imgNode);
        wz_files.push_back(file);
    } catch (...) {
        if (file) file->close();
        throw;
    }
}

bool Wz_Structure::isKMST1125WzFormat(const std::string& fileName) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(fileName)) return false;
    
    std::string ext = fs::path(fileName).extension().string();
    if (ext != ".wz" && ext != ".WZ") return false;
    
    std::string iniFile = fs::path(fileName).replace_extension(".ini").string();
    if (!fs::exists(iniFile)) return false;
    
    try {
        Wz_Structure tempStruct;
        auto tempFile = std::make_shared<Wz_File>(fileName, shared_from_this());
        if (!tempFile->isLoaded()) return false;
        
        auto tempNode = std::make_shared<Wz_Node>();
        if (!encryption->encryption_detected) {
            encryption->detectEncryption(tempFile.get());
        }
        tempFile->getDirTree(tempNode);
        return tempFile->getImageCount() == 0;
    } catch (...) {
        return false;
    }
}

void Wz_Structure::loadWzFolder(const std::string& folder, std::shared_ptr<Wz_Node> node, bool useBaseWz) {
    namespace fs = std::filesystem;
    
    if (node == nullptr) {
        node = std::make_shared<Wz_Node>(fs::path(folder).filename().string());
    }
    
    std::string baseName = fs::path(folder).filename().string();
    std::string entryWzFile = (fs::path(folder) / baseName).string() + ".wz";
    
    if (!fs::exists(entryWzFile)) {
        throw std::runtime_error("Entry wz file not found: " + entryWzFile);
    }
    
    auto entryFile = loadFile(entryWzFile, node, useBaseWz, true);
    
    // 检测额外的 wz 文件
    if (autoDetectExtFiles) {
        int fileId = 2;
        while (true) {
            std::string extWzFile = fs::path(folder).string() + "/" + baseName + "_" + std::to_string(fileId) + ".wz";
            if (!fs::exists(extWzFile)) break;
            
            auto tempNode = std::make_shared<Wz_Node>(baseName + "_" + std::to_string(fileId) + ".wz");
            auto extFile = loadFile(extWzFile, tempNode, false, true);
            entryFile->mergeWzFile(extFile);
            
            fileId++;
        }
    }
}

void Wz_Structure::loadMsFile(const std::string& fileName) {
    auto node = std::make_shared<Wz_Node>(std::filesystem::path(fileName).filename().string());
    loadMsFile(fileName, node);
}

void Wz_Structure::loadMsFile(const std::string& fileName, std::shared_ptr<Wz_Node> node) {
    // MS 文件支持稍后实现
    throw std::runtime_error("MS file loading not yet implemented");
}

void Wz_Structure::loadKMST1125DataWz(const std::string& fileName) {
    namespace fs = std::filesystem;
    auto node = std::make_shared<Wz_Node>();
    loadWzFolder(fs::path(fileName).parent_path().string(), node, true);
    calculate_img_count();
}

} // namespace WzLibCpp

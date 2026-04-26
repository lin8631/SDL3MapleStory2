#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include "Wz_File.hpp"
#include "Wz_Crypto.hpp"

namespace WzLibCpp {

class Wz_File;
class Wz_Node;
class IMapleStoryFile;

class Wz_Structure : public std::enable_shared_from_this<Wz_Structure> {
public:
    Wz_Structure();

    void clear();
    void calculate_img_count();
    void load(const std::string& fileName, bool useBaseWz = false);
    std::shared_ptr<Wz_File> loadFile(const std::string& fileName, std::shared_ptr<Wz_Node> node, 
                                       bool useBaseWz = false, bool loadWzAsFolder = false);
    void loadImg(const std::string& fileName);
    void loadImg(const std::string& fileName, std::shared_ptr<Wz_Node> node);
    void loadKMST1125DataWz(const std::string& fileName);
    bool isKMST1125WzFormat(const std::string& fileName);
    void loadWzFolder(const std::string& folder, std::shared_ptr<Wz_Node> node, bool useBaseWz = false);
    void loadMsFile(const std::string& fileName);

    std::vector<std::shared_ptr<Wz_File>>& getWzFiles() { return wz_files; }
    const std::vector<std::shared_ptr<Wz_File>>& getWzFiles() const { return wz_files; }

    std::shared_ptr<Wz_Node> getWzNode() { return WzNode; }
    void setWzNode(std::shared_ptr<Wz_Node> node) { WzNode = node; }

    int32_t getImgNumber() const { return img_number; }
    void setImgNumber(int32_t num) { img_number = num; }

    bool getHasBasewz() const { return has_basewz; }
    void setHasBasewz(bool val) { has_basewz = val; }

    bool getSorted() const { return sorted; }
    void setSorted(bool val) { sorted = val; }

    std::shared_ptr<Wz_Crypto> getEncryption() { return encryption; }
    const std::shared_ptr<Wz_Crypto> getEncryption() const { return encryption; }

    bool getAutoDetectExtFiles() const { return autoDetectExtFiles; }
    void setAutoDetectExtFiles(bool val) { autoDetectExtFiles = val; }

    bool getImgCheckDisabled() const { return imgCheckDisabled; }
    void setImgCheckDisabled(bool val) { imgCheckDisabled = val; }

    WzVersionVerifyMode getWzVersionVerifyMode() const { return wzVersionVerifyMode; }
    void setWzVersionVerifyMode(WzVersionVerifyMode mode) { wzVersionVerifyMode = mode; }

    static std::shared_ptr<Wz_Structure> DefaultEncoding;
    static bool DefaultAutoDetectExtFiles;
    static bool DefaultImgCheckDisabled;
    static WzVersionVerifyMode DefaultWzVersionVerifyMode;

private:
    void loadMsFile(const std::string& fileName, std::shared_ptr<Wz_Node> node);

    std::vector<std::shared_ptr<Wz_File>> wz_files;
    std::vector<std::shared_ptr<IMapleStoryFile>> ms_files;
    std::shared_ptr<Wz_Crypto> encryption;
    std::shared_ptr<Wz_Node> WzNode;
    int32_t img_number = 0;
    bool has_basewz = false;
    bool sorted = false;
    bool autoDetectExtFiles = true;
    bool imgCheckDisabled = false;
    WzVersionVerifyMode wzVersionVerifyMode = WzVersionVerifyMode::Detect;
};

} // namespace WzLibCpp

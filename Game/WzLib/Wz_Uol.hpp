#pragma once
#include <string>
#include <memory>
#include <vector>

namespace WzLibCpp {

class Wz_Node;

class Wz_Uol {
public:
    explicit Wz_Uol(const std::string& uolPath);

    const std::string& getUol() const { return uol; }
    void setUol(const std::string& path) { uol = path; }

    std::shared_ptr<Wz_Node> handleUol(std::shared_ptr<Wz_Node> currentNode);

private:
    std::string uol;
};

} // namespace WzLibCpp

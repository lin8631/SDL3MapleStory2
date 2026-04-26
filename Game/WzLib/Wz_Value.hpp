#pragma once
#include <string>
#include <memory>
#include <variant>

namespace WzLibCpp {

/**
 * Wz 值类型
 * 用于存储不同类型的属性值
 */
class Wz_Value {
public:
    // 支持的值类型
    using ValueType = std::variant<
        std::monostate,  // 空值
        int16_t,         // Int16
        int32_t,         // Int32
        int64_t,         // Int64
        float,           // Float
        double,          // Double
        std::string,     // String
        bool             // Bool
    >;

    Wz_Value() : value_(std::monostate{}) {}
    
    // 构造函数
    explicit Wz_Value(int16_t v) : value_(v) {}
    explicit Wz_Value(int32_t v) : value_(v) {}
    explicit Wz_Value(int64_t v) : value_(v) {}
    explicit Wz_Value(float v) : value_(v) {}
    explicit Wz_Value(double v) : value_(v) {}
    explicit Wz_Value(const std::string& v) : value_(v) {}
    explicit Wz_Value(const char* v) : value_(std::string(v)) {}
    explicit Wz_Value(bool v) : value_(v) {}

    // 类型检查
    bool isNull() const { return std::holds_alternative<std::monostate>(value_); }
    bool isInt16() const { return std::holds_alternative<int16_t>(value_); }
    bool isInt32() const { return std::holds_alternative<int32_t>(value_); }
    bool isInt64() const { return std::holds_alternative<int64_t>(value_); }
    bool isFloat() const { return std::holds_alternative<float>(value_); }
    bool isDouble() const { return std::holds_alternative<double>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isBool() const { return std::holds_alternative<bool>(value_); }
    bool isNumeric() const { return isInt16() || isInt32() || isInt64() || isFloat() || isDouble(); }

    // 获取值
    int16_t getInt16(int16_t defaultVal = 0) const {
        if (auto p = std::get_if<int16_t>(&value_)) return *p;
        if (auto p = std::get_if<int32_t>(&value_)) return static_cast<int16_t>(*p);
        if (auto p = std::get_if<int64_t>(&value_)) return static_cast<int16_t>(*p);
        return defaultVal;
    }

    int32_t getInt32(int32_t defaultVal = 0) const {
        if (auto p = std::get_if<int32_t>(&value_)) return *p;
        if (auto p = std::get_if<int16_t>(&value_)) return static_cast<int32_t>(*p);
        if (auto p = std::get_if<int64_t>(&value_)) return static_cast<int32_t>(*p);
        if (auto p = std::get_if<bool>(&value_)) return *p ? 1 : 0;
        return defaultVal;
    }

    int64_t getInt64(int64_t defaultVal = 0) const {
        if (auto p = std::get_if<int64_t>(&value_)) return *p;
        if (auto p = std::get_if<int32_t>(&value_)) return static_cast<int64_t>(*p);
        if (auto p = std::get_if<int16_t>(&value_)) return static_cast<int64_t>(*p);
        return defaultVal;
    }

    float getFloat(float defaultVal = 0.0f) const {
        if (auto p = std::get_if<float>(&value_)) return *p;
        if (auto p = std::get_if<double>(&value_)) return static_cast<float>(*p);
        if (auto p = std::get_if<int32_t>(&value_)) return static_cast<float>(*p);
        return defaultVal;
    }

    double getDouble(double defaultVal = 0.0) const {
        if (auto p = std::get_if<double>(&value_)) return *p;
        if (auto p = std::get_if<float>(&value_)) return static_cast<double>(*p);
        if (auto p = std::get_if<int64_t>(&value_)) return static_cast<double>(*p);
        return defaultVal;
    }

    std::string getString(const std::string& defaultVal = "") const {
        if (auto p = std::get_if<std::string>(&value_)) return *p;
        return defaultVal;
    }

    bool getBool(bool defaultVal = false) const {
        if (auto p = std::get_if<bool>(&value_)) return *p;
        if (auto p = std::get_if<int32_t>(&value_)) return *p != 0;
        if (auto p = std::get_if<int16_t>(&value_)) return *p != 0;
        return defaultVal;
    }

    int getInt(int defaultVal = 0) const {
        return getInt32(defaultVal);
    }

    // 设置值
    void setInt16(int16_t v) { value_ = v; }
    void setInt32(int32_t v) { value_ = v; }
    void setInt64(int64_t v) { value_ = v; }
    void setFloat(float v) { value_ = v; }
    void setDouble(double v) { value_ = v; }
    void setString(const std::string& v) { value_ = v; }
    void setBool(bool v) { value_ = v; }
    void setNull() { value_ = std::monostate{}; }

private:
    ValueType value_;
};

// 便捷的智能指针类型
using Wz_ValuePtr = std::shared_ptr<Wz_Value>;

// 创建值的工厂函数
inline Wz_ValuePtr MakeInt16(int16_t v) { return std::make_shared<Wz_Value>(v); }
inline Wz_ValuePtr MakeInt32(int32_t v) { return std::make_shared<Wz_Value>(v); }
inline Wz_ValuePtr MakeInt64(int64_t v) { return std::make_shared<Wz_Value>(v); }
inline Wz_ValuePtr MakeFloat(float v) { return std::make_shared<Wz_Value>(v); }
inline Wz_ValuePtr MakeDouble(double v) { return std::make_shared<Wz_Value>(v); }
inline Wz_ValuePtr MakeString(const std::string& v) { return std::make_shared<Wz_Value>(v); }
inline Wz_ValuePtr MakeBool(bool v) { return std::make_shared<Wz_Value>(v); }

} // namespace WzLibCpp

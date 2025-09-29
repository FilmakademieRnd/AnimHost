/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program;
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <tuple>
#include <array>
#include <type_traits>
#include <string_view>

/**
 * @brief Macro to generate both const and non-const tie() methods for config structs
 *
 * This macro eliminates boilerplate by generating both const and non-const versions
 * of the tie() method that binds all struct fields into a std::tuple. This enables
 * automatic widget generation and JSON serialization.
 *
 * **Usage:**
 * @code
 * struct MyConfig {
 *     QString name;
 *     int value;
 *     bool enabled;
 *
 *     GENERATE_TIE_METHODS(name, value, enabled)
 *
 *     static constexpr auto field_names() {
 *         return std::array{"name", "value", "enabled"};
 *     }
 * };
 * @endcode
 *
 * @param ... Comma-separated list of all struct field names
 */
#define GENERATE_TIE_METHODS(...) \
    auto tie() const { return std::tie(__VA_ARGS__); } \
    auto tie()       { return std::tie(__VA_ARGS__); }

namespace ConfigUtils {

/**
 * Convert a value to QJsonValue based on its type
 */
template<typename T>
QJsonValue toJsonValue(const T& value) {
    if constexpr (std::is_same_v<T, QString>) {
        return QJsonValue(value);
    }
    else if constexpr (std::is_same_v<T, int>) {
        return QJsonValue(value);
    }
    else if constexpr (std::is_same_v<T, double>) {
        return QJsonValue(value);
    }
    else if constexpr (std::is_same_v<T, float>) {
        return QJsonValue(static_cast<double>(value));
    }
    else if constexpr (std::is_same_v<T, bool>) {
        return QJsonValue(value);
    }
    else {
        static_assert(std::is_same_v<T, void>, "Unsupported type for JSON serialization");
        return QJsonValue();
    }
}

/**
 * Convert a QJsonValue to a typed value
 */
template<typename T>
T fromJsonValue(const QJsonValue& value, const T& defaultValue = T{}) {
    if constexpr (std::is_same_v<T, QString>) {
        return value.isString() ? value.toString() : defaultValue;
    }
    else if constexpr (std::is_same_v<T, int>) {
        return value.isDouble() ? static_cast<int>(value.toDouble()) : defaultValue;
    }
    else if constexpr (std::is_same_v<T, double>) {
        return value.isDouble() ? value.toDouble() : defaultValue;
    }
    else if constexpr (std::is_same_v<T, float>) {
        return value.isDouble() ? static_cast<float>(value.toDouble()) : defaultValue;
    }
    else if constexpr (std::is_same_v<T, bool>) {
        return value.isBool() ? value.toBool() : defaultValue;
    }
    else {
        static_assert(std::is_same_v<T, void>, "Unsupported type for JSON deserialization");
        return defaultValue;
    }
}

/**
 * Convert a config struct to JSON using tie() and field_names()
 */
template<typename ConfigStruct>
QJsonObject structToJson(const ConfigStruct& config) {
    QJsonObject obj;
    auto values = config.tie();
    auto names = ConfigStruct::field_names();

    constexpr size_t fieldCount = std::tuple_size_v<decltype(values)>;
    static_assert(fieldCount == names.size(), "Field count mismatch between tie() and field_names()");

    std::apply([&obj, &names](auto&&... args) {
        size_t i = 0;
        ((obj[names[i++]] = toJsonValue(args)), ...);
    }, values);

    return obj;
}

/**
 * Convert JSON to a config struct using tie() and field_names()
 */
template<typename ConfigStruct>
ConfigStruct jsonToStruct(const QJsonObject& obj) {
    ConfigStruct config{};  // Default construct
    auto values = config.tie();  // Non-const tie() for modification
    auto names = ConfigStruct::field_names();

    constexpr size_t fieldCount = std::tuple_size_v<decltype(values)>;
    static_assert(fieldCount == names.size(), "Field count mismatch between tie() and field_names()");

    std::apply([&obj, &names](auto&&... args) {
        size_t i = 0;
        ((args = fromJsonValue<std::decay_t<decltype(args)>>(obj[names[i]], args), ++i), ...);
    }, values);

    return config;
}

/**
 * Check if a field name indicates a path field
 */
inline bool isPathField(std::string_view name) {
    auto endsWith = [](std::string_view str, std::string_view suffix) {
        return str.length() >= suffix.length() &&
               str.substr(str.length() - suffix.length()) == suffix;
    };
    auto startsWith = [](std::string_view str, std::string_view prefix) {
        return str.length() >= prefix.length() &&
               str.substr(0, prefix.length()) == prefix;
    };
    const bool hasPathPrefix = startsWith(name, "path_") || startsWith(name, "dir_") || startsWith(name, "folder_");
    const bool hasPathSuffix = endsWith(name, "_path") || endsWith(name, "_dir") || endsWith(name, "_folder");

    return hasPathPrefix || hasPathSuffix;
}

} // namespace ConfigUtils
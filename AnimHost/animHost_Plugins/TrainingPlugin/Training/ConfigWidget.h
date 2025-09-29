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

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <tuple>
#include <array>
#include <type_traits>
#include <functional>
#include <UIUtils.h>
#include "ConfigUtils.h"
#include "../TrainingPlugin_global.h"

/**
 * @brief Base class for configuration widgets with signal support
 */
class TRAININGPLUGINSHARED_EXPORT ConfigWidgetBase : public QWidget
{
    Q_OBJECT

signals:
    /**
     * @brief Emitted when any configuration value changes
     */
    void configChanged();

protected:
    /**
     * @brief Helper method for subclasses to emit configChanged signal
     */
    void emitConfigChanged() { emit configChanged(); }

public:
    explicit ConfigWidgetBase(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual ~ConfigWidgetBase() = default;
};

/**
 * @brief Auto-generates Qt widgets for configuration structs
 *
 * NOTE: Most developers should use ConfigNode, not ConfigWidget directly.
 * ConfigWidget is the UI component used internally by ConfigNode.
 *
 * @tparam ConfigStruct Must implement tie(), field_names(), display_names()
 *
 * @code
 * // 1. Define config struct with required methods
 * struct MyConfig {
 *  QString name; int value; ...
 *  // ... tie(), field_names(), display_names() implementations
 * };
 *
 * // 2. Add to ConfigWidget.cpp:
 * template class ConfigWidget<MyConfig>;
 *
 * // 3. Use in code
 * auto* widget = new ConfigWidget<MyConfig>();
 * @endcode
 */
template<typename ConfigStruct>
class ConfigWidget : public ConfigWidgetBase
{
public:
    /**
     * @brief Constructs a ConfigWidget with automatic UI generation
     * @param parent Parent widget (optional)
     */
    explicit ConfigWidget(QWidget* parent = nullptr);

    /**
     * @brief Sets the configuration values and updates all widgets
     * @param config The configuration struct containing values to display
     */
    void setConfig(const ConfigStruct& config);

    /**
     * @brief Retrieves the current configuration from all widgets
     * @return ConfigStruct with values from the current widget state
     */
    ConfigStruct getConfig() const;

private:
    mutable ConfigStruct _config;
    QVBoxLayout* _layout;
    std::vector<QWidget*> _widgets;

    void setupUI();
    void applyStyles();
    void updateWidgets();
    void updateConfigFromWidgets() const;

    // Template recursion methods for compile-time field iteration
    template<std::size_t I> void createWidgets();
    template<std::size_t I> void updateWidgetsFromConfig();
    template<std::size_t I> void updateConfigFromWidgets() const;

    // Widget creation methods for different field types
    template<std::size_t I, typename FieldType>
    QWidget* createWidgetForField(const QString& fieldName, const QString& displayName, FieldType& field);

    QWidget* createPathWidget(const QString& displayName, const QString& value);
    QWidget* createLineEditWidget(const QString& displayName, const QString& value);
    QWidget* createSpinBoxWidget(const QString& displayName, int value);
    QWidget* createDoubleSpinBoxWidget(const QString& displayName, double value);
    QWidget* createCheckBoxWidget(const QString& displayName, bool value);

    // Widget update methods
    template<std::size_t I, typename FieldType>
    void updateWidgetValue(const FieldType& field, const QString& fieldName, const QString& displayName);

    template<std::size_t I, typename FieldType>
    void updateConfigField(FieldType& field, const QString& fieldName, const QString& displayName) const;

};
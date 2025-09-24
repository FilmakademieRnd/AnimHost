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
#include <UIUtils.h>
#include "ConfigUtils.h"

/**
 * @brief Auto-generates Qt widgets for configuration structs
 *
 * Creates appropriate input widgets based on field types:
 * - QString: QLineEdit or FolderSelectionWidget (for *_path fields)
 * - int: QSpinBox
 * - double/float: QDoubleSpinBox
 * - bool: QCheckBox
 *
 * @tparam ConfigStruct Must implement tie(), field_names(), display_names()
 *
 * @code
 * ConfigWidget<StarkeConfig> widget;
 * widget.setConfig(config);
 * auto updated = widget.getConfig();
 * @endcode
 */
template<typename ConfigStruct>
class ConfigWidget : public QWidget
{
public:
    /**
     * @brief Constructs a ConfigWidget with automatic UI generation
     * @param parent Parent widget (optional)
     */
    explicit ConfigWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , _layout(new QVBoxLayout(this))
    {
        setupUI();
        setLayout(_layout);
    }

    /**
     * @brief Sets the configuration values and updates all widgets
     * @param config The configuration struct containing values to display
     */
    void setConfig(const ConfigStruct& config) {
        _config = config;
        updateWidgets();
    }

    /**
     * @brief Retrieves the current configuration from all widgets
     * @return ConfigStruct with values from the current widget state
     */
    ConfigStruct getConfig() const {
        updateConfigFromWidgets();
        return _config;
    }

private:
    mutable ConfigStruct _config;
    QVBoxLayout* _layout;
    std::tuple<QWidget*...> _widgets;

    void setupUI() {
        createWidgets<0>();
    }

    template<std::size_t I>
    void createWidgets() {
        if constexpr (I < std::tuple_size_v<decltype(_config.tie())>) {
            auto& field = std::get<I>(_config.tie());
            auto fieldName = _config.field_names()[I];
            auto displayName = _config.display_names()[I];

            QWidget* widget = createWidgetForField<I>(fieldName, displayName, field);
            _layout->addWidget(widget);
            std::get<I>(_widgets) = widget;

            createWidgets<I + 1>();
        }
    }

    template<std::size_t I, typename FieldType>
    QWidget* createWidgetForField(const QString& fieldName, const QString& displayName, FieldType& field) {
        if constexpr (std::is_same_v<std::decay_t<FieldType>, QString>) {
            if (ConfigUtils::isPathField(fieldName.toStdString())) {
                return createPathWidget(displayName, field);
            } else {
                return createLineEditWidget(displayName, field);
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, int>) {
            return createSpinBoxWidget(displayName, field);
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, double>) {
            return createDoubleSpinBoxWidget(displayName, field);
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, float>) {
            return createDoubleSpinBoxWidget(displayName, static_cast<double>(field));
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, bool>) {
            return createCheckBoxWidget(displayName, field);
        }
        else {
            static_assert(std::is_same_v<std::decay_t<FieldType>, void>, "Unsupported field type for ConfigWidget");
            return nullptr;
        }
    }

    QWidget* createPathWidget(const QString& displayName, const QString& /*value*/) {
        auto* pathWidget = new FolderSelectionWidget(this, FolderSelectionWidget::SelectionType::Directory);
        pathWidget->setObjectName(displayName);
        return pathWidget;
    }

    QWidget* createLineEditWidget(const QString& displayName, const QString& /*value*/) {
        auto* containerWidget = new QWidget(this);
        auto* layout = new QHBoxLayout(containerWidget);

        auto* label = new QLabel(displayName, containerWidget);
        label->setFixedWidth(120);

        auto* lineEdit = new QLineEdit(containerWidget);
        lineEdit->setObjectName(displayName);

        layout->addWidget(label);
        layout->addWidget(lineEdit);
        containerWidget->setLayout(layout);

        return containerWidget;
    }

    QWidget* createSpinBoxWidget(const QString& displayName, int /*value*/) {
        auto* containerWidget = new QWidget(this);
        auto* layout = new QHBoxLayout(containerWidget);

        auto* label = new QLabel(displayName, containerWidget);
        label->setFixedWidth(120);

        auto* spinBox = new QSpinBox(containerWidget);
        spinBox->setObjectName(displayName);
        spinBox->setRange(0, 10000);

        layout->addWidget(label);
        layout->addWidget(spinBox);
        containerWidget->setLayout(layout);

        return containerWidget;
    }

    QWidget* createDoubleSpinBoxWidget(const QString& displayName, double /*value*/) {
        auto* containerWidget = new QWidget(this);
        auto* layout = new QHBoxLayout(containerWidget);

        auto* label = new QLabel(displayName, containerWidget);
        label->setFixedWidth(120);

        auto* doubleSpinBox = new QDoubleSpinBox(containerWidget);
        doubleSpinBox->setObjectName(displayName);
        doubleSpinBox->setRange(0.0, 10000.0);
        doubleSpinBox->setDecimals(3);

        layout->addWidget(label);
        layout->addWidget(doubleSpinBox);
        containerWidget->setLayout(layout);

        return containerWidget;
    }

    QWidget* createCheckBoxWidget(const QString& displayName, bool /*value*/) {
        auto* checkBox = new QCheckBox(displayName, this);
        checkBox->setObjectName(displayName);
        return checkBox;
    }

    void updateWidgets() {
        updateWidgetsFromConfig<0>();
    }

    template<std::size_t I>
    void updateWidgetsFromConfig() {
        if constexpr (I < std::tuple_size_v<decltype(_config.tie())>) {
            auto& field = std::get<I>(_config.tie());
            auto displayName = _config.display_names()[I];
            auto fieldName = _config.field_names()[I];

            updateWidgetValue<I>(field, fieldName, displayName);
            updateWidgetsFromConfig<I + 1>();
        }
    }

    template<std::size_t I, typename FieldType>
    void updateWidgetValue(const FieldType& field, const QString& fieldName, const QString& displayName) {
        QWidget* widget = std::get<I>(_widgets);

        if constexpr (std::is_same_v<std::decay_t<FieldType>, QString>) {
            if (ConfigUtils::isPathField(fieldName.toStdString())) {
                auto* pathWidget = qobject_cast<FolderSelectionWidget*>(widget);
                if (pathWidget) {
                    pathWidget->SetDirectory(field);
                }
            } else {
                auto* lineEdit = widget->findChild<QLineEdit*>(displayName);
                if (lineEdit) {
                    lineEdit->setText(field);
                }
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, int>) {
            auto* spinBox = widget->findChild<QSpinBox*>(displayName);
            if (spinBox) {
                spinBox->setValue(field);
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, double> || std::is_same_v<std::decay_t<FieldType>, float>) {
            auto* doubleSpinBox = widget->findChild<QDoubleSpinBox*>(displayName);
            if (doubleSpinBox) {
                doubleSpinBox->setValue(static_cast<double>(field));
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, bool>) {
            auto* checkBox = qobject_cast<QCheckBox*>(widget);
            if (checkBox) {
                checkBox->setChecked(field);
            }
        }
    }

    void updateConfigFromWidgets() const {
        updateConfigFromWidgets<0>();
    }

    template<std::size_t I>
    void updateConfigFromWidgets() const {
        if constexpr (I < std::tuple_size_v<decltype(_config.tie())>) {
            auto& field = std::get<I>(_config.tie());
            auto displayName = _config.display_names()[I];
            auto fieldName = _config.field_names()[I];

            updateConfigField<I>(field, fieldName, displayName);
            updateConfigFromWidgets<I + 1>();
        }
    }

    template<std::size_t I, typename FieldType>
    void updateConfigField(FieldType& field, const QString& fieldName, const QString& displayName) const {
        QWidget* widget = std::get<I>(_widgets);

        if constexpr (std::is_same_v<std::decay_t<FieldType>, QString>) {
            if (ConfigUtils::isPathField(fieldName.toStdString())) {
                auto* pathWidget = qobject_cast<FolderSelectionWidget*>(widget);
                if (pathWidget) {
                    field = pathWidget->GetSelectedDirectory();
                }
            } else {
                auto* lineEdit = widget->findChild<QLineEdit*>(displayName);
                if (lineEdit) {
                    field = lineEdit->text();
                }
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, int>) {
            auto* spinBox = widget->findChild<QSpinBox*>(displayName);
            if (spinBox) {
                field = spinBox->value();
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, double>) {
            auto* doubleSpinBox = widget->findChild<QDoubleSpinBox*>(displayName);
            if (doubleSpinBox) {
                field = doubleSpinBox->value();
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, float>) {
            auto* doubleSpinBox = widget->findChild<QDoubleSpinBox*>(displayName);
            if (doubleSpinBox) {
                field = static_cast<float>(doubleSpinBox->value());
            }
        }
        else if constexpr (std::is_same_v<std::decay_t<FieldType>, bool>) {
            auto* checkBox = qobject_cast<QCheckBox*>(widget);
            if (checkBox) {
                field = checkBox->isChecked();
            }
        }
    }
};
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

#include "ConfigWidget.h"
#include "MLFrameworkTypes.h"


/**
 * @brief Constructor - Creates ConfigWidget with automatic UI generation
 *
 * This constructor initializes the widget, sets up the layout, and triggers
 * the automatic widget generation process based on the config struct fields.
 */
template<typename ConfigStruct>
ConfigWidget<ConfigStruct>::ConfigWidget(QWidget* parent)
    : ConfigWidgetBase(parent)
    , _layout(new QVBoxLayout(this))
{
    setupUI();
    setLayout(_layout);
    applyStyles();
}

/**
 * @brief Sets configuration values and updates all widgets
 *
 * Updates the internal config and refreshes all UI widgets to reflect
 * the new values. This is typically called when loading saved configs.
 */
template<typename ConfigStruct>
void ConfigWidget<ConfigStruct>::setConfig(const ConfigStruct& config) {
    _config = config;
    updateWidgets();
}

/**
 * @brief Retrieves current configuration from all widgets
 *
 * Reads values from all UI widgets and constructs a config struct.
 * This is called when the user wants to save or use the current settings.
 */
template<typename ConfigStruct>
ConfigStruct ConfigWidget<ConfigStruct>::getConfig() const {
    updateConfigFromWidgets();
    return _config;
}

/**
 * @brief Initial UI setup - determines field count and starts widget creation
 *
 * This function uses template metaprogramming to determine how many fields
 * the config struct has, then starts the compile-time recursive process
 * to create widgets for each field.
 */
template<typename ConfigStruct>
void ConfigWidget<ConfigStruct>::setupUI() {
    // Get the number of fields in the config struct at compile time
    constexpr size_t fieldCount = std::tuple_size_v<decltype(_config.tie())>;
    _widgets.resize(fieldCount);

    // Start recursive widget creation from field index 0
    createWidgets<0>();
}

/**
 * @brief Compile-time recursive widget creation
 *
 * This function uses "template recursion" - a technique where the compiler
 * generates multiple versions of this function at compile time, one for each
 * field index (I). Here's how it works:
 *
 * 1. createWidgets<0>() processes field 0, then calls createWidgets<1>()
 * 2. createWidgets<1>() processes field 1, then calls createWidgets<2>()
 * 3. createWidgets<2>() processes field 2, then calls createWidgets<3>()
 * 4. When I >= field count, the 'if constexpr' condition fails and recursion stops
 *
 * Why not a regular for loop?
 * Because std::get<I>() requires I to be a compile-time constant, not a runtime variable.
 * Template recursion lets us "loop" at compile time, generating separate functions
 * for each field index.
 *
 * @tparam I The current field index being processed (compile-time constant)
 */
template<typename ConfigStruct>
template<std::size_t I>
void ConfigWidget<ConfigStruct>::createWidgets() {
    // 'if constexpr' is a C++17 feature that evaluates the condition at compile time
    // If the condition is false, this entire block is not compiled
    if constexpr (I < std::tuple_size_v<decltype(_config.tie())>) {
        // Get reference to the field at index I
        auto& field = std::get<I>(_config.tie());

        // Get metadata for this field
        auto fieldName = _config.field_names()[I];
        auto displayName = _config.display_names()[I];

        // Create appropriate widget based on field type
        QWidget* widget = createWidgetForField<I>(fieldName, displayName, field);
        _layout->addWidget(widget);
        _widgets[I] = widget;

        // Recursive call: generate createWidgets<I+1>() and call it
        // The compiler will generate separate functions for each I value
        createWidgets<I + 1>();
    }
    // When I >= field count, this function body is empty and recursion stops
}

/**
 * @brief Creates appropriate widget based on field type
 *
 * Uses C++ template specialization and 'if constexpr' to determine the
 * correct widget type at compile time based on the field's C++ type.
 */
template<typename ConfigStruct>
template<std::size_t I, typename FieldType>
QWidget* ConfigWidget<ConfigStruct>::createWidgetForField(const QString& fieldName, const QString& displayName, FieldType& field) {
    // Template type checking - compiler chooses the right branch at compile time
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

template<typename ConfigStruct>
QWidget* ConfigWidget<ConfigStruct>::createPathWidget(const QString& displayName, const QString& /*value*/) {
    auto* pathWidget = new FolderSelectionWidget(this, FolderSelectionWidget::SelectionType::Directory);
    pathWidget->setObjectName(displayName);
    connect(pathWidget, &FolderSelectionWidget::directoryChanged, [this]() {
        emitConfigChanged();
    });
    return pathWidget;
}

template<typename ConfigStruct>
QWidget* ConfigWidget<ConfigStruct>::createLineEditWidget(const QString& displayName, const QString& /*value*/) {
    auto* containerWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(containerWidget);

    auto* label = new QLabel(displayName, containerWidget);
    label->setFixedWidth(120);

    auto* lineEdit = new QLineEdit(containerWidget);
    lineEdit->setObjectName(displayName);
    connect(lineEdit, &QLineEdit::textChanged, [this]() {
        emitConfigChanged();
    });

    layout->addWidget(label);
    layout->addWidget(lineEdit);
    containerWidget->setLayout(layout);

    return containerWidget;
}

template<typename ConfigStruct>
QWidget* ConfigWidget<ConfigStruct>::createSpinBoxWidget(const QString& displayName, int /*value*/) {
    auto* containerWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(containerWidget);

    auto* label = new QLabel(displayName, containerWidget);
    label->setFixedWidth(120);

    auto* spinBox = new QSpinBox(containerWidget);
    spinBox->setObjectName(displayName);
    spinBox->setRange(0, 10000);
    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
        emitConfigChanged();
    });

    layout->addWidget(label);
    layout->addWidget(spinBox);
    containerWidget->setLayout(layout);

    return containerWidget;
}

template<typename ConfigStruct>
QWidget* ConfigWidget<ConfigStruct>::createDoubleSpinBoxWidget(const QString& displayName, double /*value*/) {
    auto* containerWidget = new QWidget(this);
    auto* layout = new QHBoxLayout(containerWidget);

    auto* label = new QLabel(displayName, containerWidget);
    label->setFixedWidth(120);

    auto* doubleSpinBox = new QDoubleSpinBox(containerWidget);
    doubleSpinBox->setObjectName(displayName);
    doubleSpinBox->setRange(0.0, 10000.0);
    doubleSpinBox->setDecimals(3);
    connect(doubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this]() {
        emitConfigChanged();
    });

    layout->addWidget(label);
    layout->addWidget(doubleSpinBox);
    containerWidget->setLayout(layout);

    return containerWidget;
}

template<typename ConfigStruct>
QWidget* ConfigWidget<ConfigStruct>::createCheckBoxWidget(const QString& displayName, bool /*value*/) {
    auto* checkBox = new QCheckBox(displayName, this);
    checkBox->setObjectName(displayName);
    connect(checkBox, &QCheckBox::toggled, [this]() {
        emitConfigChanged();
    });
    return checkBox;
}

template<typename ConfigStruct>
void ConfigWidget<ConfigStruct>::applyStyles()
{
    StyleSheet styleSheet;
    QString customStyleSheet = styleSheet.mainStyleSheet;
    setStyleSheet(customStyleSheet);
}

/**
 * @brief Updates all widgets to reflect current config values
 *
 * This function triggers the compile-time recursive process to
 * update each widget with the corresponding field value from
 * the internal config struct.
 */
template<typename ConfigStruct>
void ConfigWidget<ConfigStruct>::updateWidgets() {
    updateWidgetsFromConfig<0>();
}

/**
 * @brief Compile-time recursive widget updating
 *
 * Similar to createWidgets(), this uses template recursion to update
 * each widget with the corresponding config field value.
 */
template<typename ConfigStruct>
template<std::size_t I>
void ConfigWidget<ConfigStruct>::updateWidgetsFromConfig() {
    if constexpr (I < std::tuple_size_v<decltype(_config.tie())>) {
        auto& field = std::get<I>(_config.tie());
        auto displayName = _config.display_names()[I];
        auto fieldName = _config.field_names()[I];

        updateWidgetValue<I>(field, fieldName, displayName);
        updateWidgetsFromConfig<I + 1>();
    }
}

/**
 * @brief Updates a specific widget with a config field value
 */
template<typename ConfigStruct>
template<std::size_t I, typename FieldType>
void ConfigWidget<ConfigStruct>::updateWidgetValue(const FieldType& field, const QString& fieldName, const QString& displayName) {
    QWidget* widget = _widgets[I];

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

/**
 * @brief Reads values from all widgets back into the config struct
 */
template<typename ConfigStruct>
void ConfigWidget<ConfigStruct>::updateConfigFromWidgets() const {
    updateConfigFromWidgets<0>();
}

/**
 * @brief Compile-time recursive config updating from widgets
 */
template<typename ConfigStruct>
template<std::size_t I>
void ConfigWidget<ConfigStruct>::updateConfigFromWidgets() const {
    if constexpr (I < std::tuple_size_v<decltype(_config.tie())>) {
        auto& field = std::get<I>(_config.tie());
        auto displayName = _config.display_names()[I];
        auto fieldName = _config.field_names()[I];

        updateConfigField<I>(field, fieldName, displayName);
        updateConfigFromWidgets<I + 1>();
    }
}

/**
 * @brief Updates a specific config field from its corresponding widget
 */
template<typename ConfigStruct>
template<std::size_t I, typename FieldType>
void ConfigWidget<ConfigStruct>::updateConfigField(FieldType& field, const QString& fieldName, const QString& displayName) const {
    QWidget* widget = _widgets[I];

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

// ============================================================================
// Explicit Template Instantiations
// ============================================================================
//
// IMPORTANT FOR DEVELOPERS:
// When you create a new config struct, you MUST add an explicit instantiation
// here for both ConfigWidget and any ConfigNode that uses it.
//
// Example: For a new MyConfigStruct, add:
// template class ConfigWidget<MyConfigStruct>;

template class ConfigWidget<MLFramework::StarkeConfig>;
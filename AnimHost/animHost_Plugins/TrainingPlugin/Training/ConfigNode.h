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

#include "../TrainingPlugin_global.h"
#include <QMetaType>
#include <QtWidgets>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>
#include "ConfigWidget.h"

/**
 * @brief Generic configuration node template for any config struct
 *
 * Provides a reusable node that outputs configuration data and allows
 * real-time editing through an auto-generated ConfigWidget.
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
 * // 2. Add to ConfigWidget.cpp (only ConfigWidget needs instantiation):
 * template class ConfigWidget<MyConfig>;
 *
 * // 3. Create node class
 * class MyConfigNode : public ConfigNode<MyConfig> {
 *     Q_OBJECT
 * public:
 *     QString category() override { return "My Category"; }
 *     std::unique_ptr<NodeDelegateModel> Init() override {
 *         return std::make_unique<MyConfigNode>();
 *     }
 * };
 * @endcode
 */
template<typename ConfigStruct>
class TRAININGPLUGINSHARED_EXPORT ConfigNode : public PluginNodeInterface
{

private:
    ConfigWidget<ConfigStruct>* _widget;
    std::shared_ptr<AnimNodeData<ConfigStruct>> _configOut;

public:
    ConfigNode() {
        _widget = nullptr;
        _configOut = std::make_shared<AnimNodeData<ConfigStruct>>();
    }

    virtual ~ConfigNode() = default;

    // Mandatory overrides for derived classes
    virtual QString category() override = 0;
    virtual std::unique_ptr<NodeDelegateModel> Init() override = 0;

    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override {
        if (portType == QtNodes::PortType::In)
            return 0;  // No input ports - pure data provider
        else
            return 1;  // One config output port
    }

    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        if (portType == QtNodes::PortType::Out && portIndex == 0) {
            return AnimNodeData<ConfigStruct>::staticType();
        }
        return NodeDataType{};
    }

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override {
        if (port == 0) {
            return _configOut;
        }
        return nullptr;
    }

    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override {
        // No input processing needed for pure data provider
        Q_UNUSED(data)
        Q_UNUSED(portIndex)
    }

    bool isDataAvailable() override {
        return true;  // Config is always available
    }

    void run() override {
        // No run processing needed - config updates are UI-driven
    }

    QWidget* embeddedWidget() override {
        if (!_widget) {
            _widget = new ConfigWidget<ConfigStruct>();

            // Set initial config
            ConfigStruct defaultConfig{};
            _widget->setConfig(defaultConfig);

            // Connect config changes to output updates
            connect(_widget, &ConfigWidgetBase::configChanged, this, &ConfigNode::updateConfigOutput);
        }
        return _widget;
    }

private:
    void updateConfigOutput() {
        if (_widget) {
            ConfigStruct currentConfig = _widget->getConfig();
            auto configPtr = std::make_shared<ConfigStruct>(currentConfig);
            _configOut->setData(configPtr);
            emitDataUpdate(0);
        }
    }

};

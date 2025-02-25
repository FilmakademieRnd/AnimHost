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

 

#ifndef COORDINATECONVERTERPLUGINPLUGIN_H
#define COORDINATECONVERTERPLUGINPLUGIN_H

#include "CoordinateConverterPlugin_global.h"
#include <QMetaType>
#include <QtWidgets>
#include <pluginnodeinterface.h>
#include "UIUtils.h"

class COORDINATECONVERTERPLUGINSHARED_EXPORT CoordinateConverterPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.CoordinateConverter" FILE "CoordinateConverterPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

public:

    /**
     * @struct CoordinateConversionPreset
     * @brief A structure to hold the settings for a coordinate conversion preset.
     *
     * This structure encapsulates the settings for a coordinate conversion preset,
     * containing a transformation matrix and nessecary transforms to quaternion rotations.
     */
    struct CoordinateConversionPreset {
        QString name;
        glm::mat4 transformMatrix = glm::mat4(1.0f);
		glm::mat4 characterRootTransform = glm::mat4(1.0f);
		bool applyScaleOnCharacter = false;
        bool flipYZ = false;
        bool negX = false;
        bool negY = false;
        bool negZ = false;
        bool negW = false;
    };

private:
    
    std::weak_ptr<AnimNodeData<Animation>> _animationIn;
    std::shared_ptr<AnimNodeData<Animation>> _animationOut = nullptr; 
    std::vector<CoordinateConversionPreset> presets;

    CoordinateConversionPreset activePreset;

    // UI
    QWidget* widget = nullptr;
    QVBoxLayout* _layout = nullptr;
    QCheckBox* debugCheckBox = nullptr;


    // Preset selection
    QHBoxLayout* presetLayout = nullptr;
    QLabel* presetLabel = nullptr;
    QComboBox* presetComboBox = nullptr;


    // Debug Widget
    QWidget* debugWidget = nullptr;
    QVBoxLayout* debugLayout = nullptr;

    // Checkboxes
    QCheckBox* xButton = nullptr;
    QCheckBox* yButton = nullptr;
    QCheckBox* zButton = nullptr;
    QCheckBox* wButton = nullptr;
    QCheckBox* swapYzButton = nullptr;

    // Matrix Editor
    QLabel* matrixLabel = nullptr;
    MatrixEditorWidget* matrixEditor = nullptr;
    QPushButton* applyButton = nullptr;


public:
    CoordinateConverterPlugin();
    ~CoordinateConverterPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<CoordinateConverterPlugin>(new CoordinateConverterPlugin()); };

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;
    bool isDataAvailable() override;
    void run() override;

    QWidget* embeddedWidget() override;

private:
    glm::quat ConvertToTargetSystem(const glm::quat& qIN, bool flipYZ = false, bool negX = false, bool negY = false, bool negZ = false, bool negW = false);

    glm::mat4 ConvertToTargetSystem(const glm::mat4& matIn, bool flipYZ = false, bool negX = false, bool negY = false, bool negZ = false, bool negW = false);
    
    glm::vec3 ConvertToTargetSystem(const glm::vec3& matIn, bool flipYZ = false, bool negX = false, bool negY = false, bool negZ = false, bool negW = false);

private Q_SLOTS:
    void onChangedCheck(int check);

};

#endif // COORDINATECONVERTERPLUGINPLUGIN_H

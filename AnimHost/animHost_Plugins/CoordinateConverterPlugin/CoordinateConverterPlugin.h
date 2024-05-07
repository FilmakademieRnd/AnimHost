
#ifndef COORDINATECONVERTERPLUGINPLUGIN_H
#define COORDINATECONVERTERPLUGINPLUGIN_H

#include "CoordinateConverterPlugin_global.h"
#include <QMetaType>
#include <QtWidgets>
#include <pluginnodeinterface.h>

class QPushButton;

class COORDINATECONVERTERPLUGINSHARED_EXPORT CoordinateConverterPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.CoordinateConverter" FILE "CoordinateConverterPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    std::weak_ptr<AnimNodeData<Animation>> _animationIn;

    std::shared_ptr<AnimNodeData<Animation>> _animationOut = nullptr; 

private:
    QWidget* widget = nullptr;

    QVBoxLayout* _layout = nullptr;

    QCheckBox* xButton = nullptr;
    QCheckBox* yButton = nullptr;
    QCheckBox* zButton = nullptr;
    QCheckBox* wButton = nullptr;
    
    QCheckBox* swapYzButton = nullptr;


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

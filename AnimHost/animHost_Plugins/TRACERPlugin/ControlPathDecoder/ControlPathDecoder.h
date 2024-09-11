#ifndef CONTROLPATHNODE_H
#define CONTROLPATHNODE_H

#include "../TRACERPlugin_global.h"
#include "../TRACERUpdateMessage.h"
#include <QMetaType>
#include <QObject>
#include <QComboBox>
#include <QVBoxLayout>
#include <pluginnodeinterface.h>

class QPushButton;

class TRACERPLUGINSHARED_EXPORT ControlPathDecoderNode : public PluginNodeInterface
{
    Q_OBJECT
private:
    QWidget* _widget = nullptr;
    QVBoxLayout* _mainLayout = nullptr;
    QPushButton* _pushButton = nullptr;
    QComboBox* _comboBox = nullptr;


    bool _recievedControlPathControlPoints = false;
    bool _recievedControlPathOrientation = false;


    uint16_t _objectID = 1;
    uint16_t _paramControlPointID = 3;
    uint16_t _paramOrientationID = 4;



    std::weak_ptr<AnimNodeData<ParameterUpdate>> _ParamIn;

public:
    ControlPathDecoderNode();
    ~ControlPathDecoderNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<ControlPathDecoderNode>(new ControlPathDecoderNode()); };

    static QString Name() { return QString("ControlPathDecoderNode"); }

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    bool hasInputRunSignal() const override { return false; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool  isDataAvailable() override;

    void run() override;

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onButtonClicked();

};

#endif // CONTROLPATHNODE_H
#ifndef UPDATERECEIVERNODE_H
#define UPDATERECEIVERNODE_H

#include "../TRACERPlugin_global.h"
#include "../TRACERUpdateReceiver.h"
#include "../TRACERUpdateMessage.h"
#include <ZMQMessageHandler.h>
#include <QMetaType>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <QCheckBox>
#include <pluginnodeinterface.h>
#include <UIUtils.h>

class QPushButton;

class TRACERPLUGINSHARED_EXPORT UpdateReceiverNode : public PluginNodeInterface
{
    Q_OBJECT
private:

    std::shared_ptr<TRACERUpdateReceiver> _updateReceiver;


    QWidget* _widget = nullptr;
    QVBoxLayout* _mainLayout = nullptr;

    // IP Input
    QHBoxLayout* _ipAddressLayout = nullptr;
    QLineEdit* _ipAddress = nullptr;
    QRegularExpressionValidator* _ipValidator = nullptr;

    QCheckBox* _autoStart = nullptr;

    QPushButton* _connectButton = nullptr;

    SignalLightWidget* _signalLight = nullptr;

    std::shared_ptr<AnimNodeData<ParameterUpdate>> _parameterUpdateOut = nullptr;
    std::shared_ptr<AnimNodeData<RPCUpdate>> _rpcUpdateOut = nullptr;

public:
    UpdateReceiverNode(std::shared_ptr<TRACERUpdateReceiver> updateReceiver);
    ~UpdateReceiverNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  nullptr; };

    QJsonObject save() const override;
    void load(QJsonObject const& p) override;

    static QString Name() { return QString("UpdateReceiverNode"); }



    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    bool hasInputRunSignal() const override { return false; }
    bool hasOutputRunSignal() const override { return false; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool  isDataAvailable() override;

    void run() override;

    
    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onButtonClicked();

    void forwardParameterUpdateMessage(uint8_t sceneID, uint16_t objectID, uint16_t paramID, ZMQMessageHandler::ParameterType paramType, const QByteArray rawData);

    void forwardRPCMessage(uint8_t sceneID, uint16_t objectID, uint16_t paramID, ZMQMessageHandler::ParameterType paramType, const QByteArray rawData);


};

#endif // UPDATERECEIVERNODE_H
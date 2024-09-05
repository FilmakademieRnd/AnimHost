#ifndef RPCTRIGGER_H
#define RPCTRIGGER_H

#include "../TRACERPlugin_global.h"
#include "../TRACERUpdateMessage.h"
#include <QMetaType>
#include <pluginnodeinterface.h>


enum AnimHostRPCType : uint32_t {
	STOP = 0,
	STREAM = 1,
	STREAM_LOOP = 2,
    BLOCK =3
};

class QPushButton;

class TRACERPLUGINSHARED_EXPORT RPCTriggerNode : public PluginNodeInterface
{
    Q_OBJECT
private:
    QPushButton* _pushButton;


    std::weak_ptr<AnimNodeData<RPCUpdate>> _RPCIn;

public:
    RPCTriggerNode();
    ~RPCTriggerNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<RPCTriggerNode>(new RPCTriggerNode()); };

    static QString Name() { return QString("RPCTriggerNode"); }

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

#endif // RPCTRIGGER_H
#ifndef RPCTRIGGER_H
#define RPCTRIGGER_H

#include "../TRACERPlugin_global.h"
#include "../TRACERUpdateMessage.h"
#include <QMetaType>
#include <QObject>
#include <QComboBox>
#include <QVBoxLayout>
#include <pluginnodeinterface.h>
#include <QtNodes/NodeDelegateModelRegistry>

#include <UIUtils.h>

using QtNodes::NodeDelegateModelRegistry;



class QPushButton;

class TRACERPLUGINSHARED_EXPORT RPCTriggerNode : public PluginNodeInterface
{
    Q_OBJECT
private:
    QWidget* _widget = nullptr;
    QVBoxLayout* _mainLayout = nullptr;
    QPushButton* _pushButton = nullptr;
    QComboBox* _comboBox = nullptr;

	DynamicListWidget* _listWidget = nullptr;

    std::weak_ptr<AnimNodeData<RPCUpdate>> _RPCIn;

	std::weak_ptr<AnimNodeData<CharacterObject>> _characterIn;

    AnimHostRPCType _filterType = AnimHostRPCType::BLOCK;


	// List of avialable Property Targets for RPC Trigger
	QList<QPair<QString, QStringList>> _nodeElements;

    // Mapping struct
    struct TRACERPLUGINSHARED_EXPORT RPCMapping {
        int parameterID;
        QString targetNode;
        QString targetProperty;
        bool triggerRun;
        QVariant value;
    };



	// Defined Mappings for RPC Trigger
	QList<RPCMapping> _rpcMappings;


public:
    RPCTriggerNode(const NodeDelegateModelRegistry& modelRegistry);
    ~RPCTriggerNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return nullptr; };

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

	void onElementAdded();

    void onElementRemoved(int index);

	void onElementMappingChanged(int elementIdx, int paramId, QString node, QString property, int trigger);

};

#endif // RPCTRIGGER_H
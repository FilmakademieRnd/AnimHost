#ifndef ANIMATIONSENDER_H
#define ANIMATIONSENDER_H

#include "../TRACERPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>

class QPushButton;

class TRACERPLUGINSHARED_EXPORT AnimationSenderNode : public PluginNodeInterface
{
    Q_OBJECT
private:
    QPushButton* _pushButton;

public:
    AnimationSenderNode();
    ~AnimationSenderNode();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<AnimationSenderNode>(new AnimationSenderNode()); };

    static QString Name() { return QString("AnimationSenderNode"); }

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

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

#endif // ANIMATIONSENDER_H
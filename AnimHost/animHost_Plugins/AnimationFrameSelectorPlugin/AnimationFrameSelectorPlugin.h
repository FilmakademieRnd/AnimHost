
#ifndef ANIMATIONFRAMESELECTORPLUGINPLUGIN_H
#define ANIMATIONFRAMESELECTORPLUGINPLUGIN_H

#include "AnimationFrameSelectorPlugin_global.h"
#include <QtWidgets>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>

class QPushButton;

class ANIMATIONFRAMESELECTORPLUGINSHARED_EXPORT AnimationFrameSelectorPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.AnimationFrameSelector" FILE "AnimationFrameSelectorPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    QWidget* _widget;
    QSlider* _slider;

    QLabel* _l1;
    QLabel* _l2;
    QLabel* _l3;
    QLabel* _l4;

private:

    std::weak_ptr<AnimNodeData<Animation>> _animationIn;
    std::shared_ptr<AnimNodeData<Animation>> _animationOut;

public:
    AnimationFrameSelectorPlugin();
    ~AnimationFrameSelectorPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<AnimationFrameSelectorPlugin>(new AnimationFrameSelectorPlugin()); };

    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;

    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    void run() override;


    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Operator"; };  // Returns a category for the node

private Q_SLOTS:
    void onFrameChange(int value);

};

#endif // ANIMATIONFRAMESELECTORPLUGINPLUGIN_H

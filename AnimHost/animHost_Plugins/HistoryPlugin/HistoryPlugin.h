
#ifndef HISTORYPLUGINPLUGIN_H
#define HISTORYPLUGINPLUGIN_H

#include "HistoryPlugin_global.h"
#include <QMetaType>
#include <QtWidgets>
#include <pluginnodeinterface.h>
#include "HistoryHelper.h"


class HISTORYPLUGINSHARED_EXPORT HistoryPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.History" FILE "HistoryPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    
    std::weak_ptr<AnimNodeData<PoseSequence>> _inPoseSeq;

    std::shared_ptr<AnimNodeData<PoseSequence>> _outPoseSeq;
    
    int numHistoryFrames = 1;

    RingBuffer<Pose>* _poseHistory = nullptr;

private:

    QWidget* widget = nullptr;
    QHBoxLayout* _hLayout = nullptr;
    QLabel* _label = nullptr;
    QLineEdit* _lineEdit = nullptr;


public:
    HistoryPlugin();
    ~HistoryPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<HistoryPlugin>(new HistoryPlugin()); };

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

private Q_SLOTS:
    void onButtonClicked();

};

#endif // HISTORYPLUGINPLUGIN_H

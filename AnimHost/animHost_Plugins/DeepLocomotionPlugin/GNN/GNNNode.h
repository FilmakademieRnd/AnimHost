#ifndef GNN_H
#define GNN_H

#include "../DeepLocomotionPlugin_global.h"
#include <QMetaType>
#include <pluginnodeinterface.h>
#include "GNNController.h"
#include "UIUtils.h"


class DEEPLOCOMOTIONPLUGINSHARED_EXPORT GNNNode : public PluginNodeInterface
{
    Q_OBJECT

    Q_PROPERTY(float phaseBias MEMBER _phaseBias)
private:
    
	float _phaseBias = 0.0f;

    // Input Data
    std::weak_ptr<AnimNodeData<Animation>> _animationIn;
    std::weak_ptr<AnimNodeData<Skeleton>> _skeletonIn;
    std::weak_ptr<AnimNodeData<ControlPath>> _controlPathIn;
    std::weak_ptr<AnimNodeData<JointVelocitySequence>> _jointVelocitySequenceIn;


    //Output Data
    std::shared_ptr<AnimNodeData<Animation>> _animationOut;
    std::shared_ptr<AnimNodeData<DebugSignal>> _debugSignalOut;

    //Neural Network Controller
    std::unique_ptr<GNNController> controller;
    QString _NetworkPath;

    //UI
    QWidget* _widget = nullptr;
    FolderSelectionWidget* _fileSelectionWidget = nullptr;
    QDoubleSpinBox* _mixRootRotation = nullptr;
    QDoubleSpinBox* _mixRootTranslation = nullptr;
    QDoubleSpinBox* _mixControlPathTranslation = nullptr;
    QDoubleSpinBox* _mixControlPathRotation = nullptr;
	QSlider* _networkPhaseBias = nullptr;
	QSlider* _networkControlBias = nullptr;

public:
    GNNNode();
    ~GNNNode();

    QJsonObject save() const override;
    void load(QJsonObject const& p) override;
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<GNNNode>(new GNNNode()); };

    static QString Name() { return QString("GNNNode"); }

    QString category() override { return "Undefined Category"; };
    QString caption() const override { return "Locomotion Generator (2D Spline)"; }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;
    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool  isDataAvailable() override;

    void run() override;

    QWidget* embeddedWidget() override;

private Q_SLOTS:
    void onFileSelectionChanged();

};

#endif // GNN_H
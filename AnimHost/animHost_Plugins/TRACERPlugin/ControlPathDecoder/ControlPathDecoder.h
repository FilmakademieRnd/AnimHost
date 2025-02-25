#ifndef CONTROLPATHNODE_H
#define CONTROLPATHNODE_H

#include "../TRACERPlugin_global.h"
#include "../TRACERUpdateMessage.h"
#include <QMetaType>
#include <QObject>
#include <QtWidgets>
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
	QCheckBox* _pathFromBlender = nullptr;


    bool _receivedControlPathPointRotation = false;
    bool _receivedControlPathPointLocation = false;

	bool _pathFromBlenderChecked = false; // Flag to indicate if the path is from Blender, and thus needs to be transformed

    uint16_t _characterID = 2;          // It refers to the selected Character Object
    uint16_t _paramControlPath = -1;    // This is the parameterID of the Character Object that keeps the ID of the Control Path associated with the selected Character Object 
    uint16_t _controlPathID = 1;        // It refers to the Control Path for the selected Character (as indicated by the paramControlPathID)
    uint16_t _paramPointLocationID = 3; // This is the parameterID of the Locations of the points of the Control Path
    uint16_t _paramPointRotationID = 4; // This is the parameterID of the Rotations of the points of the Control Path

    std::vector<KeyFrame<glm::vec3>> _pointLocation;
    std::vector<KeyFrame<glm::quat>> _pointRotation;

    std::weak_ptr<AnimNodeData<ParameterUpdate>> _ParamIn;
    std::weak_ptr<AnimNodeData<CharacterObject>> _characterIn;

    std::shared_ptr<AnimNodeData<ControlPath>> _OutControlPath;

    // Helper functions for adaptive Beziér Sampling
    static std::vector<ControlPoint>*   adaptiveSegmentSampling(glm::vec3       knot1,      glm::vec3   handle1,    glm::vec3   handle2,    glm::vec3   knot2,
                                                                float           easeFrom,   float       easeTo,
                                                                glm::quat       quat1,      glm::quat   quat2,
                                                                int             frameStart, int         frameEnd,
                                                                ControlPoint*   prevCP = nullptr);
    static std::vector<float>           adaptiveTimingsResampling(float easeFrom, float easeTo, int nSamples);
    static glm::vec3                    sampleBezier(glm::vec3 knot1, glm::vec3 handle1, glm::vec3 handle2, glm::vec3 knot2, float t);

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
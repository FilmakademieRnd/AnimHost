
#ifndef BASICONNXPLUGINPLUGIN_H
#define BASICONNXPLUGINPLUGIN_H

#include "BasicOnnxPlugin_global.h"
#include <QMetaType>

#include <QtWidgets>
#include <pluginnodeinterface.h>
#include <onnxruntime_cxx_api.h>
#include "OnnxModel.h"

class OnnxModelViewWidget;


class BASICONNXPLUGINSHARED_EXPORT BasicOnnxPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.BasicOnnx" FILE "BasicOnnxPlugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:

    

    //GUI
    OnnxModelViewWidget* widget;

    //TodO. Link Model to ModelView
    //Add Tensor Class

    std::unique_ptr<OnnxModel> _onnxModel;

    QString OnnxModelFilePath = "";
    bool bModelDescValid = false;



    std::vector<std::weak_ptr<NodeData>> _dataIn;
    std::vector<std::shared_ptr<NodeData>> _dataOut;




public:
    BasicOnnxPlugin();
    ~BasicOnnxPlugin();
    
    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<BasicOnnxPlugin>(new BasicOnnxPlugin()); };

    QString caption() const override { return "Onnx Runtime Inference";  }
    bool captionVisible() const override { return true; }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;

    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool isDataAvailable() override;
    
    void run() override;

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Operator"; };  // Returns a category for the node
    /// It is possible to hide port caption in GUI
    bool portCaptionVisible(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override { return true; }


    /// Port caption is used in GUI to label individual ports
    QString portCaption(QtNodes::PortType, QtNodes::PortIndex) const override;

private:
    void LoadOnnxModel();
    void RunDummyInference();

    void clearPorts();
    void addPort();


    std::string shape_printer(const std::vector<int64_t>& vec) {
        return std::accumulate(vec.begin(), vec.end(), std::string{},
            [](const std::string& a, int b) {
                return a.empty() ? std::to_string(b) : a + " " + std::to_string(b);
            });
    }


private Q_SLOTS:
    void onButtonClicked();

};

#endif // BASICONNXPLUGINPLUGIN_H

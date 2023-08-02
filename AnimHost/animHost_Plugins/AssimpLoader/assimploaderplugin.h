#ifndef ASSIMPLOADERPLUGIN_H
#define ASSIMPLOADERPLUGIN_H

#include "assimploaderplugin_global.h"
#include <QMetaType>
#include <QtCore/QObject>
#include <pluginnodeinterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>
#include <QPushButton>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



class ASSIMPLOADERPLUGINSHARED_EXPORT AssimpLoaderPlugin : public PluginNodeInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.animhost.PluginNodeInterface" FILE "assimploaderplugin.json")
    Q_INTERFACES(PluginNodeInterface)

private:
    std::shared_ptr<AnimNodeData<Skeleton>> _skeleton;
    std::shared_ptr<AnimNodeData<Animation>> _animation;

    QString SourceFilePath = "C:/Users/m5940/_dev/_datasets/Mixamo_new/Doozy/Drunk Walk.bvh";

    bool bDataValid;

    QPushButton* _pushButton;

public:
    AssimpLoaderPlugin();

    AssimpLoaderPlugin(const AssimpLoaderPlugin& o) : SourceFilePath(o.SourceFilePath) { qDebug() << "Copied! " << this->name(); };

    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<AssimpLoaderPlugin>(new AssimpLoaderPlugin()); };

    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    unsigned int nPorts(QtNodes::PortType portType) const override;
    NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(QtNodes::PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override {};

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "1234"; };  // Returns a category for the node

private Q_SLOTS:
    void onButtonClicked();

private:
    void loadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode* pNode);
    void importAssimpData();




};

#endif // EXAMPLEPLUGIN_H

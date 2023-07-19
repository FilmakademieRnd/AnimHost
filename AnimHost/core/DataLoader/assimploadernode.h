#ifndef ASSIMPLOADERNODE_H
#define ASSIMPLOADERNODE_H

#define NODE_EDITOR_SHARED 1

#include <QtNodes/NodeDelegateModel>
#include <QDir>
#include "plugininterface.h"
#include <commondatatypes.h>
#include <nodedatatypes.h>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class ANIMHOSTCORESHARED_EXPORT AssimpLoaderNode : public NodeDelegateModel
{
    Q_OBJECT

public:
    AssimpLoaderNode();


public:
    QString caption() const override { return QStringLiteral("ASSIMP Loader"); }

    bool captionVisible() const override { return false; }

    QString name() const override { return "ASSIMP Loader"; }

public:
    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override {};

    QWidget *embeddedWidget() override { return nullptr; }

private:
    void loadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode*);
    void importAssimpData();

    bool bDataValid;
    


protected:
    std::vector<std::shared_ptr<NodeData>> _dataOut;

    std::shared_ptr<SkeletonNodeData> _skeleton;

    std::shared_ptr<AnimationNodeData> _animation;

    QString SourceFilePath = "C:/Users/m5940/_dev/_datasets/Mixamo_new/Doozy/Drunk Walk.bvh";

};

#endif // ANIMHOSTNODE_H

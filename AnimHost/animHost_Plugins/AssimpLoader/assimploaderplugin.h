#ifndef ASSIMPLOADERPLUGIN_H
#define ASSIMPLOADERPLUGIN_H

#include "assimploaderplugin_global.h"
#include <QMetaType>
#include <QtCore/QObject>
#include <pluginnodeinterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>
#include <QtWidgets>
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


    QString SourceDirectory = "";
    QString SourceFilePath = "";

    bool bDataValid;

    QWidget* widget;
    QPushButton* _pushButton;
    QLabel* _label;
    QHBoxLayout* _filePathLayout;

    int globalSequenceCounter = 0;

public:
    AssimpLoaderPlugin();

    AssimpLoaderPlugin(const AssimpLoaderPlugin& o) : SourceFilePath(o.SourceFilePath) { qDebug() << "Copied! " << this->name(); };

    std::unique_ptr<NodeDelegateModel> Init() override { return  std::unique_ptr<AssimpLoaderPlugin>(new AssimpLoaderPlugin()); };

    QString caption() const override { return "Animation Import"; }
    QString name() const override { return "Animation Import"; }
   
    bool captionVisible() const override { return true; }

public:
    QJsonObject save() const override;

    void load(QJsonObject const& p) override;


public:
    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;

    void processInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) {};

    void run() override;

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Import"; };  // Returns a category for the node

private Q_SLOTS:
    void onButtonClicked();

private:
    void loadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode* pNode);
    void importAssimpData();

    void selectDir();

    QStringList loadFilesFromDir();

};

#endif // EXAMPLEPLUGIN_H

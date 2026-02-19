/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

 
#ifndef ASSIMPLOADERPLUGIN_H
#define ASSIMPLOADERPLUGIN_H

#include "assimploaderplugin_global.h"
#include <QMetaType>
#include <QtCore/QObject>
#include <QComboBox>
#include <pluginnodeinterface.h>
#include <commondatatypes.h>
#include <nodedatatypes.h>
#include <QtWidgets>
#include <UIUtils.h>
#include <SkeletonConfig.h>
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
    std::shared_ptr<AnimNodeData<ValidFrames>> _validFrames;


    QString SourceDirectory = "";
    QString SourceFilePath = "";
    QString SequencesFilePath = "";  // Path to Sequences.txt file (optional)
    bool bSequencesOneIndexed = true;  // Whether Sequences.txt uses 1-indexed frame numbers

    int sequenceCounter = 1;

    bool bDataValid;

    // Skeleton type configuration
    SkeletonType _skeletonType = SkeletonType::Bipedal;

    QWidget* widget;
    FolderSelectionWidget* _folderSelect = nullptr;
    FolderSelectionWidget* _sequencesFileSelect = nullptr;  // For Sequences.txt selection
    QComboBox* _skeletonTypeCombo = nullptr;
    QComboBox* _indexingCombo = nullptr;  // For 0/1 index toggle
    QPushButton* _pushButton;
    QLabel* _label;
    QHBoxLayout* _filePathLayout;

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

    bool isDataAvailable() override;

    void run() override;

    QWidget* embeddedWidget() override;

    QString category() override { return "Import"; }; 

private Q_SLOTS:
    //void onButtonClicked();
    void onFolderSelectionChanged();
    void onSkeletonTypeChanged(int index);
    void onSequencesFileChanged();
    void onIndexingChanged(int index);

private:
    void loadAnimationData(aiAnimation* pASSIMPAnimation, Skeleton* pSkeleton, Animation* pAnimation, aiNode* pNode);


    /**
     * This function creates a sub-skeleton from the root bone and the leave bones.
     * It also updates the loaded animation to match the new sub-skeleton.
     * Modifies the loaded skeleton and animation!
     *
     * @param pRootBone The name of the root bone of the sub-skeleton.
     * @param pLeaveBones A vector of names of the leave bones of the sub-skeleton.
     */
    void UseSubSkeleton(std::string pRootBone, std::vector<std::string> pLeaveBones);
    
    void importAssimpData();
    
    void selectDir();

    QStringList loadFilesFromDir();

    /**
     * @brief Parse a Sequences.txt file and populate the ValidFrames data product.
     *
     * Format: [unknown] [frame_number] [label] [filename_stem.bvh] [hash]
     * Example: 1 180 Standard D1_001_KAN01_001.bvh 331c4ff26421fd44898a5e67ddd07067
     */
    void parseSequencesFile();

    /**
     * @brief Extract the filename stem (without extension) from a full filename.
     * @param filename The filename with extension (e.g., "D1_001_KAN01_001.bvh")
     * @return The stem without extension (e.g., "D1_001_KAN01_001")
     */
    QString extractFileStem(const QString& filename) const;

};

#endif // EXAMPLEPLUGIN_H

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



#ifndef TOYALPHANODE_H
#define TOYALPHANODE_H

#include "ToyCollectionPlugin_global.h"
#include <QtWidgets>
#include <pluginnodeinterface.h>
#include <nodedatatypes.h>

class QPushButton;

class TOYCOLLECTIONPLUGINSHARED_EXPORT ToyAlphaNode : public PluginNodeInterface
{
    Q_OBJECT

private:
    QWidget* _widget;

private:

    std::weak_ptr<AnimNodeData<Animation>> _animationIn;
    std::shared_ptr<AnimNodeData<Animation>> _animationOut;

public:
    ToyAlphaNode(const QTimer& tick);
    ~ToyAlphaNode();

    std::unique_ptr<NodeDelegateModel> Init() override { return nullptr; };//  std::unique_ptr<ToyAlphaNode>(new ToyAlphaNode();


    QString caption() const override { return this->name(); }
    bool captionVisible() const override { return true; }

    static QString Name() { return QString("ToyAlphaNode"); }

    unsigned int nDataPorts(QtNodes::PortType portType) const override;
    NodeDataType dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<NodeData> processOutData(QtNodes::PortIndex port) override;

    void processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) override;

    bool isDataAvailable() override;

    void run() override;

    QWidget* embeddedWidget() override;

    //QTNodes
    QString category() override { return "Operator"; };  // Returns a category for the node

};

#endif // ANIMATIONFRAMESELECTORPLUGINPLUGIN_H

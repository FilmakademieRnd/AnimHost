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

 
#ifndef SOURCEDATANODE_H
#define SOURCEDATANODE_H

#define NODE_EDITOR_SHARED 1

#include <QtNodes/NodeDelegateModel>
#include "plugininterface.h"
#include "commondatatypes.h"
#include "nodedatatypes.h"
#include <vector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

class ANIMHOSTCORESHARED_EXPORT SourceDataNode : public NodeDelegateModel
{
    Q_OBJECT

public:
    SourceDataNode();


public:
    QString caption() const override { return QStringLiteral("Test Data Source"); }

    bool captionVisible() const override { return false; }

    QString name() const override { return "Test Data Source"; }

public:
    unsigned int nPorts(PortType portType) const override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

    std::shared_ptr<NodeData> outData(PortIndex port) override;

    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override {};

    QWidget *embeddedWidget() override { return nullptr; }


protected:
    std::vector<std::shared_ptr<NodeData>> _dataOut;

};

#endif // ANIMHOSTNODE_H

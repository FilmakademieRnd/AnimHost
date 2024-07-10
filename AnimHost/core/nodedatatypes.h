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

 
#ifndef NODEDATATYPES_H
#define NODEDATATYPES_H
#include "animhostcore_global.h"

#define NODE_EDITOR_SHARED 1
#include "QtNodes/NodeData"

#include "commondatatypes.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;


class ANIMHOSTCORESHARED_EXPORT AnimNodeDataBase : public NodeData
{
public:

    virtual QVariant getVariant() = 0;

    virtual void setVariant(QVariant variant) = 0;

protected:
    QVariant _variant;

};


//! Templated class making common data types available for using in Qt nodes and Qt reflection system
//! 
template <typename T> class AnimNodeData : public AnimNodeDataBase
{
public:

    AnimNodeData() { _data = std::make_shared<T>(); }

    NodeDataType type() const override { return staticType(); }

    static NodeDataType staticType() { return NodeDataType{ T::getId(), T::getName() }; }

    std::shared_ptr<T> getData() { return _data; }

    void setData(std::shared_ptr<T> data) { _data = data; }

    QVariant getVariant() override { return QVariant::fromValue(_data); }

    void setVariant(QVariant variant) override { 
        _variant = variant; 
        _data = _variant.value<std::shared_ptr<T>>();
    }

private:

    std::shared_ptr<T> _data;

};

#endif // NODEDATATYPES_H

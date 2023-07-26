#ifndef NODEDATATYPES_H
#define NODEDATATYPES_H


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



template <typename T> class ANIMHOSTCORESHARED_EXPORT AnimNodeData : public AnimNodeDataBase
{
public:

    AnimNodeData() { _data = std::make_shared<T>(); }

    NodeDataType type() const override { return staticType(); }

    static NodeDataType staticType() { return NodeDataType{ T::getId(), T::getName() }; }

    std::shared_ptr<T> getData() { return _data; }

    QVariant getVariant() override { return QVariant::fromValue(_data); }

    void setVariant(QVariant variant) override { 
        _variant = variant; 
        _data = _variant.value<std::shared_ptr<T>>();
    }

private:

    std::shared_ptr<T> _data;

};

#endif // NODEDATATYPES_H

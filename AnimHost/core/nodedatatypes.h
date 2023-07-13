#ifndef NODEDATATYPES_H
#define NODEDATATYPES_H


#define NODE_EDITOR_SHARED 1
#include "QtNodes/NodeData"
#include "commondatatypes.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

//Pose
class PoseNodeData : public NodeData
{
public:
    PoseNodeData()
        : _pose()
    {}

    PoseNodeData(Pose const pose)
        : _pose(pose)
    {}

    NodeDataType type() const override { return NodeDataType{"pose", "Pose"}; }

    Pose pose() const { return _pose; }

    //QString poseAsText() const { return QString::number(_number, 'f'); }

private:
    Pose _pose;
};

#endif // NODEDATATYPES_H

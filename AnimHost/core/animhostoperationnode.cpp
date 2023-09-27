#include "animhostoperationnode.h"

std::shared_ptr<NodeData> AnimHostOperationNode::outData(PortIndex index)
{
    //QVariant data = _plugin->outputs->at(index);

    if (index < _dataOut.size()) {
        return std::static_pointer_cast<NodeData>(_dataOut[index]);
    }

    else {
        throw "PortIndex out of range!";
    }
}

void AnimHostOperationNode::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    qDebug() << this->name() << "setInData() " << portIndex;

    if (!data) {
        for(int i=0; i< _dataOut.size(); i++)
            Q_EMIT dataInvalidated(i);

        _dataIn[portIndex] = data;
        
        return;
    }

    _dataIn[portIndex] = data;

    compute();
}

void AnimHostOperationNode::compute()
{
    QVariantList list;
    QVariantList listOut;

    foreach(std::weak_ptr<QtNodes::NodeData> var, _dataIn)
    {
        auto test = std::dynamic_pointer_cast<AnimNodeDataBase>(var.lock());
        if (!test)
            return;
        auto variant = test->getVariant();

        list.append(variant);
    }
    qDebug() << this->name() << "compute()";
    _plugin->run(list, listOut);

    int counter = 0;
    foreach(QVariant var, listOut)
    {
        auto nodeData = createAnimNodeDataFromID(var.metaType());
        nodeData->setVariant(var);
        _dataOut[counter] = nodeData;
    }

    for (int i = 1; i <= nPorts(PortType::Out); i++)
    {
        Q_EMIT dataUpdated(i - 1);
    }
}

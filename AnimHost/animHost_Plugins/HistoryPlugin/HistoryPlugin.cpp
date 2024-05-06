#include "HistoryPlugin.h"
#include <QtWidgets>

HistoryPlugin::HistoryPlugin()
{
    _lineEdit = nullptr;

    _poseHistory = new RingBuffer<Pose>(100);

    _outPoseSeq = std::make_shared<AnimNodeData<PoseSequence>>();

    qDebug() << "HistoryPlugin created";
}

HistoryPlugin::~HistoryPlugin()
{
    qDebug() << "~HistoryPlugin()";
}

unsigned int HistoryPlugin::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 1;
    else            
        return 1;
}

NodeDataType HistoryPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<PoseSequence>::staticType();
    else
        return AnimNodeData<PoseSequence>::staticType();
}

void HistoryPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{
    qDebug() << "HistoryPlugin setInData";

    if (!data) {
        switch (portIndex) {
        case 0:
            _inPoseSeq.reset();
            break;
        default:
            return;
        }
        return;
    }

    switch (portIndex) {
    case 0:
        _inPoseSeq = std::static_pointer_cast<AnimNodeData<PoseSequence>>(data);
        break;
    defaul:
        return;
    }
}

void HistoryPlugin::run()
{

    if (auto sp_inPoseSeq = _inPoseSeq.lock()) {
        auto poseSeq = sp_inPoseSeq->getData();

        // Indexing 1st element of posesequence. 
        // We assume sequence in realtime scenario only contains one frame, the current frame.

        Pose pose = poseSeq->mPoseSequence[0];

        _poseHistory->updateBuffer(pose);
    }

    auto pastFrames = _poseHistory->getPastFrames(numHistoryFrames);
    _outPoseSeq->getData()->mPoseSequence = pastFrames;

}

std::shared_ptr<NodeData> HistoryPlugin::processOutData(QtNodes::PortIndex port)
{
	return _outPoseSeq;
}

QWidget* HistoryPlugin::embeddedWidget()
{
	if (!widget) {
        _lineEdit = new QLineEdit();
        _lineEdit->setValidator(new QIntValidator(0, 100));
        auto lineSize= _lineEdit->sizeHint();


        _lineEdit->setMaximumSize(lineSize.width()/2, lineSize.height());

        _label = new QLabel("Past Frames");

        _hLayout = new QHBoxLayout();
        _hLayout->addWidget(_label);
        _hLayout->addWidget(_lineEdit);

        widget = new QWidget();
        widget->setLayout(_hLayout);    

        connect(_lineEdit, &QLineEdit::textChanged, this, &HistoryPlugin::onButtonClicked);
        _lineEdit->setText(QString::number(numHistoryFrames));

        widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
            "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
            "QLineEdit{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
            "QLabel{border: 1px; border-radius: 4px; padding: 5px;}"
        );
	}

	return widget;
}

void HistoryPlugin::onButtonClicked()
{
	qDebug() << "Example Widget Clicked";
}
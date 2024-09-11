#include "ControlPathDecoder.h"
#include <QPushButton>

ControlPathDecoderNode::ControlPathDecoderNode()
{
    _pushButton = nullptr;
    qDebug() << "ControlPathDecoderNode created";
}

ControlPathDecoderNode::~ControlPathDecoderNode()
{
    qDebug() << "~ControlPathDecoderNode()";
}

unsigned int ControlPathDecoderNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 1;
    else            
        return 1;
}

NodeDataType ControlPathDecoderNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return AnimNodeData<ParameterUpdate>::staticType();
    else
        return AnimNodeData<ControlPath>::staticType();
}

void ControlPathDecoderNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{

    if (!data) {
        Q_EMIT dataInvalidated(0);
        return;
    }

    qDebug() << "ControlPathDecoderNode setInData";

    _ParamIn = std::static_pointer_cast<AnimNodeData<ParameterUpdate>>(data);

    if (auto ParaInData = _ParamIn.lock()) {
        auto spParamIn = ParaInData->getData();

        if (spParamIn->objectID == _objectID && spParamIn->paramID == _paramControlPointID) {
            qDebug() << "Control Points received" << "ObjectID: " << spParamIn->objectID;

            //Decode Raw Data

            _recievedControlPathControlPoints = true;
        }
        else if (spParamIn->objectID == _objectID && spParamIn->paramID == _paramOrientationID) {
            qDebug() << "Control Orientation recieved" << "ObjectID: " << spParamIn->objectID;

            //Decode Raw Data

            _recievedControlPathOrientation = true;
        }
    }
    else
        qDebug() << "ControlPathDecoderNode run" << "No data";


    if (_recievedControlPathControlPoints && _recievedControlPathOrientation) {
        qDebug() << "Control path parameter received" << " ...start decoding";
        _recievedControlPathControlPoints = false;
        _recievedControlPathOrientation = false;

        run();
    }

    
}

std::shared_ptr<NodeData> ControlPathDecoderNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool ControlPathDecoderNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return true;
}

void ControlPathDecoderNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    qDebug() << "ControlPathDecoderNode run";
    if(isDataAvailable()){
        
        //Construct the path

    }
}



QWidget* ControlPathDecoderNode::embeddedWidget()
{
	if (!_widget) {

        _widget = new QWidget();
        _mainLayout = new QVBoxLayout(_widget);

        _comboBox = new QComboBox();

        _mainLayout->addWidget(_comboBox);



        _pushButton = new QPushButton("Run");
        _mainLayout->addWidget(_pushButton);

		connect(_pushButton, &QPushButton::released, this, &ControlPathDecoderNode::onButtonClicked);

        _widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
            "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
            "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
            "QLabel{padding: 5px;}"
            "QComboBox{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
            "QComboBox::drop-down{"
            "background-color:rgb(98, 139, 202);"
            "subcontrol-origin: padding;"
            "subcontrol-position: top right;"
            "width: 15px;"
            "border-top-right-radius: 4px;"
            "border-bottom-right-radius: 4px;}"
            "QComboBox QAbstractItemView{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-bottom-right-radius: 4px; border-bottom-left-radius: 4px; padding: 0px;}"
            "QScrollBar:vertical {"
            "border: 1px rgb(25, 25, 25);"
            "background:rgb(25, 25, 25);"
            "border-radius: 2px;"
            "width:6px;"
            "margin: 2px 0px 2px 1px;}"
            "QScrollBar::handle:vertical {"
            "border-radius: 2px;"
            "min-height: 0px;"
            "background-color: rgb(25, 25, 25);}"
            "QScrollBar::add-line:vertical {"
            "height: 0px;"
            "subcontrol-position: bottom;"
            "subcontrol-origin: margin;}"
            "QScrollBar::sub-line:vertical {"
            "height: 0px;"
            "subcontrol-position: top;"
            "subcontrol-origin: margin;}"
        );
	}

	return _widget;
}

void ControlPathDecoderNode::onButtonClicked()
{
    emitRunNextNode();
}
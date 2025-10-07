#include "RPCTriggerNode.h"
#include <QPushButton>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaMethod>



void reportMetaObject(const QMetaObject* metaObject) {
    if (!metaObject) {
        qDebug() << "Invalid QMetaObject!";
        return;
    }

    qDebug() << "Class Name:" << metaObject->className();

    // Properties
    qDebug() << "Properties:";
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);
        qDebug() << " -" << property.name() << "(" << property.typeName() << ")";
    }

    // Methods
    qDebug() << "Methods:";
    for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
        QMetaMethod method = metaObject->method(i);
        qDebug() << " -" << method.methodSignature() << "(" << method.methodType() << ")";
    }

    // Signals
    qDebug() << "Signals:";
    for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
        QMetaMethod method = metaObject->method(i);
        if (method.methodType() == QMetaMethod::Signal) {
            qDebug() << " -" << method.methodSignature();
        }
    }
}

RPCTriggerNode::RPCTriggerNode(const NodeDelegateModelRegistry& modelRegistry)
{
    _pushButton = nullptr;

    auto modelCreators = modelRegistry.registeredModelCreators();

	//print all registered models
    for (auto it = modelCreators.begin(); it != modelCreators.end(); ++it)
    {
        qWarning() << "RPC Plugin Loaded: " << it->first;


        // add property names to the list
        auto metaObject = it->second.metaObject;

        if (metaObject->propertyCount() - metaObject->propertyOffset() > 0) {

			qDebug() << "Number of properties: " << metaObject->propertyCount();


            _nodeElements.append(qMakePair(it->first, QStringList()));

            // add property names to the list
            for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
                QMetaProperty property = metaObject->property(i);
                _nodeElements.last().second.append(property.name());
            }
        }
    }
    
    //qDebug() << "RPCTriggerNode created";
}

RPCTriggerNode::~RPCTriggerNode()
{
    qDebug() << "~RPCTriggerNode()";
}

unsigned int RPCTriggerNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 2;
    else            
        return 0;
}

NodeDataType RPCTriggerNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In) {
		if (portIndex == 0) {
			return AnimNodeData<RPCUpdate>::staticType();
		}
		else {
			return AnimNodeData<CharacterObject>::staticType();
		}
	}

    return type;
}

void RPCTriggerNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{

    if (!data) {
        Q_EMIT dataInvalidated(0);
    }

    if (portIndex == 0) {
        _RPCIn = std::static_pointer_cast<AnimNodeData<RPCUpdate>>(data);
    }
    else {
		_characterIn = std::static_pointer_cast<AnimNodeData<CharacterObject>>(data);
    }

 
    run();

    qDebug() << "RPCTriggerNode setInData";
}

std::shared_ptr<NodeData> RPCTriggerNode::processOutData(QtNodes::PortIndex port)
{
	return nullptr;
}

bool RPCTriggerNode::isDataAvailable() {
    /*
    * Use this function to check if the inbound data is available and can be processed.
    */
    return true;
}

void RPCTriggerNode::run()
{
    /*
    * Run the main node logic here. run() is called through the incoming run signal of another node.
    * run() can also be called through another signal, like a button press or in our case a timer.
    * But it is recommended to keep user interaction to a minimum. 
    */
    //qDebug() << "RPCTriggerNode run";
    if(isDataAvailable()){
        
        if(auto RPCData = _RPCIn.lock()){
			if (auto CharacterInData = _characterIn.lock()) {
			    //qDebug() << "RPCData Received";

                auto sp_rpc = RPCData->getData();

                auto spCharacter = CharacterInData->getData();


			    // Check if RPC is matching the active character and the "Animation Generation" RPC PArameter ID
                if (sp_rpc->objectID == spCharacter->sceneObjectID) {

				    // Check for the specific animation request
                    if (sp_rpc->paramID == 5 && sp_rpc->paramType == ZMQMessageHandler::INT) {
                        QByteArray data = sp_rpc->rawData;
                        uint32_t rpc;
                        //convert data to int
                        std::memcpy(&rpc, data.data(), sizeof(uint32_t));

                        _filterType = AnimHostRPCType(rpc);

                        if (_filterType <= AnimHostRPCType::BLOCK) {
                            qDebug() << "RPCData Received: " << rpc;

                            // create QVaraintMap for filtertype and _rpcMappings
                            QVariantMap parameter;
                            parameter["sendingMode"] = QVariant::fromValue(_filterType);

                            for (auto mapping : _rpcMappings) {
                                parameter[mapping.targetProperty] = mapping.value;
                            }

                            emitRunNextNode(&parameter);
                        }
                    }
				}
                else if (sp_rpc->sceneID == 255 && sp_rpc->objectID == 1) {
                    // Check for possible mapping of RPC to other properties
                    for (auto& mapping : _rpcMappings) {
                        if (mapping.parameterID == sp_rpc->paramID) {

                            auto data = sp_rpc->decodeRawData();

                            if (sp_rpc->paramType == ZMQMessageHandler::INT) {
                                auto intData = dynamic_cast<ParameterPayload<int>*>(data.get());
                                mapping.value = intData->getValue();
                            }
                            else if (sp_rpc->paramType == ZMQMessageHandler::FLOAT) {
                                auto floatData = dynamic_cast<ParameterPayload<float>*>(data.get());
                                mapping.value = floatData->getValue();
                            }
                            else if (sp_rpc->paramType == ZMQMessageHandler::VECTOR3) {
                                //auto vec3Data = dynamic_cast<ParameterPayload<glm::vec3>*>(data.get());
                                qWarning() << "Vector 3 parmaeter override currently not supported";
                            }
                            else if (sp_rpc->paramType == ZMQMessageHandler::VECTOR4) {
                                //auto vec4Data = dynamic_cast<ParameterPayload<glm::vec4>*>(data.get());
                                qWarning() << "Vector 4 parmaeter override currently not supported";
                            }
                            else if (sp_rpc->paramType == ZMQMessageHandler::QUATERNION) {
                                //auto quatData = dynamic_cast<ParameterPayload<glm::quat>*>(data.get());
                                qWarning() << "Quaternion parmaeter override currently not supported";
                            }
                            else {
                                qDebug() << "Unsupported parameter type in RPC!";
                            }
                        }
                    }
                }
				else {
						qWarning() << "Invalid RPC Target";
				}
            }
		}
		else{
			qDebug() << "RPCData not Received";
		}
    }
}



QWidget* RPCTriggerNode::embeddedWidget()
{
	if (!_widget) {

        _widget = new QWidget();
        _mainLayout = new QVBoxLayout(_widget);

        _comboBox = new QComboBox();

        _comboBox->addItem("STOP", QVariant::fromValue(AnimHostRPCType::STOP));
        _comboBox->addItem("STREAM", QVariant::fromValue(AnimHostRPCType::STREAM));
        _comboBox->addItem("STREAM_LOOP", QVariant::fromValue(AnimHostRPCType::STREAM_LOOP));
        _comboBox->addItem("BLOCK", QVariant::fromValue(AnimHostRPCType::BLOCK));
        _comboBox->setCurrentIndex(3);

        _mainLayout->addWidget(_comboBox);

        connect(_comboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
            _filterType = _comboBox->currentData().value<AnimHostRPCType>();
            });

        _pushButton = new QPushButton("Run");
        _mainLayout->addWidget(_pushButton);

		connect(_pushButton, &QPushButton::released, this, &RPCTriggerNode::onButtonClicked);

		_listWidget = new DynamicListWidget(_widget);
		_mainLayout->addWidget(_listWidget);

        connect(_listWidget, &DynamicListWidget::elementAdded, this, &RPCTriggerNode::onElementAdded);
		connect(_listWidget, &DynamicListWidget::elementRemoved, this, &RPCTriggerNode::onElementRemoved);
		connect(_listWidget, &DynamicListWidget::ItemMappingChanged, this, &RPCTriggerNode::onElementMappingChanged);


		_listWidget->SetModifyableElements(_nodeElements);

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

void RPCTriggerNode::onElementAdded()
{
	qDebug() << "ListWidget Changed";


	_rpcMappings.append({ 0, "", "", false });


	Q_EMIT embeddedWidgetSizeUpdated();
}

void RPCTriggerNode::onElementRemoved(int index)
{
	qDebug() << "ListWidget Changed";

	_rpcMappings.removeAt(index);
	
}

void RPCTriggerNode::onElementMappingChanged(int elementIdx,int paramId, QString node, QString property, int trigger)
{
	qDebug() << "Mapping Changed: " << paramId << " " << node << " " << property << " " << trigger;

	if (elementIdx >= _rpcMappings.size()) {
		qDebug() << "Invalid Element Index";
		return;
	}
	_rpcMappings[elementIdx].parameterID = paramId;
	_rpcMappings[elementIdx].targetNode = node;
	_rpcMappings[elementIdx].targetProperty = property;
	_rpcMappings[elementIdx].triggerRun = trigger;
}

void RPCTriggerNode::onButtonClicked()
{
    // Create Meta Variant Map and fill with the current set AnimHostRPCType

	QVariantMap rpcMap;

	rpcMap["sendingMode"] = QVariant::fromValue(_filterType);

    emitRunNextNode(&rpcMap);
}
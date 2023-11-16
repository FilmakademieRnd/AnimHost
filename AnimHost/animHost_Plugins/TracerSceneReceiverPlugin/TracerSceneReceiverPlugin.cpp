
#include "TracerSceneReceiverPlugin.h"
#include "SceneReceiver.h"

TracerSceneReceiverPlugin::TracerSceneReceiverPlugin() {
    _pushButton = nullptr;
    widget = nullptr;
	_connectIPAddress = nullptr;
	_ipAddressLayout = nullptr;
	_ipAddress = "127.0.0.1";

	// Validation Regex initialization for the QLineEdit Widget of the plugin
	_ipValidator = new QRegularExpressionValidator(ZMQMessageHandler::ipRegex, this);

	characterList = new CharacterPackageSequence();

	_updateSenderContext = new zmq::context_t(1);
	zeroMQSceneReceiverThread = new QThread();
	sceneReceiver = new SceneReceiver(this, _ipAddress, false, _updateSenderContext);
	QObject::connect(sceneReceiver, &SceneReceiver::sceneReceived, this, &TracerSceneReceiverPlugin::onSceneReceived);

	sceneReceiver->moveToThread(zeroMQSceneReceiverThread);
	QObject::connect(sceneReceiver, &SceneReceiver::passCharacterByteArray, this, &TracerSceneReceiverPlugin::processCharacterByteData);
	QObject::connect(this, &TracerSceneReceiverPlugin::connectSceneReceiver, sceneReceiver, &SceneReceiver::sendRequest);

	qDebug() << "TracerSceneReceiverPlugin created";
}

TracerSceneReceiverPlugin::~TracerSceneReceiverPlugin() {
    qDebug() << "~TracerSceneReceiverPlugin()";
}

unsigned int TracerSceneReceiverPlugin::nDataPorts(QtNodes::PortType portType) const {
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 1 ;
}

NodeDataType TracerSceneReceiverPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
        return AnimNodeData<CharacterPackageSequence>::staticType();
}

void TracerSceneReceiverPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) {
    qDebug() << "TracerSceneReceiverPlugin setInData";
}

std::shared_ptr<NodeData> TracerSceneReceiverPlugin::processOutData(QtNodes::PortIndex port) {
	return nullptr;
}

QWidget* TracerSceneReceiverPlugin::embeddedWidget() {
	if (!_pushButton) {
		_pushButton = new QPushButton("Connect");
		_connectIPAddress = new QLineEdit();

		_connectIPAddress->setText(_ipAddress);
		_connectIPAddress->displayText();
		_connectIPAddress->setValidator(_ipValidator);

		_pushButton->resize(QSize(30, 30));
		_ipAddressLayout = new QHBoxLayout();

		_ipAddressLayout->addWidget(_connectIPAddress);
		_ipAddressLayout->addWidget(_pushButton);

		_ipAddressLayout->setSizeConstraint(QLayout::SetMinimumSize);

		widget = new QWidget();

		widget->setLayout(_ipAddressLayout);
		connect(_pushButton, &QPushButton::released, this, &TracerSceneReceiverPlugin::onButtonClicked);
	}

	widget->setStyleSheet("QHeaderView::section {background-color:rgba(64, 64, 64, 0%);""border: 0px solid white;""}"
						  "QWidget{background-color:rgba(64, 64, 64, 0%);""color: white;}"
						  "QPushButton{border: 1px solid white; border-radius: 4px; padding: 5px; background-color:rgb(98, 139, 202);}"
						  "QLabel{background-color:rgb(25, 25, 25); border: 1px; border-color: rgb(60, 60, 60); border-radius: 4px; padding: 5px;}"
	);

	return widget;
}

void TracerSceneReceiverPlugin::onButtonClicked()
{
	//! Set IP Address
	_ipAddress = _connectIPAddress->text();
	//sceneReceiver->setIPAddress(_ipAddress);
	
	//! Send signal to SceneReceiver to request characters
	connectSceneReceiver(_ipAddress, "characters");

	//qDebug() << "Attempting RECEIVE connection to" << _ipAddress;
}

void TracerSceneReceiverPlugin::onSceneReceived(QByteArray* sceneMessage) {
	// TODO: unpack scene description message
	while (!sceneMessage->isEmpty()) {
		// process message payload unit header
		
	}

	qDebug() << "Parsing scene description";
}

void TracerSceneReceiverPlugin::run() {
	qDebug() << "TracerSceneReceiverPlugin running..." ;

	sceneReceiver->requestStart();
	zeroMQSceneReceiverThread->start();
}

//!
//! Gets a QByteArray from the SceneReceiver thread.
//! Parses the bytes populating a CharacterPackageSequence
//! 
void TracerSceneReceiverPlugin::processCharacterByteData(QByteArray* characterByteArray) {
	int byteCounter = 0; //! "Bookmark" for reading the array and skipping uninteresting sequences of bytes

	//! Execute until all the QByteArray has been read
	while (characterByteArray->size() > byteCounter + 4) {
		//! Parsing and populating data for a single CharacterPackage
		CharacterPackage character;
		
		//! Get number of bones (not to be saved in CharacterPackage) - int32
		int32_t nBones; memcpy(&nBones, characterByteArray->sliced(byteCounter, 4).data(), sizeof(nBones)); // Copies byte values directly into the new variable, which interprets it as the correct type
		byteCounter += 4;
		//! Get number of skeleton objects (not to be saved in CharacterPackage) - int32
		int32_t nSkeletonObjs; memcpy(&nSkeletonObjs, characterByteArray->sliced(byteCounter, 4), sizeof(nSkeletonObjs));
		byteCounter += 4;

		//! Get sceneID
		character.sceneID = _ipAddress.back().digitValue();

		//! Get objectID - int32
		memcpy(&character.objectID, characterByteArray->sliced(byteCounter, 4), sizeof(character.objectID));
		byteCounter += 4;

		//! TODO: get object name
		//character.objectName = ?

		//! Populating bone IDs - int32[]
		for (int i = 0; i < nBones; i++) {
			int32_t id; memcpy(&id, characterByteArray->sliced(byteCounter, 4), sizeof(id));
			character.boneIDs.push_back(id);
			byteCounter += 4;
		}

		//! Skipping skeletonMapping
		for (int i = 0; i < nSkeletonObjs; i++) {
			int32_t id; memcpy(&id, characterByteArray->sliced(byteCounter, 4), sizeof(id));
			character.skeletonObjIDs.push_back(id);
			byteCounter += 4;
		}

		//! Populating T-Pose bone Positions - float[] - size in bytes 4*3*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(byteCounter, 4), sizeof(x));
			byteCounter += 4;
			float y; memcpy(&y, characterByteArray->sliced(byteCounter, 4), sizeof(y));
			byteCounter += 4;
			float z; memcpy(&z, characterByteArray->sliced(byteCounter, 4), sizeof(z));
			byteCounter += 4;
			character.tposeBonePos.push_back(glm::vec3(x, y, z));
		}

		//! Populating T-Pose bone Rotation - float[] - size in bytes 4*4*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(byteCounter, 4), sizeof(x));
			byteCounter += 4;
			float y; memcpy(&y, characterByteArray->sliced(byteCounter, 4), sizeof(y));
			byteCounter += 4;
			float z; memcpy(&z, characterByteArray->sliced(byteCounter, 4), sizeof(z));
			byteCounter += 4;
			float w; memcpy(&w, characterByteArray->sliced(byteCounter, 4), sizeof(w));
			byteCounter += 4;
			character.tposeBoneRot.push_back(glm::quat(w, x, y, z));
		}

		//! Populating T-Pose bone Scale - float[] - size in bytes 4*3*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(byteCounter, 4), sizeof(x));
			byteCounter += 4;
			float y; memcpy(&y, characterByteArray->sliced(byteCounter, 4), sizeof(y));
			byteCounter += 4;
			float z; memcpy(&z, characterByteArray->sliced(byteCounter, 4), sizeof(z));
			byteCounter += 4;
			character.tposeBoneScale.push_back(glm::vec3(x, y, z));
		}
		
		//! Adding character to the characterList
		characterList->mCharacterPackageSequence.push_back(character);
	}
}
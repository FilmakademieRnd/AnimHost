
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

	characterListOut = std::make_shared<AnimNodeData<CharacterObjectSequence>>();

	_sceneReceiverContext = new zmq::context_t(1);
	zeroMQSceneReceiverThread = new QThread();
	sceneReceiver = new SceneReceiver(this, _ipAddress, false, _sceneReceiverContext);
	sceneReceiver->moveToThread(zeroMQSceneReceiverThread);
	QObject::connect(sceneReceiver, &SceneReceiver::passCharacterByteArray, this, &TracerSceneReceiverPlugin::processCharacterByteData);
	QObject::connect(sceneReceiver, &SceneReceiver::passSceneNodeByteArray, this, &TracerSceneReceiverPlugin::processSceneNodeByteData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestCharacterData, sceneReceiver, &SceneReceiver::requestSceneCharacterData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestSceneNodeData, sceneReceiver, &SceneReceiver::requestSceneNodeData);

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
        return AnimNodeData<CharacterObjectSequence>::staticType();
}

void TracerSceneReceiverPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) {
    qDebug() << "TracerSceneReceiverPlugin setInData";
}

std::shared_ptr<NodeData> TracerSceneReceiverPlugin::processOutData(QtNodes::PortIndex port) {
	return characterListOut;
}

QWidget* TracerSceneReceiverPlugin::embeddedWidget() {
	if (!_pushButton) {
		_pushButton = new QPushButton("Refresh");
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
	sceneReceiver->connectSocket(_ipAddress);

	//! Send signal to SceneReceiver to request characters
	requestSceneNodeData();

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

	emitRunNextNode();
}

//!
//! Gets a QByteArray from the SceneReceiver thread.
//! Parses the bytes populating a CharacterObjectSequence
//! 
void TracerSceneReceiverPlugin::processCharacterByteData(QByteArray* characterByteArray) {
	int charByteCounter = 0; //! "Bookmark" for reading the character byte array and skipping uninteresting sequences of bytes
	
	//! Clear current list of CharacterPackages
	characterListOut.get()->getData()->mCharacterObjectSequence.clear();
	//! Execute until all the QByteArray has been read
	while (characterByteArray->size() > charByteCounter) {
		//! Parsing and populating data for a single CharacterPackage
		CharacterObject character;
		
		//! Get number of bones (not to be saved in CharacterPackage) - int32
		int32_t nBones; memcpy(&nBones, characterByteArray->sliced(charByteCounter, 4).data(), sizeof(nBones)); // Copies byte values directly into the new variable, which interprets it as the correct type
		charByteCounter += 4;
		//! Get number of skeleton objects (not to be saved in CharacterPackage) - int32
		int32_t nSkeletonObjs; memcpy(&nSkeletonObjs, characterByteArray->sliced(charByteCounter, 4), sizeof(nSkeletonObjs));
		charByteCounter += 4;

		//! Get sceneID
		character.sceneID = _ipAddress.back().digitValue();

		//! Get ID of the Scene Object as seen by Unity - int32
		memcpy(&character.sceneObjectID, characterByteArray->sliced(charByteCounter, 4), sizeof(character.sceneObjectID));
		charByteCounter += 4;
		
		//! Get ID of the root bone - int32
		memcpy(&character.rootBoneID, characterByteArray->sliced(charByteCounter, 4), sizeof(character.rootBoneID));
		charByteCounter += 4;

		//! Populating bone mapping (which Unity HumanBone object corresponds to which element of the skeleton obj vector) - int32[]
		for (int i = 0; i < nBones; i++) {
			int32_t id; memcpy(&id, characterByteArray->sliced(charByteCounter, 4), sizeof(id));
			character.boneMapping.push_back(id);
			charByteCounter += 4;
		}

		//! Populating skeleton object IDs (ParameterIDs of the actual bones as seen by Unity) - int32[]
		for (int i = 0; i < nSkeletonObjs; i++) {
			int32_t id; memcpy(&id, characterByteArray->sliced(charByteCounter, 4), sizeof(id));
			character.skeletonObjIDs.push_back(id);
			charByteCounter += 4;
		}

		//! Populating T-Pose bone Positions - float[] - size in bytes 4*3*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(charByteCounter, 4), sizeof(x));
			charByteCounter += 4;
			float y; memcpy(&y, characterByteArray->sliced(charByteCounter, 4), sizeof(y));
			charByteCounter += 4;
			float z; memcpy(&z, characterByteArray->sliced(charByteCounter, 4), sizeof(z));
			charByteCounter += 4;
			character.tposeBonePos.push_back(glm::vec3(x, y, z));
		}

		//! Populating T-Pose bone Rotation - float[] - size in bytes 4*4*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(charByteCounter, 4), sizeof(x));
			charByteCounter += 4;
			float y; memcpy(&y, characterByteArray->sliced(charByteCounter, 4), sizeof(y));
			charByteCounter += 4;
			float z; memcpy(&z, characterByteArray->sliced(charByteCounter, 4), sizeof(z));
			charByteCounter += 4;
			float w; memcpy(&w, characterByteArray->sliced(charByteCounter, 4), sizeof(w));
			charByteCounter += 4;
			character.tposeBoneRot.push_back(glm::quat(w, x, y, z));
		}

		//! Populating T-Pose bone Scale - float[] - size in bytes 4*3*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(charByteCounter, 4), sizeof(x));
			charByteCounter += 4;
			float y; memcpy(&y, characterByteArray->sliced(charByteCounter, 4), sizeof(y));
			charByteCounter += 4;
			float z; memcpy(&z, characterByteArray->sliced(charByteCounter, 4), sizeof(z));
			charByteCounter += 4;
			character.tposeBoneScale.push_back(glm::vec3(x, y, z));
		}

		//! Get object name
		character.objectName = QString(characterByteArray->sliced(charByteCounter, 256)).toStdString(); // the object name has a fixed 256 bytes length
		charByteCounter += 256;
		
		//! Adding character to the characterList
		characterListOut->getData()->mCharacterObjectSequence.push_back(character);
	}
	qDebug() << "Number of character received from VPET:" << characterListOut.get()->getData()->mCharacterObjectSequence.size();

	emitDataUpdate(0);
}

//!
//! Gets a QByteArray from the SceneReceiver thread.
//! Parses the bytes populating a SceneNodeSequence
//! 
void TracerSceneReceiverPlugin::processSceneNodeByteData(QByteArray* sceneNodeByteArray) {
	int nodeByteCounter = 0;  //! "Bookmark" for reading the scene node byte array and skipping uninteresting sequences of bytes
	int sceneNodeCounter = 0; //! Counting the sceneNodes that are editable in order to retrieve the sceneObjectID used for the ParameterUpdateMessage
	CharacterObjectSequence complementaryCharList;

	while (sceneNodeByteArray->size() > nodeByteCounter) {
		CharacterObject character;

		int32_t sceneNodeType; memcpy(&sceneNodeType, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(sceneNodeType)).data(), sizeof(sceneNodeType)); // Copies byte values directly into the new variable, which interprets it as the correct type
		nodeByteCounter += sizeof(sceneNodeType);

		//! Parsing base fields (shared between all nodes)
		// Components: 1*bool + 1*int + 3*float + 3*float + 4*float + 64*byte
		// Size:            4 +     4 +      12 +      12 +      16 +      64 = 112
		// Fields
		// - bool editable
		bool editableFlag; memcpy(&editableFlag, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(editableFlag)).data(), sizeof(editableFlag)); // Copies byte values directly into the new variable, which interprets it as the correct type
		nodeByteCounter += sizeof(editableFlag);
		sceneNodeCounter += editableFlag; //! Incrementing sceneNodeCounter if editableFlag = TRUE
		// - int  childCount
		nodeByteCounter += 4;
		// - vec3 position
		//glm::vec3 objPos; memcpy(&objPos, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(objPos)).data(), sizeof(objPos));
		nodeByteCounter += 12;
		// - vec3 scale
		//glm::vec3 objScale; memcpy(&objScale, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(objPos)).data(), sizeof(objScale));
		nodeByteCounter += 12;
		// - vec4 rotation
		//glm::vec4 objRot; memcpy(&objRot, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(objPos)).data(), sizeof(objRot));
		nodeByteCounter += 16;
		// - char[] (fixed 64 char length)
		std::string nodeName = QString(sceneNodeByteArray->sliced(nodeByteCounter, 64)).toStdString(); // Save name of the scene object (string)
		nodeByteCounter += 64;

		switch (sceneNodeType) {
			case SKINNEDMESH:
				character.sceneObjectID = sceneNodeCounter;
				character.objectName = nodeName;

				//! extract information from Skinned Mesh Scene Node
				// Components: GEO.size + int + int + 3*float + 3*float + 99*16*float + 99*int
				// Size:             24 +   4 +   4 +      12 +      12 +        6336 +    396 = 6900
				
				// Skip GEO fields (24 bytes)
				nodeByteCounter += 24;
				// Skip bindPoseLength (int) and rootBoneID (int)
				nodeByteCounter += 8;
				// Skip ID of the root bone (int)
				int32_t rootID; memcpy(&rootID, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(rootID)).data(), sizeof(rootID)); // Copies byte values directly into the new variable, which interprets it as the correct type
				nodeByteCounter += sizeof(rootID);
				character.rootBoneID = rootID;
				// Skip boundExtents and boundCenter (2*3 floats)
				nodeByteCounter += 24;
				// Skip bind poses (99*16 floats) - for now...
				nodeByteCounter += 6336;
				// Read SkinnedMeshObj-to-Bone Mapping, it's going to be used to map parameterIDs to bone names
				for (int i = 0; i < 99; i++) {
					int32_t boneID; memcpy(&boneID, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(boneID)).data(), sizeof(boneID)); // Copies byte values directly into the new variable, which interprets it as the correct type
					nodeByteCounter += sizeof(boneID);
					if (boneID > 0) // add read ID to the boneMapping array only if initialised (non-zero value)
						character.boneMapping.push_back(boneID);
				}

				//! Adding character to the characterList
				characterListOut->getData()->mCharacterObjectSequence.push_back(character);
				break;
			case GEO:
				//! skip Scene Node
				// Components: GROUP.size + 1*int + 1*int + 4*float
				// Size:              112 +     4 +     4 +      16 = 136
				// Fields
				// - int geoID
				// - int  materialID
				// - vec4 color
				nodeByteCounter += 136;
				break;
			case LIGHT:
				//! skip Scene Node
				// Components: GROUP.size + 1*enum + 3*float + 3*float
				// Size:              112 +      4 +      12 +      12 = 140
				// Fields
				// - LightType lightType
				// - float intensity
				// - float angle
				// - float range
				// - vec4 color
				nodeByteCounter += 140;
				break;
			case CAMERA:
				//! skip Scene Node
				// Components: GROUP.size + 6*float
				// Size:              112 +      24 = 136
				// Fields
				// - float fov
				// - float aspect
				// - float near
				// - float far
				// - float focalDist
				// - float aperture
				nodeByteCounter += 136;
				break;
			default:
				break;
		}
	}
}
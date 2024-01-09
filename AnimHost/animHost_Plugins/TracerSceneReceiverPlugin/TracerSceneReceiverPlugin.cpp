
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
	QObject::connect(sceneReceiver, &SceneReceiver::passHeaderByteArray, this, &TracerSceneReceiverPlugin::processHeaderByteData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestCharacterData, sceneReceiver, &SceneReceiver::requestCharacterData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestSceneNodeData, sceneReceiver, &SceneReceiver::requestSceneNodeData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestHeaderData, sceneReceiver, &SceneReceiver::requestHeaderData);

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

	// TODO: Reconnecting the socket requires a correct shut-down process before creating a new connection. To be refactored.
	sceneReceiver->connectSocket(_ipAddress); // DO NOT COMMENT THIS LINE

	//! Send signal to SceneReceiver to request characters
	//requestHeaderData();
	requestCharacterData();
	//! requestSceneNodeData() is then called after processCharacterData() is done (i.e. at thew end of its body)

	//qDebug() << "Attempting RECEIVE connection to" << _ipAddress;
}

void TracerSceneReceiverPlugin::run() {
	qDebug() << "TracerSceneReceiverPlugin running..." ;

	sceneReceiver->requestStart();
	zeroMQSceneReceiverThread->start();

	emitRunNextNode();
}

//!
//! Gets a QByteArray representing the list of characters in the scene from the SceneReceiver thread.
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
		
		//! Get ID of the root bone - int32
		memcpy(&character.characterRootId, characterByteArray->sliced(charByteCounter, 4), sizeof(character.characterRootId));
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
		
		//! Adding character to the characterList
		characterListOut->getData()->mCharacterObjectSequence.push_back(character);
	}
	qDebug() << "Number of character received from VPET:" << characterListOut.get()->getData()->mCharacterObjectSequence.size();

	// Do not emit Update because not ALL of the data has been processed
	//emitDataUpdate(0);
	
	//! Requesting the complete Scenegraph in order to "fill in the blanks" of the CharacterObject
	requestSceneNodeData();
}

//!
//! Gets a QByteArray from the SceneReceiver thread.
//! Parses the bytes populating a SceneNodeSequence
//! 
void TracerSceneReceiverPlugin::processSceneNodeByteData(QByteArray* sceneNodeByteArray) {
	int nodeByteCounter = 0;  //! "Bookmark" for reading the scene node byte array and skipping uninteresting sequences of bytes
	int sceneNodeCounter = 0; //! Counting the sceneNodes in order to calculate the objectID
	int editableSceneNodeCounter = 0; //! Counting the sceneNodes that are editable in order to retrieve the sceneObjectID used for the ParameterUpdateMessage
	//SceneNodeObjectSequence nodeList; // To be populated with all the nodes in the received scene. To be exposed to every other AnimHost plugin

	int characterCounter = 0;
	CharacterObject currentChar;

	while (sceneNodeByteArray->size() > nodeByteCounter) {
		int32_t sceneNodeType; memcpy(&sceneNodeType, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(sceneNodeType)).data(), sizeof(sceneNodeType)); // Copies byte values directly into the new variable, which interprets it as the correct type
		nodeByteCounter += sizeof(sceneNodeType);

		sceneNodeCounter++;

		//! Parsing base fields (shared between all nodes)
		// Components: 1*bool + 1*int + 3*float + 3*float + 4*float + 64*byte
		// Size:            4 +     4 +      12 +      12 +      16 +      64 = 112
		// Fields
		// - bool editable
		bool editableFlag; memcpy(&editableFlag, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(editableFlag)).data(), sizeof(editableFlag)); // Copies byte values directly into the new variable, which interprets it as the correct type
		nodeByteCounter += 4;
		editableSceneNodeCounter += editableFlag; //! Incrementing activeSceneNodeCounter if editableFlag = TRUE
		// - int  childCount
		//int32_t childCount; memcpy(&childCount, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(childCount)).data(), sizeof(childCount)); // Copies byte values directly into the new variable, which interprets it as the correct type
		nodeByteCounter += 4;
		// - vec3 position
		glm::vec3 objPos; memcpy(&objPos, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(objPos)).data(), sizeof(objPos));
		nodeByteCounter += sizeof(objPos);
		// - vec3 scale
		glm::vec3 objScale; memcpy(&objScale, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(objPos)).data(), sizeof(objScale));
		nodeByteCounter += sizeof(objScale);
		// - vec4 rotation
		glm::vec4 objRot; memcpy(&objRot, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(objPos)).data(), sizeof(objRot));
		nodeByteCounter += sizeof(objRot);
		// - char[] (fixed 64 char length)
		std::string nodeName = QString(sceneNodeByteArray->sliced(nodeByteCounter, 64)).toStdString(); // Save name of the scene object (string)
		nodeByteCounter += 64;

		// if the current node has the same ID as the characterRootID of the currently selected character in the characterListOut
		// fill its sceneObjectID and objectName fields
		if (sceneNodeCounter == characterListOut->getData()->mCharacterObjectSequence.at(characterCounter).characterRootID) {
			currentChar = characterListOut->getData()->mCharacterObjectSequence.at(characterCounter);

			currentChar.sceneObjectID = editableSceneNodeCounter;
			currentChar.objectName = nodeName;

			currentChar.pos = objPos;
			currentChar.rot = objRot;
			currentChar.scl = objScale;

			characterCounter++;
		}

		
		switch (sceneNodeType) {
			case SKINNEDMESH:
				// Save the objectID of the SceneNodeSkinnedGeo that holds that information (needed for treacability)
				currentChar.sceneNodeSkinnedGeoIDs.push_back(sceneNodeCounter);
				
				//! extract information from Skinned Mesh Scene Node
				// Components: GEO.size + int + int + 3*float + 3*float + 99*16*float + 99*int
				// Size:             24 +   4 +   4 +      12 +      12 +        6336 +    396 = 6900
				
				// Skip GEO fields (24 bytes)
				nodeByteCounter += 24;
				// Save bindPoseLength (int) for later
				int32_t bindPoseLength; memcpy(&bindPoseLength, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(bindPoseLength)).data(), sizeof(bindPoseLength)); // Copies byte values directly into the new variable, which interprets it as the correct type
				nodeByteCounter += sizeof(bindPoseLength);
				// Save ID of the root bone (int)
				int32_t rootBoneID; memcpy(&rootBoneID, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(rootBoneID)).data(), sizeof(rootBoneID)); // Copies byte values directly into the new variable, which interprets it as the correct type
				nodeByteCounter += sizeof(rootBoneID);
				currentChar.rootBoneID = rootBoneID;
				// Save bounding box dimensions (3 floats)
				glm::vec3 boundExtents; memcpy(&boundExtents, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(boundExtents)).data(), sizeof(boundExtents));
				nodeByteCounter += sizeof(boundExtents);
				currentChar.boundExtents = boundExtents;
				// Save bounding box center (3 floats)
				glm::vec3 boundCenter; memcpy(&boundCenter, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(boundCenter)).data(), sizeof(boundCenter));
				nodeByteCounter += sizeof(boundCenter);
				currentChar.boundExtents = boundCenter;
				// Save bind poses (99*16 floats)
				for (int i = 0; i < 99; i++) {
					if (i >= bindPoseLength)
						break;

					glm::mat4 bindPose; memcpy(&bindPose, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(bindPose)).data(), sizeof(bindPose)); // Copies byte values directly into the new variable, which interprets it as the correct type
					nodeByteCounter += sizeof(bindPose);
					currentChar.bindPoses.push_back(bindPose);
				}
				// Read SkinnedMeshObj-to-Bone Mapping, it's going to be used to map parameterIDs to bone names
				std::vector<int> smbIDs;
				for (int i = 0; i < 99; i++) {
					int32_t boneID; memcpy(&boneID, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(boneID)).data(), sizeof(boneID)); // Copies byte values directly into the new variable, which interprets it as the correct type
					nodeByteCounter += sizeof(boneID);
					if (boneID == -1)
						break;

					smbIDs.push_back(boneID);
				}
				// Adding this mapping to the list of possible mappings of the character
				currentChar.skinnedMeshBoneIDs.push_back(smbIDs);

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
				// TODO: Create and populate SceneNodeObject
				// TODO: Add latest SceneNodeObject to the global SceneNodeSequence/SceneDescription (which should be accessible throughout AnimHost)
				break;
		}
	}
	qDebug() << "Number of character received from VPET:" << characterListOut.get()->getData()->mCharacterObjectSequence.size();

	emitDataUpdate(0);
}

// TODO: Implement processHeaderByteData(QByteArray* headerByteArray) {}
void TracerSceneReceiverPlugin::processHeaderByteData(QByteArray* headerByteArray) {
	// - float lightIntensityFactor
	// - byte  senderID
	unsigned char senderID; memcpy(&senderID, headerByteArray->sliced(4, sizeof(senderID)).data(), sizeof(senderID)); // Copies byte values directly into the new variable, which interprets it as the correct type
	ZMQMessageHandler::setTargetHostID(senderID);
}
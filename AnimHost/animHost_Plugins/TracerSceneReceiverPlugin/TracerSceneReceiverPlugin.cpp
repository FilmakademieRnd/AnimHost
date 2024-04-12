
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
	sceneNodeListOut = std::make_shared<AnimNodeData<SceneNodeObjectSequence>>();
	controlPathOut = std::make_shared<AnimNodeData<ControlPath>>();

	_sceneReceiverContext = new zmq::context_t(1);
	zeroMQSceneReceiverThread = new QThread();
	sceneReceiver = new SceneReceiver(this, _ipAddress, false, _sceneReceiverContext);
	sceneReceiver->moveToThread(zeroMQSceneReceiverThread);
	QObject::connect(sceneReceiver, &SceneReceiver::passCharacterByteArray, this, &TracerSceneReceiverPlugin::processCharacterByteData);
	QObject::connect(sceneReceiver, &SceneReceiver::passSceneNodeByteArray, this, &TracerSceneReceiverPlugin::processSceneNodeByteData);
	QObject::connect(sceneReceiver, &SceneReceiver::passHeaderByteArray, this, &TracerSceneReceiverPlugin::processHeaderByteData);
	QObject::connect(sceneReceiver, &SceneReceiver::passControlPathByteArray, this, &TracerSceneReceiverPlugin::processControlPathByteData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestCharacterData, sceneReceiver, &SceneReceiver::requestCharacterData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestSceneNodeData, sceneReceiver, &SceneReceiver::requestSceneNodeData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestHeaderData, sceneReceiver, &SceneReceiver::requestHeaderData);
	QObject::connect(this, &TracerSceneReceiverPlugin::requestControlPathData, sceneReceiver, &SceneReceiver::requestControlPathData);

	qDebug() << "TracerSceneReceiverPlugin created";
}

TracerSceneReceiverPlugin::~TracerSceneReceiverPlugin() {
    qDebug() << "~TracerSceneReceiverPlugin()";
}

unsigned int TracerSceneReceiverPlugin::nDataPorts(QtNodes::PortType portType) const {
    if (portType == QtNodes::PortType::In)
        return 0;
    else            
        return 3 ;
}

NodeDataType TracerSceneReceiverPlugin::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
    NodeDataType type;
    if (portType == QtNodes::PortType::In)
        return type;
    else
		if (portIndex == 0)
			return AnimNodeData<CharacterObjectSequence>::staticType();
		else if (portIndex == 1)
			return AnimNodeData<SceneNodeObjectSequence>::staticType();
		else if (portIndex == 2)
			return AnimNodeData<ControlPath>::staticType();
		else
			return type;
}

void TracerSceneReceiverPlugin::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex) {
    qDebug() << "TracerSceneReceiverPlugin setInData";
}

std::shared_ptr<NodeData> TracerSceneReceiverPlugin::processOutData(QtNodes::PortIndex port) {
	if (port == 0)
		return characterListOut;
	else if (port == 1)
		return sceneNodeListOut;
	
}

QWidget* TracerSceneReceiverPlugin::embeddedWidget() {
	if (!_pushButton) {
		_pushButton = new QPushButton("Request Data");
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
	// Set IP Address
	_ipAddress = _connectIPAddress->text();

	// TODO: Reconnecting the socket requires a correct shut-down process before creating a new connection. To be refactored.
	sceneReceiver->connectSocket(_ipAddress); // DO NOT COMMENT THIS LINE

	// Send signal to SceneReceiver to request characters
	requestHeaderData();
	requestCharacterData();
	// requestSceneNodeData() is then called after processCharacterData() is done (i.e. at thew end of its body)
	requestControlPathData();

	//qDebug() << "Attempting RECEIVE connection to" << _ipAddress;
}

void TracerSceneReceiverPlugin::run() {
	qDebug() << "TracerSceneReceiverPlugin running..." ;

	sceneReceiver->requestStart();
	zeroMQSceneReceiverThread->start();

	emitRunNextNode();
}

/* 
 * Gets a QByteArray representing the list of characters in the scene from the SceneReceiver thread.
 * Parses the bytes populating a CharacterObjectSequence
 */
void TracerSceneReceiverPlugin::processCharacterByteData(QByteArray* characterByteArray) {
	int charByteCounter = 0; //! "Bookmark" for reading the character byte array and skipping uninteresting sequences of bytes
	
	// Clear current list of CharacterPackages
	characterListOut.get()->getData()->mCharacterObjectSequence.clear();
	// Execute until all the QByteArray has been read
	while (characterByteArray->size() > charByteCounter) {
		// Parsing and populating data for a single CharacterPackage
		CharacterObject character;
		
		// Get number of bones (not to be saved in CharacterPackage) - int32
		int32_t nBones; memcpy(&nBones, characterByteArray->sliced(charByteCounter, 4).data(), sizeof(nBones)); // Copies byte values directly into the new variable, which interprets it as the correct type
		charByteCounter += sizeof(nBones);
		// Get number of skeleton objects (not to be saved in CharacterPackage) - int32
		int32_t nSkeletonObjs; memcpy(&nSkeletonObjs, characterByteArray->sliced(charByteCounter, 4), sizeof(nSkeletonObjs));
		charByteCounter += sizeof(nSkeletonObjs);
		
		// Get ID of the root bone - int32
		memcpy(&character.characterRootID, characterByteArray->sliced(charByteCounter, 4), sizeof(character.characterRootID));
		charByteCounter += sizeof(character.characterRootID);

		// Populating bone mapping (which Unity HumanBone object corresponds to which element of the skeleton obj vector) - int32[]
		for (int i = 0; i < nBones; i++) {
			int32_t id; memcpy(&id, characterByteArray->sliced(charByteCounter, 4), sizeof(id));
			charByteCounter += sizeof(id);
			character.boneMapping.push_back(id);
		}

		// Populating skeleton object IDs (ParameterIDs of the actual bones as seen by Unity) - int32[]
		for (int i = 0; i < nSkeletonObjs; i++) {
			int32_t id; memcpy(&id, characterByteArray->sliced(charByteCounter, 4), sizeof(id));
			charByteCounter += sizeof(id);
			character.skeletonObjIDs.push_back(id);
		}

		// Populating T-Pose bone Positions - float[] - size in bytes 4*3*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(charByteCounter, 4), sizeof(x));
			charByteCounter += sizeof(x);
			float y; memcpy(&y, characterByteArray->sliced(charByteCounter, 4), sizeof(y));
			charByteCounter += sizeof(y);
			float z; memcpy(&z, characterByteArray->sliced(charByteCounter, 4), sizeof(z));
			charByteCounter += sizeof(z);
			character.tposeBonePos.push_back(glm::vec3(x, y, z));
		}

		// Populating T-Pose bone Rotation - float[] - size in bytes 4*4*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(charByteCounter, 4), sizeof(x));
			charByteCounter += sizeof(x);
			float y; memcpy(&y, characterByteArray->sliced(charByteCounter, 4), sizeof(y));
			charByteCounter += sizeof(y);
			float z; memcpy(&z, characterByteArray->sliced(charByteCounter, 4), sizeof(z));
			charByteCounter += sizeof(z);
			float w; memcpy(&w, characterByteArray->sliced(charByteCounter, 4), sizeof(w));
			charByteCounter += sizeof(w);
			character.tposeBoneRot.push_back(glm::quat(w, x, y, z));
		}

		// Populating T-Pose bone Scale - float[] - size in bytes 4*3*N_bones
		for (int i = 0; i < nSkeletonObjs; i++) {
			float x; memcpy(&x, characterByteArray->sliced(charByteCounter, 4), sizeof(x));
			charByteCounter += sizeof(x);
			float y; memcpy(&y, characterByteArray->sliced(charByteCounter, 4), sizeof(y));
			charByteCounter += sizeof(y);
			float z; memcpy(&z, characterByteArray->sliced(charByteCounter, 4), sizeof(z));
			charByteCounter += sizeof(z);
			character.tposeBoneScale.push_back(glm::vec3(x, y, z));
		}
		
		// Adding character to the characterList
		characterListOut->getData()->mCharacterObjectSequence.push_back(character);
	}
	qDebug() << "Number of character received from VPET:" << characterListOut.get()->getData()->mCharacterObjectSequence.size();

	// Do not emit Update because not ALL of the data has been processed
	//emitDataUpdate(0);
	
	// Requesting the complete Scenegraph in order to "fill in the blanks" of the CharacterObject
	requestSceneNodeData();
}

/*
* Gets a QByteArray from the SceneReceiver thread.
* Parses the bytes populating a SceneNodeSequence
*/
void TracerSceneReceiverPlugin::processSceneNodeByteData(QByteArray* sceneNodeByteArray) {
	int nodeByteCounter = 0;  // "Bookmark" for reading the scene node byte array and skipping uninteresting sequences of bytes
	int sceneNodeCounter = 0; // Counting the sceneNodes in order to calculate the objectID
	int editableSceneNodeCounter = 0; // Counting the sceneNodes that are editable in order to retrieve the sceneObjectID used for the ParameterUpdateMessage

	int characterCounter = 0;
	CharacterObject* currentChar = new CharacterObject();

	while (sceneNodeByteArray->size() > nodeByteCounter) {
		int32_t sceneNodeType; memcpy(&sceneNodeType, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(sceneNodeType)).data(), sizeof(sceneNodeType)); // Copies byte values directly into the new variable, which interprets it as the correct type
		nodeByteCounter += sizeof(sceneNodeType);

		// Parsing base fields (shared between all nodes)
		// Components: 1*bool + 1*int + 3*float + 3*float + 4*float + 64*byte
		// Size:            4 +     4 +      12 +      12 +      16 +      64 = 112
		// Fields
		// - bool editable
		bool editableFlag; memcpy(&editableFlag, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(editableFlag)).data(), sizeof(editableFlag)); // Copies byte values directly into the new variable, which interprets it as the correct type
		nodeByteCounter += 4;
		editableSceneNodeCounter += editableFlag; // Incrementing activeSceneNodeCounter if editableFlag = TRUE
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
		QString nodeName = QString(sceneNodeByteArray->sliced(nodeByteCounter, 64)); // Save name of the scene object (string)
		nodeName.remove(QChar('\0'));
		nodeByteCounter += 64;
		nodeName.shrink_to_fit();

		SceneNodeObject sceneNode;
		SkinnedMeshComponent skinnedMesh;
		int i = 0;
		switch (sceneNodeType) {
			case CHARACTER:
				// if the current character has the same ID as the characterRootID of the currently selected character in the characterListOut
				// ...and the characterCounter is smaller than the size of the list of characters (to avoid OutOfBounds error)
				// fill its sceneObjectID and objectName fields
				if (characterCounter < characterListOut->getData()->mCharacterObjectSequence.size() &&
					sceneNodeCounter == characterListOut->getData()->mCharacterObjectSequence.at(characterCounter).characterRootID) {
					currentChar = &characterListOut->getData()->mCharacterObjectSequence.at(characterCounter);

					currentChar->sceneObjectID = editableSceneNodeCounter;
					currentChar->characterRootID = characterListOut->getData()->mCharacterObjectSequence.at(characterCounter).characterRootID;
					currentChar->objectName = nodeName.toStdString();

					currentChar->pos = objPos;
					currentChar->rot = objRot;
					currentChar->scl = objScale;

					characterCounter++;
				}

				// Adding the characterObject also to the list of all the scene nodes in the scene to retain the same structure wrt the original scene
				sceneNodeListOut.get()->getData()->mSceneNodeObjectSequence.push_back(*currentChar);

				break;
			case SKINNEDMESH:
				// Read the data encapsulated in the SceneNodeSkinnedGeo and save it in a SkinnedMeshRenderer object
				
				skinnedMesh.id = sceneNodeCounter;
				
				// extract information from Skinned Mesh Scene Node
				// Components: GEO.size + int + int + 3*float + 3*float + 99*16*float + 99*int
				// Size:             24 +   4 +   4 +      12 +      12 +        6336 +    396 = 6900
				
				// Skip GEO fields (24 bytes)
				nodeByteCounter += 24;
				// Save bindPoseLength (int) for later
				int32_t bindPoseLength; memcpy(&bindPoseLength, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(bindPoseLength)).data(), sizeof(bindPoseLength)); // Copies byte values directly into the new variable, which interprets it as the correct type
				nodeByteCounter += sizeof(bindPoseLength);
				// Save skeletonRootID (int) for later
				int32_t skeletonRootID; memcpy(&skeletonRootID, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(skeletonRootID)).data(), sizeof(skeletonRootID)); // Copies byte values directly into the new variable, which interprets it as the correct type
				nodeByteCounter += sizeof(skeletonRootID);
				// Save bounding box dimensions (3 floats)
				glm::vec3 boundExtents; memcpy(&boundExtents, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(boundExtents)).data(), sizeof(boundExtents));
				nodeByteCounter += sizeof(boundExtents);
				skinnedMesh.boundExtents = boundExtents;
				// Save bounding box center (3 floats)
				glm::vec3 boundCenter; memcpy(&boundCenter, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(boundCenter)).data(), sizeof(boundCenter));
				nodeByteCounter += sizeof(boundCenter);
				skinnedMesh.boundCenter = boundCenter;
				 
				// Save bind poses (bindPoseLength * 16) but 99*16 floats have to be skipped in the end anyway
				while (i < 99 && i < bindPoseLength) { // i starts from 0, initialized at line 261
					glm::mat4 bindPose; memcpy(&bindPose, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(bindPose)).data(), sizeof(bindPose)); // Copies byte values directly into the new variable, which interprets it as the correct type
					skinnedMesh.bindPoses.push_back(bindPose);
					nodeByteCounter += sizeof(bindPose); i++;
				}
				nodeByteCounter += ((99 - i) * 16 * 4); // skipping unused bytes
				i = 0; // resetting i for next loop
				int32_t boneID;
				// Read SkinnedMeshObj-to-Bone Mapping, it's going to be used to get parameterIDs for the parameter update messages
				while (i < 99) {
					memcpy(&boneID, sceneNodeByteArray->sliced(nodeByteCounter, sizeof(boneID)).data(), sizeof(boneID)); // Copies byte values directly into the new variable, which interprets it as the correct type
					// if boneID reads -1 then ignore it and abort the loop
					if (boneID == -1)
						break;
					// otherwise, save the value, and increment the counters
					skinnedMesh.boneMapIDs.push_back(boneID);
					nodeByteCounter += sizeof(boneID); i++;
				}
				// skip the remaining unused bytes
				nodeByteCounter += ((99 - i) * 4);

				// Double-check that the characterRootID of the skinnedMesh coincides with the one already present in the current character
				// This proves that this skinnedMesh belogs to the current character
				assert(skeletonRootID == currentChar->characterRootID);
				// Add the filled SkinnedMeshRenderer object to the currently active CharacterObject
				currentChar->skinnedMeshList.push_back(skinnedMesh);

				// Adding a barebone placeholder SceneNodeObject in order to retain the order of the received scene node list
				sceneNode.sceneObjectID = -1;
				sceneNode.characterRootID = currentChar->characterRootID;
				sceneNode.objectName = nodeName.toStdString();
				sceneNode.pos = objPos;
				sceneNode.rot = objRot;
				sceneNode.scl = objScale;

				sceneNodeListOut.get()->getData()->mSceneNodeObjectSequence.push_back(sceneNode);

				break;
			case GEO:
				// skip Geometry Node fields
				// Components: GROUP.size + 1*int + 1*int + 4*float
				// Size:              112 +     4 +     4 +      16 = 136
				// Fields
				// - int geoID
				// - int  materialID
				// - vec4 color
				nodeByteCounter += 136;
				break;
			case LIGHT:
				// skip Light Node fields
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
				// skip Camera Node fields
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
			case GROUP:
				// Create and populate SceneNodeObject
				sceneNode.sceneObjectID = (editableFlag? editableSceneNodeCounter : -1); // If this node is editable assign sceneObjectID, otherwise assign -1 (non-editable)
				sceneNode.characterRootID = -1; // This should/could be changed so that a descendent bone can have a "reference" to the character root
				sceneNode.objectName = nodeName.toStdString();
				sceneNode.pos = objPos;
				sceneNode.rot = objRot;
				sceneNode.scl = objScale;

				// TODO: Add latest SceneNodeObject to the global SceneNodeSequence/SceneDescription (which should be accessible throughout AnimHost)
				sceneNodeListOut.get()->getData()->mSceneNodeObjectSequence.push_back(sceneNode);

				break;
			default:
				break;
		}

		// After processing the current SceneNode increment the counter
		sceneNodeCounter++;
	}
	qDebug() << "Number of scene nodes received from VPET:" << sceneNodeListOut.get()->getData()->mSceneNodeObjectSequence.size();
	qDebug() << "Number of character received from VPET:" << characterListOut.get()->getData()->mCharacterObjectSequence.size();

	emitDataUpdate(0);
}

void TracerSceneReceiverPlugin::processHeaderByteData(QByteArray* headerByteArray) {
	// - float	lightIntensityFactor (skipped)
	// - byte	senderID
	// - byte	framerate
	unsigned char senderID; memcpy(&senderID, headerByteArray->sliced(4, sizeof(senderID)).data(), sizeof(senderID)); // Copies byte values directly into the new variable, which interprets it as the correct type
	ZMQMessageHandler::setTargetSceneID(senderID);
	
	unsigned char framerate; memcpy(&framerate, headerByteArray->sliced(5, sizeof(framerate)).data(), sizeof(framerate)); // Copies byte values directly into the new variable, which interprets it as the correct type
	// Set framerate only if a valid value (>0) is received
	if (framerate > 0)
		ZMQMessageHandler::setPlaybackFrameRate(framerate);
}

void TracerSceneReceiverPlugin::processControlPathByteData(QByteArray* controlPointByteArray) {
	if (controlPointByteArray->size() == 0)
		return;

	controlPathOut->getData()->mControlPath.clear();	// clearing controlPathOut every time a new path is (requested and) received

	// - int	size of data (3 * N_of_frames)
	// - float	data (frame1_xyz, frame2_xyz, frame3_xyz, ..., frameN_xyz)
	unsigned int size; memcpy(&size, controlPointByteArray->sliced(0, sizeof(size)).data(), sizeof(size)); // Copies byte values directly into the new variable, which interprets it as the correct type
	controlPointByteArray->remove(0, sizeof(size)); // removing first 4 bytes (aka first int) so that reading keyframes can start from 0

	for (int frame = 0; frame < size/3; frame++) {
		ControlPoint key;
		float x, y, z;
		int currentXPos = (sizeof(x) + sizeof(y) + sizeof(z)) * frame;
		int currentYPos = currentXPos + sizeof(x);
		int currentZPos = currentYPos + sizeof(y);
		memcpy(&x, controlPointByteArray->sliced(currentXPos, sizeof(x)), sizeof(x));
		memcpy(&y, controlPointByteArray->sliced(currentYPos, sizeof(y)), sizeof(y));
		memcpy(&z, controlPointByteArray->sliced(currentZPos, sizeof(z)), sizeof(z));
		key.frameTimestamp = frame;
		key.position = glm::vec3(x, y, z);
		qDebug() << "Frame" << frame << "- Pos(" << key.position.x << "," << key.position.y << "," << key.position.z << ")";

		int currentXDir = (sizeof(x) + sizeof(y) + sizeof(z)) * frame;
		int currentYDir = currentXDir + sizeof(x);
		int currentZDir = currentYDir + sizeof(y);
		memcpy(&x, controlPointByteArray->sliced(currentXDir, sizeof(x)), sizeof(x));
		memcpy(&y, controlPointByteArray->sliced(currentYDir, sizeof(y)), sizeof(y));
		memcpy(&z, controlPointByteArray->sliced(currentZDir, sizeof(z)), sizeof(z));
		key.lookAt = glm::quat(glm::vec3(x, y, z), glm::vec3(0, 0, 1));
		qDebug() << "Frame" << frame << "- Dir(" << key.lookAt.w << "," << key.lookAt.x << "," << key.lookAt.y << "," << key.lookAt.z << ")";
		
		controlPathOut->getData()->mControlPath.push_back(key);
	}

	qDebug() << "Control Path with" << size/3 << "frames received!";
}
#include "ControlPathDecoder.h"
#include <QPushButton>
#include "animhosthelper.h"

ControlPathDecoderNode::ControlPathDecoderNode()
{
    _pushButton = nullptr;

    _OutControlPath = std::make_shared<AnimNodeData<ControlPath>>();
    qDebug() << "ControlPathDecoderNode created";
}

ControlPathDecoderNode::~ControlPathDecoderNode()
{
    qDebug() << "~ControlPathDecoderNode()";
}

unsigned int ControlPathDecoderNode::nDataPorts(QtNodes::PortType portType) const
{
    if (portType == QtNodes::PortType::In)
        return 2;
    else            
        return 1;
}

NodeDataType ControlPathDecoderNode::dataPortType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    NodeDataType type;
    if (portType == QtNodes::PortType::In) {
        if (portIndex == 0) {
            return AnimNodeData<ParameterUpdate>::staticType();
        } else {
            return AnimNodeData<CharacterObject>::staticType();
        }
    }
    else {
        return AnimNodeData<ControlPath>::staticType();
    }
}

void ControlPathDecoderNode::processInData(std::shared_ptr<NodeData> data, QtNodes::PortIndex portIndex)
{

    if (!data) {
        Q_EMIT dataInvalidated(0);
        return;
    }

    // If a new charcter is selected, update the _objectID and the _controlPathID used for filtering the other Parameter Updates
    if (portIndex == 1) {
       _characterIn  = std::static_pointer_cast<AnimNodeData<CharacterObject>>(data);

       if (auto spCharacterIn = _characterIn.lock()) {
           auto characterInData = spCharacterIn->getData();
           _characterID = characterInData->sceneObjectID;
           _controlPathID = characterInData->controlPathID;

           // The ID of the control path parameter object of a character is its last parameter => ID = nTotalParam where nTotalParam = 3 (pos+rot+scl) + 2 * nBones (boneLocation+boneRotation)
           _paramControlPath = 3 + (2 * characterInData->boneMapping.size());
       }

    } else if (portIndex == 0) {
        _ParamIn = std::static_pointer_cast<AnimNodeData<ParameterUpdate>>(data);

        if (auto spParamIn = _ParamIn.lock()) {
            auto paramInData = spParamIn->getData();

            if (paramInData->objectID == _controlPathID && paramInData->paramID == _paramPointLocationID) {
                qDebug() << "Control Point Locations received." << "ObjectID: " << paramInData->objectID;

                // Decode Raw Data
                std::unique_ptr<AbstractParameterPayload> paramPayload = paramInData->decodeRawData();

                // Convert into concrete Parameter of 3D Vectors
                auto pointLocationParam = dynamic_cast<ParameterPayload<glm::vec3>*>(paramPayload.release());
                this->_pointLocation = pointLocationParam->getKeyList();
                // Flip the values on the y-axis - DOESN'T WORK
                /*for (int i = 0; i < this->_pointLocation.size(); i++) {
                    this->_pointLocation.at(i).value.z *= -1;
                    this->_pointLocation.at(i).inTangentValue.z *= -1;
                    this->_pointLocation.at(i).outTangentValue.z *= -1;
                }*/

                _receivedControlPathPointLocation = true;

            } else if (paramInData->objectID == _controlPathID && paramInData->paramID == _paramPointRotationID) {
                qDebug() << "Control Point Orientations recieved. " << "ObjectID: " << paramInData->objectID;

                // Decode Raw Data
                std::unique_ptr<AbstractParameterPayload> paramPayload = paramInData->decodeRawData();

                // Convert into concrete Parameter of Quaternions
                auto pointRotationParam = dynamic_cast<ParameterPayload<glm::quat>*>(paramPayload.release());
                this->_pointRotation = pointRotationParam->getKeyList();
                _receivedControlPathPointRotation = true;

            } else if (paramInData->objectID == _characterID && paramInData->paramType == ZMQMessageHandler::ParameterType::INT && paramInData->paramID == _paramControlPath) {
                // When receiving an update for the Control Path associated with the selected character, update _controlPathID
                qDebug() << "New Control Point ID received. " << "ObjectID: " << paramInData->objectID << "ParamID: " << paramInData->paramID;

                // Decode Raw Data
                std::unique_ptr<AbstractParameterPayload> paramPayload = paramInData->decodeRawData();
                // Cast to concrete type ParameterPayload<int>
                auto newParamControlPath = dynamic_cast<ParameterPayload<int>*>(paramPayload.release());
                // Update current control path ID 
                _controlPathID = newParamControlPath->getValue();
            }
        }
        else {
            qDebug() << "ControlPathDecoderNode run" << "No data";
        }
           


        if (_receivedControlPathPointLocation && _receivedControlPathPointRotation) {
            qDebug() << "Control path parameter received" << " ...start decoding";
            _receivedControlPathPointLocation = false;
            _receivedControlPathPointRotation = false;

            run();
        }
    }
    
}

std::shared_ptr<NodeData> ControlPathDecoderNode::processOutData(QtNodes::PortIndex port)
{   
    if (auto spCharacterIn = _characterIn.lock()) {
        // return pointer to the updated control Path

        _OutControlPath->setData(spCharacterIn->getData()->controlPath);
        return _OutControlPath;
        
    } else {
        return nullptr;
    }
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
    auto spCharacterIn = _characterIn.lock();
    if(spCharacterIn && isDataAvailable()){

        std::vector<ControlPoint> path = {};
        //path->initializePath();

        //Construct the path
        for (int i = 0; i < this->_pointLocation.size() - 1; i++) {
            qDebug() << "Processing Control Point" << i;
            glm::vec3   thisLoc     = this->_pointLocation.at(i).value;
            glm::vec3   outTang     = this->_pointLocation.at(i).outTangentValue;
            int         firstFrame  = this->_pointLocation.at(i).time;
            int         easeOut     = this->_pointLocation.at(i).outTangentTime;

            glm::vec3   nextLoc     = this->_pointLocation.at(i + 1).value;
            glm::vec3   inTang      = this->_pointLocation.at(i + 1).inTangentValue;
            int         lastFrame   = this->_pointLocation.at(i + 1).time;
            int         easeIn      = this->_pointLocation.at(i + 1).inTangentTime;

            glm::quat thisRot = this->_pointRotation.at(i).value;
            glm::quat nextRot = this->_pointRotation.at(i + 1).value;
            
            std::vector<ControlPoint>* sampledSegment = adaptiveSegmentSampling(thisLoc, outTang, inTang, nextLoc, easeOut, easeIn, thisRot, nextRot, firstFrame, lastFrame);

            // Grow the control path associated with the selected character. This will be passed as control signal onto the GNN
            path.insert(path.end(), sampledSegment->begin(), sampledSegment->end());
            
            // For every segnment which is not the last one,
            // remove its last frame because the first element of the next segment will duplicate it
            if (path.size() > 0 && i < this->_pointLocation.size() - 2)
                path.pop_back();
        }

        // TODO: If path is cyclic, evaluate last-to-first segment


        for (int i = 0; i < path.size(); i++) {
            // Transform the control points to the global coordinate system

            path[i].position = AnimHostHelper::GetCoordinateSystemTransformationMatrix() * glm::vec4(path[i].position, 1.0f);

            // Rotate the control points to the global coordinate system

			glm::vec3 lookAt = path[i].lookAt * glm::vec3(0, -1.f,0);
		
            lookAt = AnimHostHelper::GetCoordinateSystemTransformationMatrix() * glm::vec4(lookAt, 0.0f);

            path[i].lookAt = glm::rotation(glm::vec3(0, 0, 1), lookAt);

			//path[i].lookAt = glm::toQuat(AnimHostHelper::GetCoordinateSystemTransformationMatrix()) * path[i].lookAt;

		}

        spCharacterIn->getData()->setPath(path);
        emitDataUpdate(0);
        emitRunNextNode();
    }
}

/*********************************
***** BEZIÉR SPLINE SAMPLING *****
******** HELPER FUNCTIONS ********
*********************************/

std::vector<ControlPoint>* ControlPathDecoderNode::adaptiveSegmentSampling(glm::vec3        knot1,      glm::vec3   handle1,    glm::vec3   handle2,    glm::vec3   knot2,
                                                                           float            easeFrom,   float       easeTo,
                                                                           glm::quat        quat1,      glm::quat   quat2,
                                                                           int              frame1,     int         frame2,
                                                                           ControlPoint*    lastCP) {
    int nSamples = frame2 - frame1 + 1;
    std::vector<ControlPoint>* sampledSegment = new std::vector<ControlPoint>();
    std::vector<float> timings = adaptiveTimingsResampling(easeFrom/100, easeTo/100, nSamples);
    
    // Assuming that size of timings == nSamples
    for (int i = 0; i < nSamples; i++) {
        glm::vec3 sampledPos = sampleBezier(knot1, handle1, handle2, knot2, timings[i]);
        glm::quat sampledRot = glm::slerp(quat1, quat2, timings[i]);
        //float vel = glm::length(sampledSegment->end()->position - sampledPos) / (1.f / 60.f);

        // build Control Point 
        ControlPoint sampledPoint = ControlPoint(sampledPos, sampledRot, frame1 + i, 0.f);

        sampledSegment->push_back(sampledPoint);
    }

    return sampledSegment;
}

// Looking for y-value given a specific x-value on the oversampled Beziér Spline
// We need to obtain samples at regular intervals on the x-axis to represent the time compression/dialation expressed by the easeFrom-easeTo values
// The pre-sampling step is needed to obtain the shape of the Beziér curve representing the time easing function
// The re-sampled values will be used as timings for sampling the (2D) Beziér Spline that represents the selected control path
std::vector<float> ControlPathDecoderNode::adaptiveTimingsResampling(float easeFrom, float easeTo, int nSamples) {
    // Good results with oversampling by a factor of 10 but no interpolation or with interpolation but no oversampling
    // At the moment, doing both :)
    std::vector<float> timings(nSamples);
    std::vector<glm::vec3> preSampling(10* nSamples);

    for (int i = 0; i < preSampling.size(); i++) {
        float oversamplingT = i / (10.0f * nSamples);
        preSampling[i] = ControlPathDecoderNode::sampleBezier(glm::vec3(0, 0, 0), glm::vec3(easeFrom, 0, 0), glm::vec3(1 - easeTo, 1, 0), glm::vec3(1, 1, 0), oversamplingT);
    }

    for (int i = 0; i < nSamples; i++) {
        float t = (float) i / nSamples;
        int j = 0;

        // Finding the pre-sample with the largest x-value lower than t
        while (preSampling[j].x <= t)
            j++;
        
        float t1 = (t - preSampling[j - 1].x) / (preSampling[j].x - preSampling[j - 1].x);  // Finding the interpolating value of t wrt the j-th and j-1-th sample (i.e. the interpolating weight)
        float easedT = t1 * preSampling[j].y + (1 - t1) * preSampling[j - 1].y;             // Getting the eased t value by interpolating between the the y-values of the j-th and j-1-th, given t1 (the computed interpolating weight)

        timings[i] = easedT;
    }

    return timings;
}

// Implementation of the textbook Beziér interpolation function
glm::vec3 ControlPathDecoderNode::sampleBezier(glm::vec3 knot1, glm::vec3 handle1, glm::vec3 handle2, glm::vec3 knot2, float t) {
    glm::vec3 sample =  (                         glm::pow((1 - t), 3.0f) *   knot1 ) +
                        ( 3 *          t        * glm::pow((1 - t), 2.0f) * handle1 ) +
                        ( 3 * glm::pow(t, 2.0f) *          (1 - t)        * handle2 ) +
                        (     glm::pow(t, 3.0f)                           *   knot2 );
    return sample;
}

/********************************/

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
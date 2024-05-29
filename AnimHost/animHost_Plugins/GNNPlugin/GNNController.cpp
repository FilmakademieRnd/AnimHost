#include "GNNController.h"
#include <FileHandler.h>
#include <FrameRange.h>
#include <MathUtils.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <animhosthelper.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

GNNController::GNNController(QString networkPath) : NetworkModelPath(networkPath)
{
	network = std::make_unique<OnnxModel>();	
	network->LoadOnnxModel(NetworkModelPath);
    debugSignal = std::make_shared<DebugSignal>();
}

void GNNController::prepareInput()
{
	genJointPos.clear();
	genJointRot.clear();
	genJointVel.clear();

    qDebug() << "Generate Animation with Control Path of size: " << controlPath->mControlPath.size();


    ctrlTrajPos.clear();
    ctrlTrajForward.clear();

    for (auto& p : controlPath->mControlPath) {
        ctrlTrajPos.push_back(glm::vec2( p.position.x, p.position.z)*100.f);
        ctrlTrajForward.push_back(p.lookAt);
	}


	glm::vec2 updatedRootPos = glm::vec2(0.0, 0.0);
	glm::quat updatedRootRot = glm::quat(1.0, 0.0, 0.0, 0.0);

	TrajectoryFrameData inferredTrajectoryFrame;



	for (int genIdx = 1; genIdx < ctrlTrajPos.size(); genIdx++) {

		std::vector<glm::vec2> inTrajPos;
		std::vector<glm::vec2> inTrajDir;
		std::vector<glm::vec2> inTrajVel;
		std::vector<float> inTrajSpeed;

		TrajectoryFrameData inTrajFrame;

		//get current root
		
		glm::vec2 currentRootPos;
		glm::quat currentRootRot;

		currentRootPos = ctrlTrajPos[genIdx];
		currentRootRot = ctrlTrajForward[genIdx];

		/*if(genIdx == 1){
			currentRootPos = ctrlTrajPos[genIdx];
			currentRootRot = ctrlTrajForward[genIdx];
		}
		else{
			currentRootPos = glm::mix(ctrlTrajPos[genIdx],updatedRootPos,0.5f);
			currentRootRot = glm::mix(ctrlTrajForward[genIdx], updatedRootRot, 0.5f);
		}*/

		glm::mat4 trans = glm::translate(glm::mat4(1.0), glm::vec3(currentRootPos.x, 0.0, currentRootPos.y));
		glm::mat4 root = trans * glm::toMat4(currentRootRot);

		//glm::vec3 forward{ 0.0,0.0,1.0 };


		inTrajFrame = BuildTrajectoryFrameData(ctrlTrajPos, ctrlTrajForward, inferredTrajectoryFrame, genIdx, root);

		JointsFrameData inJointFrame;

		//prepare character input
		if (genJointPos.size() <= 0) {
            inJointFrame.jointPos = initJointPos;
            inJointFrame.jointRot = initJointRot;
            inJointFrame.jointVel = initJointVel;
		}
		else {
			//set inJointFrame
			inJointFrame.jointPos = genJointPos.back();
			inJointFrame.jointRot = genJointRot.back();
			inJointFrame.jointVel = genJointVel.back();

		}


		BuildInputTensor(inTrajFrame,
			inJointFrame);

		//Inference
		auto inferenceOutputValues = network->RunInference(input_values);

		std::vector<std::vector<glm::vec2>> outPhase2D;
		std::vector<std::vector<float>> outAmplitude;
		std::vector<std::vector<float>> outFrequency;


		TrajectoryFrameData outTrajFrame;
		JointsFrameData outJointFrame;

		glm::vec3 deltaOut = readOutput(inferenceOutputValues, outTrajFrame, outJointFrame ,outPhase2D, outAmplitude, outFrequency);

		int outsize = outTrajFrame.pos.size();
		
		phaseSequence.IncrementPastSequence();
		phaseSequence.UpdateSequence(outPhase2D, outFrequency, outAmplitude);

        
		
		//pushback frame 10 times
        for (int i = 0; i < 1; i++) {
            genJointPos.push_back(outJointFrame.jointPos);
            genJointRot.push_back(outJointFrame.jointRot);
            genJointVel.push_back(outJointFrame.jointVel);
        }

        if(genIdx == 100) {
            debugSignal->controllPathPositions.clear();
            debugSignal->controllPathLookAt.clear();
            debugSignal->currentTrajectoryPosition.clear();
            for (auto pos : ctrlTrajPos)
            {
                debugSignal->controllPathPositions.push_back({ pos.x,0.0f, pos.y });
            }

            for(auto quat: ctrlTrajForward){
                debugSignal->controllPathLookAt.push_back(quat * glm::vec3(0.0,0.0,1.0));
			}

			for(auto pos: inTrajPos){
				debugSignal->currentTrajectoryPosition.push_back({ pos.x,0.0f, pos.y });
			}
		}
	}

	BuildAnimationSequence(genJointRot);

} 

void GNNController::BuildAnimationSequence(const std::vector<std::vector<glm::quat>>& jointRotSequence){
	
	int numBones = animationIn->mBones.size();
	
	animationOut = std::make_shared<Animation>();

	animationOut->mBones.resize(numBones);

	if (animationIn->mBones.size() != jointRotSequence[0].size()) {
		qDebug() << "BuildAnimationSequence: Joint Rotations size mismatch";
		return;
	}
	else {
		qDebug() << "BuildAnimationSequence: Init new Animation";

		for (int i = 0; i < jointRotSequence[0].size(); i++) {
			animationOut->mBones[i] = Bone(animationIn->mBones[i], 0);
			animationOut->mBones[i].mRotationKeys.clear();
            animationOut->mBones[i].mPositonKeys.clear();
		}
	}

	for (int frameIdx = 0; frameIdx < jointRotSequence.size(); frameIdx++) {
        auto quats = ConvertRotationsToLocalSpace(jointRotSequence[frameIdx]);
		for (int i = 0; i < jointRotSequence[frameIdx].size(); i++) {
			animationOut->mBones[i].mRotationKeys.push_back(KeyRotation(frameIdx, quats[i]));
		}
	}

    for(int frameIdx = 0; frameIdx < genJointPos.size(); frameIdx++){

        glm::vec3 pos = genJointPos[frameIdx][0];
        qDebug() << pos.x << pos.y << pos.z;
		animationOut->mBones[0].mPositonKeys.push_back(KeyPosition(frameIdx, glm::vec3(ctrlTrajPos[frameIdx].x, pos.y, ctrlTrajPos[frameIdx].y)));

	}

    //Create custom root bone place infront of the mBone array
    animationOut->mBones.insert(animationOut->mBones.begin(), Bone());
    animationOut->mBones[0].mName = "Armature";

    for (int frameIdx = 0; frameIdx < jointRotSequence.size(); frameIdx++) {
    	


        //get current root
        glm::vec2 currentRootPos = ctrlTrajPos[frameIdx];
        glm::quat currentRootRot = ctrlTrajForward[frameIdx];
        

		animationOut->mBones[0].mRotationKeys.push_back(KeyRotation(frameIdx, currentRootRot));
		animationOut->mBones[0].mPositonKeys.push_back(KeyPosition(frameIdx, glm::vec3(currentRootPos.x, currentRootPos.y, 0.0)));

    }

	
}

void GNNController::DebugWriteOutputToFile(const std::vector<float> data, bool out=true) {
	
	QString fileNameData = "";
	if (out)
		fileNameData = "C:\\DEV\\AnimHost\\python\\data\\animhost_test\\test_inf_out.bin";
	else
		fileNameData = "C:\\DEV\\AnimHost\\python\\data\\animhost_test\\test_inf_in.bin";

	FileHandler<QDataStream>::deleteFile(fileNameData);
	FileHandler<QDataStream> fileData = FileHandler<QDataStream>(fileNameData);
	
	QDataStream& outData = fileData.getStream();

	outData.writeRawData((char*)&data[0], data.size() * sizeof(float));
}

TrajectoryFrameData GNNController::BuildTrajectoryFrameData(const std::vector<glm::vec2> controlTrajectoryPositions, const std::vector<glm::quat>& controlTrajectoryForward, 
	const TrajectoryFrameData& inferredTrajectoryFrame, int PivotFrame, glm::mat4 Root)
{

	/* @todo: interpolate between control points and inferred trajectory
	*/

	TrajectoryFrameData trajFrame;

	glm::vec3 forward{ 0.0,0.0,1.0 };


	FrameRange frameRange(totalKeys, 60, PivotFrame, 0);

	//prepare trajectory
	for (int i : frameRange) {

		int idx = glm::max(i, 0);
		idx = glm::min(idx, int(controlPath->mControlPath.size() - 1));

		//relative position
		glm::vec2 pos = MathUtils::PositionTo(ctrlTrajPos[idx], Root);
		trajFrame.pos.push_back(pos);

		//relative character forward direction
		auto fwrd = ctrlTrajForward[PivotFrame] * forward;
		glm::vec3 dir = MathUtils::DirectionTo(fwrd, Root);
		glm::vec2 dir2d = glm::vec2(dir.x, dir.z);

		trajFrame.dir.push_back(dir2d);

		//relative velocity
		glm::vec2 prevPos = ctrlTrajPos[glm::max(0, idx - 1)];
		glm::vec2 currPos = ctrlTrajPos[idx];

		glm::vec2 vel = (currPos - prevPos) / (1.f / 60.f);
		glm::vec3 rVelocity = MathUtils::VelocityTo(glm::vec3(vel.x, 0.0, vel.y), Root) / 100.f;
		trajFrame.vel.push_back({ rVelocity.x,rVelocity.z });

		//Speed
		trajFrame.speed.push_back(glm::length(rVelocity));
	}



	return trajFrame;
}

void GNNController::BuildInputTensor(const TrajectoryFrameData& inTrajFrame,
	const JointsFrameData& inJointFrame)  {

	std::vector<Rotation6D> jointRot6D = MathUtils::convertQuaternionsTo6DRotations(inJointFrame.jointRot);

	input_values.clear();

	

	for (int i = 0; i < inTrajFrame.pos.size(); i++) {
		input_values.push_back(inTrajFrame.pos[i].x);
		input_values.push_back(inTrajFrame.pos[i].y);

		input_values.push_back(inTrajFrame.dir[i].x);
		input_values.push_back(inTrajFrame.dir[i].y);

		input_values.push_back(inTrajFrame.vel[i].x);
		input_values.push_back(inTrajFrame.vel[i].y);

		input_values.push_back(inTrajFrame.speed[i]);
	}

	for (int i = 0; i < inJointFrame.jointPos.size(); i++) {
		input_values.push_back(inJointFrame.jointPos[i].x);
		input_values.push_back(inJointFrame.jointPos[i].y);
		input_values.push_back(inJointFrame.jointPos[i].z);

		input_values.push_back(jointRot6D[i][0]);
		input_values.push_back(jointRot6D[i][1]);
		input_values.push_back(jointRot6D[i][2]);
		input_values.push_back(jointRot6D[i][3]);
		input_values.push_back(jointRot6D[i][4]);
		input_values.push_back(jointRot6D[i][5]);

		input_values.push_back(inJointFrame.jointVel[i].x);
		input_values.push_back(inJointFrame.jointVel[i].y);
		input_values.push_back(inJointFrame.jointVel[i].z);
	}

	for (auto p : phaseSequence.GetFlattenedPhaseSequence()) {
		input_values.push_back(p.x);
		input_values.push_back(p.y);
	}
}

glm::vec3 GNNController::readOutput(const std::vector<float>& output_values,TrajectoryFrameData& outTrajectoryFrame, JointsFrameData& outJointFrame,
	std::vector< std::vector<glm::vec2>>& outPhase2D, 
	std::vector< std::vector<float>>& outAmplitude, std::vector< std::vector<float>>& outFrequency)
{
	int f_idx = 0; //feature index
	glm::vec3 delta_out = { output_values[f_idx], output_values[f_idx + 1], output_values[f_idx + 2] };

	f_idx += 3;
	

	for (int i = pastKeys; i < totalKeys; i++) {
		
		outTrajectoryFrame.pos.push_back({ output_values[f_idx], output_values[f_idx+1] });

		outTrajectoryFrame.dir.push_back({ output_values[f_idx + 2], output_values[f_idx + 3] });

		outTrajectoryFrame.vel.push_back({ output_values[f_idx + 4], output_values[f_idx + 5] });

		outTrajectoryFrame.speed.push_back( output_values[f_idx + 6]);

		f_idx += 7;
	}

	std::vector<glm::vec3> outJointPos;
	std::vector<Rotation6D> outJointRot;
	std::vector<glm::vec3> outJointVel;

	for (int i = 0; i < initJointPos.size(); i++) {

		outJointFrame.jointPos.push_back({ output_values[f_idx], output_values[f_idx + 1], output_values[f_idx + 2] });

		outJointRot.push_back({ output_values[f_idx+3], output_values[f_idx+4], output_values[f_idx+5],
					            output_values[f_idx+6], output_values[f_idx+7], output_values[f_idx+8]});

		outJointFrame.jointVel.push_back({output_values[f_idx+9], output_values[f_idx+10] , output_values[f_idx+11]});

		f_idx += 12;
		
	}

	for (int i = 0; i < futureKeys + 1; i++) {

		outPhase2D.push_back(std::vector<glm::vec2>());
		outAmplitude.push_back(std::vector<float>());
		outFrequency.push_back(std::vector<float>());

		for (int j = 0; j < numPhaseChannel; j++) {
			outPhase2D[i].push_back({output_values[f_idx], output_values[f_idx + 1]});
			f_idx += 2;
		}

		for (int j = 0; j < numPhaseChannel; j++) {
			outAmplitude[i].push_back({output_values[f_idx]});
			f_idx++;
		}

		for (int j = 0; j < numPhaseChannel; j++) {
			outFrequency[i].push_back({output_values[f_idx]});
			f_idx++;
		}
	}

    outJointFrame.jointRot = MathUtils::convert6DRotationToQuaternions(outJointRot);

	return delta_out;

}


std::vector<glm::quat> GNNController::ConvertRotationsToLocalSpace(const std::vector<glm::quat>& rootSpaceJointRots)
{
	//convert global rotations to local space

	std::vector<glm::quat> localRots;

	for (int idx = 0; idx < skeleton->mNumBones; idx++) {
		
		// root space rotation
		glm::quat rsJointRot = rootSpaceJointRots[idx];

		int parentBoneIdx = AnimHostHelper::FindParentBone(skeleton->bone_hierarchy, idx);

		if (parentBoneIdx != -1) {
			// parent bone rotation
			glm::quat parentBoneRot = rootSpaceJointRots[parentBoneIdx];

			// local rotation
			glm::quat localRot = glm::conjugate(parentBoneRot) * rsJointRot ;

			// set local rotation
			localRots.push_back(localRot);
		}
		else {
			localRots.push_back(rsJointRot);
		}
	}

	return localRots;
}

void GNNController::SetAnimationIn(std::shared_ptr<Animation> anim)
{
	animationIn = anim;
}

std::shared_ptr<Animation> GNNController::GetAnimationOut(){
	return animationOut;
}

std::vector<glm::vec3> GNNController::GetRelativeJointVel(const std::vector<glm::vec3>& globalJointVel, const glm::mat4& root)
{
	std::vector<glm::vec3> relativeVel;

	for (int i = 0; i < globalJointVel.size(); i++) {
		relativeVel.push_back(MathUtils::DirectionTo(globalJointVel[i], root));
	}

	return relativeVel;
}

std::vector<glm::vec3> GNNController::GetRelativeJointPos(const std::vector<glm::vec3>& globalJointPos, const glm::mat4& root)
{
	std::vector<glm::vec3> relativePos;


	for (int i = 0; i < globalJointPos.size(); i++) {

		relativePos.push_back(MathUtils::PositionTo(globalJointPos[i], root));
	}

	return relativePos;
}

void GNNController::SetSkeleton(std::shared_ptr<Skeleton> skel)
{
	skeleton = skel;
}

void GNNController::SetControlPath(std::shared_ptr<ControlPath> path)
{
    controlPath = path;
}

std::vector<glm::quat> GNNController::GetRelativeJointRot(const std::vector<glm::quat>& globalJointRot, const glm::mat4& root) {

	std::vector<glm::quat> relativeRot;

	glm::quat rotation = MathUtils::DecomposeRotation(root);

	glm::quat inv_rotation = glm::inverse(rotation);

	for (int i = 0; i < globalJointRot.size(); i++) {
		relativeRot.push_back(globalJointRot[i] * inv_rotation);
	}

	return relativeRot;
}

void GNNController::GetGlobalJointPosition(std::vector<glm::vec3>& globalJointPos, const glm::mat4& root) {
	for (auto& pos : globalJointPos) {
		glm::vec4 homogenousPos = glm::vec4(pos, 1.0f); // Convert to homogenous coordinates
		glm::vec4 globalPos = root * homogenousPos; // Transform to global coordinates
		pos = glm::vec3(globalPos); // Overwrite the original position
	}
}

void GNNController::GetGlobalJointRotation(std::vector<glm::quat>& globalJointRot, const glm::mat4& root) {
	for (auto& rot : globalJointRot) {
		glm::quat globalRot = glm::quat_cast(root) * rot; // Transform to global coordinates
		rot = globalRot; // Overwrite the original rotation
	}
}

void GNNController::GetGlobalJointVelocity(std::vector<glm::vec3>& globalJointVel, const glm::mat4& root) {
	for (auto& vel : globalJointVel) {
		glm::vec4 homogenousVel = glm::vec4(vel, 0.0f); // Convert to homogenous coordinates
		glm::vec4 globalVel = root * homogenousVel; // Transform to global coordinates
		vel = glm::vec3(globalVel); // Overwrite the original velocity
	}
}


glm::vec2 GNNController::Calc2DPhase(float phaseValue, float amplitude) {
	phaseValue *= (2.f * glm::pi<float>());

	float x_val = glm::sin(phaseValue) * amplitude;
	float y_val = glm::cos(phaseValue) * amplitude;

	return glm::vec2(x_val, y_val);
}


glm::vec2 GNNController::Update2DPhase(float amplitude, float frequency, glm::vec2 current, glm::vec2 next, float minAmplitude=0.f) {

	amplitude = glm::abs(amplitude);
	amplitude = glm::max(amplitude, minAmplitude);

	frequency = glm::abs(frequency);

	glm::vec2 updated = glm::angleAxis(-frequency * 360.f * (1.f / 60.f), glm::vec3(0.f, 0.f, 1.f)) * glm::vec3(current,0.0f);

	next = glm::normalize(next);
	updated = glm::normalize(updated);

	//slerp work around
	glm::quat a = glm::quat(glm::vec3(next, 0.f), glm::vec3(0.f, 0.f, 1.f));
	glm::quat b = glm::quat(glm::vec3(updated, 0.f), glm::vec3(0.f, 0.f, 1.f));
	glm::quat mix = glm::slerp(a,b, 0.5f);

	auto mixed = mix * glm::vec3(0.f, 0.f, 1.f);

	return glm::vec2(mixed);
}

float GNNController::CalcPhaseValue(glm::vec2 phase) {
	
	float angle = -glm::orientedAngle(glm::vec2(0.f, 1.f), glm::normalize(phase));
	if (angle < 0.f) {
		angle = 360.f + angle;
	}

	return glm::mod(angle / 360.f, 1.f);
}

glm::mat4 GNNController::BuildRootTransform(const glm::vec3& pos, const glm::quat& rot)
{
	return glm::mat4();
}



void GNNController::InitDummyData(){
}

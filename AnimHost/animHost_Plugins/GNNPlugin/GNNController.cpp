#include "GNNController.h"
#include <FileHandler.h>
#include <FrameRange.h>
#include <MathUtils.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <animhosthelper.h>
#include <matplot/matplot.h>
#include <random>
#include <algorithm>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

GNNController::GNNController(QString networkPath) : NetworkModelPath(networkPath)
{
	network = std::make_unique<OnnxModel>();	
	network->LoadOnnxModel(NetworkModelPath);
    debugSignal = std::make_shared<DebugSignal>();
}

void GNNController::clearGeneratedData() {
	genRootPos.clear();
	genRootForward.clear();
	genJointPos.clear();
	genJointRot.clear();
	genJointVel.clear();
}

void GNNController::prepareControlTrajectory() {
	ctrlTrajPos.clear();
	ctrlTrajForward.clear();

	for (auto& p : controlPath->mControlPath) {
		ctrlTrajPos.push_back(glm::vec2(p.position.x, p.position.z) * 100.f);
		ctrlTrajForward.push_back(p.lookAt);
	}
}

void GNNController::prepareInput()
{

	qDebug() << "Generate Animation with Control Path of size: " << controlPath->mControlPath.size();

	clearGeneratedData();
	prepareControlTrajectory();

	genRootPos.push_back(ctrlTrajPos[0]);
	genRootForward.push_back(ctrlTrajForward[0]);

	glm::vec2 updatedRootPos = glm::vec2(0.0, 0.0);
	glm::quat updatedRootRot = glm::quat(1.0, 0.0, 0.0, 0.0);

	TrajectoryFrameData inferredTrajectoryFrame;

	TrajectoryFrameData outTrajFrame;

	outTrajFrame.pos = std::vector<glm::vec2>(7, {0.f,0.f});
	outTrajFrame.dir = std::vector<glm::vec2>(7, {0.f,1.f});
	outTrajFrame.vel = std::vector<glm::vec2>(7, {0.f,0.f});
	outTrajFrame.speed = std::vector<float>(7, 0.f);

	glm::mat4 root = glm::mat4(1.0);

	InitPlot();

	glm::vec2 currentRootPos;
	glm::quat currentRootRot;

	TrajectoryFrameData inTrajFrame;
	JointsFrameData inJointFrame;

	JointsFrameData outJointFrame;

	for (int genIdx = 1; genIdx < ctrlTrajPos.size(); genIdx++) {
		//get current root

		if(genIdx == 1){
			currentRootPos = ctrlTrajPos[genIdx];
			currentRootRot = ctrlTrajForward[genIdx];
			glm::mat4 trans = glm::translate(glm::mat4(1.0), glm::vec3(currentRootPos.x, 0.0, currentRootPos.y));
			root = trans * glm::toMat4(currentRootRot);
		}
		else{
			currentRootPos = updatedRootPos;
			currentRootRot = updatedRootRot;
		}

		genRootPos.push_back(currentRootPos);
		genRootForward.push_back(currentRootRot);

		inTrajFrame = BuildTrajectoryFrameData(ctrlTrajPos, ctrlTrajForward, outTrajFrame, genIdx, root);
		
		if (genJointPos.size() <= 0) {
			// set initial pose for inference to the provided pose
            inJointFrame.jointPos = initJointPos;
            inJointFrame.jointRot = initJointRot;
            inJointFrame.jointVel = initJointVel;
		}
		else {
			// all other frames use the generated pose of previous frame
			inJointFrame.jointPos = genJointPos.back();
			inJointFrame.jointRot = genJointRot.back();
			inJointFrame.jointVel = genJointVel.back();
		}

		BuildInputTensor(inTrajFrame,
			inJointFrame);

		//Inference
		std::vector<float> inferenceOutputValues = network->RunInference(input_values);

		std::vector<std::vector<glm::vec2>> outPhase2D;
		std::vector<std::vector<float>> outAmplitude;
		std::vector<std::vector<float>> outFrequency;
	

		outTrajFrame.clear();
		outJointFrame.clear();

		glm::vec3 deltaOut = readOutput(inferenceOutputValues, outTrajFrame, outJointFrame ,outPhase2D, outAmplitude, outFrequency);
		
		phaseSequence.IncrementPastSequence();
		phaseSequence.UpdateSequence(outPhase2D, outFrequency, outAmplitude);

		genJointPos.push_back(outJointFrame.jointPos);
		genJointRot.push_back(outJointFrame.jointRot);
		genJointVel.push_back(outJointFrame.jointVel);
		
		// ========================================================================================================
		// Update Root Transform
		// ========================================================================================================

		// construct delta/inferred transform
		glm::quat deltaRot = glm::angleAxis(-deltaOut.z, glm::vec3(0.0, 1.0, 0.0)); // Quat from delta Angle, minus signe required??

		glm::mat4 deltaTransform = glm::translate(glm::mat4(1.0), glm::vec3(deltaOut.x, 0.0, deltaOut.y)) * glm::toMat4(deltaRot);
		glm::mat4 inferredRoot = root * deltaTransform;

		glm::vec3 tempPos = root * glm::vec4(0.0, 0.0, 0.0, 1.0);
		glm::vec2 inferredPos = { tempPos.x,tempPos.z };
		glm::quat inferredRot = currentRootRot * deltaRot;


		// mix inferred with control path for stability
		updatedRootPos = glm::mix(ctrlTrajPos[genIdx], inferredPos, 0.2f);
		updatedRootRot = glm::slerp(ctrlTrajForward[genIdx], inferredRot, 0.7f);

		//update current root
		root = glm::translate(glm::mat4(1.0), glm::vec3(updatedRootPos.x, 0.0, updatedRootPos.y)) * glm::toMat4(updatedRootRot);

		UpdatePlotData(inTrajFrame, outTrajFrame);
		DrawPlot();

	}

	BuildAnimationSequence(genJointRot);

} 



TrajectoryFrameData GNNController::BuildTrajectoryFrameData(const std::vector<glm::vec2> controlTrajectoryPositions, const std::vector<glm::quat>& controlTrajectoryForward, 
	const TrajectoryFrameData& inferredTrajectoryFrame, int PivotFrame, glm::mat4 Root)
{
	TrajectoryFrameData trajFrame;

	glm::vec3 forward{ 0.0,0.0,1.0 };


	FrameRange frameRange(totalKeys, 60, PivotFrame, 0);


	int count = 0;
	//prepare trajectory
	for (int i : frameRange) {

		qDebug() << "Frame: " << i << "On Pivot: "<< PivotFrame;

		int idx = glm::max(i, 0);
		idx = glm::min(idx, int(controlPath->mControlPath.size() - 1));


		//  relative position
		glm::vec2 pos;
		if (idx <= PivotFrame) {
			//use past positions
			pos = MathUtils::PositionTo(genRootPos[idx], Root);
		}
		else {
			//use future control path positions
			pos = MathUtils::PositionTo(ctrlTrajPos[idx], Root);

			//get infered trajectory

			auto inferedPos = inferredTrajectoryFrame.pos[count-7];
			qDebug() << "Infered Pos: " << inferedPos.x << " " << inferedPos.y;

			float blend = 1.f - ((count-7.f) / 7.f) ;

			pos = glm::mix(pos, inferedPos, blend);
		}


		trajFrame.pos.push_back(pos);


		//relative character forward directio
		glm::vec3 fwrd;
		glm::vec3 dir;
		glm::vec2 dir2d;;

		if (idx <= PivotFrame) {
			fwrd = genRootForward[idx] * forward;
			dir = MathUtils::DirectionTo(fwrd, Root);
			dir2d = glm::vec2(dir.x, dir.z);
		}
		else {
			fwrd = ctrlTrajForward[idx] * forward;
			dir = MathUtils::DirectionTo(fwrd, Root);
			dir2d = glm::vec2(dir.x, dir.z);
			auto inferedDir = inferredTrajectoryFrame.dir[count - 7];

			float blend = 1.f - ((count - 7.f) / 7.f);
			dir2d = glm::mix(dir2d, inferedDir, blend);
		}


		trajFrame.dir.push_back(dir2d);


		//relative 
		glm::vec2 prevPos, currPos;
		glm::vec3 velocity;
		if (idx <= PivotFrame) {
			prevPos = genRootPos[glm::max(0, idx - 1)];
			currPos = genRootPos[idx];

			glm::vec2 vel = (currPos - prevPos) / (1.f / 60.f);
			velocity = MathUtils::VelocityTo(glm::vec3(vel.x, 0.0, vel.y), Root) / 100.f;


			trajFrame.vel.push_back({ velocity.x,velocity.z });

			//Speed
			trajFrame.speed.push_back(glm::length(velocity));

		}
		//??? special case required
		/*else if (idx == PivotFrame+1) {
			prevPos = genRootPos[idx-1];
			currPos = ctrlTrajPos[idx];


		}*/
		else {
			prevPos = ctrlTrajPos[glm::max(0, idx - 1)];
			currPos = ctrlTrajPos[idx];

			glm::vec2 vel = (currPos - prevPos) / (1.f / 60.f);
			velocity = MathUtils::VelocityTo(glm::vec3(vel.x, 0.0, vel.y), Root) / 100.f;

			auto inferedVel = glm::vec3(inferredTrajectoryFrame.vel[count - 7].x, 0.0f, inferredTrajectoryFrame.vel[count - 7].y);

			float blend = 1.f - ((count - 7.f) / 7.f);
			velocity = glm::mix(velocity, inferedVel, 0.0f);

			trajFrame.vel.push_back({ velocity.x,velocity.z });

			//Speed
			trajFrame.speed.push_back(glm::length(velocity));
		}

		

		count++;
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

void GNNController::BuildAnimationSequence(const std::vector<std::vector<glm::quat>>& jointRotSequence) {

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

	for (int frameIdx = 0; frameIdx < genJointPos.size(); frameIdx++) {

		glm::vec3 pos = genJointPos[frameIdx][0];
		qDebug() << pos.x << pos.y << pos.z;
		animationOut->mBones[0].mPositonKeys.push_back(KeyPosition(frameIdx, glm::vec3(genRootPos[frameIdx].x, pos.y, genRootPos[frameIdx].y)));

	}

	//Create custom root bone place infront of the mBone array
	animationOut->mBones.insert(animationOut->mBones.begin(), Bone());
	animationOut->mBones[0].mName = "Armature";

	for (int frameIdx = 0; frameIdx < jointRotSequence.size(); frameIdx++) {



		//get current root
		glm::vec2 currentRootPos = genRootPos[frameIdx];
		glm::quat currentRootRot = genRootForward[frameIdx];


		animationOut->mBones[0].mRotationKeys.push_back(KeyRotation(frameIdx, currentRootRot));
		animationOut->mBones[0].mPositonKeys.push_back(KeyPosition(frameIdx, glm::vec3(currentRootPos.x, currentRootPos.y, 0.0)));

	}


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


void GNNController::SetSkeleton(std::shared_ptr<Skeleton> skel)
{
	skeleton = skel;
}

void GNNController::SetControlPath(std::shared_ptr<ControlPath> path)
{
    controlPath = path;
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

glm::mat4 GNNController::updateRootTranform(const glm::mat4& pos, const glm::vec3& delta, int genIdx)
{
	return glm::mat4();
}


void GNNController::InitPlot() {
	
	figure = matplot::figure(true);
	figure->size(800, 800);
	


	auto ax = matplot::subplot(figure, 2,2, 0);
	matplot::xlim(ax, { -600, 600 });
	matplot::ylim(ax, { -600, 600 });


	//matplot::show();
	auto ax2 = matplot::subplot(figure, 2, 2, 1);
	std::vector<double> c_xin(ctrlTrajPos.size());
	std::transform(ctrlTrajPos.begin(), ctrlTrajPos.end(), c_xin.begin(), [&](glm::vec2 pos) {return pos.x; });
	std::vector<double> c_yin(c_xin.size());
	std::transform(ctrlTrajPos.begin(), ctrlTrajPos.end(), c_yin.begin(), [&](glm::vec2 pos) {return pos.y; });

	ax2->scatter(c_xin, c_yin);
	figure->draw();


}

void GNNController::UpdatePlotData(const TrajectoryFrameData& inTrajFrame, const TrajectoryFrameData& outTrajFrame) {

			std::vector<double> xin(inTrajFrame.pos.size());
			std::transform(inTrajFrame.pos.begin(), inTrajFrame.pos.end(), xin.begin(), [&](glm::vec2 pos) {return pos.x; });
			std::vector<double> yin(xin.size());
			std::transform(inTrajFrame.pos.begin(), inTrajFrame.pos.end(), yin.begin(), [&](glm::vec2 pos) {return pos.y; });

			auto ax = matplot::subplot(figure, 2, 2, 0, true);
			matplot::xlim(ax, { -600, 600 });
			matplot::ylim(ax, { -600, 600 });
			
			ax->scatter(xin, yin);

			std::vector<double> xout(outTrajFrame.pos.size());
			std::transform(outTrajFrame.pos.begin(), outTrajFrame.pos.end(), xout.begin(), [&](glm::vec2 pos) {return pos.x; });
			std::vector<double> yout(xout.size());
			std::transform(outTrajFrame.pos.begin(), outTrajFrame.pos.end(), yout.begin(), [&](glm::vec2 pos) {return pos.y; });
			matplot::hold(ax, true);
			ax->scatter(xout, yout);
			matplot::hold(ax, false);


			std::vector<float> freq = phaseSequence.GetFrequencySequence(0);
			auto ax2 = matplot::subplot(figure, 2, 2, 2, true);
			ax2->plot(freq);
			


			auto ax3 = matplot::subplot(figure, 2, 2, 3, true);
			matplot::xlim(ax3, { -4, 4 });
			matplot::ylim(ax3, { -4, 4 });

			auto phases2D = phaseSequence.GetFlattenedPhaseSequence();
			std::vector<double> xphase(phases2D.size());
			std::transform(phases2D.begin(), phases2D.end(), xphase.begin(), [&](glm::vec2 pos) {return pos.x; });
			std::vector<double> yphase(xphase.size());
			std::transform(phases2D.begin(), phases2D.end(), yphase.begin(), [&](glm::vec2 pos) {return pos.y; });

			std::vector<double> filteredXphase;
			std::vector<double> filteredYphase;

			for (size_t i = 0; i < std::min(xphase.size(), yphase.size()); i += 5) {
				filteredXphase.push_back(xphase[i]);
				filteredYphase.push_back(yphase[i]);
			}

			ax3->plot(filteredXphase, filteredYphase);

			filteredXphase.clear();
			filteredYphase.clear();
			for (size_t i = 1; i < std::min(xphase.size(), yphase.size()); i += 5) {
				filteredXphase.push_back(xphase[i]);
				filteredYphase.push_back(yphase[i]);
			}

			matplot::hold(ax3, true);
			ax3->plot(filteredXphase, filteredYphase);
			matplot::hold(ax3, false);



			figure->draw();

}

void GNNController::DrawPlot() {
}

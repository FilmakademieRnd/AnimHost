#include "GNNController.h"
#include <FileHandler.h>
#include <FrameRange.h>
#include <MathUtils.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/quaternion_float.hpp>
#include "DummyData.h"
#include <animhosthelper.h>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>

GNNController::GNNController()
{
	network = std::make_unique<OnnxModel>();	
	network->LoadOnnxModel(NetworkModelPath);
}

void GNNController::prepareInput()
{
	genJointPos.clear();
	genJointRot.clear();
	genJointVel.clear();

	for (int genIdx = 0; genIdx < 100; genIdx++) {

		std::vector<glm::vec2> inTrajPos;
		std::vector<glm::vec2> inTrajDir;
		std::vector<glm::vec2> inTrajVel;
		std::vector<float> inTrajSpeed;

		std::vector<glm::vec3> inJointPos;
		std::vector<glm::quat> inJointRot;
		std::vector<glm::vec3> inJointVel;

		std::vector<float> futurePhase;

		//get current root
		glm::vec2 currentRootPos = ctrlTrajPos[genIdx];
		glm::quat currentRootRot = ctrlTrajForward[genIdx];
		//glm::quat currentRootRot = glm::quat(1.0, 0.0, 0.0, 0.0);
		glm::mat4 trans = glm::translate(glm::mat4(1.0), glm::vec3(currentRootPos.x, 0.0, currentRootPos.y));
		glm::mat4 root = trans * glm::toMat4(currentRootRot);
		//glm::mat4 root = animationIn->CalculateRootTransform(genIdx, 0);

		//glm::mat4 root = animationIn->CalculateRootTransform(1, 0);

		glm::vec3 forward{ 0.0,0.0,1.0 };



		FrameRange frameRange(totalKeys, 60, genIdx, 0);

		//prepare trajectory
		for (int i : frameRange) {

			int idx = glm::max(i, 0);
			idx = glm::min(idx, int(ctrlTrajPos.size() - 1));

			//relative position
			glm::vec2 pos = MathUtils::PositionTo(ctrlTrajPos[idx], root);
			inTrajPos.push_back(pos);

			//relative character forward direction
			auto fwrd = ctrlTrajForward[idx] * forward;
			glm::vec3 dir = MathUtils::DirectionTo(fwrd, root);
			glm::vec2 dir2d = glm::vec2(dir.x, dir.z);
			//glm::vec2 dir2d = glm::vec2(0, 1);
			//glm::vec2 dir = DummyFrwrd[idx]

			inTrajDir.push_back(dir2d);

			//relative velocity
			glm::vec2 prevPos = ctrlTrajPos[glm::max(0, idx - 1)];
			glm::vec2 currPos = ctrlTrajPos[idx];

			glm::vec2 vel = (currPos - prevPos) / (1.f / 60.f);
			glm::vec3 rVelocity = MathUtils::VelocityTo(glm::vec3(vel.x, 0.0, vel.y), root) / 100.f;
			inTrajVel.push_back({ rVelocity.x,rVelocity.z });

			//Speed
			inTrajSpeed.push_back(glm::length(rVelocity));
		}
		//prepare character input

		//initJointVel = std::vector<glm::vec3>(initJointPos.size());

		//HOTFIX: Move Char to first root pos
		/*glm::mat4 hfTranslation = glm::mat4(1.0);
		hfTranslation = glm::translate(hfTranslation, initJointPos[0]);
        hfTranslation = animationIn->CalculateRootTransform(20, 0);
		initJointPos = GetRelativeJointPos(initJointPos, hfTranslation);

		for (int i = 0; i < initJointPos.size(); i++) {
			initJointPos[i] = initJointPos[i] + glm::vec3(ctrlTrajPos[0].x, 0.0, ctrlTrajPos[0].y);
		}*/
		//!HOTFIX

		if (genJointPos.size() <= 0) {
            inJointPos = initJointPos;// GetRelativeJointPos(initJointPos, root);
            inJointRot = initJointRot;// GetRelativeJointRot(initJointRot, root);
            inJointVel = initJointVel;//GetRelativeJointVel(initJointVel, root);
		}
		else {
			inJointPos = genJointPos.back();//GetRelativeJointPos(genJointPos.back(), root);
			inJointRot = genJointRot.back();//GetRelativeJointRot(genJointRot.back(), root);
			inJointVel = genJointVel.back();//GetRelativeJointVel(genJointVel.back(), root);

		}


		BuildInputTensor(inTrajPos, inTrajDir, inTrajVel, inTrajSpeed,
			inJointPos, inJointRot, inJointVel);

	


		/*DebugWriteOutputToFile(input_values, false);*/
		

		/*int dummyIdx = genIdx % DummyIN.size();
		input_values = DummyIN[dummyIdx];*/


		/*if (genJointPos.size() <= 0) {
			input_values = DummyIN[0];
		}*/

		//Inference
		auto outputValues = network->RunInference(input_values);

        //outputValues = DummyOUT[1];

		

		std::vector<glm::vec3> outJointPosition;
		std::vector<glm::quat> outJointRotation;
		std::vector<glm::vec3> outJointVelocity;

		std::vector<std::vector<glm::vec2>> outPhase2D;
		std::vector<std::vector<float>> outAmplitude;
		std::vector<std::vector<float>> outFrequency;


		readOutput(outputValues,outJointPosition, outJointRotation, outJointVelocity,outPhase2D, outAmplitude, outFrequency);


		phaseSequence.IncrementSequence(0, pastKeys);
		phaseSequence.UpdateSequence(outPhase2D, outFrequency, outAmplitude);

		//GetGlobalJointPosition(outJointPosition, root);
		//GetGlobalJointRotation(outJointRotation, root);
		//GetGlobalJointVelocity(outJointVelocity, root);
		
        //pushback frame 10 times
        for (int i = 0; i < 1; i++) {
            genJointPos.push_back(outJointPosition);
            genJointRot.push_back(outJointRotation);
            genJointVel.push_back(outJointVelocity);
        }

		

	}

	BuildAnimationSequence(genJointRot);

	//DebugWriteOutputToFile(outputValues, true);
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
		animationOut->mBones[0].mPositonKeys.push_back(KeyPosition(frameIdx, glm::vec3(ctrlTrajPos[frameIdx].x, 0.0, ctrlTrajPos[frameIdx].y)));

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

void GNNController::BuildInputTensor(const std::vector<glm::vec2>& pos, const std::vector<glm::vec2>& dir, const std::vector<glm::vec2>& vel, const std::vector<float>& speed, 
	const std::vector<glm::vec3>& jointPos, const std::vector<glm::quat>& jointRot, const std::vector<glm::vec3>& jointVel)  {

	std::vector<Rotation6D> jointRot6D = MathUtils::convertQuaternionsTo6DRotations(jointRot);

	input_values.clear();

	

	for (int i = 0; i < pos.size(); i++) {
		input_values.push_back(pos[i].x);
		input_values.push_back(pos[i].y);

		input_values.push_back(dir[i].x);
		input_values.push_back(dir[i].y);

		input_values.push_back(vel[i].x);
		input_values.push_back(vel[i].y);

		input_values.push_back(speed[i]);
	}

	for (int i = 0; i < jointPos.size(); i++) {
		input_values.push_back(jointPos[i].x);
		input_values.push_back(jointPos[i].y);
		input_values.push_back(jointPos[i].z);

		input_values.push_back(jointRot6D[i][0]);
		input_values.push_back(jointRot6D[i][1]);
		input_values.push_back(jointRot6D[i][2]);
		input_values.push_back(jointRot6D[i][3]);
		input_values.push_back(jointRot6D[i][4]);
		input_values.push_back(jointRot6D[i][5]);

		input_values.push_back(jointVel[i].x);
		input_values.push_back(jointVel[i].y);
		input_values.push_back(jointVel[i].z);
	}

	

	for (auto p : phaseSequence.GetFlattenedPhaseSequence()) {
		input_values.push_back(p.x);
		input_values.push_back(p.y);
	}

	for(int i = input_values.size(); i < DummyIN[0].size(); i++) {
		input_values.push_back(DummyIN[0][i]);
	
	}
}

void GNNController::readOutput(const std::vector<float>& output_values, std::vector<glm::vec3>& outJointPosition, 
	std::vector<glm::quat>& outJointRotation, std::vector<glm::vec3>& outJointVelocity, std::vector< std::vector<glm::vec2>>& outPhase2D, std::vector< std::vector<float>>& outAmplitude, std::vector< std::vector<float>>& outFrequency)
{
	int f_idx = 0; //feature index
	
	glm::vec2 delta_pos = { output_values[f_idx], output_values[f_idx + 1] };
	float delta_angle = output_values[f_idx + 2];

	f_idx += 3;
	
	
	std::vector<glm::vec2> outTrajPos;
	std::vector<glm::vec2> outTrajDir;
	std::vector<glm::vec2> outTrajVel;
	std::vector<float> outTrajSpeed;


	for (int i = pastKeys; i < totalKeys; i++) {
		
		outTrajPos.push_back({ output_values[f_idx], output_values[f_idx+1] });

		outTrajDir.push_back({ output_values[f_idx + 2], output_values[f_idx + 3] });

		outTrajVel.push_back({ output_values[f_idx + 4], output_values[f_idx + 5] });

		outTrajSpeed.push_back( output_values[f_idx + 6]);

		f_idx += 7;
	}

	std::vector<glm::vec3> outJointPos;
	std::vector<Rotation6D> outJointRot;
	std::vector<glm::vec3> outJointVel;

	for (int i = 0; i < initJointPos.size(); i++) {

		outJointPosition.push_back({ output_values[f_idx], output_values[f_idx + 1], output_values[f_idx + 2] });

		outJointRot.push_back({ output_values[f_idx+3], output_values[f_idx+4], output_values[f_idx+5],
					            output_values[f_idx+6], output_values[f_idx+7], output_values[f_idx+8]});

		outJointVelocity.push_back({output_values[f_idx+9], output_values[f_idx+10] , output_values[f_idx+11]});

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



	/* Test */
	//std::vector<Rotation6D> jointRot6D = MathUtils::convertQuaternionsTo6DRotations(initJointRot);
	//auto outQuat = MathUtils::convert6DRotationToQuaternions(jointRot6D);
	//auto localJointRotations = ConvertRotationsToLocalSpace(outQuat);
	/* !Test */


    outJointRotation = MathUtils::convert6DRotationToQuaternions(outJointRot);
	//outJointRotation = ConvertRotationsToLocalSpace(outQuat);
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
			//localRots.push_back(glm::conjugate(rsJointRot) * rsJointRot);
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
    ctrlTrajPos = {
        {124.14142284549956, 0.0},
        {124.13461609251871, 1.2999821799216549},
        {124.11419658001336, 2.5998218021478134},
        {124.08016654721311, 3.8993763246160373},
        {124.03252972589459, 5.198503236528291},
        {123.97129133997205, 6.497060073978853},
        {123.8964581049246, 7.794904435577088},
        {123.8080382270598, 9.091893998063375},
        {123.70604140261361, 10.387886531916443},
        {123.59047881668722, 11.682739916950442},
        {123.46136314202047, 12.976312157900036},
        {123.31870853760215, 14.268461399991772},
        {123.1625306471171, 15.559045944500049},
        {122.99284659723104, 16.847924264286004},
        {122.80967499571217, 18.134955019317527},
        {122.61303592939079, 19.41999707216882},
        {122.40295096195642, 20.702909503497665},
        {122.17944313159317, 21.983551627498873},
        {121.9425369484534, 23.261783007332014},
        {121.69225839196972, 24.53746347052193},
        {121.42863490800625, 25.81045312433023},
        {121.15169540584878, 27.08061237109607},
        {120.86147025503455, 28.34780192354466},
        {120.55799128202186, 29.611882820061634},
        {120.24129176670004, 30.872716439931807},
        {119.91140643873979, 32.13016451854045},
        {119.56837147378481, 33.384089162535595},
        {119.21222448948464, 34.63435286494956},
        {118.84300454136954, 35.88081852027821},
        {118.46075211856756, 37.12334943951604},
        {118.0655091393644, 38.3618093651457},
        {117.65731894660676, 39.59606248608018},
        {117.23622630294919, 40.82597345255599},
        {116.80227738594527, 42.051407390975825},
        {116.35551978298399, 43.27222991869894},
        {115.89600248607096, 44.488307158777765},
        {115.42377588645608, 45.699505754638984},
        {114.93889176910751, 46.90569288470758},
        {114.4414033070329, 48.10673627697225},
        {113.93136505544828, 49.30250422349051},
        {113.40883294579552, 50.49286559483194},
        {112.87386427960888, 51.677689854458016},
        {112.32651772223116, 52.856847073036924},
        {111.76685329638036, 54.030207942691746},
        {111.19493237556767, 55.197643791180546},
        {110.61081767736702, 56.35902659600673},
        {110.0145732565374, 57.514228998458165},
        {109.40626449799865, 58.66312431757347},
        {108.78595810966117, 59.80558656403402},
        {108.15372211511063, 60.94149045398012},
        {107.5096258461485, 62.070711422749774},
        {106.85373993518897, 63.19312563853857},
        {106.18613630751337, 64.30861001597931},
        {105.50688817338262, 65.41704222963965},
        {104.81607002000912, 66.51830072743641},
        {104.11375760338815, 67.61226474396531},
        {103.4000279399906, 68.69881431374405},
        {102.67495929831699, 69.77783028436805},
        {101.9386311903147, 70.84919432957666},
        {101.19112436265844, 71.9127889622292},
        {100.4325207878955, 72.96849754718855},
        {99.6629036554565, 74.0162043141116},
        {98.88235736253287, 75.05579437014482},
        {98.09096750482153, 76.08715371252343},
        {97.28882086713855, 77.11016924107318},
        {96.47600541390213, 78.124728770613},
        {95.65261027948632, 79.13072104325745},
        {94.8187257584464, 80.12803574061735},
        {93.97444329561718, 81.1165634958974},
        {93.11985547608485, 82.09619590588952},
        {92.25505601503419, 83.06682554286051},
        {91.38013974747147, 84.02834596633271},
        {90.49520261782487, 84.98065173475631},
        {89.600341669423, 85.9236384170723},
        {88.695655033853, 86.85720260416443},
        {87.78124192019934, 87.78124192019932},
        {86.85720260416446, 88.69565503385297},
        {85.92363841707231, 89.60034166942299},
        {84.98065173475632, 90.49520261782487},
        {84.02834596633275, 91.38013974747146},
        {83.06682554286054, 92.25505601503417},
        {82.09619590588954, 93.11985547608484},
        {81.11656349589742, 93.97444329561715},
        {80.12803574061736, 94.81872575844639},
        {79.1307210432575, 95.65261027948631},
        {78.12472877061302, 96.47600541390212},
        {77.11016924107318, 97.28882086713853},
        {76.08715371252346, 98.09096750482152},
        {75.05579437014484, 98.88235736253286},
        {74.01620431411163, 99.66290365545649},
        {72.96849754718856, 100.43252078789547},
        {71.91278896222921, 101.19112436265843},
        {70.84919432957668, 101.9386311903147},
        {69.77783028436805, 102.67495929831699},
        {68.69881431374408, 103.40002793999057},
        {67.61226474396533, 104.11375760338814},
        {66.51830072743644, 104.8160700200091},
        {65.41704222963966, 105.5068881733826},
        {64.30861001597933, 106.18613630751334},
        {63.1931256385386, 106.85373993518895},
        {62.0707114227498, 107.5096258461485},
        {60.94149045398015, 108.15372211511063},
        {59.80558656403404, 108.78595810966117},
        {58.66312431757348, 109.40626449799865},
        {57.51422899845817, 110.0145732565374},
        {56.35902659600674, 110.610817677367},
        {55.19764379118055, 111.19493237556767},
        {54.03020794269178, 111.76685329638036},
        {52.856847073036946, 112.32651772223113},
        {51.67768985445805, 112.87386427960888},
        {50.49286559483197, 113.40883294579552},
        {49.30250422349054, 113.93136505544827},
        {48.10673627697227, 114.4414033070329},
        {46.905692884707605, 114.93889176910751},
        {45.699505754639, 115.42377588645607},
        {44.48830715877778, 115.89600248607096},
        {43.272229918698955, 116.35551978298398},
        {42.05140739097583, 116.80227738594527},
        {40.825973452556, 117.23622630294919},
        {39.59606248608019, 117.65731894660676},
        {38.36180936514571, 118.0655091393644},
        {37.123349439516076, 118.46075211856754},
        {35.88081852027825, 118.84300454136954},
        {34.6343528649496, 119.21222448948463},
        {33.384089162535616, 119.5683714737848},
        {32.130164518540475, 119.91140643873977},
        {30.872716439931835, 120.24129176670003},
        {29.611882820061663, 120.55799128202185},
        {28.34780192354468, 120.86147025503453},
        {27.08061237109609, 121.15169540584876},
        {25.810453124330248, 121.42863490800623},
        {24.53746347052195, 121.69225839196972},
        {23.26178300733203, 121.94253694845337},
        {21.983551627498883, 122.17944313159317},
        {20.70290950349768, 122.4029509619564},
        {19.419997072168826, 122.61303592939079},
        {18.134955019317562, 122.80967499571217},
        {16.847924264286036, 122.99284659723104},
        {15.559045944500081, 123.1625306471171},
        {14.2684613999918, 123.31870853760215},
        {12.976312157900063, 123.46136314202047},
        {11.682739916950467, 123.59047881668722},
        {10.387886531916465, 123.70604140261361},
        {9.091893998063398, 123.8080382270598},
        {7.794904435577108, 123.8964581049246},
        {6.4970600739788695, 123.97129133997205},
        {5.198503236528307, 124.03252972589459},
        {3.899376324616051, 124.08016654721311},
        {2.5998218021478254, 124.11419658001336},
        {1.2999821799216642, 124.13461609251871},
        {3.516640299702716e-14, 124.14142284549956},
        {-1.2999821799216216, 124.13461609251871},
        {-2.599821802147782, 124.11419658001336},
        {-3.899376324616009, 124.08016654721311},
        {-5.198503236528263, 124.03252972589459},
        {-6.497060073978826, 123.97129133997205},
        {-7.794904435577065, 123.8964581049246},
        {-9.091893998063354, 123.8080382270598},
        {-10.387886531916424, 123.70604140261361},
        {-11.682739916950425, 123.59047881668722},
        {-12.976312157900022, 123.4613631420205},
        {-14.268461399991757, 123.31870853760215},
        {-15.55904594450004, 123.1625306471171},
        {-16.847924264285993, 122.99284659723104},
        {-18.134955019317495, 122.80967499571217},
        {-19.419997072168783, 122.61303592939079},
        {-20.702909503497636, 122.40295096195642},
        {-21.983551627498848, 122.1794431315932},
        {-23.261783007331985, 121.9425369484534},
        {-24.537463470521907, 121.69225839196972},
        {-25.810453124330202, 121.42863490800625},
        {-27.08061237109605, 121.15169540584878},
        {-28.347801923544637, 120.86147025503455},
        {-29.611882820061624, 120.55799128202186},
        {-30.872716439931793, 120.24129176670004},
        {-32.13016451854043, 119.91140643873979},
        {-33.384089162535574, 119.56837147378481},
        {-34.634352864949555, 119.21222448948464},
        {-35.880818520278176, 118.84300454136955},
        {-37.12334943951601, 118.46075211856756},
        {-38.36180936514567, 118.06550913936441},
        {-39.59606248608015, 117.6573189466068},
        {-40.825973452555964, 117.23622630294919},
        {-42.0514073909758, 116.80227738594529},
        {-43.27222991869892, 116.35551978298399},
        {-44.488307158777744, 115.89600248607096},
        {-45.69950575463896, 115.42377588645608},
        {-46.90569288470756, 114.93889176910753},
        {-48.106736276972235, 114.44140330703293},
        {-49.302504223490494, 113.93136505544828},
        {-50.492865594831926, 113.40883294579552},
        {-51.67768985445801, 112.8738642796089},
        {-52.85684707303688, 112.32651772223117},
        {-54.03020794269173, 111.76685329638036},
        {-55.19764379118051, 111.19493237556769},
        {-56.35902659600673, 110.61081767736702},
        {-57.514228998458144, 110.0145732565374},
        {-58.66312431757347, 109.40626449799865},
        {-59.805586564034, 108.78595810966118},
        {-60.94149045398013, 108.15372211511063},
        {-62.07071142274975, 107.5096258461485},
        {-63.19312563853853, 106.85373993518898},
        {-64.3086100159793, 106.18613630751337},
        {-65.41704222963959, 105.50688817338263},
        {-66.5183007274364, 104.81607002000912},
        {-67.61226474396528, 104.11375760338818},
        {-68.69881431374405, 103.4000279399906},
        {-69.77783028436802, 102.674959298317},
        {-70.84919432957666, 101.93863119031471},
        {-71.91278896222917, 101.19112436265846},
        {-72.96849754718853, 100.4325207878955},
        {-74.01620431411159, 99.66290365545653},
        {-75.05579437014484, 98.88235736253287},
        {-76.08715371252342, 98.09096750482153},
        {-77.11016924107312, 97.28882086713858},
        {-78.12472877061299, 96.47600541390214},
        {-79.13072104325744, 95.65261027948634},
        {-80.12803574061734, 94.81872575844643},
        {-81.11656349589737, 93.97444329561718},
        {-82.0961959058895, 93.11985547608485},
        {-83.0668255428605, 92.2550560150342},
        {-84.02834596633271, 91.38013974747147},
        {-84.9806517347563, 90.4952026178249},
        {-85.92363841707228, 89.600341669423},
        {-86.85720260416441, 88.69565503385301},
        {-87.78124192019932, 87.78124192019934},
        {-88.69565503385296, 86.85720260416447},
        {-89.60034166942299, 85.9236384170723},
        {-90.49520261782484, 84.98065173475635},
        {-91.38013974747142, 84.02834596633276},
        {-92.25505601503417, 83.06682554286056},
        {-93.11985547608481, 82.09619590588957},
        {-93.97444329561715, 81.11656349589742},
        {-94.81872575844638, 80.12803574061739},
        {-95.65261027948631, 79.1307210432575},
        {-96.47600541390209, 78.12472877061305},
        {-97.28882086713853, 77.1101692410732},
        {-98.09096750482149, 76.08715371252349},
        {-98.88235736253283, 75.05579437014484},
        {-99.66290365545649, 74.01620431411165},
        {-100.43252078789547, 72.96849754718856},
        {-101.19112436265843, 71.91278896222923},
        {-101.93863119031465, 70.84919432957673},
        {-102.67495929831695, 69.77783028436808},
        {-103.40002793999055, 68.69881431374411},
        {-104.11375760338814, 67.61226474396534},
        {-104.81607002000908, 66.51830072743647},
        {-105.50688817338259, 65.41704222963966},
        {-106.18613630751332, 64.30861001597937},
        {-106.85373993518895, 63.1931256385386},
        {-107.50962584614848, 62.07071142274982},
        {-108.15372211511063, 60.94149045398015},
        {-108.78595810966115, 59.805586564034066},
        {-109.40626449799865, 58.663124317573484},
        {-110.01457325653737, 57.514228998458215},
        {-110.610817677367, 56.35902659600675},
        {-111.19493237556765, 55.19764379118059},
        {-111.76685329638035, 54.0302079426918},
        {-112.32651772223113, 52.85684707303696},
        {-112.87386427960887, 51.67768985445807},
        {-113.40883294579551, 50.492865594831976},
        {-113.93136505544824, 49.30250422349058},
        {-114.44140330703289, 48.10673627697228},
        {-114.9388917691075, 46.90569288470763},
        {-115.42377588645607, 45.69950575463901},
        {-115.89600248607093, 44.48830715877782},
        {-116.35551978298398, 43.27222991869897},
        {-116.80227738594526, 42.05140739097587},
        {-117.23622630294919, 40.825973452556006},
        {-117.65731894660676, 39.59606248608023},
        {-118.0655091393644, 38.36180936514572},
        {-118.46075211856754, 37.12334943951609},
        {-118.84300454136952, 35.88081852027828},
        {-119.21222448948463, 34.6343528649496},
        {-119.56837147378477, 33.38408916253565},
        {-119.91140643873977, 32.13016451854049},
        {-120.2412917667, 30.872716439931867},
        {-120.55799128202185, 29.611882820061666},
        {-120.86147025503453, 28.347801923544715},
        {-121.15169540584876, 27.0806123710961},
        {-121.42863490800623, 25.81045312433028},
        {-121.6922583919697, 24.537463470521956},
        {-121.94253694845337, 23.261783007332063},
        {-122.17944313159317, 21.983551627498894},
        {-122.4029509619564, 20.70290950349771},
        {-122.61303592939078, 19.41999707216889},
        {-122.80967499571214, 18.13495501931757},
        {-122.99284659723101, 16.84792426428607},
        {-123.1625306471171, 15.559045944500088},
        {-123.31870853760212, 14.268461399991834},
        {-123.46136314202047, 12.97631215790007},
        {-123.59047881668722, 11.682739916950505},
        {-123.70604140261361, 10.387886531916472},
        {-123.8080382270598, 9.091893998063432},
        {-123.8964581049246, 7.7949044355771155},
        {-123.97129133997205, 6.497060073978905},
        {-124.03252972589459, 5.198503236528314},
        {-124.08016654721311, 3.8993763246160866},
        {-124.11419658001336, 2.5998218021478325},
        {-124.13461609251871, 1.2999821799216997},
        {-124.14142284549956, 7.033280599405433e-14},
        {-124.13461609251871, -1.2999821799216138},
        {-124.11419658001336, -2.5998218021477473},
        {-124.08016654721311, -3.8993763246160014},
        {-124.03252972589459, -5.198503236528229},
        {-123.97129133997205, -6.497060073978819},
        {-123.8964581049246, -7.79490443557703},
        {-123.8080382270598, -9.091893998063346},
        {-123.70604140261361, -10.387886531916386},
        {-123.59047881668722, -11.682739916950418},
        {-123.4613631420205, -12.976312157899985},
        {-123.31870853760215, -14.268461399991748},
        {-123.1625306471171, -15.559045944500003},
        {-122.99284659723104, -16.847924264285933},
        {-122.80967499571217, -18.134955019317484},
        {-122.61303592939079, -19.41999707216875},
        {-122.40295096195642, -20.70290950349763},
        {-122.1794431315932, -21.98355162749881},
        {-121.9425369484534, -23.261783007331974},
        {-121.69225839196972, -24.537463470521875},
        {-121.42863490800625, -25.8104531243302},
        {-121.1516954058488, -27.080612371096013},
        {-120.86147025503455, -28.347801923544633},
        {-120.55799128202189, -29.611882820061584},
        {-120.24129176670004, -30.872716439931782},
        {-119.91140643873979, -32.130164518540404},
        {-119.56837147378481, -33.38408916253557},
        {-119.21222448948464, -34.63435286494952},
        {-118.84300454136955, -35.88081852027815},
        {-118.46075211856756, -37.123349439516005},
        {-118.06550913936444, -38.36180936514564},
        {-117.6573189466068, -39.59606248608015},
        {-117.2362263029492, -40.82597345255593},
        {-116.80227738594529, -42.05140739097578},
        {-116.355519782984, -43.27222991869888},
        {-115.89600248607096, -44.48830715877774},
        {-115.42377588645608, -45.69950575463893},
        {-114.93889176910753, -46.90569288470756},
        {-114.44140330703293, -48.10673627697221},
        {-113.93136505544828, -49.30250422349049},
        {-113.40883294579555, -50.492865594831905},
        {-112.8738642796089, -51.677689854457995},
        {-112.32651772223117, -52.85684707303688},
        {-111.76685329638039, -54.03020794269168},
        {-111.19493237556769, -55.1976437911805},
        {-110.61081767736704, -56.359026596006665},
        {-110.0145732565374, -57.51422899845813},
        {-109.40626449799869, -58.66312431757341},
        {-108.78595810966118, -59.805586564033995},
        {-108.15372211511065, -60.94149045398007},
        {-107.50962584614851, -62.070711422749746},
        {-106.85373993518898, -63.19312563853853},
        {-106.18613630751338, -64.3086100159793},
        {-105.50688817338266, -65.41704222963959},
        {-104.81607002000912, -66.5183007274364},
        {-104.11375760338818, -67.61226474396528},
        {-103.40002793999064, -68.698814313744},
        {-102.674959298317, -69.777830284368},
        {-101.93863119031475, -70.8491943295766},
        {-101.19112436265846, -71.91278896222916},
        {-100.43252078789553, -72.96849754718849},
        {-99.66290365545653, -74.01620431411159},
        {-98.88235736253289, -75.05579437014478},
        {-98.09096750482153, -76.08715371252342},
        {-97.28882086713858, -77.11016924107312},
        {-96.47600541390217, -78.12472877061299},
        {-95.65261027948637, -79.13072104325742},
        {-94.81872575844643, -80.12803574061734},
        {-93.9744432956172, -81.11656349589734},
        {-93.11985547608487, -82.0961959058895},
        {-92.2550560150342, -83.0668255428605},
        {-91.38013974747153, -84.02834596633268},
        {-90.49520261782493, -84.9806517347563},
        {-89.60034166942305, -85.92363841707224},
        {-88.69565503385303, -86.8572026041644},
        {-87.78124192019938, -87.78124192019929},
        {-86.85720260416447, -88.69565503385296},
        {-85.92363841707235, -89.60034166942296},
        {-84.98065173475635, -90.49520261782484},
        {-84.02834596633278, -91.38013974747142},
        {-83.06682554286057, -92.25505601503414},
        {-82.09619590588957, -93.1198554760848},
        {-81.11656349589742, -93.97444329561715},
        {-80.12803574061736, -94.81872575844639},
        {-79.13072104325752, -95.65261027948625},
        {-78.12472877061306, -96.47600541390209},
        {-77.1101692410732, -97.28882086713851},
        {-76.08715371252353, -98.09096750482146},
        {-75.0557943701449, -98.88235736253282},
        {-74.01620431411166, -99.66290365545649},
        {-72.96849754718856, -100.43252078789547},
        {-71.91278896222927, -101.19112436265839},
        {-70.84919432957673, -101.93863119031465},
        {-69.7778302843681, -102.67495929831695},
        {-68.69881431374408, -103.40002793999057},
        {-67.6122647439654, -104.11375760338811},
        {-66.51830072743648, -104.81607002000906},
        {-65.41704222963968, -105.50688817338259},
        {-64.30861001597933, -106.18613630751334},
        {-63.19312563853866, -106.85373993518894},
        {-62.07071142274984, -107.50962584614848},
        {-60.94149045398016, -108.15372211511063},
        {-59.80558656403412, -108.78595810966111},
        {-58.66312431757354, -109.40626449799862},
        {-57.514228998458215, -110.01457325653737},
        {-56.35902659600675, -110.610817677367},
        {-55.197643791180646, -111.19493237556763},
        {-54.03020794269181, -111.76685329638035},
        {-52.85684707303697, -112.32651772223113},
        {-51.67768985445804, -112.87386427960888},
        {-50.49286559483203, -113.4088329457955},
        {-49.30250422349058, -113.93136505544824},
        {-48.10673627697229, -114.44140330703289},
        {-46.90569288470769, -114.93889176910749},
        {-45.69950575463906, -115.42377588645604},
        {-44.48830715877783, -115.89600248607093},
        {-43.272229918698976, -116.35551978298398},
        {-42.051407390975925, -116.80227738594523},
        {-40.82597345255606, -117.23622630294916},
        {-39.596062486080235, -117.65731894660676},
        {-38.361809365145724, -118.0655091393644},
        {-37.12334943951614, -118.4607521185675},
        {-35.88081852027829, -118.84300454136952},
        {-34.63435286494961, -119.21222448948463},
        {-33.3840891625356, -119.5683714737848},
        {-32.130164518540546, -119.91140643873976},
        {-30.872716439931875, -120.2412917667},
        {-29.611882820061677, -120.55799128202185},
        {-28.347801923544775, -120.86147025503452},
        {-27.08061237109616, -121.15169540584874},
        {-25.810453124330284, -121.42863490800623},
        {-24.537463470521963, -121.6922583919697},
        {-23.261783007332124, -121.94253694845337},
        {-21.983551627498958, -122.17944313159316},
        {-20.702909503497718, -122.4029509619564},
        {-19.41999707216884, -122.61303592939078},
        {-18.134955019317633, -122.80967499571214},
        {-16.84792426428608, -122.99284659723101},
        {-15.559045944500095, -123.1625306471171},
        {-14.268461399991788, -123.31870853760215},
        {-12.976312157900132, -123.46136314202047},
        {-11.68273991695051, -123.59047881668722},
        {-10.387886531916482, -123.70604140261358},
        {-9.091893998063494, -123.80803822705978},
        {-7.7949044355771795, -123.8964581049246},
        {-6.497060073978912, -123.97129133997205},
        {-5.198503236528322, -124.03252972589459},
        {-3.8993763246161497, -124.08016654721311},
        {-2.5998218021478956, -124.11419658001336},
        {-1.2999821799217073, -124.13461609251871},
        {-2.2804409419400872e-14, -124.14142284549956},
        {1.2999821799215514, -124.13461609251871},
        {2.5998218021477393, -124.11419658001336},
        {3.8993763246159934, -124.08016654721311},
        {5.198503236528277, -124.03252972589459},
        {6.497060073978756, -123.97129133997205},
        {7.794904435577022, -123.8964581049246},
        {9.09189399806334, -123.8080382270598},
        {10.387886531916326, -123.70604140261361},
        {11.682739916950355, -123.59047881668724},
        {12.97631215789998, -123.4613631420205},
        {14.268461399991741, -123.31870853760215},
        {15.559045944499939, -123.16253064711711},
        {16.847924264285922, -122.99284659723104},
        {18.13495501931748, -122.80967499571217},
        {19.419997072168794, -122.61303592939079},
        {20.70290950349757, -122.40295096195644},
        {21.9835516274988, -122.1794431315932},
        {23.26178300733197, -121.9425369484534},
        {24.53746347052192, -121.69225839196972},
        {25.810453124330138, -121.42863490800627},
        {27.080612371096002, -121.1516954058488},
        {28.347801923544623, -120.86147025503455},
        {29.611882820061524, -120.5579912820219},
        {30.872716439931725, -120.24129176670006},
        {32.1301645185404, -119.91140643873979},
        {33.38408916253557, -119.56837147378481},
        {34.634352864949456, -119.21222448948467},
        {35.88081852027814, -118.84300454136955},
        {37.123349439516, -118.46075211856757},
        {38.36180936514568, -118.06550913936441},
        {39.59606248608008, -117.65731894660681},
        {40.82597345255592, -117.2362263029492},
        {42.051407390975776, -116.80227738594529},
        {43.27222991869882, -116.35551978298402},
        {44.48830715877768, -115.89600248607098},
        {45.69950575463892, -115.42377588645608},
        {46.90569288470755, -114.93889176910753},
        {48.10673627697215, -114.44140330703296},
        {49.30250422349043, -113.93136505544831},
        {50.49286559483189, -113.40883294579555},
        {51.677689854457995, -112.8738642796089},
        {52.85684707303683, -112.32651772223119},
        {54.03020794269167, -111.76685329638039},
        {55.1976437911805, -111.19493237556769},
        {56.359026596006714, -110.61081767736702},
        {57.51422899845807, -110.01457325653745},
        {58.663124317573406, -109.40626449799869},
        {59.80558656403398, -108.78595810966118},
        {60.94149045398002, -108.1537221151107},
        {62.070711422749696, -107.50962584614854},
        {63.19312563853851, -106.85373993518898},
        {64.3086100159793, -106.18613630751338},
        {65.41704222963953, -105.50688817338268},
        {66.51830072743634, -104.81607002000918},
        {67.61226474396527, -104.11375760338818},
        {68.69881431374405, -103.40002793999061},
        {69.77783028436795, -102.67495929831705},
        {70.8491943295766, -101.93863119031475},
        {71.91278896222916, -101.19112436265846},
        {72.96849754718852, -100.43252078789551},
        {74.01620431411153, -99.66290365545656},
        {75.05579437014478, -98.8823573625329},
        {76.0871537125234, -98.09096750482155},
        {77.11016924107307, -97.28882086713863},
        {78.12472877061293, -96.47600541390219},
        {79.13072104325742, -95.65261027948637},
        {80.12803574061734, -94.81872575844643},
        {81.11656349589731, -93.97444329561726},
        {82.09619590588945, -93.11985547608491},
        {83.06682554286049, -92.25505601503423},
        {84.02834596633271, -91.38013974747149},
        {84.98065173475624, -90.49520261782494},
        {85.92363841707223, -89.60034166942306},
        {86.8572026041644, -88.69565503385303},
        {87.7812419201993, -87.78124192019934},
        {88.69565503385292, -86.85720260416451},
        {89.60034166942296, -85.92363841707235},
        {90.49520261782483, -84.98065173475635},
        {91.38013974747139, -84.02834596633282},
        {92.25505601503411, -83.06682554286061},
        {93.1198554760848, -82.09619590588957},
        {93.97444329561715, -81.11656349589744},
        {94.81872575844633, -80.12803574061745},
        {95.65261027948625, -79.13072104325755},
        {96.47600541390209, -78.12472877061306},
        {97.28882086713851, -77.11016924107322},
        {98.09096750482146, -76.08715371252353},
        {98.8823573625328, -75.0557943701449},
        {99.66290365545647, -74.01620431411166},
        {100.43252078789547, -72.96849754718858},
        {101.19112436265839, -71.91278896222927},
        {101.93863119031465, -70.84919432957673},
        {102.67495929831695, -69.7778302843681},
        {103.40002793999051, -68.69881431374418},
        {104.11375760338811, -67.6122647439654},
        {104.81607002000906, -66.51830072743648},
        {105.50688817338259, -65.41704222963968},
        {106.18613630751328, -64.30861001597943},
        {106.85373993518894, -63.19312563853867},
        {107.50962584614847, -62.07071142274984},
        {108.1537221151106, -60.94149045398016},
        {108.78595810966111, -59.80558656403413},
        {109.40626449799862, -58.66312431757355},
        {110.01457325653737, -57.51422899845822},
        {110.61081767736694, -56.35902659600686},
        {111.19493237556763, -55.197643791180646},
        {111.76685329638032, -54.03020794269182},
        {112.32651772223113, -52.856847073036974},
        {112.87386427960882, -51.677689854458144},
        {113.4088329457955, -50.49286559483203},
        {113.93136505544824, -49.30250422349059},
        {114.44140330703289, -48.10673627697229},
        {114.93889176910749, -46.9056928847077},
        {115.42377588645604, -45.69950575463908},
        {115.89600248607091, -44.488307158777836},
        {116.35551978298398, -43.27222991869898},
        {116.80227738594523, -42.05140739097593},
        {117.23622630294915, -40.82597345255607},
        {117.65731894660676, -39.596062486080235},
        {118.06550913936435, -38.36180936514584},
        {118.4607521185675, -37.12334943951615},
        {118.84300454136952, -35.8808185202783},
        {119.21222448948463, -34.63435286494962},
        {119.56837147378477, -33.38408916253571},
        {119.91140643873976, -32.13016451854055},
        {120.2412917667, -30.872716439931885},
        {120.55799128202185, -29.61188282006168},
        {120.86147025503452, -28.347801923544782},
        {121.15169540584874, -27.08061237109617},
        {121.42863490800623, -25.810453124330298},
        {121.6922583919697, -24.53746347052197},
        {121.94253694845337, -23.26178300733213},
        {122.17944313159316, -21.98355162749896},
        {122.4029509619564, -20.702909503497725},
        {122.61303592939078, -19.419997072168957},
        {122.80967499571214, -18.13495501931764},
        {122.99284659723101, -16.847924264286085},
        {123.1625306471171, -15.5590459445001},
        {123.31870853760212, -14.268461399991905},
        {123.46136314202047, -12.976312157900141},
        {123.59047881668722, -11.682739916950519},
        {123.70604140261358, -10.387886531916486},
        {123.80803822705978, -9.091893998063503},
        {123.8964581049246, -7.794904435577187},
        {123.97129133997205, -6.497060073978919},
        {124.03252972589459, -5.198503236528329},
        {124.08016654721311, -3.8993763246161572},
        {124.11419658001336, -2.5998218021479027},
        {124.13461609251871, -1.2999821799217146},
    };

	ctrlTrajForward = {
		{0.9999999843805264, 0.0, 0.0001767454297266162, 0.0},
		{0.9980237298390068, 0.0, -0.06283816259437569, 0.0},
		{0.9921162917583065, 0.0, -0.12532064326257972, 0.0},
		{0.982286628672931, 0.0, -0.18738457548679752, 0.0},
		{0.9685833734783976, 0.0, -0.2486890601156532, 0.0},
		{0.9510564496595229, 0.0, -0.30901719945825573, 0.0},
		{0.9297765055251489, 0.0, -0.3681245030875327, 0.0},
		{0.9048270469824131, 0.0, -0.42577930321832913, 0.0},
		{0.876306681497577, 0.0, -0.4817536714574204, 0.0},
		{0.8443279251378263, 0.0, -0.5358267955528663, 0.0},
		{0.8090169944600089, 0.0, -0.5877852521753961, 0.0},
		{0.7705132427578505, 0.0, -0.637423989770374, 0.0},
		{0.7289686274245416, 0.0, -0.6845471059253555, 0.0},
		{0.6845471059283893, -0.0, -0.7289686274216927, 0.0},
		{0.6374239897486049, -0.0, -0.7705132427758593, 0.0},
		{0.5877852522925443, -0.0, -0.8090169943748957, 0.0},
		{0.5358267949789639, -0.0, -0.8443279255020358, 0.0},
		{0.481753674101728, -0.0, -0.8763066800438566, 0.0},
		{0.4257792915650682, -0.0, -0.9048270524660215, 0.0},
		{0.3681245526846789, -0.0, -0.929776485888251, 0.0},
		{0.30901699437494706, -0.0, -0.9510565162951536, 0.0},
		{0.24868988716485496, -0.0, -0.9685831611286311, 0.0},
		{0.1873813145857243, -0.0, -0.9822872507286887, 0.0},
		{0.12533323356430404, -0.0, -0.9921147013144779, 0.0},
		{0.06279051952931308, -0.0, -0.9980267284282716, 0.0},
		{6.123233995736766e-17, -0.0, 1.0, 0.0},
		{0.06279051952931308, -0.0, 0.9980267284282716, 0.0},
		{0.12533323356430448, -0.0, 0.9921147013144778, 0.0},
		{0.18738131458572474, -0.0, 0.9822872507286886, 0.0},
		{0.24868988716485455, -0.0, 0.9685831611286312, 0.0},
		{0.3090169943749494, -0.0, 0.951056516295153, 0.0},
		{0.36812455268467315, -0.0, 0.9297764858882533, 0.0},
		{0.4257792915650857, -0.0, 0.9048270524660134, 0.0},
		{0.4817536741016813, -0.0, 0.8763066800438822, 0.0},
		{0.5358267949790714, -0.0, 0.8443279255019677, 0.0},
		{0.5877852522923839, -0.0, 0.8090169943750123, 0.0},
		{0.6374239897483734, -0.0, 0.7705132427760508, 0.0},
		{0.6845471059320221, -0.0, 0.7289686274182812, 0.0},
		{0.7289686274021466, 0.0, 0.6845471059492038, 0.0},
		{0.7705132428680344, 0.0, 0.6374239896371845, 0.0},
		{0.8090169939754435, 0.0, 0.5877852528423432, 0.0},
		{0.8443279271188966, 0.0, 0.5358267924311991, 0.0},
		{0.876306673839364, 0.0, 0.4817536853876579, 0.0},
		{0.9048270751783986, 0.0, 0.42577924329880706, 0.0},
		{0.9297764065068237, 0.0, 0.3681247531792147, 0.0},
		{0.9510567801567453, 0.0, 0.30901618229129063, 0.0},
		{0.9685823355466857, 0.0, 0.2486931025720808, 0.0},
		{0.9822896285857262, 0.0, 0.18736884899287842, 0.0},
		{0.9921087171094368, 0.0, 0.12538059433368243, 0.0},
		{0.9980378107769778, 0.0, 0.06261412188554114, 0.0},
		{0.9999999843805264, 0.0, 0.0001767454297266162, 0.0},
		{0.9980237298390068, 0.0, -0.06283816259437569, 0.0},
		{0.9921162917583065, 0.0, -0.12532064326257972, 0.0},
		{0.982286628672931, 0.0, -0.18738457548679752, 0.0},
		{0.9685833734783976, 0.0, -0.2486890601156532, 0.0},
		{0.9510564496595229, 0.0, -0.30901719945825573, 0.0},
		{0.9297765055251489, 0.0, -0.3681245030875327, 0.0},
		{0.9048270469824131, 0.0, -0.42577930321832913, 0.0},
		{0.876306681497577, 0.0, -0.4817536714574204, 0.0},
		{0.8443279251378263, 0.0, -0.5358267955528663, 0.0},
		{0.8090169944600089, 0.0, -0.5877852521753961, 0.0},
		{0.7705132427578505, 0.0, -0.637423989770374, 0.0},
		{0.7289686274245416, 0.0, -0.6845471059253555, 0.0},
		{0.6845471059283893, -0.0, -0.7289686274216927, 0.0},
		{0.6374239897486049, -0.0, -0.7705132427758593, 0.0},
		{0.5877852522925443, -0.0, -0.8090169943748957, 0.0},
		{0.5358267949789639, -0.0, -0.8443279255020358, 0.0},
		{0.481753674101728, -0.0, -0.8763066800438566, 0.0},
		{0.4257792915650682, -0.0, -0.9048270524660215, 0.0},
		{0.3681245526846789, -0.0, -0.929776485888251, 0.0},
		{0.30901699437494706, -0.0, -0.9510565162951536, 0.0},
		{0.24868988716485496, -0.0, -0.9685831611286311, 0.0},
		{0.1873813145857243, -0.0, -0.9822872507286887, 0.0},
		{0.12533323356430404, -0.0, -0.9921147013144779, 0.0},
		{0.06279051952931308, -0.0, -0.9980267284282716, 0.0},
		{6.123233995736766e-17, -0.0, 1.0, 0.0},
		{0.06279051952931308, -0.0, 0.9980267284282716, 0.0},
		{0.12533323356430448, -0.0, 0.9921147013144778, 0.0},
		{0.18738131458572474, -0.0, 0.9822872507286886, 0.0},
		{0.24868988716485455, -0.0, 0.9685831611286312, 0.0},
		{0.3090169943749494, -0.0, 0.951056516295153, 0.0},
		{0.36812455268467315, -0.0, 0.9297764858882533, 0.0},
		{0.4257792915650857, -0.0, 0.9048270524660134, 0.0},
		{0.4817536741016813, -0.0, 0.8763066800438822, 0.0},
		{0.5358267949790714, -0.0, 0.8443279255019677, 0.0},
		{0.5877852522923839, -0.0, 0.8090169943750123, 0.0},
		{0.6374239897483734, -0.0, 0.7705132427760508, 0.0},
		{0.6845471059320221, -0.0, 0.7289686274182812, 0.0},
		{0.7289686274021466, 0.0, 0.6845471059492038, 0.0},
		{0.7705132428680344, 0.0, 0.6374239896371845, 0.0},
		{0.8090169939754435, 0.0, 0.5877852528423432, 0.0},
		{0.8443279271188966, 0.0, 0.5358267924311991, 0.0},
		{0.876306673839364, 0.0, 0.4817536853876579, 0.0},
		{0.9048270751783986, 0.0, 0.42577924329880706, 0.0},
		{0.9297764065068237, 0.0, 0.3681247531792147, 0.0},
		{0.9510567801567453, 0.0, 0.30901618229129063, 0.0},
		{0.9685823355466857, 0.0, 0.2486931025720808, 0.0},
		{0.9822896285857262, 0.0, 0.18736884899287842, 0.0},
		{0.9921087171094368, 0.0, 0.12538059433368243, 0.0},
		{0.9980378107769778, 0.0, 0.06261412188554114, 0.0},
		{ 0.9999999843805264, 0.0, 0.0001767454297266162, 0.0 },
		{ 0.9980237298390068, 0.0, -0.06283816259437569, 0.0 },
		{ 0.9921162917583065, 0.0, -0.12532064326257972, 0.0 },
		{ 0.982286628672931, 0.0, -0.18738457548679752, 0.0 },
		{ 0.9685833734783976, 0.0, -0.2486890601156532, 0.0 },
		{ 0.9510564496595229, 0.0, -0.30901719945825573, 0.0 },
		{ 0.9297765055251489, 0.0, -0.3681245030875327, 0.0 },
		{ 0.9048270469824131, 0.0, -0.42577930321832913, 0.0 },
		{ 0.876306681497577, 0.0, -0.4817536714574204, 0.0 },
		{ 0.8443279251378263, 0.0, -0.5358267955528663, 0.0 },
		{ 0.8090169944600089, 0.0, -0.5877852521753961, 0.0 },
		{ 0.7705132427578505, 0.0, -0.637423989770374, 0.0 },
		{ 0.7289686274245416, 0.0, -0.6845471059253555, 0.0 },
		{ 0.6845471059283893, -0.0, -0.7289686274216927, 0.0 },
		{ 0.6374239897486049, -0.0, -0.7705132427758593, 0.0 },
		{ 0.5877852522925443, -0.0, -0.8090169943748957, 0.0 },
		{ 0.5358267949789639, -0.0, -0.8443279255020358, 0.0 },
		{ 0.481753674101728, -0.0, -0.8763066800438566, 0.0 },
		{ 0.4257792915650682, -0.0, -0.9048270524660215, 0.0 },
		{ 0.3681245526846789, -0.0, -0.929776485888251, 0.0 },
		{ 0.30901699437494706, -0.0, -0.9510565162951536, 0.0 },
		{ 0.24868988716485496, -0.0, -0.9685831611286311, 0.0 },
		{ 0.1873813145857243, -0.0, -0.9822872507286887, 0.0 },
		{ 0.12533323356430404, -0.0, -0.9921147013144779, 0.0 },
		{ 0.06279051952931308, -0.0, -0.9980267284282716, 0.0 },
		{ 6.123233995736766e-17, -0.0, 1.0, 0.0 },
		{ 0.06279051952931308, -0.0, 0.9980267284282716, 0.0 },
		{ 0.12533323356430448, -0.0, 0.9921147013144778, 0.0 },
		{ 0.18738131458572474, -0.0, 0.9822872507286886, 0.0 },
		{ 0.24868988716485455, -0.0, 0.9685831611286312, 0.0 },
		{ 0.3090169943749494, -0.0, 0.951056516295153, 0.0 },
		{ 0.36812455268467315, -0.0, 0.9297764858882533, 0.0 },
		{ 0.4257792915650857, -0.0, 0.9048270524660134, 0.0 },
		{ 0.4817536741016813, -0.0, 0.8763066800438822, 0.0 },
		{ 0.5358267949790714, -0.0, 0.8443279255019677, 0.0 },
		{ 0.5877852522923839, -0.0, 0.8090169943750123, 0.0 },
		{ 0.6374239897483734, -0.0, 0.7705132427760508, 0.0 },
		{ 0.6845471059320221, -0.0, 0.7289686274182812, 0.0 },
		{ 0.7289686274021466, 0.0, 0.6845471059492038, 0.0 },
		{ 0.7705132428680344, 0.0, 0.6374239896371845, 0.0 },
		{ 0.8090169939754435, 0.0, 0.5877852528423432, 0.0 },
		{ 0.8443279271188966, 0.0, 0.5358267924311991, 0.0 },
		{ 0.876306673839364, 0.0, 0.4817536853876579, 0.0 },
		{ 0.9048270751783986, 0.0, 0.42577924329880706, 0.0 },
		{ 0.9297764065068237, 0.0, 0.3681247531792147, 0.0 },
		{ 0.9510567801567453, 0.0, 0.30901618229129063, 0.0 },
		{ 0.9685823355466857, 0.0, 0.2486931025720808, 0.0 },
		{ 0.9822896285857262, 0.0, 0.18736884899287842, 0.0 },
		{ 0.9921087171094368, 0.0, 0.12538059433368243, 0.0 },
		{ 0.9980378107769778, 0.0, 0.06261412188554114, 0.0 }, { 0.9999999843805264, 0.0, 0.0001767454297266162, 0.0 },
        { 0.9980237298390068, 0.0, -0.06283816259437569, 0.0 },
        { 0.9921162917583065, 0.0, -0.12532064326257972, 0.0 },
        { 0.982286628672931, 0.0, -0.18738457548679752, 0.0 },
        { 0.9685833734783976, 0.0, -0.2486890601156532, 0.0 },
        { 0.9510564496595229, 0.0, -0.30901719945825573, 0.0 },
        { 0.9297765055251489, 0.0, -0.3681245030875327, 0.0 },
        { 0.9048270469824131, 0.0, -0.42577930321832913, 0.0 },
        { 0.876306681497577, 0.0, -0.4817536714574204, 0.0 },
        { 0.8443279251378263, 0.0, -0.5358267955528663, 0.0 },
        { 0.8090169944600089, 0.0, -0.5877852521753961, 0.0 },
        { 0.7705132427578505, 0.0, -0.637423989770374, 0.0 },
        { 0.7289686274245416, 0.0, -0.6845471059253555, 0.0 },
        { 0.6845471059283893, -0.0, -0.7289686274216927, 0.0 },
        { 0.6374239897486049, -0.0, -0.7705132427758593, 0.0 },
        { 0.5877852522925443, -0.0, -0.8090169943748957, 0.0 },
        { 0.5358267949789639, -0.0, -0.8443279255020358, 0.0 },
        { 0.481753674101728, -0.0, -0.8763066800438566, 0.0 },
        { 0.4257792915650682, -0.0, -0.9048270524660215, 0.0 },
        { 0.3681245526846789, -0.0, -0.929776485888251, 0.0 },
        { 0.30901699437494706, -0.0, -0.9510565162951536, 0.0 },
        { 0.24868988716485496, -0.0, -0.9685831611286311, 0.0 },
        { 0.1873813145857243, -0.0, -0.9822872507286887, 0.0 },
        { 0.12533323356430404, -0.0, -0.9921147013144779, 0.0 },
        { 0.06279051952931308, -0.0, -0.9980267284282716, 0.0 },
        { 6.123233995736766e-17, -0.0, 1.0, 0.0 },
        { 0.06279051952931308, -0.0, 0.9980267284282716, 0.0 },
        { 0.12533323356430448, -0.0, 0.9921147013144778, 0.0 },
        { 0.18738131458572474, -0.0, 0.9822872507286886, 0.0 },
        { 0.24868988716485455, -0.0, 0.9685831611286312, 0.0 },
        { 0.3090169943749494, -0.0, 0.951056516295153, 0.0 },
        { 0.36812455268467315, -0.0, 0.9297764858882533, 0.0 },
        { 0.4257792915650857, -0.0, 0.9048270524660134, 0.0 },
        { 0.4817536741016813, -0.0, 0.8763066800438822, 0.0 },
        { 0.5358267949790714, -0.0, 0.8443279255019677, 0.0 },
        { 0.5877852522923839, -0.0, 0.8090169943750123, 0.0 },
        { 0.6374239897483734, -0.0, 0.7705132427760508, 0.0 },
        { 0.6845471059320221, -0.0, 0.7289686274182812, 0.0 },
        { 0.7289686274021466, 0.0, 0.6845471059492038, 0.0 },
        { 0.7705132428680344, 0.0, 0.6374239896371845, 0.0 },
        { 0.8090169939754435, 0.0, 0.5877852528423432, 0.0 },
        { 0.8443279271188966, 0.0, 0.5358267924311991, 0.0 },
        { 0.876306673839364, 0.0, 0.4817536853876579, 0.0 },
        { 0.9048270751783986, 0.0, 0.42577924329880706, 0.0 },
        { 0.9297764065068237, 0.0, 0.3681247531792147, 0.0 },
        { 0.9510567801567453, 0.0, 0.30901618229129063, 0.0 },
        { 0.9685823355466857, 0.0, 0.2486931025720808, 0.0 },
        { 0.9822896285857262, 0.0, 0.18736884899287842, 0.0 },
        { 0.9921087171094368, 0.0, 0.12538059433368243, 0.0 },
        { 0.9980378107769778, 0.0, 0.06261412188554114, 0.0 },
        { 0.9999999843805264, 0.0, 0.0001767454297266162, 0.0 },
        { 0.9980237298390068, 0.0, -0.06283816259437569, 0.0 },
        { 0.9921162917583065, 0.0, -0.12532064326257972, 0.0 },
        { 0.982286628672931, 0.0, -0.18738457548679752, 0.0 },
        { 0.9685833734783976, 0.0, -0.2486890601156532, 0.0 },
        { 0.9510564496595229, 0.0, -0.30901719945825573, 0.0 },
        { 0.9297765055251489, 0.0, -0.3681245030875327, 0.0 },
        { 0.9048270469824131, 0.0, -0.42577930321832913, 0.0 },
        { 0.876306681497577, 0.0, -0.4817536714574204, 0.0 },
        { 0.8443279251378263, 0.0, -0.5358267955528663, 0.0 },
        { 0.8090169944600089, 0.0, -0.5877852521753961, 0.0 },
        { 0.7705132427578505, 0.0, -0.637423989770374, 0.0 },
        { 0.7289686274245416, 0.0, -0.6845471059253555, 0.0 },
        { 0.6845471059283893, -0.0, -0.7289686274216927, 0.0 },
        { 0.6374239897486049, -0.0, -0.7705132427758593, 0.0 },
        { 0.5877852522925443, -0.0, -0.8090169943748957, 0.0 },
        { 0.5358267949789639, -0.0, -0.8443279255020358, 0.0 },
        { 0.481753674101728, -0.0, -0.8763066800438566, 0.0 },
        { 0.4257792915650682, -0.0, -0.9048270524660215, 0.0 },
        { 0.3681245526846789, -0.0, -0.929776485888251, 0.0 },
        { 0.30901699437494706, -0.0, -0.9510565162951536, 0.0 },
        { 0.24868988716485496, -0.0, -0.9685831611286311, 0.0 },
        { 0.1873813145857243, -0.0, -0.9822872507286887, 0.0 },
        { 0.12533323356430404, -0.0, -0.9921147013144779, 0.0 },
        { 0.06279051952931308, -0.0, -0.9980267284282716, 0.0 },
        { 6.123233995736766e-17, -0.0, 1.0, 0.0 },
	};

	//Phase = std::vector<float>(numPhaseChannel * (totalKeys), 1.f);
	//Amplitude = std::vector<float>(numPhaseChannel * (totalKeys), 0.1f);
	


	//dummyIn = { -0.5249618,0.3652196,0.050114945,0.9987435,0.024682527,-0.0368779,0.044375744,-0.4721159,0.29933763,0.039552484,0.99921757,0.031707548,-0.039529156,0.050674677,-0.40563864,0.23342197,0.029805059,0.99955577,0.039886363,-0.039549403,0.056170072,-0.3253349,0.16469434,0.021180019,0.9997757,0.048182234,-0.041236583,0.06341911,-0.23143342,0.09592797,0.01337378,0.99991065,0.056340877,-0.041259814,0.06983314,-0.12269627,0.039731465,0.006319913,0.9999801,0.06524228,-0.0337179,0.07344013,0.0,0.0,8.940697e-08,1.0,0.07361776,-0.02383888,0.077381305,0.1375273,-0.027176056,-0.005564216,0.99998456,0.08251639,-0.016305635,0.084111996,0.29360315,-0.040792674,-0.010379107,0.9999462,0.093645506,-0.0081699705,0.09400122,0.46892047,-0.035759285,-0.0144901695,0.9998951,0.105190374,0.003020029,0.105233714,0.6627863,-0.017166466,-0.017846584,0.99984074,0.11631951,0.011155691,0.11685323,0.87698805,0.0007813573,-0.020312598,0.9997937,0.12852104,0.010768698,0.1289714,0.12910461,-0.035591125,0.00036621094,0.093496464,0.3045007,-0.061553467,0.9459118,-2.2200331e-05,2.530788e-06,-9.628723e-06,-0.45329285,12.832298,-1.3668518,0.003578581,0.31076595,-0.096991204,0.9455181,-3.0836613e-06,-3.6028127e-06,-9.071083e-06,-1.6724396,23.008118,-0.55895996,-0.03644597,0.31261733,-0.11246876,0.94249296,-0.0025533917,0.00011075399,0.00075471995,-3.057495,32.083015,0.848053,-0.07640294,0.313889,-0.12773779,0.93772036,-0.0059147947,0.0002433921,0.0017176751,-4.7408447,40.968422,2.9617004,-0.10628178,0.3144609,-0.13903569,0.9329993,-0.0103507005,0.0003801775,0.002961092,-7.4688416,53.424736,6.7285156,-0.34804773,0.2604817,-0.20679264,0.8764981,-0.017853437,0.0006289495,0.004978505,-10.3146515,59.101234,13.049667,-0.4967764,0.20400451,-0.2353148,0.8100756,-0.01644701,0.0005751248,0.004600319,-10.3146515,59.101234,13.049667,-0.4967764,0.20400451,-0.2353148,0.8100756,-0.01644701,0.0005751248,0.004600319,-9.224045,47.65798,3.895691,-0.13127023,0.28795117,-0.23886749,0.9180384,-0.014895381,-0.00092799164,0.0029188264,-23.269348,43.471672,-1.9860382,0.1478492,0.29163226,-0.69756913,0.6375646,-0.014768544,0.0036464375,0.006686166,-22.897903,13.592201,-10.790375,0.1461924,0.14464276,-0.71137834,0.672047,-0.061553814,-0.008013716,0.074284546,-23.44345,-11.840904,-13.01799,0.055584047,0.0302119,-0.6127838,0.7877143,-0.03441965,-0.0817943,0.38103595,-23.44345,-11.840904,-13.01799,0.055584047,0.0302119,-0.6127838,0.7877143,-0.03441965,-0.0817943,0.38103595,-3.3152466,48.240196,6.249649,-0.22177337,0.33379534,-0.08773682,0.91197574,-0.014194122,0.0019672606,0.005256643,11.196121,46.796013,12.309753,-0.32182357,0.25575602,0.50666165,0.75783414,-0.012966066,0.008701038,0.0037794127,20.078735,17.205536,8.3172455,-0.2951553,0.4792899,0.478033,0.67427677,0.0069677937,0.019547144,-0.021241395,23.06044,-7.3895874,14.50531,-0.35676333,0.5422009,0.32968175,0.68560064,-0.038426638,0.015416099,-0.43419617,23.06044,-7.3895874,14.50531,-0.35676333,0.5422009,0.32968175,0.68560064,-0.038426638,0.015416099,-0.43419617,-10.084747,-0.7055893,-3.2736511,0.093788065,0.34067336,-0.037543237,0.9347385,-2.083442e-05,2.058244e-06,-5.2852506e-06,-10.49205,-44.199303,1.3803864,-0.029644618,0.3070932,-0.07164339,0.9485159,0.031687,-0.006467104,0.051388256,-7.6237946,-86.89494,-4.6470947,0.045759097,0.32357132,-0.04032668,0.94423604,0.012050844,0.0032427176,0.033214267,-14.226334,-96.437584,13.414307,0.045759097,0.32357132,-0.04032668,0.94423604,0.08898066,-0.009495643,0.009319687,-14.226334,-96.437584,13.414307,0.045759097,0.32357132,-0.04032668,0.94423604,0.08898066,-0.009495643,0.009319687,10.321411,0.9135208,3.272461,0.16383849,0.25735775,-0.041340757,0.95142794,-1.0170393e-05,-2.438504e-06,-1.636467e-05,11.263809,-41.557896,13.704117,0.0019372636,0.24303852,-0.097756766,0.9650763,-0.0632962,-0.01206442,-0.28037715,16.368408,-84.297745,9.86702,0.04189016,0.23517151,-0.028839707,0.97062254,-0.19016287,-0.08371982,0.013856202,13.03653,-94.03786,28.70578,0.04189016,0.23517151,-0.028839707,0.97062254,-0.18569425,-0.007847124,0.07751545,13.03653,-94.03786,28.70578,0.04189016,0.23517151,-0.028839707,0.97062254,-0.18569425,-0.007847124,0.07751545,-0.46579292,-0.78616685,-0.16909796,-2.1233103,0.5937081,0.647611,1.24903,-2.9916744,-0.54313123,1.7403276,-0.6365925,-0.6620605,-0.4358837,-2.1070628,0.7115901,0.53905845,0.87582505,-3.1223345,-0.3220514,1.8286908,-0.7800078,-0.509032,-0.69145393,-2.0609167,0.8051946,0.4225367,0.5037407,-3.2005548,-0.056030456,1.8920282,-0.88479185,-0.32865044,-0.9378868,-1.9804652,0.88087004,0.2859074,0.13156423,-3.228958,0.23140678,1.9082749,-0.94189346,-0.11877778,-1.1715592,-1.8670785,0.93377715,0.12049462,-0.23332092,-3.214965,0.5229669,1.8758278,-0.9405904,0.11571294,-1.3847,-1.7306207,0.9521955,-0.07322357,-0.59821904,-3.1592572,0.79528934,1.7973675,-0.8677854,0.35251498,-1.5835589,-1.5703589,0.92433167,-0.2901494,-0.97032815,-3.0499558,1.0141115,1.6901269,-0.7248708,0.5610609,-1.7679161,-1.3852172,0.8366697,-0.51878196,-1.3280591,-2.8865643,1.1849209,1.5666616,-0.5278809,0.7189952,-1.933052,-1.1790262,0.6772067,-0.73704594,-1.657056,-2.681405,1.3276112,1.4291434,-0.31118622,0.8089544,-2.0766277,-0.95199466,0.46629885,-0.9034194,-1.9606686,-2.433053,1.4505528,1.278103,-0.08280067,0.8416452,-2.1967149,-0.70377773,0.23881045,-1.0070195,-2.2366197,-2.134478,1.5616405,1.1047102,0.13152619,0.82792073,-2.2887297,-0.43861833,0.0013227656,-1.0557903,-2.470366,-1.7998703,1.6583652,0.90994847,0.30473104,0.78896296,-2.350462,-0.16240768,-0.25338233,-1.0431032,-2.6532857,-1.4425114,1.7366314,0.6900943 };
	//dummyPhase = { -0.46579292,-0.78616685,-0.16909796,-2.1233103,0.5937081,0.647611,1.24903,-2.9916744,-0.54313123,1.7403276,-0.6365925,-0.6620605,-0.4358837,-2.1070628,0.7115901,0.53905845,0.87582505,-3.1223345,-0.3220514,1.8286908,-0.7800078,-0.509032,-0.69145393,-2.0609167,0.8051946,0.4225367,0.5037407,-3.2005548,-0.056030456,1.8920282,-0.88479185,-0.32865044,-0.9378868,-1.9804652,0.88087004,0.2859074,0.13156423,-3.228958,0.23140678,1.9082749,-0.94189346,-0.11877778,-1.1715592,-1.8670785,0.93377715,0.12049462,-0.23332092,-3.214965,0.5229669,1.8758278,-0.9405904,0.11571294,-1.3847,-1.7306207,0.9521955,-0.07322357,-0.59821904,-3.1592572,0.79528934,1.7973675,-0.8677854,0.35251498,-1.5835589,-1.5703589,0.92433167,-0.2901494,-0.97032815,-3.0499558,1.0141115,1.6901269,-0.7248708,0.5610609,-1.7679161,-1.3852172,0.8366697,-0.51878196,-1.3280591,-2.8865643,1.1849209,1.5666616,-0.5278809,0.7189952,-1.933052,-1.1790262,0.6772067,-0.73704594,-1.657056,-2.681405,1.3276112,1.4291434,-0.31118622,0.8089544,-2.0766277,-0.95199466,0.46629885,-0.9034194,-1.9606686,-2.433053,1.4505528,1.278103,-0.08280067,0.8416452,-2.1967149,-0.70377773,0.23881045,-1.0070195,-2.2366197,-2.134478,1.5616405,1.1047102,0.13152619,0.82792073,-2.2887297,-0.43861833,0.0013227656,-1.0557903,-2.470366,-1.7998703,1.6583652,0.90994847,0.30473104,0.78896296,-2.350462,-0.16240768,-0.25338233,-1.0431032,-2.6532857,-1.4425114,1.7366314,0.6900943 };

	meanIn = {
		-9.96921,-78.645325,0.056528147,0.8044563,0.12847732,0.6465221,1.1766946,-8.061361,-68.25563,0.04796116,0.8461919,0.12589402,0.72970545,1.1812031,-6.207762,-56.619255,0.039281774,0.8865724,0.1213748,0.81185085,1.1849443,-4.446136,-43.813343,0.030400198,0.9241334,0.11394971,0.88569,1.1874478,-2.8056223,-29.99028,0.02073568,0.95851606,0.105625324,0.94847447,1.188972,-1.3065239,-15.323643,0.010410322,0.98703974,0.09473686,0.99946886,1.189611,-1.6657866e-07,6.690206e-08,-5.940222e-11,1.0,0.08094699,1.036974,1.1895864,1.0981715,15.765864,-0.010194613,0.98702854,0.06669387,1.0603286,1.1891674,1.9549899,31.75298,-0.020305479,0.9581124,0.04892276,1.0676095,1.1880596,2.5312924,47.698048,-0.029776353,0.9228255,0.030274644,1.0565747,1.1861508,2.8408096,63.32398,-0.03857873,0.884468,0.0130403545,1.0271026,1.183657,2.89174,78.35558,-0.04667192,0.8429542,-0.0043105544,0.9798917,1.1802777,2.6927323,92.580605,-0.053891256,0.79922605,-0.020021593,0.9220041,1.1774817,-2.1452391e-07,88.42864,5.9775054e-08,0.9972861,0.0059563746,-0.003203388,-0.006653985,0.87362593,-0.47237557,-1.1941267e-09,-1.5374284e-09,-3.1395013e-09,-0.034337875,92.938324,-2.4385085,0.9950039,0.00044105135,0.003733368,-0.0006951925,0.9880472,-0.1038355,-1.4309248e-08,1.61835e-10,-1.3096024e-09,-0.044908386,107.964066,-4.0175905,0.98614013,0.0028169688,0.016122298,-0.0067463485,0.98239785,0.14471917,-0.00028468942,-2.5532606e-06,-0.0003796863,-0.33744252,149.3253,2.0752766,0.9860847,0.019415924,0.012981625,-0.022983097,0.7067245,0.68935245,-0.0011025784,-6.5311824e-06,-0.0019503105,-0.60274345,157.48683,10.03644,0.9836422,0.03741855,0.0143980095,-0.03565325,0.9793921,0.14417066,-0.001075592,9.658388e-06,-0.0011285861,-1.1171291,171.61702,12.1164465,0.953807,-0.008241565,0.024746282,-0.01141817,0.977808,0.14596257,-0.0012551995,-3.1353568e-06,-0.0013139229,-1.2388306,181.99062,13.686968,0.953807,-0.008241534,0.024746241,-0.010947784,0.9807472,0.12705727,-0.0016802668,-8.269997e-05,-2.2402142e-05,-1.5233328,207.47925,16.989037,0.953807,-0.008241535,0.024746247,-0.010947787,0.98074716,0.12705728,-0.0027258133,-0.00027002016,0.0028933038,8.9374,141.54576,-0.5896517,0.03851655,-0.20430474,0.92971665,0.9712396,0.056834143,-0.033027966,-0.00027280216,0.00017580959,-0.0018303057,22.119396,142.31699,-1.038314,-0.65265614,-0.35372943,-0.5807883,0.52233744,-0.7319871,-0.14903861,0.00088430924,0.00050405104,-0.0024438333,40.11024,117.10613,-6.172644,-0.8710886,-0.20649523,-0.3268059,-0.13240519,-0.6567031,0.6070772,0.0041823764,0.0027903786,-0.003696258,35.783638,95.647194,13.664945,-0.8207724,0.050654404,-0.33716285,-0.27678758,-0.67344874,0.36541852,0.0068649957,0.0006806571,-0.0013685452,-9.581116,141.49286,-0.8919276,0.04362837,0.31324866,-0.9001637,-0.96313685,0.050612062,-0.040258747,-0.0016707653,-0.00019092383,-0.0018779372,-22.653135,142.17969,-1.4378669,-0.63908327,0.41349268,0.5703684,-0.49012974,-0.75628304,-0.042197455,-0.002589636,-0.0007414069,-0.0021953103,-39.535007,116.13204,-2.889862,-0.8798989,0.19417712,0.30645657,0.1535922,-0.6021291,0.62631,-0.0049860994,-0.0027632832,-0.0006638172,-34.515415,96.45605,17.578188,-0.8516585,0.093475975,0.38817924,0.22317407,-0.6591806,0.43626457,-0.006791007,-0.0025288048,0.0011566192,4.8177757,88.11364,0.016811976,-0.06940568,-0.36567843,0.9220925,0.98202705,-0.17309886,0.004281542,7.271114e-10,-2.427552e-10,-4.846869e-09,15.047564,86.310455,0.061454486,0.9834561,-0.031402018,-0.08692672,-0.0019526117,-0.9051502,0.24464275,2.176644e-08,4.8550164e-09,-4.631992e-09,14.967132,49.10947,10.117103,0.98449963,0.002936319,-0.08032767,-0.044565383,-0.77193147,-0.5570633,-0.0076159006,-0.0010054949,-0.006302428,13.268235,19.686054,-11.116055,0.9399576,-0.02362253,-0.2754928,0.16535108,-0.67667556,0.6170795,-0.009819431,-0.0035345391,-0.007097196,16.577482,6.146561,1.2317901,-0.93893635,0.010354105,0.27733946,0.26298258,-0.14842235,0.8691142,-0.015823938,-0.003232487,-0.0032387292,22.110264,3.0249445,19.51407,-0.93893635,0.010353853,0.27733946,0.2629826,-0.14842246,0.8691142,-0.020217517,-0.0019949449,-0.0032426254,7.7809386,8.840593,-17.274317,0.26240548,-0.1993998,0.8653707,0.9389456,-0.019742481,-0.2791829,-0.009887235,-0.004766458,-0.003921323,15.292835,8.682663,-19.508196,0.262405,-0.19939993,0.86537075,0.9389457,-0.019742554,-0.2791825,-0.010719602,-0.0046510976,-0.0036592502,-4.8134975,88.05613,0.04772906,-0.07958565,0.3647891,-0.9216138,-0.97986925,-0.18481554,0.010583356,-3.569145e-09,-2.0229178e-09,-8.185977e-09,-15.020814,86.13092,0.15796036,0.98751134,0.02608154,0.03279027,0.0064791776,-0.8858102,0.3325784,1.3784317e-08,2.5893496e-09,-9.8467e-09,-14.754484,49.725792,13.826356,0.9867591,-0.020074211,0.02409659,-0.0029980019,-0.78513736,-0.52895635,0.0050742887,-5.8894413e-05,-0.0010100415,-14.868562,19.798052,-6.3377523,0.9849829,0.0058267047,0.0025808443,0.007547159,-0.6865948,0.6264699,0.007415122,0.004086805,-0.0071998877,-14.717598,6.0603566,6.197652,-0.98324794,-0.0016783505,-0.0070931166,-0.00013192547,-0.17288919,0.88292116,0.0121741155,0.004504849,-0.01314362,-14.720415,2.4231749,24.770523,-0.983248,-0.0016782065,-0.0070931176,-0.00013196269,-0.17288919,0.88292116,0.015374608,0.0029682121,-0.01970767,-11.608121,9.05737,-13.962615,0.003675287,0.21514308,-0.8999364,-0.985009,-0.0018873987,-0.0063364487,0.008148347,0.005318886,-0.0071817953,-19.488699,9.042266,-14.013303,0.0036752461,0.21514341,-0.89993626,-0.985009,-0.0018871033,-0.0063363453,0.00866651,0.005373069,-0.0073558725,0.0067338604,0.0070597176,-0.006257151,0.009851918,-0.0035819365,0.0075944816,0.021254793,-0.01344784,0.009231995,-0.014016086,0.006444192,0.0069217067,-0.0066864667,0.009542117,-0.0033599182,0.0077622277,0.021193856,-0.0140297,0.0094189225,-0.013531045,0.0061334097,0.006803172,-0.0071836957,0.009331888,-0.003112814,0.007902233,0.021050753,-0.014613737,0.009600508,-0.013087622,0.0058177714,0.006706344,-0.0076946085,0.009202059,-0.002834603,0.008017535,0.020816114,-0.015196326,0.009789805,-0.012602512,0.0054854867,0.006636333,-0.008211076,0.00916317,-0.002570011,0.008052763,0.020544153,-0.015712285,0.010034998,-0.012201249,0.0051407777,0.0065989755,-0.008736892,0.009281112,-0.002292831,0.008084056,0.020196661,-0.016190873,0.010346694,-0.011751361,0.0047909818,0.0065856557,-0.009203862,0.009455521,-0.0020265358,0.008024099,0.01981054,-0.016607888,0.010661776,-0.0113190375,0.0044353744,0.0066061453,-0.009636586,0.009743874,-0.001773089,0.007953662,0.019362237,-0.016994618,0.010995522,-0.011002089,0.0040697856,0.00667124,-0.009949928,0.010106176,-0.0015278357,0.0078008426,0.018868182,-0.017329466,0.0114040775,-0.010660053,0.0037049055,0.0067621353,-0.010206184,0.010502948,-0.001325074,0.007657255,0.018364888,-0.017598592,0.011810369,-0.010336947,0.0033402466,0.006894798,-0.010337914,0.010975855,-0.0011529899,0.007490413,0.017781941,-0.017776746,0.0122659495,-0.010122463,0.002978212,0.007062127,-0.010409214,0.011423895,-0.0009822808,0.00733395,0.017249024,-0.017898545,0.012728829,-0.009870967,0.0026149792,0.0072738403,-0.010375772,0.011890126,-0.0008528665,0.0071523567,0.016711468,-0.017940162,0.013229876,-0.009678778
	};

	stdIn = {
		60.001236,76.525085,0.44855937,0.385322,0.8864665,0.9159223,0.8213151,47.498787,64.49731,0.42183232,0.32208154,0.8455319,0.89670384,0.81977046,35.71034,52.202564,0.38652486,0.25111705,0.7843738,0.88680017,0.81935763,24.911396,39.60206,0.3384554,0.17467622,0.7080688,0.88512844,0.8198607,15.2771015,26.705889,0.26719806,0.09709743,0.62055653,0.8896736,0.8207807,6.9405837,13.52955,0.15721388,0.030496096,0.52306205,0.898866,0.8211371,1.2828501e-05,1.0203146e-05,9.209403e-09,1.0,0.42089432,0.91112185,0.82099056,5.4717736,13.699838,0.15729778,0.030496202,0.33039594,0.9218073,0.8206699,9.527206,27.3602,0.26860222,0.097298324,0.28411025,0.92677975,0.81873167,12.721042,40.937397,0.34177008,0.17524305,0.30393666,0.92819077,0.81499195,15.845264,54.32257,0.39111036,0.2515463,0.38128957,0.926284,0.8108155,20.028368,67.36319,0.42771947,0.32299566,0.4827713,0.92117953,0.80545473,25.850378,79.97121,0.45653817,0.38720912,0.576549,0.9189052,0.80049264,1.3054844e-05,3.274067,1.0640526e-05,0.0073767905,0.06556898,0.031959653,0.07295033,0.036517635,0.083309405,1.5547352e-05,9.312003e-06,1.3625957e-05,0.3765801,3.2453854,0.43088144,0.0077691628,0.06269454,0.077220626,0.06257613,0.011621014,0.094501615,2.4275221e-05,9.740199e-06,1.9633922e-05,1.3115126,3.3002057,1.855495,0.016577331,0.05963908,0.15306914,0.056846615,0.03184422,0.09828989,0.039015803,0.01566397,0.03855448,3.450152,3.9430425,5.942909,0.0181877,0.05782253,0.15303303,0.11430043,0.0845775,0.06774487,0.16673793,0.10107041,0.17609923,4.240122,4.539964,6.665938,0.026020588,0.06121006,0.16253908,0.06270198,0.032603826,0.1172248,0.2112012,0.1395686,0.23128569,5.062639,4.8818774,8.166369,0.11887729,0.05593329,0.26891956,0.071347386,0.031221231,0.12803283,0.2586599,0.17230625,0.28041616,5.6557508,5.094381,8.885505,0.11887729,0.055933297,0.26891953,0.06925725,0.029310266,0.12734745,0.28633618,0.19222482,0.30804998,7.18119,5.630392,11.165474,0.11887729,0.055933293,0.26891953,0.06925725,0.029310267,0.12734745,0.36059716,0.24458799,0.38230392,3.0091898,3.8287563,5.493224,0.2005672,0.22091708,0.058119975,0.043403897,0.09055686,0.20565,0.15824836,0.095564805,0.1564426,3.132701,4.000295,6.9890184,0.20020632,0.21949573,0.15279898,0.12379853,0.113308676,0.37548983,0.25517488,0.17922565,0.22377244,4.9683275,5.713498,17.065685,0.12169341,0.17129724,0.21821217,0.22900842,0.27692488,0.23140384,0.864688,0.7114595,0.6818626,7.5936413,9.751567,21.992971,0.17951013,0.29406244,0.30233443,0.26718912,0.36287224,0.36507782,1.3734609,0.68510187,1.0738014,3.0050716,3.6758473,5.2808714,0.22476095,0.18631528,0.06672743,0.049459163,0.12104951,0.2260321,0.1614352,0.10026949,0.15717755,3.143287,3.8567996,6.6172543,0.19226818,0.20596871,0.12606683,0.10442911,0.117850885,0.40155587,0.2663689,0.21864213,0.23162948,4.2125683,5.9564037,17.750252,0.115421735,0.15506622,0.23832007,0.28011075,0.3091675,0.21807113,0.87755454,0.7482558,0.718784,10.208639,12.256949,22.52525,0.1336367,0.2048957,0.2354231,0.2627768,0.37144393,0.34401852,1.3733246,0.7209402,1.1581031,0.067987055,3.3527272,0.15969451,0.055846356,0.07999897,0.041104995,0.013331696,0.06430427,0.03645641,1.6722528e-05,9.820554e-06,1.4700217e-05,0.2441022,3.6006904,0.5355476,0.028211685,0.11626459,0.09980062,0.1142453,0.107680865,0.31017855,2.031612e-05,1.6886599e-05,1.7660426e-05,4.802853,5.2029448,12.844595,0.027367717,0.11955132,0.09622654,0.09940615,0.18017127,0.22243963,0.7895119,0.4123149,0.62219083,7.7097425,6.691513,18.35846,0.063832104,0.098386645,0.16209207,0.14665703,0.16699082,0.29085603,0.9605766,0.9204354,0.7722101,8.139507,4.7536464,21.324732,0.0660377,0.09833676,0.16536644,0.16573295,0.31110108,0.17091149,1.1994427,0.91974187,0.9552051,9.734191,5.814477,23.040483,0.066037685,0.09833675,0.16536641,0.16573295,0.31110102,0.1709115,1.4939754,1.0247989,1.2096606,7.8699126,9.758471,19.871334,0.16373716,0.29019633,0.17749132,0.064397596,0.098720476,0.16173542,1.0241915,1.2460287,0.8204117,7.9629226,10.084449,19.974478,0.16373716,0.29019636,0.17749134,0.064397536,0.098720394,0.16173546,1.0124192,1.281361,0.8172462,0.07036602,3.2222548,0.15531191,0.05575135,0.08006043,0.041303743,0.014399534,0.06479196,0.034387212,1.6386635e-05,9.874574e-06,1.4532856e-05,0.25697607,3.2088003,0.5089387,0.024889464,0.12360615,0.08467266,0.116722666,0.1214366,0.27627197,2.0792742e-05,1.7270026e-05,1.7578866e-05,4.9332647,5.3310056,11.375388,0.025725173,0.13270147,0.08398292,0.102389865,0.18703413,0.24144877,0.7809836,0.37187353,0.6253949,8.244221,7.207326,17.823124,0.027872847,0.078624524,0.15103467,0.12323291,0.16651054,0.3052267,0.98082876,0.93363893,0.7971225,9.024313,5.1531034,20.950762,0.031656593,0.091682136,0.154159,0.15447329,0.36055604,0.1916,1.2123547,0.95959705,0.96828014,10.676858,6.7149734,23.135426,0.031656582,0.091682136,0.15415898,0.15447327,0.36055604,0.19159998,1.6602556,1.1798092,1.3238083,8.382419,10.610179,19.365732,0.15222955,0.29664955,0.18069236,0.028055422,0.078597404,0.15083408,1.0309116,1.2736855,0.83676654,8.37071,10.661655,19.255346,0.15222953,0.2966495,0.18069246,0.028055413,0.0785974,0.15083408,1.0186204,1.2828366,0.8285045,1.4483656,1.5574969,0.6769256,0.6857786,1.0394847,1.057056,0.9939248,1.0023812,1.8064611,1.8342892,1.4487444,1.5569408,0.67755365,0.6856676,1.0397727,1.0573113,0.9938467,1.0017556,1.8066225,1.8344786,1.4491713,1.5563432,0.67821777,0.68551403,1.0401406,1.0574806,0.99376553,1.0011274,1.8067306,1.834727,1.4495916,1.5557212,0.6788622,0.6853751,1.0405762,1.0575793,0.9936479,1.0005292,1.8067983,1.8350215,1.4500189,1.5550938,0.6794326,0.68529886,1.0410705,1.057617,0.9934917,0.9999599,1.806826,1.8353593,1.450425,1.5544896,0.6799052,0.68531626,1.0415784,1.0576375,0.99326557,0.99944764,1.806827,1.8357277,1.4507788,1.5539272,0.68025017,0.6854661,1.0420665,1.0576731,0.9929766,0.99899167,1.8067997,1.8361262,1.451049,1.5534345,0.6804765,0.6857417,1.0425007,1.0577533,0.9926137,0.9986099,1.8067539,1.836545,1.4512383,1.5530369,0.680575,0.68615294,1.0428511,1.0579045,0.9921977,0.99828595,1.8067018,1.8369718,1.4513065,1.5527092,0.68055904,0.6866874,1.0431248,1.0581188,0.9917451,0.9980047,1.8066458,1.8374026,1.4512826,1.5524738,0.6804675,0.68730474,1.0433321,1.0583899,0.9912772,0.99774396,1.806604,1.8378185,1.4511627,1.5523231,0.68033224,0.6879723,1.043494,1.0586964,0.9907779,0.99751383,1.8065856,1.8382115,1.4509197,1.5522802,0.6802054,0.68862987,1.0436553,1.0589929,0.99029016,0.99727845,1.806607,1.8385632
	};


	meanOut = {
		0.15058589,2.5692136,0.0069640875,-1.6694953e-07,6.717178e-08,-6.150943e-11,1.0,0.08105741,1.0367501,1.1895193,1.0997814,15.763291,-0.01020057,0.98702097,0.06679675,1.0601201,1.1890626,1.9582744,31.746868,-0.020324064,0.95806617,0.049045913,1.0673597,1.1879225,2.5368884,47.68695,-0.029758442,0.92275935,0.030476956,1.0562129,1.1858848,2.8500593,63.307453,-0.038535297,0.8843842,0.013317056,1.0267168,1.1833566,2.9050462,78.33192,-0.04658078,0.84284884,-0.004016418,0.9793909,1.1799028,2.711276,92.54958,-0.05379833,0.79913056,-0.01966126,0.9214777,1.1771557,-2.1567867e-07,88.42901,6.014592e-08,0.9967177,0.0059561506,-0.0043361206,-0.00703789,0.87362605,-0.47145468,1.3694667e-09,-1.5374279e-09,-1.4483004e-10,-0.034213834,92.93859,-2.4386926,0.9947071,0.00044114387,0.002601542,-0.00070867245,0.9880473,-0.10317268,-1.4294242e-08,1.6183463e-10,2.6236704e-09,-0.044565856,107.964294,-4.0183954,0.98597664,0.0028170715,0.015015511,-0.0065196473,0.98239774,0.14519916,-0.00030321645,-2.5532606e-06,-0.00038885718,-0.33612144,149.32588,2.072829,0.9859722,0.019415986,0.011892103,-0.02214963,0.7067244,0.68963045,-0.0009482178,-6.531178e-06,-0.0019687416,-0.6011145,157.48776,10.033691,0.9835793,0.037418574,0.013350752,-0.03537221,0.979392,0.14471827,-0.00072652707,9.658388e-06,-0.0010921593,-1.1154102,171.61804,12.113126,0.95416766,-0.008241484,0.024085872,-0.011578159,0.9778079,0.14655477,-0.00080617896,-3.13534e-06,-0.0012552196,-1.23688,181.99173,13.683433,0.95416766,-0.008241453,0.024085842,-0.011121328,0.98074704,0.12764513,-0.0011874798,-8.269997e-05,4.0026e-05,-1.5208211,207.48055,16.984968,0.95416766,-0.008241454,0.024085844,-0.011121329,0.98074704,0.12764513,-0.0021364284,-0.0002700202,0.0029625518,8.938524,141.54602,-0.59223735,0.03951889,-0.20430474,0.9290254,0.97072446,0.056833677,-0.034005728,-0.00012676264,0.00017580959,-0.0020283519,22.12042,142.31696,-1.0419228,-0.65297806,-0.35372996,-0.579871,0.52173793,-0.731987,-0.14992741,0.0011563427,0.0005040511,-0.0030119347,40.112385,117.10596,-6.181713,-0.87105507,-0.2064956,-0.32572347,-0.13148579,-0.6567044,0.6066458,0.0049806633,0.0027903784,-0.0058718263,35.787155,95.64361,13.6542015,-0.82021886,0.05065307,-0.33582854,-0.2754623,-0.67344916,0.36455438,0.009018192,0.0006806572,-0.0037039695,-9.580131,141.49347,-0.89364994,0.042566556,0.31324857,-0.8999161,-0.9631097,0.050611787,-0.03915992,-0.0016396991,-0.00019092385,-0.001736254,-22.652384,142.1801,-1.4390264,-0.63820994,0.41349202,0.57095546,-0.48985302,-0.75628334,-0.041799653,-0.0027539546,-0.0007414069,-0.0016900597,-39.536053,116.13203,-2.885576,-0.8788779,0.19417578,0.30726352,0.1544548,-0.60212916,0.62513536,-0.0055251517,-0.0027632832,0.0010379291,-34.513725,96.45926,17.586618,-0.85040754,0.093474366,0.38869274,0.22374575,-0.65918046,0.43500948,-0.006591057,-0.0025288048,0.003869999,4.8177543,88.113914,0.016894,-0.0683769,-0.36567837,0.9213663,0.9814447,-0.1730991,0.00303129,-5.879396e-10,-2.427556e-10,-3.3264205e-09,15.047472,86.31054,0.06175596,0.98281085,-0.031402558,-0.088079974,-0.0019910915,-0.9051498,0.24338682,2.0219183e-08,4.8550195e-09,-6.123977e-09,14.965056,49.11046,10.1220875,0.98390394,0.0029357783,-0.08146825,-0.045934595,-0.7719336,-0.5575346,-0.007513483,-0.001005495,-0.0056440593,13.266234,19.683298,-11.1066065,0.9389184,-0.023622496,-0.27629745,0.16393915,-0.6766764,0.6167845,-0.0098012285,-0.0035345387,-0.0063849417,16.575861,6.145617,1.2440221,-0.9378887,0.010353685,0.27812514,0.26279363,-0.1484239,0.8682446,-0.015450772,-0.003232487,-0.0023545173,22.108265,3.0266829,19.526522,-0.93788874,0.010353434,0.27812514,0.26279363,-0.14842401,0.8682446,-0.019480675,-0.0019949449,-0.0021407856,7.779688,8.835891,-17.263311,0.26208547,-0.19940098,0.8645774,0.93791455,-0.019742444,-0.27998573,-0.009937646,-0.004766458,-0.0031771793,15.29161,8.678072,-19.496864,0.26208496,-0.19940111,0.8645776,0.9379147,-0.019742515,-0.27998534,-0.01074146,-0.004651098,-0.0030217662,-4.813491,88.0566,0.047674634,-0.08052953,0.36478907,-0.9207183,-0.9793334,-0.18481533,0.011561435,-3.3455818e-09,-2.0229178e-09,-7.605771e-09,-15.020819,86.1316,0.15783581,0.98701835,0.02608085,0.031691644,0.0068003233,-0.88581115,0.33110276,1.6187188e-08,2.589348e-09,-8.369948e-09,-14.756716,49.725086,13.824704,0.9862805,-0.020075027,0.0229516,-0.0033881662,-0.78513694,-0.529863,0.0053119576,-5.8894464e-05,-0.0011200609,-14.870633,19.801098,-6.3436065,0.9841892,0.005826982,0.001395524,0.0096388105,-0.6865949,0.6259154,0.007578,0.004086805,-0.007693546,-14.720255,6.063024,6.1891956,-0.98246235,-0.0016777,-0.005901757,0.0017295033,-0.17289314,0.882176,0.012592446,0.0045048487,-0.013743362,-14.723709,2.422651,24.76051,-0.98246235,-0.0016775556,-0.0059017586,0.0017294658,-0.17289314,0.882176,0.016225992,0.0029682121,-0.020370869,-11.610322,9.061818,-13.969014,0.0018541592,0.21514569,-0.8991822,-0.98422766,-0.0018876655,-0.0051478455,0.008181922,0.005318886,-0.007850696,-19.490772,9.046996,-14.019811,0.0018541176,0.21514602,-0.89918214,-0.98422766,-0.0018873704,-0.0051477402,0.008617196,0.005373069,-0.007897939,0.0044353744,0.0066061453,-0.009636586,0.009743874,-0.001773089,0.007953662,0.019362237,-0.016994618,0.010995522,-0.011002089,1.7149845,0.7470354,1.3542717,1.3387759,2.3993766,1.2871614,1.8646427,2.0377555,1.6394438,1.4721067,0.0040697856,0.00667124,-0.009949928,0.010106176,-0.0015278357,0.0078008426,0.018868182,-0.017329466,0.0114040775,-0.010660053,1.714897,0.74720556,1.3545728,1.3383963,2.3997192,1.2872007,1.8654207,2.0383637,1.6398824,1.4728526,0.0037049055,0.0067621353,-0.010206184,0.010502948,-0.001325074,0.007657255,0.018364888,-0.017598592,0.011810369,-0.010336947,1.714798,0.7473828,1.3548648,1.338021,2.400062,1.2872416,1.866228,2.0389266,1.6403294,1.4736862,0.0033402466,0.006894798,-0.010337914,0.010975855,-0.0011529899,0.007490413,0.017781941,-0.017776746,0.0122659495,-0.010122463,1.7147032,0.74756444,1.3551519,1.3376477,2.4004066,1.2872828,1.8670719,2.0394394,1.6407597,1.4745091,0.002978212,0.007062127,-0.010409214,0.011423895,-0.0009822808,0.00733395,0.017249024,-0.017898545,0.012728829,-0.009870967,1.7146093,0.7477519,1.3554332,1.3372723,2.4007535,1.2873203,1.8679394,2.039913,1.6411881,1.4753015,0.0026149792,0.0072738403,-0.010375772,0.011890126,-0.0008528665,0.0071523567,0.016711468,-0.017940162,0.013229876,-0.009678778,1.7145154,0.74794,1.3557055,1.3368993,2.4011016,1.2873567,1.8687953,2.0403934,1.6416246,1.476079,0.0022611043,0.007518231,-0.01028076,0.012338555,-0.0007332307,0.0069650705,0.016157433,-0.017894363,0.013669979,-0.009564223,1.7144202,0.74812585,1.3559712,1.3365291,2.40145,1.287391,1.8696716,2.0408702,1.6420462,1.4768962
	};

	stdOut = {
		0.619276,0.8588207,0.020678796,1.2825451e-05,1.0203195e-05,9.212793e-09,1.0,0.42095184,0.91116774,0.8208915,5.471709,13.70127,0.15734397,0.03050102,0.33035532,0.92178744,0.82052135,9.526309,27.36069,0.26874727,0.097348824,0.28408068,0.9266679,0.81847286,12.719505,40.9334,0.3419247,0.17529221,0.3040004,0.9277817,0.8144729,15.844669,54.31268,0.39127797,0.25158828,0.38156193,0.9258626,0.8104219,20.031202,67.34399,0.4278898,0.32305717,0.4830885,0.92048234,0.80478543,25.860739,79.94233,0.4566818,0.38724735,0.57697505,0.91818684,0.7998371,1.305291e-05,3.273336,1.0640595e-05,0.008536006,0.06556867,0.04613023,0.07833787,0.036517188,0.08360138,1.5552907e-05,9.312003e-06,1.3632432e-05,0.37679064,3.244775,0.43074936,0.008493747,0.062694475,0.08092562,0.06311055,0.011620043,0.09486998,2.4229916e-05,9.740199e-06,1.9690975e-05,1.3121473,3.2996166,1.8548965,0.016858185,0.05963935,0.15419935,0.05499806,0.031844176,0.09864857,0.039094336,0.01566397,0.03847285,3.4513545,3.9418302,5.941285,0.018695598,0.05782279,0.15378343,0.11274885,0.08457898,0.067792475,0.16682337,0.10107041,0.1760172,4.241268,4.53837,6.664143,0.026727943,0.061210394,0.16289437,0.061131645,0.032603618,0.117464915,0.21112075,0.1395686,0.23135653,5.064362,4.8801346,8.164154,0.12057922,0.05593319,0.26693514,0.06917434,0.031221714,0.12853466,0.25858957,0.17230625,0.28047803,5.6576467,5.0925198,8.882905,0.12057922,0.055933192,0.26693514,0.067244016,0.029310685,0.12782377,0.28632292,0.19222482,0.30805898,7.183412,5.6282578,11.161788,0.12057921,0.055933196,0.26693514,0.067244016,0.029310683,0.12782376,0.36071828,0.24458799,0.38218656,3.010364,3.8278158,5.4918966,0.20333035,0.22091697,0.0588962,0.04455934,0.09055729,0.20766564,0.15838452,0.095564805,0.15630199,3.1338902,3.9995348,6.9876657,0.20046946,0.21949655,0.15454996,0.123907804,0.113311395,0.3759329,0.25537556,0.17922565,0.22353229,4.968794,5.713589,17.061619,0.12190065,0.17129888,0.2198418,0.23027113,0.27692443,0.23180287,0.86465675,0.7114595,0.6818691,7.5894256,9.748858,21.987103,0.18063901,0.29406244,0.3046412,0.26953232,0.36287203,0.3652222,1.3733548,0.68510187,1.0738895,3.0062091,3.6748521,5.279145,0.22592112,0.18631501,0.06683555,0.049312588,0.12104969,0.22637324,0.16159813,0.10026949,0.15700656,3.1441977,3.856122,6.614329,0.1926045,0.20596546,0.12732223,0.10458889,0.117853776,0.4018922,0.26660746,0.21864213,0.23134117,4.2124634,5.955155,17.746006,0.116591156,0.15506223,0.24047379,0.28248173,0.30916846,0.21777116,0.87769467,0.7482558,0.71853614,10.206944,12.25774,22.518997,0.13572691,0.20489822,0.23788837,0.26512638,0.371446,0.3434303,1.3727179,0.7209402,1.1587194,0.06803093,3.3520503,0.15984169,0.067620516,0.079998486,0.041434195,0.014042491,0.06430434,0.049619474,1.672854e-05,9.820554e-06,1.4702169e-05,0.24431211,3.6001952,0.53599083,0.028982429,0.116263196,0.1048017,0.117302984,0.107679665,0.31002635,2.0303518e-05,1.6886599e-05,1.7676539e-05,4.8064017,5.201098,12.842573,0.028130012,0.11955037,0.10102233,0.09751069,0.18017079,0.22181131,0.7893199,0.4123149,0.6224451,7.7169075,6.6906214,18.354834,0.06599862,0.09838597,0.16583487,0.14801034,0.16699392,0.2915909,0.96071905,0.9204354,0.7720074,8.145083,4.753308,21.318901,0.06833056,0.098335296,0.16902094,0.16833332,0.3111032,0.17306261,1.1987681,0.91974187,0.95611835,9.738656,5.8160644,23.034637,0.06833056,0.09833527,0.1690209,0.1683333,0.31110322,0.17306264,1.4929069,1.0247989,1.2110392,7.877609,9.756166,19.86665,0.16605112,0.2901951,0.1796676,0.06654459,0.09871975,0.16541983,1.0241405,1.2460287,0.820497,7.970336,10.082463,19.97015,0.16605116,0.29019514,0.17966762,0.06654453,0.09871968,0.16541994,1.0124451,1.281361,0.8172407,0.07037065,3.2215073,0.1554414,0.067540206,0.08005997,0.041850846,0.01500937,0.064791374,0.046830412,1.6384485e-05,9.874574e-06,1.4534534e-05,0.25697288,3.2081687,0.5092727,0.025766227,0.12360706,0.09038508,0.120890535,0.1214367,0.27624145,2.0778796e-05,1.7270026e-05,1.7595967e-05,4.934678,5.3303075,11.372603,0.026611013,0.13270223,0.08947077,0.098685205,0.18702987,0.2410019,0.78121376,0.37187353,0.6250606,8.24361,7.2114325,17.819824,0.029788349,0.07862499,0.15578401,0.12558657,0.1665157,0.30534267,0.98153365,0.93363893,0.7962507,9.025537,5.1567616,20.950035,0.03345073,0.091683865,0.1587694,0.15801,0.36055383,0.19214085,1.2132078,0.95959705,0.9672463,10.681101,6.714414,23.135551,0.033450723,0.09168387,0.15876938,0.15800999,0.36055383,0.19214085,1.6614423,1.1798092,1.3223659,8.381317,10.61628,19.363329,0.15586068,0.29665035,0.1813734,0.029962774,0.078597814,0.1555403,1.0315533,1.2736855,0.8360086,8.369549,10.668059,19.252728,0.1558607,0.29665026,0.18137348,0.02996277,0.0785978,0.15554032,1.0192617,1.2828366,0.8277593,1.451049,1.5534345,0.6804765,0.6857417,1.0425007,1.0577533,0.9926137,0.9986099,1.8067539,1.836545,1.25594,0.6126798,0.60953635,0.43671465,0.9381276,0.32203442,0.88478,0.88959485,1.8489679,1.6255643,1.4512383,1.5530369,0.680575,0.68615294,1.0428511,1.0579045,0.9921977,0.99828595,1.8067018,1.8369718,1.2557856,0.6130528,0.60972637,0.43618354,0.9379877,0.3219882,0.88823974,0.8912927,1.8495984,1.6269151,1.4513065,1.5527092,0.68055904,0.6866874,1.0431248,1.0581188,0.9917451,0.9980047,1.8066458,1.8374026,1.2555938,0.61342824,0.60991526,0.4356509,0.93784803,0.32194135,0.89203674,0.8926992,1.8502719,1.6287026,1.4512826,1.5524738,0.6804675,0.68730474,1.0433321,1.0583899,0.9912772,0.99774396,1.806604,1.8378185,1.2554044,0.613807,0.6100999,0.4351185,0.93770427,0.32189396,0.89625335,0.8938167,1.8509055,1.630423,1.4511627,1.5523231,0.68033224,0.6879723,1.043494,1.0586964,0.9907779,0.99751383,1.8065856,1.8382115,1.2552075,0.6141859,0.61028135,0.4345905,0.9375542,0.32184222,0.90079385,0.89473695,1.8515407,1.631982,1.4509197,1.5522802,0.6802054,0.68862987,1.0436553,1.0589929,0.99029016,0.99727845,1.806607,1.8385632,1.2550023,0.61456144,0.6104643,0.4340668,0.9373985,0.32178873,0.90524983,0.89571464,1.8522091,1.6334584,1.4505857,1.5523138,0.6801175,0.6892422,1.0438414,1.0592555,0.9898357,0.997018,1.8066721,1.838871,1.2547886,0.61493146,0.61064565,0.4335485,0.93724054,0.321732,0.90994036,0.89669806,1.8528786,1.6351663
	};

}

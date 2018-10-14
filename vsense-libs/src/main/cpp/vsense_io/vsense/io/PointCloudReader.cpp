#include <vsense/io/PointCloudReader.h>

#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace vsense::io;
using namespace vsense::pc;

bool PointCloudReader::read(const std::string& filename, PointCloud& pc, PointCloudMetadata& pcData, float minConf) {
	ifstream file(filename, ios::in | ios::binary);
	if (file.is_open()) {
		file.read((char*)&pcData.width_, sizeof(uint32_t));
		file.read((char*)&pcData.height_, sizeof(uint32_t));
		file.read((char*)&pcData.f_.x, sizeof(double) * 2);
		file.read((char*)&pcData.c_.x, sizeof(double) * 2);
		file.read((char*)pcData.distortion_, sizeof(double) * 5);
		file.read((char*)&pcData.nbrPoints_, sizeof(uint32_t));
		file.read((char*)&pcData.timestamp_, sizeof(double));
		file.read((char*)pcData.translation_, sizeof(double) * 3);
		file.read((char*)pcData.orientation_, sizeof(double) * 4);
		file.read((char*)&pcData.accuracy_, sizeof(float));

		for (int i = 0; i < 5; i++)
			pcData.distortionF_[i] = pcData.distortion_[i];
		
		glm::vec3 pos;
		Point pt;
		float conf;

		for (unsigned int i = 0; i < pcData.nbrPoints_; i++) {									
			for(int j = 0; j < 3; j++)
				file.read((char*)&pt.pos[j], sizeof(float));
			file.read((char*)&conf, sizeof(float));

			if (conf >= minConf)
				pc.addPoint(pt);
		}

		pcData.nbrPoints_ = (unsigned int)pc.size();

		file.close();

		return true;
	}	

	return false;
}

glm::mat4 PointCloudMetadata::asPose() {
	glm::quat q((float)orientation_[3], (float)orientation_[0], (float)orientation_[1], (float)orientation_[2]);
	glm::mat4 pose = glm::mat4_cast(q);
	pose[3][0] = translation_[0];
	pose[3][1] = translation_[1];
	pose[3][2] = translation_[2];

	return pose;
}
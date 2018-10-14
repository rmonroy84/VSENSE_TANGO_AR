#include <vsense/io/ImageReader.h>
#include <vsense/io/Image.h>

#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;
using namespace vsense::io;

bool ImageReader::read(const std::string& filename, std::shared_ptr<Image>& img, ImageMetadata& imData) {
	ifstream file(filename, ios::in | ios::binary);
	if (file.is_open()) {
		file.read((char*)&imData.width_, sizeof(uint32_t));
		file.read((char*)&imData.height_, sizeof(uint32_t));
		file.read((char*)&imData.exposure_, sizeof(int64_t));
		file.read((char*)&imData.timestamp_, sizeof(double));
		file.read((char*)&imData.f_.x, sizeof(double) * 2);
		file.read((char*)&imData.c_.x, sizeof(double) * 2);
		file.read((char*)imData.distortion_, sizeof(double) * 5);
		file.read((char*)imData.translation_, sizeof(double) * 3);
		file.read((char*)imData.orientation_, sizeof(double) * 4);
		file.read((char*)&imData.accuracy_, sizeof(float));

		for (int i = 0; i < 5; i++)
			imData.distortionF_[i] = (float)imData.distortion_[i];

		std::shared_ptr<uchar> Y;
		std::shared_ptr<uchar> C;
		Y.reset(new uchar[imData.height_*imData.width_], std::default_delete<uchar[]>());
		C.reset(new uchar[imData.height_*imData.width_/2], std::default_delete<uchar[]>());		

		file.read((char*)Y.get(), sizeof(char)*imData.width_*imData.height_);
		file.read((char*)C.get(), sizeof(char)*imData.width_*imData.height_/2);
		
		file.close();

		img.reset(new Image(imData.width_, imData.height_));
		uchar* rowDataC;
		float crVal, cbVal;
		for (uint32_t row = 0; row < imData.height_; row++) {
			uchar* rowDataOut = img->row(row);
			uchar* rowDataY = Y.get() + imData.width_*row;
			
			rowDataC = C.get() + imData.width_*(row >> 1);
			
			for (uint32_t col = 0; col < imData.width_; col++) {
				float yVal = *rowDataY++;

				if (col % 2 == 0) {
					crVal = *rowDataC++;
					cbVal = *rowDataC++;
				}
				
				*rowDataOut++ = (uchar)std::max(std::min(yVal + (1.370705f*(crVal - 128)), 255.f), 0.f);
				*rowDataOut++ = (uchar)std::max(std::min(yVal - (0.698001f*(crVal - 128)) - (0.337633f*(cbVal - 128)), 255.f), 0.f);
				*rowDataOut++ = (uchar)std::max(std::min(yVal + (1.732446f*(cbVal - 128)), 255.f), 0.f);
				*rowDataOut++ = 255;
			}
		}

		return true;
	}

	return false;
}

glm::mat4 ImageMetadata::asPose() {
	glm::quat q((float)orientation_[3], (float)orientation_[0], (float)orientation_[1], (float)orientation_[2]);
	glm::mat4 pose = glm::mat4_cast(q);
	pose[3][0] = translation_[0];
	pose[3][1] = translation_[1];
	pose[3][2] = translation_[2];

	return pose;
}
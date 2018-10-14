#include "Viewer.h"

#include <vsense/gl/ImageObject.h>
#include <vsense/gl/Texture.h>
#include <vsense/em/Process.h>
#include <vsense/em/EnvironmentMap.h>

#include <QKeyEvent>
#include <QFile>

#include <fstream>
#include <iostream>
#include <chrono>

using namespace vsense;
using namespace std::chrono;
using namespace std;

const std::string ImageTag = "image";

Viewer::Viewer(QWidget* parent) : gl::GLViewer(parent) {		

}

void Viewer::initializeObjects() {
	imgObject_.reset(new gl::ImageObject());
	objects_[ImageTag] = imgObject_;

	process_.reset(new em::Process);	
}

void Viewer::loadDepthMap(const QString& filePC, const QString& fileIM, float& timeF) {
	clock_t t = clock();

	float minConfidence = 0.7f;
	process_->loadDepthMap(filePC.toStdString(), fileIM.toStdString(), minConfidence);

	imgObject_->updateTexture(process_->getEnvironmentMap());

	update();
}

std::shared_ptr<gl::Texture> Viewer::getEnvironmentMap() {
	return process_->getEnvironmentMap();
}

std::shared_ptr<glm::vec4> Viewer::getSHCoefficients() {
	return process_->getSHCoefficients();
}

bool firstTime = true;
void Viewer::moveOrigin(const glm::vec3& displacement) {
	if (!firstTime) {
		// Test code for Gardner
		std::shared_ptr<gl::Texture> emTex = process_->getEnvironmentMap();
		std::shared_ptr<float> data;
		data.reset(new float[emTex->width()*emTex->height()*emTex->channels()], std::default_delete<float[]>());
		emTex->writeTo(data.get());

		em::EnvironmentMap em;
		em.loadFromData(data.get(), process_->getOrigin());

		em::EnvironmentMap emW;
		if (emW.fromWarp(em, process_->getOrigin() + displacement)) {
			std::cout << "EM warped successfully!" << std::endl;

			QImage img(emTex->width(), emTex->height(), QImage::Format_RGBA8888);
			const float* colorPtr = (const float*)emW.getColorPtr();
			const float* depthPtr = emW.getDepthPtr();
			for (size_t row = 0; row < emTex->height(); row++) {
				uchar* imgPtr = img.scanLine(row);
				for (size_t col = 0; col < emTex->width(); col++) {
					for (size_t chan = 0; chan < 4; chan++) {
						if (chan == 3) // Alpha
							*imgPtr++ = (*depthPtr++) == 0.0 ? 0 : 255;
						else
							*imgPtr++ = std::min(std::max(*colorPtr++, 0.f), 1.f) * 255;
					}
				}
			}			

			img.save(QString::fromStdString("D:/data/out/EMFF.png"));

			std::cout << "EM Warp saved!" << std::endl;
		}
	} else
		firstTime = false;
	
	
	process_->updateEMOrigin(process_->getOrigin() + displacement, true);

	glm::vec3 origin = process_->getOrigin();
	std::cout << origin.x << ", " << origin.y << ", " << origin.z << std::endl;

	imgObject_->updateTexture(process_->getEnvironmentMap());	

	process_->saveEM(false, 100);

	update();
}

int curIdx = 0;
void Viewer::saveEM() {
	std::cout << "Current index: " << curIdx << std::endl;

	process_->saveEM(false, curIdx++);	

	std::cout << "Saved!" << std::endl;
}

void Viewer::clearEM() {
	process_->clear();
}
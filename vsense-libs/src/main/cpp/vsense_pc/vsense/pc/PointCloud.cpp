#include <vsense/pc/PointCloud.h>

#include <vsense/io/Image.h>
#include <vsense/io/ImageReader.h>
#include <vsense/io/PointCloudReader.h>

#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <fstream>

using namespace vsense;
using namespace vsense::pc;

void PointCloud::resize(size_t size) {
	pos_.resize(size);
	col_.resize(size);
	flag_.resize(size);
}

void PointCloud::addPoint(const Point& pt) {
	pos_.push_back(pt.pos);
	col_.push_back(glm::vec4(pt.color, 1.f));
	flag_.push_back(pt.flags);	
}

Point PointCloud::at(size_t idx) const {
	Point pt(pos_[idx], col_[idx], flag_[idx]);
	return pt;
}

#ifdef _WINDOWS
void PointCloud::loadFromFile(const std::string &pcFilename, const std::string &imFilename) {
  io::PointCloudMetadata pcData;

	std::shared_ptr<io::Image> img;
	io::ImageMetadata imData;

	PointCloud pc;
	io::PointCloudReader::read(pcFilename, pc, pcData);
	io::ImageReader::read(imFilename, img, imData);

	glm::quat q((float)imData.orientation_[3], (float)imData.orientation_[0], (float)imData.orientation_[1], (float)imData.orientation_[2]);
	glm::vec3 t((float)imData.translation_[0], (float)imData.translation_[1], (float)imData.translation_[2]);
	glm::mat4 cdPose = glm::mat4_cast(q);
	cdPose[3][0] = t.x;
	cdPose[3][1] = t.y;
	cdPose[3][2] = t.z;

	pc::PointCloud transCloud;
	transformPointCloud(cdPose, transCloud);

	for (size_t j = 0; j < size(); j++) {
		pc::Point pt = transCloud.at(j);
		pc::Point orPt = pc.at(j);

		int x = (int)((float)imData.f_[0] * pt.pos.x / pt.pos.z + (float)imData.c_[0]);

		if (x < 0)
			continue;
		if (x >= img->cols())
			continue;

		int y = (int)((float)imData.f_[1] * pt.pos.y / pt.pos.z + (float)imData.c_[1]);

		if (y < 0)
			continue;
		if (y >= img->rows())
			continue;

		uchar* px = img->pixel(y, x);

		// Linearize RGB
		orPt.color.b = pow((float)(*px++)/255.f, color::Gamma);
		orPt.color.g = pow((float)(*px++)/255.f, color::Gamma);
		orPt.color.r = pow((float)(*px)/255.f, color::Gamma);

		addPoint(orPt);
	}

	q = glm::quat((float)pcData.orientation_[3], (float)pcData.orientation_[0], (float)pcData.orientation_[1], (float)pcData.orientation_[2]);
	t = glm::vec3((float)pcData.translation_[0], (float)pcData.translation_[1], (float)pcData.translation_[2]);
	cdPose = glm::mat4_cast(q);
	cdPose[3][0] = t.x;
	cdPose[3][1] = t.y;
	cdPose[3][2] = t.z;
	transformPointCloud(cdPose, transCloud);

	for (size_t j = 0; j < size(); j++) {
		pc::Point* pt = &transCloud.at(j);
		pc::Point* orPt = &at(j);

		orPt->pos = pt->pos;
	}

	recalculateBoundingBox();
}

void PointCloud::transformPointCloud(const glm::mat4& pose, PointCloud& pcOut) {
	pcOut.resize(this->size());

	for (size_t i = 0; i < pcOut.size(); i++) {
		const Point* ptIn = &this->at(i);
		Point* ptOut = &pcOut.at(i);

		glm::vec3 pt(ptIn->pos.x, ptIn->pos.y, ptIn->pos.z);		
		ptOut->pos.x = static_cast<float> (pose[0][0] * pt[0] + pose[1][0] * pt[1] + pose[2][0] * pt[2] + pose[3][0]);
		ptOut->pos.y = static_cast<float> (pose[0][1] * pt[0] + pose[1][1] * pt[1] + pose[2][1] * pt[2] + pose[3][1]);
		ptOut->pos.z = static_cast<float> (pose[0][2] * pt[0] + pose[1][2] * pt[1] + pose[2][2] * pt[2] + pose[3][2]);

		ptOut->color.r = ptIn->color.r;
		ptOut->color.g = ptIn->color.g;
		ptOut->color.b = ptIn->color.b;
	}
	
	pcOut.recalculateBoundingBox();	
}

void PointCloud::recalculateBoundingBox() {
	for (size_t i = 0; i < pos_.size(); i++) {
		Point* pt = &this->at(i);

		bb_.min.x = std::min(bb_.min.x, pt->pos.x);
		bb_.min.y = std::min(bb_.min.y, pt->pos.y);
		bb_.min.z = std::min(bb_.min.z, pt->pos.z);

		bb_.max.x = std::max(bb_.max.x, pt->pos.x);
		bb_.max.y = std::max(bb_.max.y, pt->pos.y);
		bb_.max.z = std::max(bb_.max.z, pt->pos.z);		
	}

	bb_.valid = true;
}

void PointCloud::saveToXYZFile(const std::string& filename) {
	std::ofstream xyzFile;
	xyzFile.open(filename, std::ios::out);

	glm::vec3* curPos = &pos_[0];
	glm::vec4* curCol = &col_[0];
	for (size_t i = 0; i < pos_.size(); i++) {
		xyzFile << curPos->x << " " << curPos->y << " " << curPos->z << " ";
		xyzFile << (int)(curCol->r*255) << " " << (int)(curCol->g*255) << " " << (int)(curCol->b*255) << std::endl;

		curPos++;
		curCol++;
	}
	
	xyzFile.close();

	std::cout << "Point cloud saved!" << std::endl;
}
#endif

const glm::vec3* PointCloud::getPositionPtr() const {
	if (!pos_.size())
		return nullptr;

	return &pos_[0];
}

const glm::vec4* PointCloud::getColorsPtr() const {
	if (!pos_.size())
		return nullptr;

	return &col_[0];
}
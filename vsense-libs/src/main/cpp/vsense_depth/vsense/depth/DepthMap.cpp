#include <vsense/depth/DepthMap.h>

#include <vsense/io/ImageReader.h>
#include <vsense/io/PointCloudReader.h>
#include <vsense/io/Image.h>

#include <vsense/pc/PointCloud.h>

#ifdef __ANDROID__
#include <tango_client_api.h>
#include <tango_support_api.h>

#include <glm/gtc/quaternion.hpp>
#endif

#include <iostream>
#include <fstream>

using namespace std;
using namespace vsense;
using namespace vsense::depth;

size_t DepthMap::width_ = 224;
size_t DepthMap::height_ = 172;
size_t DepthMap::nbrPixels_ = DepthMap::width_*DepthMap::height_;

bool DepthMap::fillHoles_ = true;
bool DepthMap::fillWithMax_ = false;

const float MaxExposure = 0.95f;
const float MinExposure = 0.05f;

const float MaxDepth = 2.f;

std::shared_ptr<glm::vec2> DepthMap::ptMap_;

#ifdef _WINDOWS
const std::string MaskFile = "D:/dev/vsense_AR/data/ptMap.bin";

#elif __ANDROID__
const std::string MaskFile = "/sdcard/TCD/map/ptMap.bin";
#endif

const float LimitChiSquare = 14.07f;

DepthMap::DepthMap() {
	if (!ptMap_)
		readDepthMappingFile();
}

void DepthMap::clearData() {
	DepthPoint* curPt = pts_.get();
	for (size_t i = 0; i < nbrPixels_; i++) {
		curPt->depth = FLT_MAX;
		curPt->flags = pc::UnknownPoint;
		++curPt;
	}
}

#ifdef __ANDROID__
glm::mat4 asPose(const TangoPoseData& tangoPose) {
	glm::quat q((float)tangoPose.orientation[3], (float)tangoPose.orientation[0], (float)tangoPose.orientation[1], (float)tangoPose.orientation[2]);
	glm::mat4 pose = glm::mat4_cast(q);
	pose[3][0] = tangoPose.translation[0];
	pose[3][1] = tangoPose.translation[1];
	pose[3][2] = tangoPose.translation[2];

	return pose;
}

bool DepthMap::fillWithFrame(const TangoPointCloud* pointCloud, const TangoPoseData* posePC, const TangoCameraIntrinsics* pcData, const io::Image* image, const TangoPoseData* poseIM, const TangoCameraIntrinsics* imData, float confidence) {
	if (!pts_)
		pts_.reset(new DepthPoint[nbrPixels_], std::default_delete<DepthPoint[]>());
	else
		clearData();
	
	pose_ = asPose(*posePC);
	glm::mat4 imPose = asPose(*poseIM);

	glm::dvec2 f_i = glm::dvec2(imData->fx, imData->fy);
	glm::dvec2 c_i = glm::dvec2(imData->cx, imData->cy);

	glm::dvec2 f_p = glm::dvec2(pcData->fx, pcData->fy);
	glm::dvec2 c_p = glm::dvec2(pcData->cx, pcData->cy);

	size_t imWidth = image->cols();
	size_t imHeight = image->rows();

	// Store points we're confident about
  const float* pt = &pointCloud->points[0][0] - 4;
	for (size_t i = 0; i < pointCloud->num_points; i++) {
    pt += 4;

		if(pt[3] < confidence)
			continue;

		glm::vec3 ptTrans = imPose*glm::vec4(pt[0], pt[1], pt[2], 1.f);
		glm::vec2 ptColor = io::Image::undistortAndProject(ptTrans, imData->distortion, f_i, c_i);
		ptColor.x = floor(ptColor.x + 0.5f);
		ptColor.y = floor(ptColor.y + 0.5f);

		if (ptColor.x < 0)
			continue;
		if (ptColor.x >= imWidth)
			continue;
		if (ptColor.y < 0)
			continue;
		if (ptColor.y >= imHeight)
			continue;

		glm::vec3 pos(pt[0], pt[1], pt[2]);
		glm::vec2 ptDepth = io::Image::undistortAndProject(pos, pcData->distortion, f_p, c_p);
		ptDepth = glm::vec2(width_ - 1.f, height_ - 1.f) - ptDepth;
		ptDepth.x = std::max(0.f, std::min(width_ - 1.f, floor(ptDepth.x + 0.5f)));
		ptDepth.y = std::max(0.f, std::min(height_ - 1.f, floor(ptDepth.y + 0.5f)));

		DepthPoint* curPt = &pts_.get()[(int)(ptDepth.y*width_ + ptDepth.x)];
		curPt->color = image->pixelAsVector((size_t)ptColor.y, (size_t)ptColor.x, true);
		curPt->pos = pos;
		curPt->depth = ptTrans.z;
		curPt->flags = pc::KnownPoint;
	}

	markReliablePoints();

	if (!fillHoles_)
		return true;

	DepthPoint *curPt = pts_.get();
  glm::vec2 *ptDepth = ptMap_.get();
  for (int row = 0; row < height_; row++) {
    for (int col = 0; col < width_; col++) {
      bool validDepthPx = true;

      if (curPt->flags & pc::KnownPoint)
        validDepthPx = false;
      else if (ptDepth->x == 0 && ptDepth->y == 0) // The pixel is not mapped to a valid location
        validDepthPx = false;

      if (validDepthPx) {
        glm::vec3 ptTrans = imPose * glm::vec4(ptDepth->x, ptDepth->y, 1.f, 1.f);
        glm::vec2 ptColor = io::Image::undistortAndProject(ptTrans, imData->distortion, f_i, c_i);
        ptColor.x = floor(ptColor.x + 0.5f);
        ptColor.y = floor(ptColor.y + 0.5f);

        bool validColorPx = true;
        if (ptColor.x < 0)
          validColorPx = false;
        else if (ptColor.x >= imWidth)
          validColorPx = false;
        else if (ptColor.y < 0)
          validColorPx = false;
        else if (ptColor.y >= imHeight)
          validColorPx = false;

        if (validColorPx) {
          curPt->color = image->pixelAsVector((size_t) ptColor.y, (size_t) ptColor.x, true);
						
					if (fillWithMax_)
						curPt->depth = MaxDepth;
					else
						curPt->depth = estimateDepth(glm::i16vec2(col, row));

          curPt->pos.z = (curPt->depth - imPose[3][2]) /
                          (imPose[0][2] * ptDepth->x + imPose[1][2] * ptDepth->y + imPose[2][2]);
          curPt->pos.x = ptDepth->x * curPt->pos.z;
          curPt->pos.y = ptDepth->y * curPt->pos.z;
          curPt->flags |= pc::KnownPoint;
        }
      }

      ++ptDepth;
      ++curPt;
    }    
  }

	return true;
}
#elif _WINDOWS
bool DepthMap::readFiles(const std::string& filenamePC, const std::string& filenameIM, float confidence) {
	pc::PointCloud pc;
	io::PointCloudMetadata pcData;

	if (!io::PointCloudReader::read(filenamePC, pc, pcData, confidence))
		return false;
	
	io::ImageMetadata imData;

	if (!io::ImageReader::read(filenameIM, img_, imData))
		return false;

	pose_ = pcData.asPose();
	glm::mat4 imPose = imData.asPose();

	if (!pts_)
		pts_.reset(new DepthPoint[nbrPixels_], std::default_delete<DepthPoint[]>());
	else
		clearData();

	// Store points we're confident about	
	for (size_t i = 0; i < pcData.nbrPoints_; i++) {		
		const pc::Point* pt = &pc.at(i);
		
		glm::vec3 ptTrans = imPose*glm::vec4(pt->pos.x, pt->pos.y, pt->pos.z, 1.f);
		glm::vec2 ptColor = io::Image::undistortAndProject(ptTrans, imData.distortion_, imData.f_, imData.c_);
		ptColor.x = floor(ptColor.x + 0.5f);
		ptColor.y = floor(ptColor.y + 0.5f);

		if (ptColor.x < 0)
			continue;
		if (ptColor.x >= imData.width_)
			continue;
		if (ptColor.y < 0)
			continue;
		if (ptColor.y >= imData.height_)
			continue;	

		glm::vec2 ptDepth = io::Image::undistortAndProject(pt->pos, pcData.distortion_, pcData.f_, pcData.c_);
		ptDepth = glm::vec2(width_ - 1.f, height_ - 1.f) - ptDepth;
		ptDepth.x = std::max(0.f, std::min(width_ - 1.f, floor(ptDepth.x + 0.5f)));
		ptDepth.y = std::max(0.f, std::min(height_ - 1.f, floor(ptDepth.y + 0.5f)));

		DepthPoint* curPt = &pts_.get()[(int)(ptDepth.y*width_ + ptDepth.x)];
		curPt->color = img_->pixelAsVector((size_t)ptColor.y, (size_t)ptColor.x, true);
		curPt->pos = pt->pos;

		curPt->depth = ptTrans.z;
		curPt->flags = pc::KnownPoint;
		
#ifdef _WINDOWS
		curPt->x = ptColor.x;
		curPt->y = ptColor.y;
#endif
	}

	markReliablePoints();

	// Fill-in missing pixels	
	if (!fillHoles_)
		return true;

	DepthPoint* curPt = pts_.get();
	glm::vec2* ptDepth = ptMap_.get();
	for (int row = 0; row < height_; row++) {
		for (int col = 0; col < width_; col++) {
			bool validDepthPx = true;

			if (curPt->flags & pc::KnownPoint) 
				validDepthPx = false;
			else if (ptDepth->x == 0 && ptDepth->y == 0) // The pixel is not mapped to a valid location
				validDepthPx = false;

			if (validDepthPx) {
				glm::vec3 ptTrans = imPose*glm::vec4(ptDepth->x, ptDepth->y, 1.f, 1.f);
				glm::vec2 ptColor = io::Image::undistortAndProject(ptTrans, imData.distortion_, imData.f_, imData.c_);
				ptColor.x = floor(ptColor.x + 0.5f);
				ptColor.y = floor(ptColor.y + 0.5f);

				bool validColorPx = true;
				if (ptColor.x < 0)
					validColorPx = false;
				else if (ptColor.x >= imData.width_)
					validColorPx = false;
				else if (ptColor.y < 0)
					validColorPx = false;
				else if (ptColor.y >= imData.height_)
					validColorPx = false;

				if (validColorPx) {
					curPt->color = img_->pixelAsVector((size_t)ptColor.y, (size_t)ptColor.x, true);
					
					if (fillWithMax_)
						curPt->depth = MaxDepth;
					else
						curPt->depth = estimateDepth(glm::i16vec2(col, row));

					if (!isnan(curPt->depth)) {
						float den = (imPose[0][2] * ptDepth->x + imPose[1][2] * ptDepth->y + imPose[2][2]);
						curPt->pos.z = (curPt->depth - imPose[3][2]) / den;
						curPt->pos.x = ptDepth->x*curPt->pos.z;
						curPt->pos.y = ptDepth->y*curPt->pos.z;
						curPt->flags |= pc::KnownPoint;
					}					
				}
			}
			
			++ptDepth;
			++curPt;
		}
	}	

	return true;
}
#endif

void DepthMap::readDepthMappingFile() {
	ifstream file(MaskFile, ios::in | ios::binary);
	if (file.is_open()) {
		ptMap_.reset(new glm::vec2[width_*height_], std::default_delete<glm::vec2[]>());

		file.read((char*)ptMap_.get(), sizeof(float)*width_*height_*2);
		file.close();
	}
}

float DepthMap::estimateDepth(const glm::i16vec2& pos) {
	float d[8];
	float dt[8];

	findKnownDepth(pos, glm::i16vec2(-1, -1), d[0], dt[0]);
	findKnownDepth(pos, glm::i16vec2(-1,  0), d[1], dt[1]);
	findKnownDepth(pos, glm::i16vec2(-1,  1), d[2], dt[2]);
	findKnownDepth(pos, glm::i16vec2( 0, -1), d[3], dt[3]);
	findKnownDepth(pos, glm::i16vec2( 0,  1), d[4], dt[4]);
	findKnownDepth(pos, glm::i16vec2( 1, -1), d[5], dt[5]);
	findKnownDepth(pos, glm::i16vec2( 1,  0), d[6], dt[6]);
	findKnownDepth(pos, glm::i16vec2( 1,  1), d[7], dt[7]);

	float dSum = 0.f;
	float dSumWeight = 0.f;
	for (int i = 0; i < 8; i++) {
		float dtWeight = dt[i] != 0 ? (width_ - dt[i]) / width_ : 0.f;
		dSum += d[i] * dtWeight;
		dSumWeight += dtWeight;
	}

	return dSum / dSumWeight;	
}

void DepthMap::findKnownDepth(const glm::i16vec2& pos, const glm::i16vec2& dir, float& depth, float& dt) {
	glm::i16vec2 curPos = pos;
	dt = 0.f;
	depth = 0.f;

	while (true) {
		curPos += dir;

		if (curPos.x < 0)
			break;
		else if (curPos.x >= width_)
			break;

		if (curPos.y < 0)
			break;
		else if (curPos.y >= height_)
			break;

		const DepthPoint* ptDepth = &pts_.get()[curPos.y*width_ + curPos.x];
		if (ptDepth->flags == pc::KnownPoint) {
			depth = ptDepth->depth;

			float dx = (float)(curPos.x - pos.x);
			float dy = (float)(curPos.y - pos.y);

			dt = sqrt(dx*dx + dy*dy);

			break;
		}
	}
}

 void DepthMap::asPointCloud(std::shared_ptr<pc::PointCloud>& pc) {
	DepthPoint* curPt = pts_.get();
	for (int row = 0; row < height_; row++) {
		for (int col = 0; col < width_; col++) {
			if (curPt->flags & pc::KnownPoint) { // Known or estimated
				pc::Point pt = *curPt;

				pt.pos.x = static_cast<float> (pose_[0][0] * curPt->pos[0] + pose_[1][0] * curPt->pos[1] + pose_[2][0] * curPt->pos[2] + pose_[3][0]);
				pt.pos.y = static_cast<float> (pose_[0][1] * curPt->pos[0] + pose_[1][1] * curPt->pos[1] + pose_[2][1] * curPt->pos[2] + pose_[3][1]);
				pt.pos.z = static_cast<float> (pose_[0][2] * curPt->pos[0] + pose_[1][2] * curPt->pos[1] + pose_[2][2] * curPt->pos[2] + pose_[3][2]);

				pc->addPoint(pt);
			}

			++curPt;
		}
	}

	pc->setPosition(glm::vec3(pose_[3][0], pose_[3][1], pose_[3][2]));
}

 const std::shared_ptr<io::Image>& DepthMap::colorCameraImage() {
	 return img_;
 }

 std::shared_ptr<io::Image> DepthMap::asImage() {
	 std::shared_ptr<io::Image> img;
	 img.reset(new io::Image(width_, height_));

	 DepthPoint* curPtr = pts_.get();
	 for (size_t row = 0; row < height_; row++) {
		 uchar* imgPtr = img->row(row);

		 for (size_t col = 0; col < width_; col++) {
			 glm::vec3 sRGB = color::Color::linRGB2sRGB(curPtr->color);
			 *imgPtr++ = uchar(sRGB.r * 255);
			 *imgPtr++ = uchar(sRGB.g * 255);
			 *imgPtr++ = uchar(sRGB.b * 255);
			 *imgPtr++ = 255;

			 ++curPtr;
		 }
	 }

	 return img;
 }
 
 void DepthMap::markReliablePoints() {	 
	 DepthPoint* curPt = pts_.get() - 1;
	 for (int row = 0; row < height_; row++) {
		 for (int col = 0; col < width_; col++) {
			 ++curPt;

			 // Border pixels are not reliable
			 if (row == 0)
				 continue;
			 else if (row == (height_ - 1))
				 continue;

			 if (col == 0)
				 continue;
			 else if (col == (width_ - 1))
				 continue;
			 
			 if (curPt->flags != pc::KnownPoint) // Only trusted pixels are marked as reliable
				 continue;

			 // Under- and over-exposed pixels aren't considered
			 if (curPt->color.r < MinExposure)
				 continue;
			 if (curPt->color.r > MaxExposure)
				 continue;
			 if (curPt->color.g < MinExposure)
				 continue;
			 if (curPt->color.g > MaxExposure)
				 continue;
			 if (curPt->color.b < MinExposure)
				 continue;
			 if (curPt->color.b > MaxExposure)
				 continue;

			 DepthPoint* neighPt = curPt - width_ - 1;
			 float sqSumD = 0.f;
			 float sumD = 0.f;
			 bool valid = true;
			 for (int i = 0; i < 3; i++) {
				 for (int j = 0; j < 3; j++) {
					 if (neighPt != curPt) {
						 if ((neighPt->flags == pc::KnownPoint) || (neighPt->flags == pc::ReliableKnownPoint)) { // Only trusted pixels are marked as reliable
							 sqSumD += neighPt->depth*neighPt->depth;
							 sumD += neighPt->depth;
						 } else {
							 valid = false;
							 break;
						 }
					 }

					 ++neighPt;
				 }

				 if (!valid)
					 break;

				 neighPt += width_ - 3;
			 }

			 if (valid) {
				 float chiSqValue = (sqSumD - 2 * curPt->depth*sumD + 8 * curPt->depth*curPt->depth) / curPt->depth;

				 if (chiSqValue <= LimitChiSquare)
					 curPt->flags |= pc::ReliablePoint;					 				 
			 }			 
		 }
	 }
 }
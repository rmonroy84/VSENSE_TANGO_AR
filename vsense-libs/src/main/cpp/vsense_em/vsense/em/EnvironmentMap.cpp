#include <vsense/em/EnvironmentMap.h>

#include <vsense/depth/DepthMap.h>
#include <vsense/io/Image.h>
#include <vsense/common/Util.h>

#include <iostream>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <time.h>

using namespace vsense;
using namespace vsense::em;

const float BetaAt = 2.f;
const float BetaCt = 8.f;
const float Gamma = 0.2f;

const glm::vec3 FrontDir(0.f, 0.f, 1.f);

const float MaxDepthDiff = 0.01f; // 1cm
const float MaxTrustedCos = cos(glm::radians(2.5f));

const float TrustedRadius = 0.3; // 30cm

const float NeighbordHoodDelta = glm::radians(0.25f);
const float MaxVariance = 0.0075f;

uint32_t EnvironmentMap::width_  = 2000;
uint32_t EnvironmentMap::height_ = EnvironmentMap::width_/2;

float EnvironmentMap::maxError_ = 0.1f;
int EnvironmentMap::minNbrPoints_ = 100;
bool EnvironmentMap::colorCorrection_ = true;
float EnvironmentMap::maxAllowedWarpDif_ = 0.01f;

const int NeighborPixels = 2;

const int Precision = 18;

#define WITH_EXTRA
//#define CHECK_EM_UNIFORM
#define CHECK_RELIABLE_REFERENCE
#define CHECK_RELIABLE_CURRENT

EnvironmentMap::EnvironmentMap(const glm::vec3& origin) : origin_(origin), isEmpty_(true), depthRange_(FLT_MAX, -FLT_MAX), lastError_(-1.f) {

}

void EnvironmentMap::clearEM() {
	isEmpty_ = true;
}

bool EnvironmentMap::addDepthMapFrame(const depth::DepthMap* dm, bool projectPts, bool renderImage) {
#ifdef _WINDOWS
	clock_t t = clock();
  uint16_t discardedNotUniform = 0;
#endif

	if (isEmpty_) { // Initializing maps
		color_.reset(new glm::vec3[width_*height_], std::default_delete<glm::vec3[]>());
		depth_.reset(new float[width_*height_], std::default_delete<float[]>());
		flags_.reset(new uchar[width_*height_], std::default_delete<uchar[]>());

		float* depthPtr = depth_.get();
		glm::vec3* colorPtr = color_.get();
		for (size_t i = 0; i < width_*height_; i++) {
			*depthPtr = -1;
			colorPtr->r = colorPtr->g = colorPtr->b = -1;
			
			++depthPtr;
			++colorPtr;			
		}

		memset(flags_.get(), 0, sizeof(uchar)*width_*height_);

		lastSamples_.reset(new EMSample[depth::DepthMap::width()*depth::DepthMap::height()], std::default_delete<EMSample[]>());
	}
	
	const float* pose = &(*dm->getPose())[0][0];
	const depth::DepthPoint* curPt = dm->getDataPtr() - 1;

	glm::vec3 ptDir;
	glm::vec3 ptPos;

	glm::vec3 devPos(pose[12], pose[13], pose[14]);
	
	glm::vec3 devOr(devPos - origin_);
	float distToDev = sqrt(devOr.x*devOr.x + devOr.y*devOr.y + devOr.z*devOr.z);	

	glm::vec3 devDir(pose[8], pose[9], pose[10]);
	devDir = glm::normalize(devDir);	

	uint16_t totalPts = 0;
	nbrSamples_ = 0;

	EMSample* samplesPtr = lastSamples_.get();

	size_t nbrPixels = depth::DepthMap::nbrPixels();
	for(size_t k = 0; k < nbrPixels; k++) {
		totalPts++;

		++curPt;

		if (curPt->flags & pc::KnownPoint) { // Known or estimated
			EMSample sample;
			sample.curColor = curPt->color;

#ifdef _WINDOWS
			sample.x = curPt->x;
			sample.y = curPt->y;
#endif

			ptPos.x = pose[0] * curPt->pos.x + pose[4] * curPt->pos.y + pose[8] * curPt->pos.z + devOr.x;
			ptPos.y = pose[1] * curPt->pos.x + pose[5] * curPt->pos.y + pose[9] * curPt->pos.z + devOr.y;
			ptPos.z = pose[2] * curPt->pos.x + pose[6] * curPt->pos.y + pose[10] * curPt->pos.z + devOr.z;

			sample.curDepth = sqrt(ptPos.x*ptPos.x + ptPos.y*ptPos.y + ptPos.z*ptPos.z);

			ptDir = ptPos/sample.curDepth;

			glm::vec3 devPt = ptPos - devPos;
			sample.curDevDist = sqrt(devPt.x*devPt.x + devPt.y*devPt.y + devPt.z*devPt.z);
			sample.curCosPlane = glm::dot(ptDir, devDir);

#ifdef CHECK_EM_UNIFORM
			sample.sphCoords.x = acos(ptDir.y); // theta
			sample.sphCoords.y = atan2(ptDir.x, ptDir.z); //phi

			if (sample.sphCoords.y < 0.f)
				sample.sphCoords.y = M_2PI + sample.sphCoords.y;
				
			uint32_t u = (int)floor(sample.sphCoords.y*(width_ - 1) / M_2PI + 0.5f);
			uint32_t v = (int)floor(sample.sphCoords.x*(height_ - 1) / M_PI + 0.5f);
#else
			float theta = acos(ptDir.y);
			float phi = atan2(ptDir.x, ptDir.z);

			if (phi < 0.f)
				phi += M_2PI;

			uint32_t u = (int)floor(phi*(width_ - 1) / M_2PI + 0.5f);
			uint32_t v = (int)floor(theta*(height_ - 1) / M_PI + 0.5f);
#endif

			sample.uvOffset = v*width_ + u;

			sample.refColor = *(color_.get() + sample.uvOffset);
			sample.refDepth = *(depth_.get() + sample.uvOffset);

#ifdef CHECK_EM_UNIFORM
			if (sample.refDepth >= 0) {
				if (!isEMNeighborhoodUniform(sample)) {
#ifdef _WINDOWS
					discardedNotUniform++;
#endif
					continue;
				}
			}
#endif

			sample.refIsReliable = *(flags_.get() + sample.uvOffset) != 0;
			sample.curIsReliable = (curPt->flags == pc::ReliableKnownPoint);
			sample.used = true;

			samplesPtr[nbrSamples_++] = sample;
		}		
	}

#ifdef _WINDOWS
	system("cls");
	std::cout << "Total points: " << totalPts << std::endl;
	std::cout << "Discarded Non-Uniform: " << discardedNotUniform << std::endl;

	t = clock() - t;
	lastElapsedTime_ = (float) t / CLOCKS_PER_SEC;
#endif

	glm::mat3 corrMtx;
	if (!isEmpty_ && colorCorrection_) {
		if (calculateCorrectionMtx()) {
			lastError_ = calculateError();

			if (lastError_ > maxError_)
				return false;
		} else {
			lastError_ = -1.f;
			return false;
		}
	}

	if (!projectPts)
		return true;

	projectPoints(distToDev);
	isEmpty_ = false;

	if (renderImage)
		renderToImage();

	return true;
}

bool EnvironmentMap::calculateCorrectionMtx() {
	glm::dmat3 matA(0.f);
	glm::dmat3 matB(0.f);

	glm::vec3 wCur(0.f, 0.f, 0.f);
	glm::vec3 wRef(0.f, 0.f, 0.f);

	lastNbrUsedPts_ = 0;

#ifdef _WINDOWS
	uint16_t discNoRef = 0;
	uint16_t discLargeDepthDelta = 0;
	uint16_t discCurUnreliable = 0;
	uint16_t discRefUnreliable = 0;
#endif

	EMSample* pSample = lastSamples_.get() - 1;
	for (size_t i = 0; i < nbrSamples_; i++) {
		++pSample;

		if (pSample->refDepth < 0.f) { // No reference data available
#ifdef _WINDOWS
			discNoRef++;
#endif
			pSample->used = false;
			continue;
		}

#ifdef CHECK_RELIABLE_REFERENCE
		if (!pSample->refIsReliable) { // Ignore reference unreliable points
#ifdef _WINDOWS
			discRefUnreliable++;
#endif
			pSample->used = false;
			continue;
		}
#endif

#ifdef CHECK_RELIABLE_CURRENT
		if (!pSample->curIsReliable) { // Ignore current unreliable points
#ifdef _WINDOWS
			discCurUnreliable++;
#endif
			pSample->used = false;
			continue;
		}
#endif

		if (std::abs(pSample->curDepth - pSample->refDepth) >= MaxDepthDiff) { // If the difference with the current and current depth is larger than allowed
#ifdef _WINDOWS
			discLargeDepthDelta++;
#endif
			pSample->used = false;
			continue;
		}

#ifdef WITH_EXTRA
		glm::vec3 curHSV = color::Color::rgb2hsv(pSample->curColor);
		glm::vec3 refHSV = color::Color::rgb2hsv(pSample->refColor);

		float difH = curHSV[0] - refHSV[0];
		float difS = curHSV[1] - refHSV[1];
		float dist = sqrt(difH*difH + difS*difS);

		float wAt = pow(1 - std::min(1.f, dist), BetaAt);
		float wCt = pow(1 - std::min(1.f, dist), BetaCt);

		wCur += wAt*pSample->curColor;
		wRef += wAt*pSample->refColor;

		matA[0][0] += pSample->curColor.r*pSample->curColor.r*wCt;
		matA[1][1] += pSample->curColor.g*pSample->curColor.g*wCt;
		matA[2][2] += pSample->curColor.b*pSample->curColor.b*wCt;
		matA[0][1] += pSample->curColor.r*pSample->curColor.g*wCt;
		matA[0][2] += pSample->curColor.r*pSample->curColor.b*wCt;
		matA[1][2] += pSample->curColor.g*pSample->curColor.b*wCt;

		matB[0][0] += pSample->curColor.r*pSample->refColor.r*wCt;
		matB[1][1] += pSample->curColor.g*pSample->refColor.g*wCt;
		matB[2][2] += pSample->curColor.b*pSample->refColor.b*wCt;
		matB[0][1] += pSample->curColor.r*pSample->refColor.g*wCt;
		matB[0][2] += pSample->curColor.r*pSample->refColor.b*wCt;
		matB[1][0] += pSample->curColor.g*pSample->refColor.r*wCt;
		matB[1][2] += pSample->curColor.g*pSample->refColor.b*wCt;
		matB[2][0] += pSample->curColor.b*pSample->refColor.r*wCt;
		matB[2][1] += pSample->curColor.b*pSample->refColor.g*wCt;
#else
		matA[0][0] += pSample->curColor.r*pSample->curColor.r;
		matA[1][1] += pSample->curColor.g*pSample->curColor.g;
		matA[2][2] += pSample->curColor.b*pSample->curColor.b;
		matA[0][1] += pSample->curColor.r*pSample->curColor.g;
		matA[0][2] += pSample->curColor.r*pSample->curColor.b;
		matA[1][2] += pSample->curColor.g*pSample->curColor.b;

		matB[0][0] += pSample->curColor.r*pSample->refColor.r;
		matB[1][1] += pSample->curColor.g*pSample->refColor.g;
		matB[2][2] += pSample->curColor.b*pSample->refColor.b;
		matB[0][1] += pSample->curColor.r*pSample->refColor.g;
		matB[0][2] += pSample->curColor.r*pSample->refColor.b;
		matB[1][0] += pSample->curColor.g*pSample->refColor.r;
		matB[1][2] += pSample->curColor.g*pSample->refColor.b;
		matB[2][0] += pSample->curColor.b*pSample->refColor.r;
		matB[2][1] += pSample->curColor.b*pSample->refColor.g;
#endif		

		lastNbrUsedPts_++;
	}

#ifdef _WINDOWS
	std::cout << "Discarded No Reference: " << discNoRef << std::endl;
	std::cout << "Discarded Large Depth Delta: " << discLargeDepthDelta << std::endl;
	std::cout << "Discarded Reference Unreliable: " << discRefUnreliable << std::endl;
	std::cout << "Discarded Current Unreliable: " << discCurUnreliable << std::endl;
#endif

	std::cout << "Used points: " << lastNbrUsedPts_ << std::endl;

	if (lastNbrUsedPts_ < minNbrPoints_)
		return false;

	// It's a symmetric matrix
	matA[1][0] = matA[0][1];
	matA[2][0] = matA[0][2];
	matA[2][1] = matA[1][2];	

#ifdef WITH_EXTRA
	glm::vec3 s = wRef / wCur;

	std::cout << "S: " << s.x << ", " << s.y << ", " << s.z << std::endl;

	matA[0][0] += Gamma;
	matA[1][1] += Gamma;
	matA[2][2] += Gamma;

	matB[0][0] += Gamma*s.r;
	matB[1][1] += Gamma*s.g;
	matB[2][2] += Gamma*s.b;
#endif

	if (glm::determinant(matA) == 0) { // Non-invertible																		
		std::cout << "Determinant is zero!" << std::endl;

		return false;
	}

	glm::dmat3 matInvA = glm::inverse(matA);
	lastCorrMtx_ = matB*matInvA;

	return true;
}

void EnvironmentMap::asSHCoefficients(std::shared_ptr<sh::SHCoefficients3>& coeff, long nbrSamples, bool skipEmpty, int order) {
	const std::shared_ptr<glm::vec2>& randSph = sh::SphericalHarmonics::getRandomSphericalCoords();

	const float* curSample = &randSph->x;
	std::vector<sh::SphericalSample3> samples;
	for (long n = 0; n < nbrSamples; n++) {
		float theta = *curSample++;
		float phi = *curSample++;

		int x = (int)floor(phi*(width_ - 1) / M_2PI + 0.5f);
		int y = (int)floor(theta*(height_ - 1) / M_PI + 0.5f);
		
		sh::SphericalSample3 sample;
		sample.first = glm::vec2(theta, phi);
		sample.second = *(color_.get() + y*width_ + x);

		if(sample.second.r < 0) {
			if (skipEmpty)
				continue;
			else
				sample.second = glm::vec3(0.f, 0.f, 0.f);
		}

		samples.push_back(sample);
	}

	coeff = sh::SphericalHarmonics::projectSamples(order, samples);
}

void EnvironmentMap::projectPoints(float distToDev) {
	size_t nbrAdded = 0;
	EMSample* sample = lastSamples_.get() - 1;
	glm::vec3* colorPtr = color_.get();
	float* depthPtr = depth_.get();
	uchar* flagsPtr = flags_.get();

	for (size_t i = 0; i < nbrSamples_; i++) {
		++sample;
		
		if(!isEmpty_ && colorCorrection_)
			sample->curColor = glm::vec3(lastCorrMtx_*sample->curColor);

		if (sample->curColor.r < 0)
			continue;
		if (sample->curColor.g < 0)
			continue;
		if (sample->curColor.b < 0)
			continue;
		
		if (!sample->curIsReliable || sample->refIsReliable || (sample->curDepth > sample->refDepth)) {			
			if (distToDev > TrustedRadius) { // If device is located outside the trusted sphere (near the virtual object)
				if (sample->refDepth >= 0.f) { // Existing data already
					if (sample->curCosPlane < MaxTrustedCos) // If orientation is not closely aligned with orientation of the point
						continue;
					if (sample->refDepth < sample->curDevDist) // If device is behind the previous data, we can't really confirm it's gone
						continue;
				}
			}
		}

		*(colorPtr + sample->uvOffset) = sample->curColor;

    float* curDepthPtr = depthPtr + sample->uvOffset;
		if(sample->refDepth < 0.f)
			*curDepthPtr = sample->curDepth;
		else
			*curDepthPtr = (sample->curDepth + sample->refDepth)/2.f;

		*(flagsPtr + sample->uvOffset) = sample->curIsReliable ? 1 : 0;

    if(*curDepthPtr >= 0) {
      depthRange_.x = std::min(depthRange_.x, *curDepthPtr);
      depthRange_.y = std::max(depthRange_.y, *curDepthPtr);
    }

		nbrAdded++;					
	}

	if(nbrAdded > 0)
		isEmpty_ = false;

	std::cout << "Added: " << nbrAdded << "/" << nbrSamples_ << std::endl;
}

float EnvironmentMap::calculateError() {
	float meanError = 0.f;

	size_t nbrPts = 0;
	const EMSample* pSample = lastSamples_.get() - 1;
	for (size_t i = 0; i < nbrSamples_; i++) {
		++pSample;

		if (!pSample->used) // Not used for calibration
			continue;
		
		meanError += getSquaredError(*pSample);

		nbrPts++;
	}

	meanError /= nbrPts;
	std::cout << "Correction error: " << meanError << std::endl;

	return meanError;
}

float EnvironmentMap::getSquaredError(const EMSample& sample) {
	glm::vec3 diff = sample.refColor - glm::vec3(lastCorrMtx_*sample.curColor);
	
	return diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
}

void EnvironmentMap::renderToImage() {	
	if (!img_)
		img_.reset(new io::Image(width_, height_));

	if (color_) {
		for (size_t row = 0; row < img_->rows(); row++) {
			uchar *rowPtr = img_->row(row);
			glm::vec3 *rowColorPtr = color_.get() + row * width_;
			float* rowDepthPtr = depth_.get() + row*width_;

			for (size_t col = 0; col < img_->cols(); col++) {
				glm::vec3 sRGB = *rowColorPtr;

				for (int c = 0; c < 3; c++)
					sRGB[c] = std::max(0.f, sRGB[c]);

				sRGB = color::Color::linRGB2sRGB(sRGB);
				glm::u8vec3 color = glm::uvec3(std::min(1.f, sRGB[0]) * 255, std::min(1.f, sRGB[1]) * 255,
					std::min(1.f, sRGB[2]) * 255);

				for (int c = 0; c < 3; c++)
					*rowPtr++ = static_cast<unsigned char>(color[c]);		

				if(*rowDepthPtr < 0)
					*rowPtr++ = 0;
				else
					*rowPtr++ = static_cast<unsigned char>((depthRange_.y - *rowDepthPtr) / (depthRange_.y - depthRange_.x)*255);

				++rowColorPtr;
				++rowDepthPtr;
			}
		}
	} else {
		for (size_t row = 0; row < img_->rows(); row++) {
			uchar *rowPtr = img_->row(row);

			for (size_t col = 0; col < img_->cols(); col++) {
				for (int c = 0; c < 4; c++)
					*rowPtr++ = 0;				
			}
		}
	}
}

const std::shared_ptr<io::Image>& EnvironmentMap::asImage() {
	if (!img_)
		renderToImage();

	return img_;
}

void EnvironmentMap::updateOrigin(const glm::vec3 &origin) {
	origin_ = origin;
}

bool EnvironmentMap::isEMNeighborhoodUniform(const EMSample& sample) {
	glm::vec3 sumSq(0.f, 0.f, 0.f);
	glm::vec3 sum(0.f, 0.f, 0.f);

	float theta, phi;
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			theta = sample.sphCoords.x + i*NeighbordHoodDelta;
			phi = sample.sphCoords.y + j*NeighbordHoodDelta;
			
			if (phi < 0)
				phi += M_2PI;

			if (theta < 0) {
				theta += M_PI;
				phi += M_PI; // Invert direction

				if (phi < M_2PI)
					phi -= M_2PI;				
			}

			if (theta > M_PI) {
				theta -= M_PI;

				phi += M_PI; // Invert direction

				if (phi < M_2PI)
					phi -= M_2PI;
			}
			
			if (phi > M_2PI)
				phi -= M_2PI;
			
			
			size_t offsetX = (int)floor(phi*(width_ - 1) / M_2PI + 0.5f); 
			size_t offsetY = (int)floor(theta*(height_ - 1) / M_PI + 0.5f); 
			size_t offset = offsetY*width_ + offsetX;

			glm::vec3 curColor = *(color_.get() + offset);
			sum += curColor;
			sumSq += curColor * curColor;
		}
	}

	sumSq /= 9.f;
	sum /= 9.f;
	sum *= sum;	

	sumSq -= sum;

	for (int i = 0; i < 3; i++) {
		if (sumSq[i] > MaxVariance)
			return false;
	}
	
	return true;
}

#ifdef _WINDOWS
void EnvironmentMap::saveSamplesToFile() {
	std::ofstream samplesFile;
	samplesFile.open("D:/data/out/samples.csv");

	glm::dmat3 matA(0.f);
	glm::dmat3 matB(0.f);

	glm::vec3 wCur(0.f, 0.f, 0.f);
	glm::vec3 wRef(0.f, 0.f, 0.f);

	uint32_t lastNbrUsedPts = 0;

	const EMSample* pSample = lastSamples_.get() - 1;
	for (size_t i = 0; i < nbrSamples_; i++) {
		++pSample;

		samplesFile << std::setprecision(Precision) << pSample->refColor.r << ", " << pSample->refColor.g << ", " << pSample->refColor.b << ", ";
		samplesFile << std::setprecision(Precision) << pSample->curColor.r << ", " << pSample->curColor.g << ", " << pSample->curColor.b << std::endl;

		if (pSample->refDepth < 0.f) // No reference data available
			continue;

#ifdef CHECK_RELIABLE_REFERENCE
		if (!pSample->refIsReliable) // Ignore reference unreliable points
			continue;
#endif

#ifdef CHECK_RELIABLE_CURRENT
		if (!pSample->curIsReliable) // Ignore current unreliable points
			continue;
#endif

		if (std::abs(pSample->curDepth - pSample->refDepth) >= MaxDepthDiff) // If the difference with the current and current depth is larger than allowed
			continue;

		glm::vec3 curHSV = color::Color::rgb2hsv(pSample->curColor);
		glm::vec3 refHSV = color::Color::rgb2hsv(pSample->refColor);

		float difH = curHSV[0] - refHSV[0];
		float difS = curHSV[1] - refHSV[1];
		float dist = sqrt(difH*difH + difS*difS);

		float wAt = pow(1 - std::min(1.f, dist), BetaAt);
#ifdef WITH_EXTRA
		float wCt = pow(1 - std::min(1.f, dist), BetaCt);
#else
		float wCt = 1.f;
#endif

		wCur += wAt*pSample->curColor;
		wRef += wAt*pSample->refColor;

		matA[0][0] += pSample->curColor.r*pSample->curColor.r*wCt;
		matA[1][1] += pSample->curColor.g*pSample->curColor.g*wCt;
		matA[2][2] += pSample->curColor.b*pSample->curColor.b*wCt;
		matA[0][1] += pSample->curColor.r*pSample->curColor.g*wCt;
		matA[0][2] += pSample->curColor.r*pSample->curColor.b*wCt;
		matA[1][2] += pSample->curColor.g*pSample->curColor.b*wCt;

		matB[0][0] += pSample->curColor.r*pSample->refColor.r*wCt;
		matB[1][1] += pSample->curColor.g*pSample->refColor.g*wCt;
		matB[2][2] += pSample->curColor.b*pSample->refColor.b*wCt;
		matB[0][1] += pSample->curColor.r*pSample->refColor.g*wCt;
		matB[0][2] += pSample->curColor.r*pSample->refColor.b*wCt;
		matB[1][0] += pSample->curColor.g*pSample->refColor.r*wCt;
		matB[1][2] += pSample->curColor.g*pSample->refColor.b*wCt;
		matB[2][0] += pSample->curColor.b*pSample->refColor.r*wCt;
		matB[2][1] += pSample->curColor.b*pSample->refColor.g*wCt;

		lastNbrUsedPts++;
	}

	// It's a symmetric matrix
	matA[1][0] = matA[0][1];
	matA[2][0] = matA[0][2];
	matA[2][1] = matA[1][2];

	glm::vec3 s = wRef / wCur;

#ifdef WITH_EXTRA
	matA[0][0] += Gamma;
	matA[1][1] += Gamma;
	matA[2][2] += Gamma;

	matB[0][0] += Gamma*s.r;
	matB[1][1] += Gamma*s.g;
	matB[2][2] += Gamma*s.b;
#endif

	samplesFile.close();

	if (glm::determinant(matA) == 0) // Non-invertible																		
		return;

	glm::dmat3 matInvA = glm::inverse(matA);
	glm::dmat3 corr = matB*matInvA;

	std::ofstream matAFile;
	matAFile.open("D:/data/out/matA.csv");
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			matAFile << std::setprecision(Precision) << matA[i][j];
			if (j != 2)
				matAFile << ", ";
		}

		matAFile << std::endl;
	}
	matAFile.close();

	std::ofstream matBFile;
	matBFile.open("D:/data/out/matB.csv");
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			matBFile << std::setprecision(Precision) << matB[i][j];
			if (j != 2)
				matBFile << ", ";
		}

		matBFile << std::endl;
	}
	matBFile.close();

	std::ofstream matCorrFile;
	matCorrFile.open("D:/data/out/matCorr.csv");
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			matCorrFile << std::setprecision(Precision) << corr[i][j];
			if (j != 2)
				matCorrFile << ", ";
		}

		matCorrFile << std::endl;
	}
	matCorrFile.close();
}

void EnvironmentMap::loadFromData(const uchar* data, size_t bytesPerRow) {
	color_.reset(new glm::vec3[width_*height_], std::default_delete<glm::vec3[]>());
	depth_.reset(new float[width_*height_], std::default_delete<float[]>());
	flags_.reset(new uchar[width_*height_], std::default_delete<uchar[]>());

	float* colorPtr = &color_.get()->x;
	const uchar* dataPtr;
	for (size_t row = 0; row < height_; row++) {
		dataPtr = data + row*bytesPerRow;
		for (size_t col = 0; col < width_; col++) {
			for (int c = 0; c < 3; c++)
				*colorPtr++ = (float)(*dataPtr++) / 255.f;
		}
	}
}

void EnvironmentMap::loadFromData(const float* data, const glm::vec3& origin) {
	color_.reset(new glm::vec3[width_*height_], std::default_delete<glm::vec3[]>());
	depth_.reset(new float[width_*height_], std::default_delete<float[]>());
	flags_.reset(new uchar[width_*height_], std::default_delete<uchar[]>());
	origin_ = origin;

	float* colorPtr = &color_.get()->x;
	float* depthPtr = depth_.get();

	const float* dataPtr = data;
	for (size_t row = 0; row < height_; row++) {
		for (size_t col = 0; col < width_; col++) {
			for (int c = 0; c < 4; c++) {
				if (c < 3)
					*colorPtr++ = *dataPtr++;
				else
					*depthPtr++ = *dataPtr++;
			}
		}
	}
}

void EnvironmentMap::copy(const EnvironmentMap& srcEM) {
	color_.reset(new glm::vec3[width_*height_], std::default_delete<glm::vec3[]>());
	depth_.reset(new float[width_*height_], std::default_delete<float[]>());
	flags_.reset(new uchar[width_*height_], std::default_delete<uchar[]>());

	memcpy(color_.get(), srcEM.getColorPtr(), sizeof(glm::vec3)*width_*height_);
	memcpy(depth_.get(), srcEM.getDepthPtr(), sizeof(float)*width_*height_);
	memcpy(flags_.get(), srcEM.getFlagsPtr(), sizeof(uchar)*width_*height_);
}

void EnvironmentMap::setEMSize(size_t width, size_t height) {
	width_ = width;
	height_ = height;
}

void EnvironmentMap::saveMaps() {
	std::ofstream colorFile;
	colorFile.open("D:/data/out/colorEM.bin", std::ios::out | std::ios::binary);

	std::ofstream depthFile;
	depthFile.open("D:/data/out/depth.bin", std::ios::out | std::ios::binary);

	colorFile.write((const char*)&width_, sizeof(uint32_t));
	colorFile.write((const char*)&height_, sizeof(uint32_t));

	depthFile.write((const char*)&width_, sizeof(uint32_t));
	depthFile.write((const char*)&height_, sizeof(uint32_t));
	
	float* depthPtr = depth_.get();
	for (int channel = 0; channel < 3; channel++) {
		float* colorPtr = &color_.get()->r + channel;		
		for (uint32_t row = 0; row < height_; row++) {
			for (uint32_t col = 0; col < width_; col++) {
				colorFile.write((const char*)colorPtr, sizeof(float));
				colorPtr += 3;

				if (channel == 0) {
					float curDepth = *depthPtr++;

					if (curDepth < 0.f)
						curDepth = 0.f;
					
					depthFile.write((const char*)&curDepth, sizeof(float));
				}
			}
		}
	}

	colorFile.close();
	depthFile.close();	
}

#endif

glm::vec3 EnvironmentMap::findDisplacementUS(const glm::vec3& posWorld) const {
	glm::vec3 dir = posWorld - origin_;
	float sqDist = dir.x*dir.x + dir.z*dir.z; // Projected squared distance on the X/Z plane
	dir = glm::normalize(dir);

	float phi = sh::SphericalHarmonics::toSphericalCoords(dir).x;	

	double deltaTheta = M_PI / height_;
	double deltaPhi = M_2PI / width_;
	uint32_t halfHeight = height_ / 2;	

	float hFactor = height_ / M_PI;
	float wFactor = width_ / M_2PI;	

	phi -= deltaPhi*NeighborPixels;
	if (phi < 0)
		phi += M_2PI;			

	float bestTheta;
	float bestSqDiff = FLT_MAX;

	// Search will be done only in lower hemisphere (theta > PI/2)	
	for(uint32_t col = 0; col < NeighborPixels*2 + 1; col++) {
		float theta = deltaTheta*halfHeight;

		if (phi > M_2PI)
			phi -= M_2PI;

		uint16_t srcRow = (uint16_t)std::min(height_ - 1.f, std::max(0.f, floorf(theta*hFactor + 0.5f)));
		uint16_t srcCol = (uint16_t)std::min(width_ - 1.f, std::max(0.f, floorf(phi*wFactor + 0.5f)));

		float* curDepth = depth_.get() + srcRow*width_ + srcCol;		

		for (uint32_t row = halfHeight; row < height_; row++) {
			if (*curDepth > 0.f) {
				glm::vec3 curDir = sh::SphericalHarmonics::toVector(phi, theta);
				glm::vec3 curPos = curDir*(*curDepth);

				float curSqDist = curPos.x*curPos.x + curPos.z*curPos.z;
				float curSqDiff = std::abs(curSqDist - sqDist);

				if (curSqDiff < bestSqDiff) {
					bestTheta = theta;
					bestSqDiff = curSqDiff;
				}
			}

			curDepth += width_;
			theta += deltaTheta;
		}

		phi += deltaPhi;
	}

	if (bestSqDiff <= maxAllowedWarpDif_) {
		std::cout << "Best found distance difference: " << sqrt(bestSqDiff) << std::endl;
		return dir*sinf(bestTheta);
	}

	std::cout << "Couldn't find a suitable point" << std::endl;
	
	// Couldn't find close enough point
	return glm::vec3(0.f, 0.f, 0.f);
}

bool EnvironmentMap::fromWarp(const EnvironmentMap& srcEM, const glm::vec3& posWorld) {
  glm::vec3 posUS = srcEM.findDisplacementUS(posWorld);

	return fromWarp(srcEM, posUS, posWorld);
}

bool EnvironmentMap::fromWarp(const EnvironmentMap& srcEM, const glm::vec3& posUS, const glm::vec3& posWorld) {
	color_.reset(new glm::vec3[width_*height_], std::default_delete<glm::vec3[]>());
	depth_.reset(new float[width_*height_], std::default_delete<float[]>());
	flags_.reset(new uchar[width_*height_], std::default_delete<uchar[]>());

	glm::vec3* thisColor = color_.get();	
	float* thisDepth = depth_.get();
	uchar* thisFlags = flags_.get();

	const glm::vec3* srcColor = srcEM.getColorPtr();
	const float* srcDepth = srcEM.getDepthPtr();
	const uchar* srcFlags = srcEM.getFlagsPtr();

	double deltaTheta = M_PI / height_;
	double deltaPhi = M_2PI / width_;

	float thetaP = 0.f;
	float phiP = 0.f;

	float dotPos_1 = glm::dot(posUS, posUS) - 1.f;
	float hFactor = height_ / M_PI;
	float wFactor = width_ / M_2PI;

	for (uint32_t row = 0; row < height_; row++) {
		phiP = 0.f;

		for (uint32_t col = 0; col < width_; col++) {
			glm::vec3 curDir = sh::SphericalHarmonics::toVector(phiP, thetaP);

			// Find intersection with unit sphere
			float dotDir = glm::dot(curDir, curDir);			
			float dotDirPos = glm::dot(curDir, posUS);

			float num = sqrt(dotDirPos*dotDirPos - dotPos_1*dotDir) - dotDirPos;
			float t = num / dotDir;

			// Point on the unit sphere as seen from new origin
			glm::vec3 newDir = curDir*t + posUS;

			glm::vec2 sphCoords = sh::SphericalHarmonics::toSphericalCoords(newDir);

			if(sphCoords.x < 0)
				sphCoords.x += M_2PI;

			uint16_t srcRow = (uint16_t)std::min(height_ - 1.f, std::max(0.f, floorf(sphCoords.y*hFactor + 0.5f)));
			uint16_t srcCol = (uint16_t)std::min(width_ - 1.f, std::max(0.f, floorf(sphCoords.x*wFactor + 0.5f)));
			uint32_t offset = srcRow*width_ + srcCol;

			// Update RGB map
			*thisColor++ = *(srcColor + offset);
			
			// Update depth map
			float curSrcDepth = *(srcDepth + offset);
			if (curSrcDepth < 0)
				*thisDepth++ = curSrcDepth;
			else {
				glm::vec3 curWorldPt = curDir*curSrcDepth + srcEM.getOrigin();
				curWorldPt -= posWorld;
				*thisDepth++ = sqrt(glm::dot(curWorldPt, curWorldPt));
			}

			// Update flags map
			*thisFlags++ = *(srcFlags + offset);

			phiP += deltaPhi;
		}

		thetaP += deltaTheta;
	}

  // Update origin
  bool validWarp = false;
  if((posUS.x != 0.f) || (posUS.y != 0.f) || (posUS.z != 0.f))
    validWarp = true;

  if(validWarp)
    origin_ = posWorld;
  else
    origin_ = srcEM.getOrigin();

  isEmpty_ = false;

	depthRange_ = srcEM.getDepthRange();
	lastCorrMtx_ = srcEM.getLastCorrectionMatrix();
  lastSamples_ = srcEM.getLastSamples();

  return validWarp;
}
#include <vsense/em/Process.h>

#include <vsense/gl/Texture.h>
#include <vsense/gl/Util.h>
#include <vsense/sh/SphericalHarmonics.h>

#include <iostream>

#ifdef __ANDROID__
#include <android/asset_manager_jni.h>
#endif

using namespace vsense::em;
using namespace vsense;
using namespace std;

#define WITH_EXTRA

#define COLOR_CORRECTION_GPU
#define WITH_SINGLE_PRECISION

#ifdef WITH_SINGLE_PRECISION
const float BetaAt = 2.f;
const float BetaCt = 8.f;
const float Gamma = 0.05f;
#else
const double BetaAt = 2.0;
const double BetaCt = 8.0;
const double Gamma = 0.05;
#endif

const uint32_t ColorImageWidth = 1920;
const uint32_t ColorImageHeight = 1080;

const uint32_t DepthMapWidth = 224;
const uint32_t DepthMapHeight = 172;
const uint32_t DepthNbrPoints = DepthMapHeight*DepthMapWidth;

const uint32_t EnvironmentMapWidth = 1000;
const uint32_t EnvironmentMapHeight = EnvironmentMapWidth / 2;

// Constants for 18,432 samples
const uint32_t RandSamplesWidth = 72;
const uint32_t RandSamplesHeight = 128;
const glm::ivec2 SHWorkGroups(9, 8);
const int MaxOrder = 9;
const int NbrCoefficients = (MaxOrder + 1)*(MaxOrder + 1);

const float TrustedRadius = 0.10f; // 10cm

const float NbrDiv = 8.f;

const float MaxDepthDiff = 0.01f;

const size_t MinNbrPoints = 500;

const float MaxAllowedError = 0.1f;

#ifdef KEEP_STATS
#define STAT_START(idx)                                              \
		curTimeStats_.start(idx);                                           

#define STAT_STOP(idx)                                               \
		curTimeStats_.stop(idx);                                            
	
#else
#define STAT_START(idx){}
#define STAT_STOP(idx){}
#endif

#ifdef _WINDOWS
const std::string MaskFile = "D:/dev/vsense_AR/data/ptMap.bin";

#elif __ANDROID__
const std::string MaskFile = "/sdcard/TCD/map/ptMap.bin";
#endif

#ifdef _WINDOWS
void TimeStats::printStats() {
	//std::cout << "Frames: " << nbrFrames << std::endl;
	for (int i = 0; i <= EMTranslate; i++)
		std::cout << i << " " << timeMS[i] << std::endl;
}

#elif __ANDROID__

long curFrame = 0;
void TimeStats::printStats(std::ofstream& file) {	
	file << curFrame << ",";
	for (int i = 0; i <= EMTranslate; i++)
		file << timeMS[i] << ",";
	file << std::endl;

	curFrame++;
}
#endif

#ifdef _WINDOWS
Process::Process() : emIsEmpty_(true), emTranslatedOrigin_(0.f, 0.f, 0.f), emOrigin_(0.f, 0.f, 0.f), lastCorrError_(-1.f), overwriteOld_(true), 
	maxMSE_(MaxAllowedError), needsTranslateEM_(false), maxOrder_(9), curProject_(false), doColorCorrection_(true) {
	initializeOpenGLFunctions();

	initializeShaders();

	readPointMappingFile();
}

void Process::initializeShaders() {
	// Convert YUV420 -> Color
	shaderProgram1_ = new QOpenGLShaderProgram;
	shaderProgram1_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/colorImage.comp");
	shaderProgram1_->link();
	shaderProgram1_->bind();
	
	GL_CHECK(textureColorImg_.reset(new gl::Texture(ColorImageWidth, ColorImageHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureY_.reset(new gl::Texture(ColorImageWidth / 4, ColorImageHeight, 4, GL_UNSIGNED_BYTE, NULL)));
	GL_CHECK(textureC_.reset(new gl::Texture(ColorImageWidth / 4, ColorImageHeight / 2, 4, GL_UNSIGNED_BYTE, NULL)));

	shaderProgram1_->release();

	// Depth map Initialization
	shaderProgram2_ = new QOpenGLShaderProgram;
	shaderProgram2_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/depthMapInit.comp");
	shaderProgram2_->link();
	shaderProgram2_->bind();

	GL_CHECK(textureDepthMap1_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureDepthMap2_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(texturePointsMap1_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(texturePointsMap2_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(texturePointCloud_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));

	f_pcLocation_ = shaderProgram2_->uniformLocation("f_pc");
	c_pcLocation_ = shaderProgram2_->uniformLocation("c_pc");
	coeff_pcLocation_ = shaderProgram2_->uniformLocation("coeff_pc");

	f_imLocation2_ = shaderProgram2_->uniformLocation("f_im");
	c_imLocation2_ = shaderProgram2_->uniformLocation("c_im");
	pose_imLocation2_ = shaderProgram2_->uniformLocation("pose_im");
	coeff_imLocation2_ = shaderProgram2_->uniformLocation("coeff_im");

	minConfidenceLocation_ = shaderProgram2_->uniformLocation("minConfidence");
	nbrPointsLocation_ = shaderProgram2_->uniformLocation("nbrPoints");

	shaderProgram2_->release();

	// Mark points as reliable
	shaderProgram3_ = new QOpenGLShaderProgram;
	shaderProgram3_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/depthMapMarkReliable.comp");
	shaderProgram3_->link();
	shaderProgram3_->bind();

	shaderProgram3_->release();

	// Fill holes
	shaderProgram4_ = new QOpenGLShaderProgram;
	shaderProgram4_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/depthMapFillHoles.comp");
	shaderProgram4_->link();
	shaderProgram4_->bind();

	f_imLocation4_ = shaderProgram4_->uniformLocation("f_im");
	c_imLocation4_ = shaderProgram4_->uniformLocation("c_im");
	pose_imLocation4_ = shaderProgram4_->uniformLocation("pose_im");
	coeff_imLocation4_ = shaderProgram4_->uniformLocation("coeff_im");
	fillWithMaxLocation_ = shaderProgram4_->uniformLocation("fillWithMax");

	shaderProgram4_->release();

	// Environment map samples
	shaderProgram5_ = new QOpenGLShaderProgram;
	shaderProgram5_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/environmentMapSample.comp");
	shaderProgram5_->link();
	shaderProgram5_->bind();

	GL_CHECK(textureEnvironmentMap1_.reset(new gl::Texture(EnvironmentMapWidth, EnvironmentMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureEnvironmentMap2_.reset(new gl::Texture(EnvironmentMapWidth, EnvironmentMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureSamplesRef_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureSamplesCur_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureSamplesData_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL)));
	textureEnvironmentMapCur_ = textureEnvironmentMap1_;

	pose_pcLocation_ = shaderProgram5_->uniformLocation("pose_pc");
	devPosLocation_ = shaderProgram5_->uniformLocation("devPos");
	devOrLocation_ = shaderProgram5_->uniformLocation("devOr");
	devDirLocation_ = shaderProgram5_->uniformLocation("devDir");

	shaderProgram5_->release();

	// Environment map colour correction matrix
	shaderProgram6_ = new QOpenGLShaderProgram;
	shaderProgram6_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/environmentMapCorrect.comp");
	shaderProgram6_->link();
	shaderProgram6_->bind();

	int accWidth = DepthMapWidth / (NbrDiv*4.f);
	int accHeight = DepthMapHeight / (NbrDiv*4.f);
	textureMatAAcc_.reset(new gl::Texture(accWidth * 3, accHeight * 3, 1, GL_FLOAT, NULL));
	textureMatBAcc_.reset(new gl::Texture(accWidth * 3, accHeight * 3, 1, GL_FLOAT, NULL));
	textureWCurAcc_.reset(new gl::Texture(accWidth * 3, accHeight, 1, GL_FLOAT, NULL));
	textureWRefAcc_.reset(new gl::Texture(accWidth * 3, accHeight, 1, GL_FLOAT, NULL));
	textureUsedAcc_.reset(new gl::Texture(accWidth, accHeight, 1, GL_FLOAT, NULL));	
	textureFinalData_.reset(new gl::Texture(7, 3, 1, GL_FLOAT, NULL));
	
	numWGLocation6_ = shaderProgram6_->uniformLocation("numWG");

	shaderProgram6_->release();

	// Environment map error estimation
	shaderProgram7_ = new QOpenGLShaderProgram;
	shaderProgram7_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/environmentMapError.comp");
	shaderProgram7_->link();
	shaderProgram7_->bind();

	textureErrorAcc_.reset(new gl::Texture(accWidth, accHeight, 1, GL_FLOAT, NULL));
	textureCountAcc_.reset(new gl::Texture(accWidth, accHeight, 1, GL_FLOAT, NULL));
	textureFinalError_.reset(new gl::Texture(1, 1, 1, GL_FLOAT, NULL));

	corrMtxLocation7_ = shaderProgram7_->uniformLocation("corrMtx");
	numWGLocation7_ = shaderProgram7_->uniformLocation("numWG");

	shaderProgram7_->release();

	// Environment map project
	shaderProgram8_ = new QOpenGLShaderProgram;
	shaderProgram8_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/environmentMapProject.comp");
	shaderProgram8_->link();
	shaderProgram8_->bind();

	corrMtxLocation8_ = shaderProgram8_->uniformLocation("corrMtx");
	withinTrustedSphereLocation_ = shaderProgram8_->uniformLocation("withinTrustedSphere");

	shaderProgram8_->release();

	// Spherical coefficients
	shaderProgram9_ = new QOpenGLShaderProgram;
	shaderProgram9_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/envMapSHCoefficients.comp");
	shaderProgram9_->link();
	shaderProgram9_->bind();

	const std::shared_ptr<glm::vec2> randomSphCoords = sh::SphericalHarmonics::getRandomSphericalCoords();
	GL_CHECK(textureRandomSamples_.reset(new gl::Texture(RandSamplesWidth, RandSamplesHeight, 4, GL_FLOAT, (unsigned char*)randomSphCoords.get())));
	GL_CHECK(textureCoeffRAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffGAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffBAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffAAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffFinal1_.reset(new gl::Texture(1, NbrCoefficients, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffFinal2_.reset(new gl::Texture(1, NbrCoefficients, 4, GL_FLOAT, NULL)));
	textureCoeffFinalCur_ = textureCoeffFinal1_;

	numWGLocation9_ = shaderProgram9_->uniformLocation("numWG");
	maxOrderLocation_ = shaderProgram9_->uniformLocation("maxOrder");

	shaderProgram9_->release();	

	// Environment map relocate
	shaderProgram10_ = new QOpenGLShaderProgram;
	shaderProgram10_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/environmentMapRelocate.comp");
	shaderProgram10_->link();
	shaderProgram10_->bind();

	curOriginLocation_ = shaderProgram10_->uniformLocation("curOrigin");
	newOriginLocation_ = shaderProgram10_->uniformLocation("newOrigin");

	shaderProgram10_->release();

	// EM sampling simulation (fill holes)
	shaderProgram11_ = new QOpenGLShaderProgram;
	shaderProgram11_->addShaderFromSourceFile(QOpenGLShader::Compute, ":/resources/shaders/envMapSimulate.comp");
	shaderProgram11_->link();
	shaderProgram11_->bind();
	GL_CHECK(textureOutputMap_.reset(new gl::Texture(EnvironmentMapWidth, EnvironmentMapHeight, 4, GL_FLOAT, NULL)));

	shaderProgram11_->release();
}

void Process::readImage(const std::string& fileIM) {
	ifstream file(fileIM, ios::in | ios::binary);
	if (file.is_open()) {
		file.read((char*)&imData_.width_, sizeof(uint32_t));
		file.read((char*)&imData_.height_, sizeof(uint32_t));
		file.read((char*)&imData_.exposure_, sizeof(int64_t));
		file.read((char*)&imData_.timestamp_, sizeof(double));
		file.read((char*)&imData_.f_.x, sizeof(double) * 2);
		file.read((char*)&imData_.c_.x, sizeof(double) * 2);
		file.read((char*)imData_.distortion_, sizeof(double) * 5);
		file.read((char*)imData_.translation_, sizeof(double) * 3);
		file.read((char*)imData_.orientation_, sizeof(double) * 4);
		file.read((char*)&imData_.accuracy_, sizeof(float));

		for (int i = 0; i < 5; i++)
			imData_.distortionF_[i] = imData_.distortion_[i];

		std::shared_ptr<uchar> Y;
		std::shared_ptr<uchar> C;
		Y.reset(new uchar[imData_.height_*imData_.width_], std::default_delete<uchar[]>());
		C.reset(new uchar[imData_.height_*imData_.width_ / 2], std::default_delete<uchar[]>());

		file.read((char*)Y.get(), sizeof(char)*imData_.width_*imData_.height_);
		file.read((char*)C.get(), sizeof(char)*imData_.width_*imData_.height_ / 2);

		textureY_->updateData(Y.get());
		textureC_->updateData(C.get());

		imPose_ = glm::inverse(imData_.asPose());		

		file.close();
	}
}

void Process::readPointCloud(const std::string& filePC) {
	ifstream file(filePC, ios::in | ios::binary);
	if (file.is_open()) {
		file.read((char*)&pcData_.width_, sizeof(uint32_t));
		file.read((char*)&pcData_.height_, sizeof(uint32_t));
		file.read((char*)&pcData_.f_.x, sizeof(double) * 2);
		file.read((char*)&pcData_.c_.x, sizeof(double) * 2);
		file.read((char*)pcData_.distortion_, sizeof(double) * 5);
		file.read((char*)&pcData_.nbrPoints_, sizeof(uint32_t));
		file.read((char*)&pcData_.timestamp_, sizeof(double));
		file.read((char*)pcData_.translation_, sizeof(double) * 3);
		file.read((char*)pcData_.orientation_, sizeof(double) * 4);
		file.read((char*)&pcData_.accuracy_, sizeof(float));

		for (int i = 0; i < 5; i++)
			pcData_.distortionF_[i] = pcData_.distortion_[i];

		pcPose_ = pcData_.asPose();

		std::shared_ptr<glm::vec4> points;
		size_t nbrPoints = DepthMapWidth*DepthMapHeight;
		points.reset(new glm::vec4[DepthMapWidth*DepthMapHeight], std::default_delete<glm::vec4[]>());		
		memset(points.get(), 0, sizeof(float)*nbrPoints * 4);
		file.read((char*)points.get(), sizeof(float)*pcData_.nbrPoints_ * 4);
		
		texturePointCloud_->updateData(points.get());

		file.close();
	}
}

void Process::loadDepthMap(const std::string& filePC, const std::string& fileIM, float& confidence) {
	curTimeStats_ = TimeStats();

	STAT_START(TransferGPU);
	readImage(fileIM);
	readPointCloud(filePC);	
	STAT_STOP(TransferGPU);

	runEMShaders(confidence, true, true);
}

#elif __ANDROID__

Process::Process(AAssetManager* assetManager) : emIsEmpty_(true), assetManager_(assetManager), emOrigin_(0.f, 0.f, 0.f), overwriteOld_(true), 
	maxMSE_(MaxAllowedError), needsTranslateEM_(false), curProject_(false), doColorCorrection_(true) {
	
	initializeShaders();

	readPointMappingFile();
}

GLuint Process::createComputeShaderProgram(const std::string& filename) {
	AAsset* computeShaderAsset = AAssetManager_open(assetManager_, filename.c_str(), AASSET_MODE_BUFFER);
	const void *computeShaderBuf = AAsset_getBuffer(computeShaderAsset);
	off_t computeShaderLength = AAsset_getLength(computeShaderAsset);
	std::string computeShaderSource = std::string((const char*)computeShaderBuf, (size_t)computeShaderLength);
	AAsset_close(computeShaderAsset);

	return gl::util::createProgram(computeShaderSource.c_str());
}

void Process::initializeShaders() {
  // Convert YUV420 -> Color
	LOGI("colorImage.comp");
	shaderProgram1_ = createComputeShaderProgram("shaders/colorImage.comp");	
	
	textureColorImg_.reset(new gl::Texture(ColorImageWidth, ColorImageHeight, 4, GL_FLOAT, NULL));
  textureY_.reset(new gl::Texture(ColorImageWidth / 4, ColorImageHeight, 4, GL_UNSIGNED_BYTE, NULL));
  textureC_.reset(new gl::Texture(ColorImageWidth / 4, ColorImageHeight / 2, 4, GL_UNSIGNED_BYTE, NULL));

	// Depth map Initialization
	LOGI("depthMapInit.comp");
	shaderProgram2_ = createComputeShaderProgram("shaders/depthMapInit.comp");
	
	textureDepthMap1_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));
	textureDepthMap2_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));
	texturePointsMap1_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));
	texturePointsMap2_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));
  texturePointCloud_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));

	f_pcLocation_ = glGetUniformLocation(shaderProgram2_, "f_pc");
	c_pcLocation_ = glGetUniformLocation(shaderProgram2_, "c_pc");
	coeff_pcLocation_ = glGetUniformLocation(shaderProgram2_, "coeff_pc");

	f_imLocation2_ = glGetUniformLocation(shaderProgram2_, "f_im");
	c_imLocation2_ = glGetUniformLocation(shaderProgram2_, "c_im");
	pose_imLocation2_ = glGetUniformLocation(shaderProgram2_, "pose_im");
	coeff_imLocation2_ = glGetUniformLocation(shaderProgram2_, "coeff_im");

	minConfidenceLocation_ = glGetUniformLocation(shaderProgram2_, "minConfidence");
	nbrPointsLocation_ = glGetUniformLocation(shaderProgram2_, "nbrPoints");

	// Mark points as reliable
	LOGI("depthMapMarkReliable.comp");
	shaderProgram3_ = createComputeShaderProgram("shaders/depthMapMarkReliable.comp");
	
	// Fill holes
	LOGI("depthMapFillHoles.comp");
	shaderProgram4_ = createComputeShaderProgram("shaders/depthMapFillHoles.comp");
	
	f_imLocation4_ = glGetUniformLocation(shaderProgram4_, "f_im");
	c_imLocation4_ = glGetUniformLocation(shaderProgram4_, "c_im");
	pose_imLocation4_ = glGetUniformLocation(shaderProgram4_, "pose_im");
	coeff_imLocation4_ = glGetUniformLocation(shaderProgram4_, "coeff_im");
	fillWithMaxLocation_ = glGetUniformLocation(shaderProgram4_, "fillWithMax");

	// Environment map samples
	LOGI("environmentMapSample.comp");
	shaderProgram5_ = createComputeShaderProgram("shaders/environmentMapSample.comp");

  textureEnvironmentMap1_.reset(new gl::Texture(EnvironmentMapWidth, EnvironmentMapHeight, 4, GL_FLOAT, NULL));
  textureEnvironmentMap2_.reset(new gl::Texture(EnvironmentMapWidth, EnvironmentMapHeight, 4, GL_FLOAT, NULL));
  textureEnvironmentMapCur_ = textureEnvironmentMap1_;
	textureSamplesRef_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));
	textureSamplesCur_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));
	textureSamplesData_.reset(new gl::Texture(DepthMapWidth, DepthMapHeight, 4, GL_FLOAT, NULL));

	pose_pcLocation_ = glGetUniformLocation(shaderProgram5_, "pose_pc");
	devPosLocation_ = glGetUniformLocation(shaderProgram5_, "devPos");
	devOrLocation_ = glGetUniformLocation(shaderProgram5_, "devOr");
	devDirLocation_ = glGetUniformLocation(shaderProgram5_, "devDir");

	// Environment map colour correction matrix
	LOGI("environmentMapCorrect.comp");
	shaderProgram6_ = createComputeShaderProgram("shaders/environmentMapCorrect.comp");
	
	int accWidth = DepthMapWidth / (NbrDiv*4.f);
	int accHeight = DepthMapHeight / (NbrDiv*4.f);
	textureMatAAcc_.reset(new gl::Texture(accWidth * 3, accHeight * 3, 1, GL_FLOAT, NULL));
	textureMatBAcc_.reset(new gl::Texture(accWidth * 3, accHeight * 3, 1, GL_FLOAT, NULL));
	textureWCurAcc_.reset(new gl::Texture(accWidth * 3, accHeight, 1, GL_FLOAT, NULL));
	textureWRefAcc_.reset(new gl::Texture(accWidth * 3, accHeight, 1, GL_FLOAT, NULL));
	textureUsedAcc_.reset(new gl::Texture(accWidth, accHeight, 1, GL_FLOAT, NULL));
	textureFinalData_.reset(new gl::Texture(7, 3, 1, GL_FLOAT, NULL));

	numWGLocation6_ = glGetUniformLocation(shaderProgram6_, "numWG");

	// Environment map error estimation
	LOGI("environmentMapError.comp");
	shaderProgram7_ = createComputeShaderProgram("shaders/environmentMapError.comp");

	textureErrorAcc_.reset(new gl::Texture(accWidth, accHeight, 1, GL_FLOAT, NULL));
	textureCountAcc_.reset(new gl::Texture(accWidth, accHeight, 1, GL_FLOAT, NULL));
	textureFinalError_.reset(new gl::Texture(1, 1, 1, GL_FLOAT, NULL));

	corrMtxLocation7_ = glGetUniformLocation(shaderProgram7_, "corrMtx");
  numWGLocation7_ = glGetUniformLocation(shaderProgram7_, "numWG");

	// Environment map project
	LOGI("environmentMapProject.comp");
	shaderProgram8_ = createComputeShaderProgram("shaders/environmentMapProject.comp");
	
	corrMtxLocation8_ = glGetUniformLocation(shaderProgram8_, "corrMtx");
	withinTrustedSphereLocation_ = glGetUniformLocation(shaderProgram8_, "withinTrustedSphere");

  // Spherical coefficients
  LOGI("envMapSHCoefficients.comp");
  shaderProgram9_ = createComputeShaderProgram("shaders/envMapSHCoefficients.comp");

  const std::shared_ptr<glm::vec2> randomSphCoords = sh::SphericalHarmonics::getRandomSphericalCoords();
  GL_CHECK(textureRandomSamples_.reset(new gl::Texture(RandSamplesWidth, RandSamplesHeight, 4, GL_FLOAT, (unsigned char*)randomSphCoords.get())));
  GL_CHECK(textureCoeffRAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
  GL_CHECK(textureCoeffGAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
  GL_CHECK(textureCoeffBAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffAAcc_.reset(new gl::Texture(SHWorkGroups.x*NbrCoefficients, SHWorkGroups.y, 1, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffFinal1_.reset(new gl::Texture(1, NbrCoefficients, 4, GL_FLOAT, NULL)));
	GL_CHECK(textureCoeffFinal2_.reset(new gl::Texture(1, NbrCoefficients, 4, GL_FLOAT, NULL)));
	textureCoeffFinalCur_ = textureCoeffFinal1_;

  numWGLocation9_ = glGetUniformLocation(shaderProgram9_, "numWG");
	maxOrderLocation_ = glGetUniformLocation(shaderProgram9_, "maxOrder");

	// Environment map relocate
	LOGI("envMapRelocate.comp");
	shaderProgram10_ = createComputeShaderProgram("shaders/environmentMapRelocate.comp");

	curOriginLocation_ = glGetUniformLocation(shaderProgram10_, "curOrigin");
	newOriginLocation_ = glGetUniformLocation(shaderProgram10_, "newOrigin");
}

void Process::addFrame(const TangoPointCloud* pointCloud, const TangoPoseData* posePC, const TangoCameraIntrinsics* pcData, const TangoImageBuffer* imgBuffer, const TangoPoseData* poseIM, const TangoCameraIntrinsics* imData, float confidence, bool project, bool calculateSH) {
	curTimeStats_ = TimeStats();
	
	STAT_START(TransferGPU);
	// Load color image data
	imData_.width_ = imData->width;
	imData_.height_ = imData->height;
	imData_.exposure_ = imgBuffer->exposure_duration_ns;
	imData_.timestamp_ = imgBuffer->timestamp;
	imData_.f_.x = imData->fx;
	imData_.f_.y = imData->fy;
	imData_.c_.x = imData->cx;
	imData_.c_.y = imData->cy;
	for(int i = 0; i < 5; i++)
		imData_.distortionF_[i] = imData->distortion[i];
	memcpy(imData_.distortion_, imData->distortion, sizeof(double)*5);
	memcpy(imData_.translation_, poseIM->translation, sizeof(double)*3);
	memcpy(imData_.orientation_, poseIM->orientation, sizeof(double)*4);
	imData_.accuracy_ = poseIM->accuracy;

	glm::quat q((float)imData_.orientation_[3], (float)imData_.orientation_[0], (float)imData_.orientation_[1], (float)imData_.orientation_[2]);
	imPose_ = glm::mat4_cast(q);
	imPose_[3][0] = imData_.translation_[0];
	imPose_[3][1] = imData_.translation_[1];
	imPose_[3][2] = imData_.translation_[2];

	textureY_->updateData(imgBuffer->data);
	textureC_->updateData(imgBuffer->data + ColorImageWidth*ColorImageHeight);

	// Load point cloud data
	pcData_.width_ = pcData->width;
	pcData_.height_ = pcData->height;
	pcData_.f_.x = pcData->fx;
	pcData_.f_.y = pcData->fy;
	pcData_.c_.x = pcData->cx;
	pcData_.c_.y = pcData->cy;
	for(int i = 0; i < 5; i++)
		pcData_.distortionF_[i] = pcData->distortion[i];
	memcpy(pcData_.distortion_, pcData->distortion, sizeof(double)*5);
	memcpy(pcData_.translation_, posePC->translation, sizeof(double)*3);
	memcpy(pcData_.orientation_, posePC->orientation, sizeof(double)*4);
	pcData_.nbrPoints_ = pointCloud->num_points;
	pcData_.timestamp_ = pointCloud->timestamp;

	q = glm::quat((float)pcData_.orientation_[3], (float)pcData_.orientation_[0], (float)pcData_.orientation_[1], (float)pcData_.orientation_[2]);
	pcPose_ = glm::mat4_cast(q);
	pcPose_[3][0] = pcData_.translation_[0];
	pcPose_[3][1] = pcData_.translation_[1];
	pcPose_[3][2] = pcData_.translation_[2];

	if(!points_)
    points_.reset(new glm::vec4[DepthNbrPoints], std::default_delete<glm::vec4[]>());
  memset(points_.get(), 0, sizeof(glm::vec4)*DepthNbrPoints);
	memcpy(points_.get(), pointCloud->points, sizeof(float)*4*pointCloud->num_points);

  texturePointCloud_->updateData(points_.get());
	STAT_STOP(TransferGPU);

	runEMShaders(confidence, project, calculateSH);
}
#endif

void Process::runEMShaders(float confidence, bool project, bool calculateSH) {
	/*if (curProject_ != project) {
#ifdef __ANDROID__
		if (project)
			statsFile_.open("/sdcard/TCD/out/TimingsC.csv");
		else
			statsFile_.close();
#endif

		curProject_ = project;
	}*/

	GLuint wgX, wgY;
	
	// Convert YUV420 -> Color
	STAT_START(ConvertRGB);
#ifdef _WINDOWS
	shaderProgram1_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram1_);
#endif
	wgX = ceil(textureColorImg_->width() / NbrDiv);
	wgY = ceil(textureColorImg_->height() / NbrDiv);
	GL_CHECK(textureY_->bind(0, GL_READ_ONLY));
	GL_CHECK(textureC_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureColorImg_->bind(2, GL_WRITE_ONLY));
	glDispatchCompute(wgX, wgY, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram1_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif
	STAT_STOP(ConvertRGB);
	
	// Depth map Initialization
	STAT_START(DepthMapInit);
#ifdef _WINDOWS
	shaderProgram2_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram2_);
#endif
	textureDepthMap1_->clearTexture();
	textureDepthMap2_->clearTexture();
	texturePointsMap1_->clearTexture();
	texturePointsMap2_->clearTexture();
	wgX = ceil(textureDepthMap1_->width() / NbrDiv);
	wgY = ceil(textureDepthMap1_->height() / NbrDiv);
	GL_CHECK(texturePointCloud_->bind(0, GL_READ_ONLY));
	GL_CHECK(textureColorImg_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureDepthMap1_->bind(2, GL_WRITE_ONLY));
	GL_CHECK(texturePointsMap1_->bind(3, GL_WRITE_ONLY));
	glUniform2fv(f_pcLocation_, 1, glm::value_ptr(glm::vec2(pcData_.f_)));
	glUniform2fv(c_pcLocation_, 1, glm::value_ptr(glm::vec2(pcData_.c_)));
	glUniform1fv(coeff_pcLocation_, 5, pcData_.distortionF_);
	glUniform2fv(f_imLocation2_, 1, glm::value_ptr(glm::vec2(imData_.f_)));
	glUniform2fv(c_imLocation2_, 1, glm::value_ptr(glm::vec2(imData_.c_)));
	glUniform1fv(coeff_imLocation2_, 5, imData_.distortionF_);
	glUniformMatrix4fv(pose_imLocation2_, 1, GL_FALSE, glm::value_ptr(imPose_));
	glUniform1f(minConfidenceLocation_, confidence);
	glUniform1i(nbrPointsLocation_, pcData_.nbrPoints_);
	glDispatchCompute(wgX, wgY, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
  shaderProgram2_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif
	STAT_STOP(DepthMapInit);

	// Mark points as reliable
	STAT_START(DepthMapReliable);
#ifdef _WINDOWS
	shaderProgram3_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram3_);
#endif
	GL_CHECK(textureDepthMap1_->bind(0, GL_READ_ONLY));
	GL_CHECK(texturePointsMap1_->bind(1, GL_READ_ONLY));
	GL_CHECK(texturePointsMap2_->bind(2, GL_WRITE_ONLY));
	GL_CHECK(glDispatchCompute(wgX, wgY, 1));
	GL_CHECK(glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F));
	GL_CHECK(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
#ifdef _WINDOWS
  shaderProgram3_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif
	STAT_STOP(DepthMapReliable);

	// Fill holes
	STAT_START(DepthMapHoleFilling);
#ifdef _WINDOWS
	shaderProgram4_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram4_);
#endif
	GL_CHECK(textureColorImg_->bind(0, GL_READ_ONLY));
	GL_CHECK(texturePtMappingMap_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureDepthMap1_->bind(2, GL_READ_ONLY));
	GL_CHECK(textureDepthMap2_->bind(3, GL_WRITE_ONLY));
	GL_CHECK(texturePointsMap2_->bind(4, GL_READ_ONLY));
	GL_CHECK(texturePointsMap1_->bind(5, GL_WRITE_ONLY));
	glUniform2fv(f_imLocation4_, 1, glm::value_ptr(glm::vec2(imData_.f_)));
	glUniform2fv(c_imLocation4_, 1, glm::value_ptr(glm::vec2(imData_.c_)));
	glUniform1fv(coeff_imLocation4_, 5, imData_.distortionF_);
	glUniformMatrix4fv(pose_imLocation4_, 1, GL_FALSE, glm::value_ptr(imPose_));
	glUniform1i(fillWithMaxLocation_, false);
	glDispatchCompute(wgX, wgY, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
  shaderProgram4_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif
	STAT_STOP(DepthMapHoleFilling);

	// Environment map samples
	STAT_START(EMSampling);
	glm::vec3 devPos(pcPose_[3][0], pcPose_[3][1], pcPose_[3][2]);
	glm::vec3 devOr(devPos - emOrigin_);
	glm::vec3 devDir(pcPose_[2][0], pcPose_[2][1], pcPose_[2][2]);
	devDir = glm::normalize(devDir);

#ifdef _WINDOWS
	shaderProgram5_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram5_);
#endif
	textureSamplesRef_->clearTexture();
	textureSamplesCur_->clearTexture();
	textureSamplesData_->clearTexture();
	GL_CHECK(textureDepthMap2_->bind(0, GL_READ_ONLY));
	GL_CHECK(texturePointsMap1_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureEnvironmentMapCur_->bind(2, GL_READ_ONLY));
	GL_CHECK(textureSamplesRef_->bind(3, GL_WRITE_ONLY));
	GL_CHECK(textureSamplesCur_->bind(4, GL_WRITE_ONLY));
	GL_CHECK(textureSamplesData_->bind(5, GL_WRITE_ONLY));
	glUniformMatrix4fv(pose_pcLocation_, 1, GL_FALSE, glm::value_ptr(pcPose_));
	glUniform3f(devPosLocation_, devPos.x, devPos.y, devPos.z);
	glUniform3f(devOrLocation_, devOr.x, devOr.y, devOr.z);
	glUniform3f(devDirLocation_, devDir.x, devDir.y, devDir.z);
	glDispatchCompute(wgX, wgY, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
  shaderProgram5_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif
	STAT_STOP(EMSampling);

	// For double-precision accuracy, it's done on CPU
	if (!emIsEmpty_ && doColorCorrection_)
		corrMtx_ = calculateCorrectionMatrix();

	if (project && (((lastCorrError_ >= 0.f) && (lastCorrError_ < maxMSE_)) || emIsEmpty_)) {
		// Environment map project
		STAT_START(EMProjection);
		float distToDev = sqrt(glm::dot(devOr, devOr));
		bool trustedRadius = (distToDev <= TrustedRadius);

#ifdef _WINDOWS
		shaderProgram8_->bind();
#elif __ANDROID__
		glUseProgram(shaderProgram8_);
#endif
		GL_CHECK(textureSamplesRef_->bind(0, GL_READ_ONLY));
		GL_CHECK(textureSamplesCur_->bind(1, GL_READ_ONLY));
		GL_CHECK(textureSamplesData_->bind(2, GL_READ_ONLY));
		GL_CHECK(textureEnvironmentMapCur_->bind(3, GL_WRITE_ONLY));
		glUniformMatrix3fv(corrMtxLocation8_, 1, GL_FALSE, glm::value_ptr(corrMtx_));
		glUniform1i(withinTrustedSphereLocation_, trustedRadius);
		glDispatchCompute(wgX, wgY, 1);
		glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
    shaderProgram8_->release();
#elif __ANDROID__
		glUseProgram(0);
#endif
		STAT_STOP(EMProjection);

		if (calculateSH) {
			STAT_START(EMSHCoefficients);
			updateSHCoefficients(textureEnvironmentMapCur_);
			STAT_STOP(EMSHCoefficients);

/*#ifdef _WINDOWS
      curTimeStats_.printStats();
#elif __ANDROID__
			if(doColorCorrection_)
				curTimeStats_.printStats(statsFile_);
#endif*/
		}

		emIsEmpty_ = false;
	}
}

void Process::clear() {
	textureEnvironmentMap1_->clearTexture();
	textureEnvironmentMap2_->clearTexture();
	emIsEmpty_ = true;
}

void Process::readPointMappingFile() {
	if(texturePtMappingMap_)
		return;

	ifstream file(MaskFile, ios::in | ios::binary);
	if (file.is_open()) {
		std::shared_ptr<glm::vec2> ptMap;
		ptMap.reset(new glm::vec2[DepthMapWidth*DepthMapHeight], std::default_delete<glm::vec2[]>());

		file.read((char*)ptMap.get(), sizeof(float)*DepthMapWidth*DepthMapHeight * 2);

		texturePtMappingMap_.reset(new gl::Texture(DepthMapWidth / 2, DepthMapHeight, 4, GL_FLOAT, (unsigned char*)ptMap.get()));

		file.close();
	}
}

glm::mat3 Process::calculateCorrectionMatrix() {
#ifdef COLOR_CORRECTION_GPU
	// Color correction - GPU Version
	// First pass (Color correction)
	STAT_START(EMColorCorrection);
#ifdef _WINDOWS
	shaderProgram6_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram6_);
#endif
	GL_CHECK(textureMatAAcc_->clearTexture());
	GL_CHECK(textureMatBAcc_->clearTexture());
	GL_CHECK(textureWCurAcc_->clearTexture());
	GL_CHECK(textureWRefAcc_->clearTexture());
	GL_CHECK(textureUsedAcc_->clearTexture());
	GL_CHECK(textureFinalData_->clearTexture());
	GL_CHECK(textureSamplesRef_->bind(0, GL_READ_ONLY));
	GL_CHECK(textureSamplesCur_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureMatAAcc_->bind(2, GL_READ_WRITE));
	GL_CHECK(textureMatBAcc_->bind(3, GL_READ_WRITE));
	GL_CHECK(textureWCurAcc_->bind(4, GL_READ_WRITE));
	GL_CHECK(textureWRefAcc_->bind(5, GL_READ_WRITE));
	GL_CHECK(textureUsedAcc_->bind(6, GL_READ_WRITE));
	GL_CHECK(textureFinalData_->bind(7, GL_WRITE_ONLY));
	glDispatchCompute(textureUsedAcc_->width(), textureUsedAcc_->height(), 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram6_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif

	// Final accumulation
#ifdef _WINDOWS
	shaderProgram6_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram6_);
#endif
	GL_CHECK(textureSamplesRef_->bind(0, GL_READ_ONLY));
	GL_CHECK(textureSamplesCur_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureMatAAcc_->bind(2, GL_READ_WRITE));
	GL_CHECK(textureMatBAcc_->bind(3, GL_READ_WRITE));
	GL_CHECK(textureWCurAcc_->bind(4, GL_READ_WRITE));
	GL_CHECK(textureWRefAcc_->bind(5, GL_READ_WRITE));
	GL_CHECK(textureUsedAcc_->bind(6, GL_READ_WRITE));
	GL_CHECK(textureFinalData_->bind(7, GL_WRITE_ONLY));
	glUniform2i(numWGLocation6_, textureUsedAcc_->width(), textureUsedAcc_->height());
	glDispatchCompute(1, 1, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram6_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif

	float finalData[30];
	textureFinalData_->writeTo(finalData);

	glm::dmat3 gpuMatA;
	glm::dmat3 gpuMatB;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			gpuMatA[j][i] = finalData[i * 7 + j];
			gpuMatB[j][i] = finalData[i * 7+ j + 3];
		}
	}
	uint32_t gpuUsedPoints = (uint32_t)finalData[6];

	if (gpuUsedPoints < MinNbrPoints) {
		lastCorrError_ = -1.0;
		return corrMtx_;
	}

	if (glm::determinant(gpuMatA) == 0) { // Non-invertible																		
		std::cout << "Determinant is zero!" << std::endl;

		lastCorrError_ = -1.0;
		return corrMtx_;
	}

	glm::dmat3 gpuMatInvA = glm::inverse(gpuMatA);
	glm::dmat3 gpuMatInvB = glm::inverse(gpuMatB);
	tmpInvCorrMtx_ = (gpuMatA * gpuMatInvB);


	glm::mat3 newCorrMtx = glm::mat3(gpuMatB * gpuMatInvA);
	STAT_STOP(EMColorCorrection);

#else
	// Color correction - CPU Version
	STAT_START(EMColorCorrection);
	if (!samplesRef_)
		samplesRef_.reset(new glm::vec4[DepthMapWidth * DepthMapHeight], std::default_delete<glm::vec4[]>());
	if (!samplesCur_)
		samplesCur_.reset(new glm::vec4[DepthMapWidth * DepthMapHeight], std::default_delete<glm::vec4[]>());

	textureSamplesRef_->writeTo(samplesRef_.get());
	textureSamplesCur_->writeTo(samplesCur_.get());

#ifdef WITH_SINGLE_PRECISION
	glm::mat3 matA(0.0);
	glm::mat3 matB(0.0);

	glm::vec3 wCur(0.0, 0.0, 0.0);
	glm::vec3 wRef(0.0, 0.0, 0.0);
#else
	glm::dmat3 matA(0.0);
	glm::dmat3 matB(0.0);

	glm::dvec3 wCur(0.0, 0.0, 0.0);
	glm::dvec3 wRef(0.0, 0.0, 0.0);
#endif

	glm::vec4* sampleRef = samplesRef_.get() - 1;
	glm::vec4* sampleCur = samplesCur_.get() - 1;

	size_t usedPoints = 0;

	for (size_t i = 0; i < DepthMapHeight * DepthMapWidth; i++) {
		++sampleRef;
		++sampleCur;

		if (sampleRef->a == 0.f) // No available reliable reference data
			continue;

		if (sampleCur->a == 0.f) // No available reliable current data
			continue;

		if (std::abs(std::abs(sampleRef->a) - std::abs(sampleCur->a)) >= MaxDepthDiff) // If the difference with the current and current depth is larger than allowed
			continue;

#ifdef WITH_EXTRA

#ifdef WITH_SINGLE_PRECISION
		glm::vec3 curHSV = color::Color::rgb2hsv(glm::vec3(*sampleCur));
		glm::vec3 refHSV = color::Color::rgb2hsv(glm::vec3(*sampleRef));

		float difH = curHSV[0] - refHSV[0];
		float difS = curHSV[1] - refHSV[1];
		float dist = sqrt(difH*difH + difS*difS);

		float wAt = pow(1 - std::min(1.f, dist), BetaAt);
		float wCt = pow(1 - std::min(1.f, dist), BetaCt);

		glm::vec3 wSampleCur(*sampleCur);

		wCur += wAt*wSampleCur;
		wRef += wAt*glm::vec3((*sampleRef));
#else
		glm::dvec3 curHSV = color::Color::rgb2hsv(glm::vec3(*sampleCur));
		glm::dvec3 refHSV = color::Color::rgb2hsv(glm::vec3(*sampleRef));

		double difH = curHSV[0] - refHSV[0];
		double difS = curHSV[1] - refHSV[1];
		double dist = sqrt(difH*difH + difS*difS);

		double wAt = pow(1 - std::min(1.0, dist), BetaAt);
		double wCt = pow(1 - std::min(1.0, dist), BetaCt);

		glm::dvec3 wSampleCur(*sampleCur);

		wCur += wAt*wSampleCur;
		wRef += wAt*glm::dvec3((*sampleRef));
#endif

		wSampleCur *= wCt;

		matA[0][0] += wSampleCur.r * sampleCur->r;
		matA[1][1] += wSampleCur.g * sampleCur->g;
		matA[2][2] += wSampleCur.b * sampleCur->b;
		matA[0][1] += wSampleCur.r * sampleCur->g;
		matA[0][2] += wSampleCur.r * sampleCur->b;
		matA[1][2] += wSampleCur.g * sampleCur->b;

		matB[0][0] += wSampleCur.r * sampleRef->r;
		matB[1][1] += wSampleCur.g * sampleRef->g;
		matB[2][2] += wSampleCur.b * sampleRef->b;
		matB[0][1] += wSampleCur.r * sampleRef->g;
		matB[0][2] += wSampleCur.r * sampleRef->b;
		matB[1][0] += wSampleCur.g * sampleRef->r;
		matB[1][2] += wSampleCur.g * sampleRef->b;
		matB[2][0] += wSampleCur.b * sampleRef->r;
		matB[2][1] += wSampleCur.b * sampleRef->g;
#else
		matA[0][0] += sampleCur->r * sampleCur->r;
		matA[1][1] += sampleCur->g * sampleCur->g;
		matA[2][2] += sampleCur->b * sampleCur->b;
		matA[0][1] += sampleCur->r * sampleCur->g;
		matA[0][2] += sampleCur->r * sampleCur->b;
		matA[1][2] += sampleCur->g * sampleCur->b;

		matB[0][0] += sampleCur->r * sampleRef->r;
		matB[1][1] += sampleCur->g * sampleRef->g;
		matB[2][2] += sampleCur->b * sampleRef->b;
		matB[0][1] += sampleCur->r * sampleRef->g;
		matB[0][2] += sampleCur->r * sampleRef->b;
		matB[1][0] += sampleCur->g * sampleRef->r;
		matB[1][2] += sampleCur->g * sampleRef->b;
		matB[2][0] += sampleCur->b * sampleRef->r;
		matB[2][1] += sampleCur->b * sampleRef->g;
#endif

		usedPoints++;
	}

	if (usedPoints < MinNbrPoints) {
		lastCorrError_ = -1.0;
		return corrMtx_;
	}

	// It's a symmetric matrix
	matA[1][0] = matA[0][1];
	matA[2][0] = matA[0][2];
	matA[2][1] = matA[1][2];

#ifdef WITH_EXTRA

#ifdef WITH_SINGLE_PRECISION
	glm::vec3 s = wRef / wCur;
#else
	glm::dvec3 s = wRef / wCur;
#endif
	
	matA[0][0] += Gamma;
	matA[1][1] += Gamma;
	matA[2][2] += Gamma;

	matB[0][0] += Gamma*s.r;
	matB[1][1] += Gamma*s.g;
	matB[2][2] += Gamma*s.b;
#endif

	if (glm::determinant(matA) == 0) { // Non-invertible																		
		std::cout << "Determinant is zero!" << std::endl;

		lastCorrError_ = -1.0;
		return corrMtx_;
	}

#ifdef WITH_SINGLE_PRECISION
	glm::dmat3 matInvA = glm::inverse(glm::dmat3(matA));

	glm::mat3 newCorrMtx = glm::mat3(glm::dmat3(matB) * matInvA);
#else
	glm::dmat3 matInvA = glm::inverse(matA);

	glm::mat3 newCorrMtx = glm::mat3(matB * matInvA);
#endif

	STAT_STOP(EMColorCorrection);
#endif

	// First pass (MSE Error)
	STAT_START(EMError);
#ifdef _WINDOWS
	shaderProgram7_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram7_);
#endif
	GL_CHECK(textureErrorAcc_->clearTexture());
	GL_CHECK(textureCountAcc_->clearTexture());
	GL_CHECK(textureFinalError_->clearTexture());
	GL_CHECK(textureSamplesRef_->bind(0, GL_READ_ONLY));
	GL_CHECK(textureSamplesCur_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureErrorAcc_->bind(2, GL_READ_WRITE));
	GL_CHECK(textureCountAcc_->bind(3, GL_READ_WRITE));
	GL_CHECK(textureFinalError_->bind(4, GL_WRITE_ONLY));
	glUniformMatrix3fv(corrMtxLocation7_, 1, GL_FALSE, glm::value_ptr(newCorrMtx));
	glDispatchCompute(textureErrorAcc_->width(), textureErrorAcc_->height(), 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram7_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif

	// Final accumulation
#ifdef _WINDOWS
	shaderProgram7_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram7_);
#endif
	GL_CHECK(textureSamplesRef_->bind(0, GL_READ_ONLY));
	GL_CHECK(textureSamplesCur_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureErrorAcc_->bind(2, GL_READ_WRITE));
	GL_CHECK(textureCountAcc_->bind(3, GL_READ_WRITE));
	GL_CHECK(textureFinalError_->bind(4, GL_WRITE_ONLY));
	glUniform2i(numWGLocation7_, textureErrorAcc_->width(), textureErrorAcc_->height());
	glDispatchCompute(1, 1, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram7_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif

	textureFinalError_->writeTo(&lastCorrError_);
	STAT_STOP(EMError);

	if (lastCorrError_ < maxMSE_) {
		invCorrMtx_ = glm::mat3(tmpInvCorrMtx_);

		return newCorrMtx;
	}

	return corrMtx_;
}

void Process::updateEMOrigin(const glm::vec3 &origin, bool overwriteOld) {
	overwriteOld_ = overwriteOld;	

	if(emIsEmpty_)
    emTranslatedOrigin_ = emOrigin_ = origin;
  else {
		needsTranslateEM_ = true;
    emTranslatedOrigin_ = origin;

#ifdef _WINDOWS
    translateEM();
	
		std::shared_ptr<gl::Texture> curEM = textureEnvironmentMapCur_;
		if (!overwriteOld_) {
			if (textureEnvironmentMapCur_ == textureEnvironmentMap1_)
				curEM = textureEnvironmentMap2_;
			else
				curEM = textureEnvironmentMap1_;
		}

		updateSHCoefficients(curEM);
#endif
  }
}

std::shared_ptr<glm::vec4> Process::getSHCoefficients() {
	std::shared_ptr<glm::vec4> shCoeffs;
	shCoeffs.reset(new glm::vec4[NbrCoefficients], std::default_delete<glm::vec4[]>());

	textureCoeffFinalCur_->writeTo(shCoeffs.get());

	return shCoeffs;
}

void Process::translateEM() {
	STAT_START(EMTranslate);

	needsTranslateEM_ = false;

	if (emIsEmpty_)
		return;

	std::shared_ptr<gl::Texture> destEnvMap;

	if (textureEnvironmentMapCur_ == textureEnvironmentMap1_)
		destEnvMap = textureEnvironmentMap2_;
	else
		destEnvMap = textureEnvironmentMap1_;

#ifdef _WINDOWS
	shaderProgram10_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram10_);
#endif
	destEnvMap->clearTexture();
	GL_CHECK(textureEnvironmentMapCur_->bind(0, GL_READ_ONLY));
	GL_CHECK(destEnvMap->bind(1, GL_WRITE_ONLY));
	glUniform3f(curOriginLocation_, emOrigin_.x, emOrigin_.y, emOrigin_.z);
	glUniform3f(newOriginLocation_, emTranslatedOrigin_.x, emTranslatedOrigin_.y, emTranslatedOrigin_.z);
	glDispatchCompute(textureEnvironmentMapCur_->width() / NbrDiv, textureEnvironmentMapCur_->height() / NbrDiv, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram10_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif

	STAT_STOP(EMTranslate);

	//LOGI("VSENSE_DEBUG: %f", curTimeStats_.timeMS[EMTranslate]);

	if (overwriteOld_) { // Swap environment maps
		textureEnvironmentMapCur_ = destEnvMap;
		emOrigin_ = emTranslatedOrigin_;
	}	
}

void Process::updateSHCoefficients() {
	std::shared_ptr<gl::Texture> curEM = textureEnvironmentMapCur_;
	if (!overwriteOld_) {
		if (textureEnvironmentMapCur_ == textureEnvironmentMap1_)
			curEM = textureEnvironmentMap2_;
		else
			curEM = textureEnvironmentMap1_;
	}

	updateSHCoefficients(curEM);
}

void Process::updateSHCoefficients(std::shared_ptr<gl::Texture> emTexture) {
	std::shared_ptr<gl::Texture> desTexture = textureCoeffFinal1_;
	if(textureCoeffFinalCur_ == textureCoeffFinal1_)
		desTexture = textureCoeffFinal2_;

	// SH coefficients	
#ifdef _WINDOWS
	shaderProgram9_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram9_);
#endif
	textureCoeffRAcc_->clearTexture();
	textureCoeffGAcc_->clearTexture();
	textureCoeffBAcc_->clearTexture();
	textureCoeffAAcc_->clearTexture();
	desTexture->clearTexture();
	GL_CHECK(emTexture->bind(0, GL_READ_ONLY));
	GL_CHECK(textureRandomSamples_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureCoeffRAcc_->bind(2, GL_READ_WRITE));
	GL_CHECK(textureCoeffGAcc_->bind(3, GL_READ_WRITE));
	GL_CHECK(textureCoeffBAcc_->bind(4, GL_READ_WRITE));
	GL_CHECK(textureCoeffAAcc_->bind(5, GL_READ_WRITE));
	GL_CHECK(desTexture->bind(6, GL_WRITE_ONLY));
	glUniform1i(maxOrderLocation_, maxOrder_);
	glDispatchCompute(SHWorkGroups.x, SHWorkGroups.y, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram9_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif

#ifdef _WINDOWS
	shaderProgram9_->bind();
#elif __ANDROID__
	glUseProgram(shaderProgram9_);
#endif
	GL_CHECK(emTexture->bind(0, GL_READ_ONLY));
	GL_CHECK(textureRandomSamples_->bind(1, GL_READ_ONLY));
	GL_CHECK(textureCoeffRAcc_->bind(2, GL_READ_WRITE));
	GL_CHECK(textureCoeffGAcc_->bind(3, GL_READ_WRITE));
	GL_CHECK(textureCoeffBAcc_->bind(4, GL_READ_WRITE));
	GL_CHECK(textureCoeffAAcc_->bind(5, GL_READ_WRITE));
	GL_CHECK(desTexture->bind(6, GL_WRITE_ONLY));
	glUniform2i(numWGLocation9_, SHWorkGroups.x, SHWorkGroups.y);
	glUniform1i(maxOrderLocation_, maxOrder_);
	glDispatchCompute(1, 1, 1);
	glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
#ifdef _WINDOWS
	shaderProgram9_->release();
#elif __ANDROID__
	glUseProgram(0);
#endif

	textureCoeffFinalCur_ = desTexture;
}

#ifdef _WINDOWS
void Process::saveEM(bool simulateSampling, int idx) {
	QString filename;
	filename.sprintf("D:/data/out/EMT%d.png", idx*10);

	if (simulateSampling) {
		shaderProgram11_->bind();
		textureOutputMap_->clearTexture();
		GL_CHECK(textureEnvironmentMapCur_->bind(0, GL_READ_ONLY));
		GL_CHECK(textureOutputMap_->bind(1, GL_WRITE_ONLY));
		glDispatchCompute(textureEnvironmentMapCur_->width() / NbrDiv, textureEnvironmentMapCur_->height() / NbrDiv, 1);
		glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		shaderProgram11_->release();		

		textureOutputMap_->exportToImage(filename.toStdString());
	}
	else {
		//filename.sprintf("D:/data/out/frames/EM%d.csv", idx);
		//textureEnvironmentMapCur_->exportToFile(filename.toStdString());
		textureEnvironmentMapCur_->exportToImage(filename.toStdString());
	}
}
#endif
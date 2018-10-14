#include <vsense/gl/SHMeshPlaneDotObject.h>
#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Camera.h>
#include <vsense/gl/Texture.h>
#include <vsense/gl/Util.h>
#include <vsense/io/Image.h>
#include <vsense/io/ObjReader.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <fstream>

#ifdef _WINDOWS
#include <QOpenGLShaderProgram>
#elif __ANDROID__
#include <android/asset_manager_jni.h>
#endif

using namespace vsense;
using namespace vsense::gl;

const int VerticesPerRow = 40;

const float MinVal = -10.f;
const float MaxVal = 10.f;
const int NbrSteps = 200;
const float StepSize = (MaxVal - MinVal) / NbrSteps;

#ifdef _WINDOWS
SHMeshPlaneDotObject::SHMeshPlaneDotObject(float scale) : DrawableObject(Mesh) {
	createMesh(scale);
  
  shaderProgram_ = new QOpenGLShaderProgram;
  shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/shmeshPlaneDot.vert");
  shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/shmeshPlaneDot.frag");
  shaderProgram_->link();
  shaderProgram_->bind();

  vertexLocation_ = shaderProgram_->attributeLocation("vertex");
  normalLocation_ = shaderProgram_->attributeLocation("normal");

  mvpLocation_ = shaderProgram_->uniformLocation("mvp");

	coeffNbrLocation_ = shaderProgram_->uniformLocation("coeffNbr");

	coeffsPlaneLocation_ = shaderProgram_->uniformLocation("coeffsPlane");

	renderCoeffNbrLocation_ = shaderProgram_->uniformLocation("renderCoeffNbr");

	colorCorrectionMtxLocation_ = shaderProgram_->uniformLocation("colorCorrectionMtx");
	colorCorrectionLocation_ = shaderProgram_->uniformLocation("colorCorrection");

	shFactorLocation_ = shaderProgram_->uniformLocation("shFactor");
	shadowColorLocation_ = shaderProgram_->uniformLocation("shadowColor");

	shadowCoeffsTextureLocation_ = shaderProgram_->uniformLocation("shadowCoeffs");
	coeffAmbTextureLocation_ = shaderProgram_->uniformLocation("coeffAmb");

	renderCoeffNbr_ = 100;
	colorCorrectionLocation_ = false;

  shaderProgram_->release();

  initialized_ = true;
}
#elif __ANDROID__
SHMeshPlaneDotObject::SHMeshPlaneDotObject(AAssetManager* assetManager, float scale) : DrawableObject(assetManager, Mesh) {
	createMesh(scale);

	AAsset* vertexShaderAsset = AAssetManager_open(assetManager, "shaders/shmeshPlaneDot.vert", AASSET_MODE_BUFFER);
	const void *vertexShaderBuf = AAsset_getBuffer(vertexShaderAsset);
	off_t vertexShaderLength = AAsset_getLength(vertexShaderAsset);
	std::string vertexShaderSource = std::string((const char*)vertexShaderBuf, (size_t)vertexShaderLength);
	AAsset_close(vertexShaderAsset);

	AAsset* fragmentShaderAsset;
	fragmentShaderAsset = AAssetManager_open(assetManager, "shaders/shmeshPlaneDot.frag", AASSET_MODE_BUFFER);
	const void *fragmentShaderBuf = AAsset_getBuffer(fragmentShaderAsset);
	off_t fragmentShaderLength = AAsset_getLength(fragmentShaderAsset);
	std::string fragmentShaderSource = std::string((const char*)fragmentShaderBuf, (size_t)fragmentShaderLength);
	AAsset_close(fragmentShaderAsset);

	shaderProgram_ = util::createProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

	if (!shaderProgram_) {
		LOGE("Could not create program.");
	}

	vertexLocation_ = glGetAttribLocation(shaderProgram_, "vertex");
	normalLocation_ = glGetAttribLocation(shaderProgram_, "normal");

	mvpLocation_ = glGetUniformLocation(shaderProgram_, "mvp");

	coeffNbrLocation_ = glGetUniformLocation(shaderProgram_, "coeffNbr");

	coeffsPlaneLocation_ = glGetUniformLocation(shaderProgram_, "coeffsPlane");

	renderCoeffNbrLocation_ = glGetUniformLocation(shaderProgram_, "renderCoeffNbr");

	colorCorrectionMtxLocation_ = glGetUniformLocation(shaderProgram_, "colorCorrectionMtx");
	colorCorrectionLocation_ = glGetUniformLocation(shaderProgram_, "colorCorrection");

	shadowCoeffsTextureLocation_ = glGetUniformLocation(shaderProgram_, "shadowCoeffs");
	coeffAmbTextureLocation_ = glGetUniformLocation(shaderProgram_, "coeffAmb");

	shFactorLocation_ = glGetUniformLocation(shaderProgram_, "shFactor");
	shadowColorLocation_ = glGetUniformLocation(shaderProgram_, "shadowColor");

	renderCoeffNbr_ = 100;
	colorCorrectionLocation_ = false;

	shadowColor_ = 0.1f;

	initialized_ = true;
}
#endif

void SHMeshPlaneDotObject::createMesh(float scale) {
	mesh_.reset(new gl::StaticMesh());

	size_t nbrVertPerRow = NbrSteps + 1;
	size_t nbrVertices = nbrVertPerRow*nbrVertPerRow;

	mesh_->vertices_.resize(nbrVertices);
	mesh_->indices_.resize(NbrSteps*NbrSteps*6);
	mesh_->normals_.resize(nbrVertices);

	glm::vec3* curVert = &mesh_->vertices_[0];
	glm::vec3* curNorm = &mesh_->normals_[0];

	for (size_t i = 0; i < mesh_->vertices_.size(); i++) {
		float xCoord = MinVal + (i % nbrVertPerRow)*StepSize;
		float zCoord = MinVal + (i / nbrVertPerRow)*StepSize;

		*curVert = glm::vec3(xCoord, 0.f, zCoord)*scale;
		*curNorm = glm::vec3(0.f, 1.f, 0.f);

		++curVert;
		++curNorm;
	}

	GLuint* curIdx = &mesh_->indices_[0];
	for (size_t row = 0; row < NbrSteps; row++) {
		uint32_t rowIdx = row*nbrVertPerRow;
		for (size_t col = 0; col < NbrSteps; col++) {
			*(curIdx++) = rowIdx;
			*(curIdx++) = rowIdx + nbrVertPerRow;
			*(curIdx++) = rowIdx + 1;

			*(curIdx++) = rowIdx + 1;
			*(curIdx++) = rowIdx + nbrVertPerRow;
			*(curIdx++) = rowIdx + nbrVertPerRow + 1;

			rowIdx++;
		}
	}

	mesh_->renderMode_ = GL_TRIANGLES;
}

void SHMeshPlaneDotObject::render(const glm::mat4 &viewMat, const glm::mat4 &projMat) {
	if (!initialized_ || !visible_)
		return;

	glm::mat4 modelMat = getTransformMatrix();

#ifdef _WINDOWS
	shaderProgram_->bind();
#elif __ANDROID__
	GL_CHECK(glUseProgram(shaderProgram_));
#endif

	if (vertexLocation_ != -1) {
		GL_CHECK(glEnableVertexAttribArray(vertexLocation_));
		GL_CHECK(glVertexAttribPointer(vertexLocation_, 3, GL_FLOAT, GL_FALSE, 0, mesh_->vertices_.data()));
	}

	if ((normalLocation_ != -1) && mesh_->normals_.size()) {
		GL_CHECK(glEnableVertexAttribArray(normalLocation_));
		GL_CHECK(glVertexAttribPointer(normalLocation_, 3, GL_FLOAT, GL_FALSE, 0, mesh_->normals_.data()));
	}

	if (mvpLocation_ != -1) {
		glm::mat4 mvpMat = projMat * viewMat * modelMat;
		GL_CHECK(glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, glm::value_ptr(mvpMat)));
	}

	if (coeffNbrLocation_ != -1)
	GL_CHECK(glUniform1i(coeffNbrLocation_, coeffNbr_));

	if (coeffsPlaneLocation_ != -1)
	GL_CHECK(glUniform1fv(coeffsPlaneLocation_, 100, (GLfloat*)planeCoeff_.get()));

	if (shadowTexture_ && (shadowCoeffsTextureLocation_ != -1)) {
		GL_CHECK(glUniform1i(shadowCoeffsTextureLocation_, 0));
		shadowTexture_->bind(0);
	}

	if (coeffsAmb_ && (coeffAmbTextureLocation_ != -1)) {
		GL_CHECK(glUniform1i(coeffAmbTextureLocation_, 1));
		coeffsAmb_->bind(1);
	}

	if (colorCorrectionMtxLocation_ != -1)
	GL_CHECK(glUniformMatrix3fv(colorCorrectionMtxLocation_, 1, GL_FALSE, glm::value_ptr(colorCorrectionMtx_)));

	/*if (colorCorrectionLocation_ != -1)
    GL_CHECK(glUniform1i(colorCorrectionLocation_, colorCorrection_));*/

	if (shFactorLocation_ != -1)
	GL_CHECK(glUniform1f(shFactorLocation_, shFactor_));

	if(shadowColorLocation_ != -1)
	GL_CHECK(glUniform1f(shadowColorLocation_, shadowColor_));

	if (renderCoeffNbrLocation_ != -1)
	GL_CHECK(glUniform1i(renderCoeffNbrLocation_, renderCoeffNbr_));

	GL_CHECK(glDrawElements(mesh_->renderMode_, (GLsizei)mesh_->indices_.size(), GL_UNSIGNED_INT, mesh_->indices_.data()));

#ifdef _WINDOWS
	shaderProgram_->disableAttributeArray(vertexLocation_);
  shaderProgram_->disableAttributeArray(normalLocation_);

  shaderProgram_->release();
#elif __ANDROID__
	GL_CHECK(glUseProgram(0));
#endif
}

void SHMeshPlaneDotObject::updateCoefficients(const std::string& shadowFilename, const std::string& planeFilename) {
	std::ifstream shadowFile;
	shadowFile.open(shadowFilename, std::ios::in | std::ios::binary);

	uint32_t nbrVertices;
	uint8_t nbrOrder;
	shadowFile.read((char*)&nbrVertices, sizeof(uint32_t));
	shadowFile.read((char*)&nbrOrder, sizeof(uint8_t));

	coeffNbr_ = (nbrOrder + 1)*(nbrOrder + 1);

	glm::ivec2 coeffTexDim;
	coeffTexDim.x = coeffNbr_ * VerticesPerRow;
	coeffTexDim.y = (floor(nbrVertices / coeffTexDim.x) + 1)*coeffTexDim.x/(coeffTexDim.x/coeffNbr_);

	shadowCoeff_.reset(new float[(int)coeffTexDim.x*(int)coeffTexDim.y], std::default_delete<float[]>());
	float* curCoeff = shadowCoeff_.get();
	for (size_t i = 0; i < nbrVertices; i++) {
		shadowFile.read((char*)curCoeff, sizeof(float)*coeffNbr_);
		curCoeff += coeffNbr_;
	}
	shadowFile.close();

	std::vector<TexParam> texParams;
	texParams.push_back(TexParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	texParams.push_back(TexParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST));

	shadowTexture_.reset(new gl::Texture(coeffTexDim.x/4, coeffTexDim.y, 4, GL_FLOAT, texParams, (unsigned char*)shadowCoeff_.get()));

	std::ifstream planeFile;
	planeFile.open(planeFilename, std::ios::in | std::ios::binary);

	planeFile.read((char*)&nbrVertices, sizeof(uint32_t));
	planeFile.read((char*)&nbrOrder, sizeof(uint8_t));

	planeCoeff_.reset(new float[coeffNbr_], std::default_delete<float[]>());

	planeFile.read((char*)planeCoeff_.get(), sizeof(float) * coeffNbr_);

	planeFile.close();
}

void SHMeshPlaneDotObject::loadFromFile(const std::string& filename) {
	mesh_.reset(new StaticMesh());

	io::ObjReader::loadFromFile(filename, mesh_);
}
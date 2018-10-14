#include <vsense/gl/SHMeshDotObject.h>
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

#ifdef _WINDOWS
SHMeshDotObject::SHMeshDotObject(std::shared_ptr<gl::StaticMesh>& mesh) : DrawableObject(Mesh), baseColor_(1.f, 1.f, 1.f) {
	mesh_ = mesh;

	shaderProgram_ = new QOpenGLShaderProgram;
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":resources/shaders/shmeshDot.vert");
	shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":resources/shaders/shmeshDot.frag");
	shaderProgram_->link();
	shaderProgram_->bind();

	vertexLocation_ = shaderProgram_->attributeLocation("vertex");
	uvLocation_ = shaderProgram_->attributeLocation("uv");;
	normalLocation_ = shaderProgram_->attributeLocation("normal");

	mvpLocation_ = shaderProgram_->uniformLocation("mvp");

	coeffNbrLocation_ = shaderProgram_->uniformLocation("coeffNbr");

	baseColorLocation_ = shaderProgram_->uniformLocation("baseColor");

	renderCoeffNbrLocation_ = shaderProgram_->uniformLocation("renderCoeffNbr");

	selfOccCoeffTextureLocation_ = shaderProgram_->uniformLocation("selfOccCoeff");
	coeffAmbTextureLocation_ = shaderProgram_->uniformLocation("coeffAmb");

	colorCorrectionLocation_ = shaderProgram_->uniformLocation("colorCorrection");
	materialParamLocation_ = shaderProgram_->uniformLocation("materialParams");
	diffTextureLocation_ = shaderProgram_->uniformLocation("diffTexture");

	shFactorLocation_ = shaderProgram_->uniformLocation("shFactor");

	colorCorrectionMtxLocation_ = shaderProgram_->uniformLocation("colorCorrectionMtx");

	renderCoeffNbr_ = 25;

	shaderProgram_->release();

	initialized_ = true;
}
#elif __ANDROID__
SHMeshDotObject::SHMeshDotObject(AAssetManager* assetManager, std::shared_ptr<gl::StaticMesh>& mesh) : DrawableObject(assetManager, Mesh), baseColor_(1.f, 1.f, 1.f) {
  mesh_ = mesh;

  AAsset* vertexShaderAsset = AAssetManager_open(assetManager, "shaders/shmeshDot.vert", AASSET_MODE_BUFFER);
  const void *vertexShaderBuf = AAsset_getBuffer(vertexShaderAsset);
  off_t vertexShaderLength = AAsset_getLength(vertexShaderAsset);
  std::string vertexShaderSource = std::string((const char*)vertexShaderBuf, (size_t)vertexShaderLength);
  AAsset_close(vertexShaderAsset);

  AAsset* fragmentShaderAsset;
  fragmentShaderAsset = AAssetManager_open(assetManager, "shaders/shmeshDot.frag", AASSET_MODE_BUFFER);
  const void *fragmentShaderBuf = AAsset_getBuffer(fragmentShaderAsset);
  off_t fragmentShaderLength = AAsset_getLength(fragmentShaderAsset);
  std::string fragmentShaderSource = std::string((const char*)fragmentShaderBuf, (size_t)fragmentShaderLength);
  AAsset_close(fragmentShaderAsset);

  shaderProgram_ = util::createProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

  if (!shaderProgram_) {
    LOGE("Could not create program.");
  }

  vertexLocation_ = glGetAttribLocation(shaderProgram_, "vertex");
  uvLocation_ = glGetAttribLocation(shaderProgram_, "uv");;
  normalLocation_ = glGetAttribLocation(shaderProgram_, "normal");

  mvpLocation_ = glGetUniformLocation(shaderProgram_, "mvp");

  coeffNbrLocation_ = glGetUniformLocation(shaderProgram_, "coeffNbr");

  baseColorLocation_ = glGetUniformLocation(shaderProgram_, "baseColor");

  renderCoeffNbrLocation_ = glGetUniformLocation(shaderProgram_, "renderCoeffNbr");

  selfOccCoeffTextureLocation_ = glGetUniformLocation(shaderProgram_, "selfOccCoeff");
  coeffAmbTextureLocation_ = glGetUniformLocation(shaderProgram_, "coeffAmb");

  colorCorrectionLocation_ = glGetUniformLocation(shaderProgram_, "colorCorrection");
  materialParamLocation_ = glGetUniformLocation(shaderProgram_, "materialParams");
  diffTextureLocation_ = glGetUniformLocation(shaderProgram_, "diffTexture");

  shFactorLocation_ = glGetUniformLocation(shaderProgram_, "shFactor");

  colorCorrectionMtxLocation_ = glGetUniformLocation(shaderProgram_, "colorCorrectionMtx");

  renderCoeffNbr_ = 25;

#ifdef _WINDOWS
  shaderProgram_->release();
#elif __ANDROID__
  glUseProgram(0);
#endif

  initialized_ = true;
}
#endif

void SHMeshDotObject::render(const glm::mat4 &viewMat, const glm::mat4 &projMat) {
  if (!initialized_ || !visible_)
    return;

  glm::mat4 modelMat = getTransformMatrix();

#ifdef _WINDOWS
  shaderProgram_->bind();
#elif __ANDROID__
  glUseProgram(shaderProgram_);
#endif

  if (vertexLocation_ != -1) {
    glEnableVertexAttribArray(vertexLocation_);
    glVertexAttribPointer(vertexLocation_, 3, GL_FLOAT, GL_FALSE, 0, mesh_->vertices_.data());
  }

  if ((uvLocation_ != -1) && mesh_->uv_.size()) {
    glEnableVertexAttribArray(uvLocation_);
    glVertexAttribPointer(uvLocation_, 2, GL_FLOAT, GL_FALSE, 0, mesh_->uv_.data());
  }

  if ((normalLocation_ != -1) && mesh_->normals_.size()) {
    glEnableVertexAttribArray(normalLocation_);
    glVertexAttribPointer(normalLocation_, 3, GL_FLOAT, GL_FALSE, 0, mesh_->normals_.data());
  }

  if (mvpLocation_ != -1) {
    glm::mat4 mvpMat = projMat * viewMat * modelMat;
    glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, glm::value_ptr(mvpMat));
  }

  if (coeffNbrLocation_ != -1)
    glUniform1i(coeffNbrLocation_, coeffNbr_);

  if (baseColorLocation_ != -1)
    glUniform3f(baseColorLocation_, baseColor_.r, baseColor_.g, baseColor_.b);

  if (soTexture_ && (selfOccCoeffTextureLocation_ != -1)) {
    glUniform1i(selfOccCoeffTextureLocation_, 0);
    soTexture_->bind(0);
  }

  if (coeffsAmb_ && (coeffAmbTextureLocation_ != -1)) {
    glUniform1i(coeffAmbTextureLocation_, 1);
    coeffsAmb_->bind(1);
  }

  if (diffTexture_ && (diffTextureLocation_ != -1)) {
    glUniform1i(diffTextureLocation_, 2);
    diffTexture_->bind(2);
  }

  if (materialParamLocation_ != -1)
    glUniform4f(materialParamLocation_, ambient_, diffuse_, specular_, specularPower_);

  if (colorCorrectionLocation_ != -1)
    glUniform1i(colorCorrectionLocation_, colorCorrection_);

  if (renderCoeffNbrLocation_ != -1)
    glUniform1i(renderCoeffNbrLocation_, renderCoeffNbr_);

  if (shFactorLocation_ != -1)
    glUniform1f(shFactorLocation_, shFactor_);

  if (colorCorrectionMtxLocation_ != -1)
    glUniformMatrix3fv(colorCorrectionMtxLocation_, 1, GL_FALSE, glm::value_ptr(colorCorrectionMtx_));

  glDrawElements(mesh_->renderMode_, (GLsizei)mesh_->indices_.size(), GL_UNSIGNED_INT, mesh_->indices_.data());

#ifdef _WINDOWS
  shaderProgram_->disableAttributeArray(vertexLocation_);
  shaderProgram_->disableAttributeArray(uvLocation_);
  shaderProgram_->disableAttributeArray(normalLocation_);

  shaderProgram_->release();
#elif __ANDROID__
  glUseProgram(0);

  initialized_ = true;
#endif
}

void SHMeshDotObject::updateCoefficients(const std::string& meshFilename) {
  std::ifstream soFile;
  soFile.open(meshFilename, std::ios::in | std::ios::binary);

  uint32_t nbrVertices;
  uint8_t nbrOrder;
  soFile.read((char*)&nbrVertices, sizeof(uint32_t));
  soFile.read((char*)&nbrOrder, sizeof(uint8_t));

  coeffNbr_ = (nbrOrder + 1)*(nbrOrder + 1);

  glm::ivec2 coeffTexDim;
  coeffTexDim.x = coeffNbr_ * VerticesPerRow;
  coeffTexDim.y = (floor(nbrVertices / coeffTexDim.x) + 1)*coeffTexDim.x / (coeffTexDim.x / coeffNbr_);

  soCoeff_.reset(new float[(int)coeffTexDim.x*(int)coeffTexDim.y], std::default_delete<float[]>());
  float* curCoeff = soCoeff_.get();
  for (size_t i = 0; i < nbrVertices; i++) {
    soFile.read((char*)curCoeff, sizeof(float)*coeffNbr_);
    curCoeff += coeffNbr_;
  }
  soFile.close();

  std::vector<TexParam> texParams;
  texParams.push_back(TexParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  texParams.push_back(TexParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST));

  soTexture_.reset(new gl::Texture(coeffTexDim.x/4, coeffTexDim.y, 4, GL_FLOAT, texParams, (unsigned char*)soCoeff_.get()));
}

void SHMeshDotObject::loadFromFile(const std::string& filename) {
  mesh_.reset(new StaticMesh());

  io::ObjReader::loadFromFile(filename, mesh_);
}

void SHMeshDotObject::setMaterialProperty(float ambient, float diffuse, float specular, float specularPower) {
  ambient_ = ambient;
  diffuse_ = diffuse;
  specular_ = specular;
  specularPower_ = specularPower;
}
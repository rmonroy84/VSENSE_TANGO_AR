#include <vsense/gl/MeshObject.h>
#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Camera.h>
#include <vsense/gl/Texture.h>
#include <vsense/gl/Util.h>
#include <vsense/io/Image.h>
#include <vsense/io/ObjReader.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef _WINDOWS
#include <QOpenGLShaderProgram>
#elif __ANDROID__
#include <android/asset_manager_jni.h>
#endif

using namespace vsense;
using namespace vsense::gl;

#ifdef _WINDOWS
MeshObject::MeshObject(std::shared_ptr<gl::StaticMesh>& mesh) : DrawableObject(Mesh) {
  mesh_ = mesh;
  texture_.reset(new Texture());

  shaderProgram_ = new QOpenGLShaderProgram;
  shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/mesh.vert");
  shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/mesh.frag");
  shaderProgram_->link();
  shaderProgram_->bind();

  vertexLocation_ = shaderProgram_->attributeLocation("vertex");
  colorLocation_ = shaderProgram_->attributeLocation("color");
  normalLocation_ = shaderProgram_->attributeLocation("normal");
  uvLocation_ = shaderProgram_->attributeLocation("uv");

  mvpLocation_ = shaderProgram_->uniformLocation("mvp");
  mvLocation_ = shaderProgram_->uniformLocation("mv");
  lightVecLocation_ = shaderProgram_->uniformLocation("lightVec");
  textureLocation_ = shaderProgram_->uniformLocation("texture");

  shaderProgram_->release();

  initialized_ = true;
}
#elif __ANDROID__
MeshObject::MeshObject(AAssetManager* assetManager, std::shared_ptr<gl::StaticMesh>& mesh) : DrawableObject(assetManager, Mesh) {
	mesh_ = mesh;
  texture_.reset(new Texture());

  AAsset* vertexShaderAsset = AAssetManager_open(assetManager, "shaders/mesh.vert", AASSET_MODE_BUFFER);
  const void *vertexShaderBuf = AAsset_getBuffer(vertexShaderAsset);
  off_t vertexShaderLength = AAsset_getLength(vertexShaderAsset);
  std::string vertexShaderSource = std::string((const char*)vertexShaderBuf, (size_t)vertexShaderLength);
  AAsset_close(vertexShaderAsset);

  AAsset* fragmentShaderAsset;
  fragmentShaderAsset = AAssetManager_open(assetManager, "shaders/mesh.frag", AASSET_MODE_BUFFER);
  const void *fragmentShaderBuf = AAsset_getBuffer(fragmentShaderAsset);
  off_t fragmentShaderLength = AAsset_getLength(fragmentShaderAsset);
  std::string fragmentShaderSource = std::string((const char*)fragmentShaderBuf, (size_t)fragmentShaderLength);
  AAsset_close(fragmentShaderAsset);

  shaderProgram_ = util::createProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

  if (!shaderProgram_) {
    LOGE("Could not create program.");
  }

  vertexLocation_ = glGetAttribLocation(shaderProgram_, "vertex");
  colorLocation_ = glGetAttribLocation(shaderProgram_, "color");
  normalLocation_ = glGetAttribLocation(shaderProgram_, "normal");
  uvLocation_ = glGetAttribLocation(shaderProgram_, "uv");

  mvpLocation_ = glGetUniformLocation(shaderProgram_, "mvp");
  mvLocation_ = glGetUniformLocation(shaderProgram_, "mv");
  lightVecLocation_ = glGetUniformLocation(shaderProgram_, "lightVec");
  textureLocation_ = glGetUniformLocation(shaderProgram_, "texture");

  initialized_ = true;
}
#endif

void MeshObject::render(const glm::mat4 &viewMat, const glm::mat4 &projMat) {
  if (!initialized_ || !visible_)
    return;

  glm::mat4 modelMat = getTransformMatrix();

#ifdef _WINDOWS
  shaderProgram_->bind();
#elif __ANDROID__
  glUseProgram(shaderProgram_);
#endif

  if (mvpLocation_ != -1) {
    glm::mat4 mvpMat = projMat * viewMat * modelMat;
    glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, glm::value_ptr(mvpMat));
  }

  if (mvLocation_ != -1) {
    glm::mat4 mvMat = viewMat * modelMat;
    glUniformMatrix4fv(mvLocation_, 1, GL_FALSE, glm::value_ptr(mvMat));
  }

  if (lightVecLocation_ != -1) {
    glm::vec3 lightDirection = glm::mat3(viewMat) * glm::normalize(glm::vec3(-1.0f, -3.0f, -1.0f));
    glUniform3fv(lightVecLocation_, 1, glm::value_ptr(lightDirection));
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(texture_->getTextureTarget(), texture_->getTextureID());
  glUniform1i(textureLocation_, 0);

  if (vertexLocation_ != -1) {
    glEnableVertexAttribArray(vertexLocation_);
    glVertexAttribPointer(vertexLocation_, 3, GL_FLOAT, GL_FALSE, 0, mesh_->vertices_.data());
  }

  if ((normalLocation_ != -1) && mesh_->normals_.size()) {
    glEnableVertexAttribArray(normalLocation_);
    glVertexAttribPointer(normalLocation_, 3, GL_FLOAT, GL_FALSE, 0, mesh_->normals_.data());
  }

  if ((colorLocation_ != -1) && mesh_->colors_.size()) {
    glEnableVertexAttribArray(colorLocation_);
    glVertexAttribPointer(colorLocation_, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), mesh_->colors_.data());
  }

  if ((uvLocation_ != -1) && mesh_->uv_.size()) {
    glEnableVertexAttribArray(uvLocation_);
    glVertexAttribPointer(uvLocation_, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), mesh_->uv_.data());
  }

  glDrawElements(mesh_->renderMode_, (GLsizei)mesh_->indices_.size(), GL_UNSIGNED_INT, mesh_->indices_.data());

#ifdef _WINDOWS
  shaderProgram_->disableAttributeArray(vertexLocation_);
  shaderProgram_->disableAttributeArray(normalLocation_);
  shaderProgram_->disableAttributeArray(colorLocation_);
  shaderProgram_->disableAttributeArray(uvLocation_);

  shaderProgram_->release();
#elif __ANDROID__
  glUseProgram(0);
#endif
}

void MeshObject::updateImage(const std::shared_ptr<io::Image>& img) {
  texture_->updateTexture(img);
}

void MeshObject::loadFromFile(const std::string& filename) {
	mesh_.reset(new StaticMesh());

	io::ObjReader::loadFromFile(filename, mesh_);
}
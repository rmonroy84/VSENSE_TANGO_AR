#include <vsense/gl/GizmoObject.h>
#include <vsense/gl/Camera.h>
#include <vsense/gl/Util.h>
#include <vsense/gl/StaticMesh.h>

#include <string>

#ifdef _WINDOWS
#include <QOpenGLShaderProgram>
#elif __ANDROID__
#include <android/asset_manager_jni.h>
#endif

#include <glm/gtc/type_ptr.hpp>

#include <vector>

using namespace vsense::gl;

#ifdef _WINDOWS
GizmoObject::GizmoObject() : DrawableObject(Lines) {
	shaderProgram_ = new QOpenGLShaderProgram;
  shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/gizmo.vert");
  shaderProgram_->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/gizmo.frag");
  shaderProgram_->link();
  shaderProgram_->bind();

  vertexLocation_ = shaderProgram_->attributeLocation("vertex");
	colorLocation_ = shaderProgram_->attributeLocation("color");
	matrixLocation_ = shaderProgram_->uniformLocation("mvp");

	mesh_.reset(new StaticMesh());
	mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(0.1f, 0.f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(0.f, 0.1f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.f));
	mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.1f));

	mesh_->colors_.push_back(glm::vec4(1.f, 0.f, 0.f, 1.f));
	mesh_->colors_.push_back(glm::vec4(1.f, 0.f, 0.f, 1.f));
	mesh_->colors_.push_back(glm::vec4(0.f, 1.f, 0.f, 1.f));
	mesh_->colors_.push_back(glm::vec4(0.f, 1.f, 0.f, 1.f));
	mesh_->colors_.push_back(glm::vec4(0.f, 0.f, 1.f, 1.f));
	mesh_->colors_.push_back(glm::vec4(0.f, 0.f, 1.f, 1.f));

	mesh_->renderMode_ = GL_LINES;

  shaderProgram_->release();

  initialized_ = true;
}
#elif __ANDROID__
GizmoObject::GizmoObject(AAssetManager* assetManager) : DrawableObject(assetManager, Lines) {
  AAsset* vertexShaderAsset = AAssetManager_open(assetManager, "shaders/gizmo.vert", AASSET_MODE_BUFFER);
  const void *vertexShaderBuf = AAsset_getBuffer(vertexShaderAsset);
  off_t vertexShaderLength = AAsset_getLength(vertexShaderAsset);
  std::string vertexShaderSource = std::string((const char*)vertexShaderBuf, (size_t)vertexShaderLength);
  AAsset_close(vertexShaderAsset);

  AAsset* fragmentShaderAsset;
  fragmentShaderAsset = AAssetManager_open(assetManager, "shaders/gizmo.frag", AASSET_MODE_BUFFER);
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
  matrixLocation_ = glGetUniformLocation(shaderProgram_, "mvp");

  mesh_.reset(new StaticMesh());
  mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.f));
  mesh_->vertices_.push_back(glm::vec3(0.1f, 0.f, 0.f));
  mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.f));
  mesh_->vertices_.push_back(glm::vec3(0.f, 0.1f, 0.f));
  mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.f));
  mesh_->vertices_.push_back(glm::vec3(0.f, 0.f, 0.1f));

  mesh_->colors_.push_back(glm::vec4(1.f, 0.f, 0.f, 1.f));
  mesh_->colors_.push_back(glm::vec4(1.f, 0.f, 0.f, 1.f));
  mesh_->colors_.push_back(glm::vec4(0.f, 1.f, 0.f, 1.f));
  mesh_->colors_.push_back(glm::vec4(0.f, 1.f, 0.f, 1.f));
  mesh_->colors_.push_back(glm::vec4(0.f, 0.f, 1.f, 1.f));
  mesh_->colors_.push_back(glm::vec4(0.f, 0.f, 1.f, 1.f));

  mesh_->renderMode_ = GL_LINES;

  initialized_ = true;
}
#endif

void GizmoObject::render(const glm::mat4 &viewMat, const glm::mat4 &projMat) {
  if (!initialized_ || !visible_)
    return;

#ifdef _WINDOWS
  shaderProgram_->bind();
#elif __ANDROID__
  glUseProgram(shaderProgram_);
#endif

  if (matrixLocation_ != -1) {
    glm::mat4 mvpMat = projMat * viewMat;
    glUniformMatrix4fv(matrixLocation_, 1, GL_FALSE, glm::value_ptr(mvpMat));
  }

  if (vertexLocation_ != -1) {
    glEnableVertexAttribArray(vertexLocation_);
    glVertexAttribPointer(vertexLocation_, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), mesh_->vertices_.data());
  }

  if (colorLocation_ != -1) {
    glEnableVertexAttribArray(colorLocation_);
    glVertexAttribPointer(colorLocation_, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), mesh_->colors_.data());
  }

  glDrawArrays(mesh_->renderMode_, 0, (GLsizei)mesh_->vertices_.size());

#ifdef _WINDOWS
  shaderProgram_->disableAttributeArray(vertexLocation_);
  shaderProgram_->disableAttributeArray(colorLocation_);

	shaderProgram_->release();
#elif __ANDROID__
  glUseProgram(0);
#endif
}
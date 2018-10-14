#include <vsense/gl/DrawableObject.h>

#include <vsense/gl/Camera.h>

#ifdef __ANDROID__
#include <android/asset_manager_jni.h>
#endif

using namespace vsense::gl;

#ifdef _WINDOWS
DrawableObject::DrawableObject(ObjectType type) : initialized_(false), visible_(true), type_(type) {
  initializeOpenGLFunctions();
}
#elif __ANDROID__
DrawableObject::DrawableObject(AAssetManager* assetManager, ObjectType type) : assetManager_(assetManager), initialized_(false), visible_(true), type_(type) {

}
#endif

DrawableObject::~DrawableObject() {
  release();
}

void DrawableObject::release() {
  if (initialized_) {
    mesh_.reset();
    initialized_ = false;
  }
}

void DrawableObject::translate(const glm::vec3& dt) {
  transform_.translate(dt);
}

void DrawableObject::transform(const double* translation, const double* orientation) {
  transform(glm::vec3(translation[0], translation[1], translation[2]), glm::quat(orientation[3], orientation[0], orientation[1], orientation[2]));
}

void DrawableObject::transform(const glm::vec3& translation, const glm::quat& orientation) {
  transform_.setPosition(translation);
  transform_.setRotation(orientation);
}

glm::mat4 DrawableObject::getTransformMatrix() const {
  return transform_.getTransformationMatrix();
}

void DrawableObject::render(const Camera *camera) {
  render(camera->getViewMatrix(), camera->getProjectionMatrix());
}
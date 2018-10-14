#include <vsense/gl/Camera.h>
#include <vsense/gl/Util.h>

using namespace vsense::gl;

Camera::Camera() {
  fov_ = 45.0f * DEGREE_2_RADIANS;
  aspectRatio_ = 4.0f / 3.0f;
  nearClipPlane_ = 0.1f;
  farClipPlane_ = 100.0f;
  updateProjectionMatrix();
}

Camera::~Camera() {}

void Camera::updateProjectionMatrix() {
  projectionMatrix_ = glm::perspective(fov_, aspectRatio_, nearClipPlane_, farClipPlane_);
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::inverse(getTransformationMatrix());
}

glm::mat4 Camera::getProjectionMatrix() const {
  return projectionMatrix_;
}

void Camera::setAspectRatio(float aspectRatio) {
  aspectRatio_ = aspectRatio;
  updateProjectionMatrix();
}

void Camera::setFieldOfView(float fov) {
  fov_ = fov * DEGREE_2_RADIANS;
  updateProjectionMatrix();
}

void Camera::setProjectionMatrix(const glm::mat4& projectionMatrix) {
  projectionMatrix_ = projectionMatrix;
}

glm::mat4 Camera::projectionMatrixForCameraIntrinsics(float width, float height, float fx, float fy, float cx, float cy, float nearPlane, float farPlane) {
  const float xScale = nearPlane / fx;
  const float yScale = nearPlane / fy;

  const float xOffset = (cx - (width / 2.f)) * xScale;
  // Color camera's coordinates has y pointing downwards so we negate this term.
  const float yOffset = -(cy - (height / 2.f)) * yScale;

  return glm::frustum(xScale * -width / 2.0f - xOffset,
                      xScale * width / 2.0f - xOffset,
                      yScale * -height / 2.0f - yOffset,
                      yScale * height / 2.0f - yOffset, nearPlane, farPlane);
}


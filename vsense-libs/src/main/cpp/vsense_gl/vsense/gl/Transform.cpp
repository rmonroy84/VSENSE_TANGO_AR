/*
 * Copyright 2014 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <vsense/gl/Transform.h>
#include <vsense/gl/Util.h>

using namespace vsense::gl;

const glm::vec3 Transform::LocalForward(0.0, 0.0, -1.0);
const glm::vec3 Transform::LocalUp(0.0, 1.0, 0.0);
const glm::vec3 Transform::LocalRight(1.0, 0.0, 0.0);

Transform::Transform()
    : parent_(nullptr),
      position_(0.0f, 0.0f, 0.0f),
      rotation_(1.0f, 0.0f, 0.0f, 0.0f),
      scale_(1.0f, 1.0f, 1.0f) {}

Transform::~Transform() {
  // Objects are not responsible for deleting their parents.
}

void Transform::translate(const glm::vec3& dt) {
  position_ += dt;
}

void Transform::rotate(const glm::quat& dr) {
  rotation_ = dr*rotation_;
}

void Transform::rotate(float angle, const glm::vec3& axis) {
  rotate(glm::angleAxis(angle, axis));
}

void Transform::setPosition(const glm::vec3& position) { position_ = position; }

glm::vec3 Transform::getPosition() const { return position_; }

void Transform::setRotation(const glm::quat& rotation) { rotation_ = rotation; }

glm::quat Transform::getRotation() const { return rotation_; }

void Transform::setScale(const glm::vec3& scale) { scale_ = scale; }

glm::vec3 Transform::getScale() const { return scale_; }

void Transform::setTransformationMatrix(const glm::mat4& transformMat) {
  util::decomposeMatrix(transformMat, &position_, &rotation_, &scale_);
}

glm::mat4 Transform::getTransformationMatrix() const {
  glm::mat4 transMat = glm::scale(glm::mat4_cast(rotation_), scale_);
  transMat[3][0] = position_.x;
  transMat[3][1] = position_.y;
  transMat[3][2] = position_.z;
  glm::mat4 parentMat = glm::mat4(1.0f);
  if (parent_ != nullptr) {
    parentMat = parent_->getTransformationMatrix();
    transMat = parentMat * transMat;
  }
  return transMat;
}

void Transform::setParent(Transform* transform) { parent_ = transform; }

const Transform* Transform::getParent() const { return parent_; }

Transform* Transform::getParent() { return parent_; }

const glm::vec3 Transform::forward() const {
  return glm::mat3_cast(getRotation())*LocalForward;
}

const glm::vec3 Transform::right() const {
  return glm::mat3_cast(getRotation())*LocalRight;
}

const glm::vec3 Transform::up() const {
  return glm::mat3_cast(getRotation())*LocalUp;
}


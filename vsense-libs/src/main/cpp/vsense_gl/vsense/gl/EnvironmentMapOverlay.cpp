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
#include <cstdlib>

#include <vsense/gl/EnvironmentMapOverlay.h>
#include <vsense/gl/Camera.h>
#include <vsense/gl/Util.h>
#include <vsense/gl/Texture.h>
#include <vsense/io/Image.h>

#include <android/asset_manager_jni.h>
#include <android/asset_manager_jni.h>

const GLfloat kVertices[] = {-0.96085, -0.13056, 0.0, -0.96085, -0.93056, 0.0,
                             -0.56085,  -0.13056, 0.0, -0.56085,  -0.93056, 0.0};

const GLushort kIndices[] = {0, 1, 2, 2, 1, 3};

using namespace vsense::gl;

EnvironmentMapOverlay::EnvironmentMapOverlay(AAssetManager* assetManager)
    : DrawableObject(assetManager),
      displayRotation_(TangoSupportRotation::ROTATION_IGNORED),
      uOffset_(0.0f), vOffset_(0.0f) {

  texture_.reset(new Texture());
  initialize(assetManager);
}

void EnvironmentMapOverlay::initialize(AAssetManager* assetManager) {
  AAsset* vertexShaderAsset = AAssetManager_open(assetManager, "shaders/environmentMapOverlay.vert", AASSET_MODE_BUFFER);
  const void *vertexShaderBuf = AAsset_getBuffer(vertexShaderAsset);
  off_t vertexShaderLength = AAsset_getLength(vertexShaderAsset);
  std::string vertexShaderSource = std::string((const char*)vertexShaderBuf, (size_t)vertexShaderLength);
  AAsset_close(vertexShaderAsset);

  AAsset* fragmentShaderAsset;
  fragmentShaderAsset = AAssetManager_open(assetManager, "shaders/environmentMapOverlay.frag", AASSET_MODE_BUFFER);
  const void *fragmentShaderBuf = AAsset_getBuffer(fragmentShaderAsset);
  off_t fragmentShaderLength = AAsset_getLength(fragmentShaderAsset);
  std::string fragmentShaderSource = std::string((const char*)fragmentShaderBuf, (size_t)fragmentShaderLength);
  AAsset_close(fragmentShaderAsset);

  setDisplayRotation(displayRotation_);

  shaderProgram_ = util::createProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

  if (!shaderProgram_) {
    LOGE("Could not create program.");
  }

  verticesLocation_ = glGetAttribLocation(shaderProgram_, "vertex");
  texCoordLocation_ = glGetAttribLocation(shaderProgram_, "textureCoords");
  mvpLocation_ = glGetUniformLocation(shaderProgram_, "mvp");
  textureLocation_ = glGetUniformLocation(shaderProgram_, "textureImg");

  // Allocate vertices buffer.
  GL_CHECK(glGenBuffers(2, vertexBuffers_));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers_[0]));
  GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 4, kVertices, GL_STATIC_DRAW));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

  // Allocate triangle indices buffer.
  GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffers_[1]));
  GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 6, kIndices, GL_STATIC_DRAW));
  GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

  // Assign the vertices attribute data.
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers_[0]));
  GL_CHECK(glEnableVertexAttribArray(verticesLocation_));
  GL_CHECK(glVertexAttribPointer(verticesLocation_, 3, GL_FLOAT, GL_FALSE, 0, nullptr));
  GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void EnvironmentMapOverlay::setDisplayRotation(TangoSupportRotation displayRotation) {
  displayRotation_ = displayRotation;
  float texCoords0[8] = {
      0.0f + uOffset_, 0.0f + vOffset_, 0.0f + uOffset_, 1.0f - vOffset_,
      1.0f - uOffset_, 0.0f + vOffset_, 1.0f - uOffset_, 1.0f - vOffset_};
  float outputTexCoords[8];
  TangoErrorType r = TangoSupport_getVideoOverlayUVBasedOnDisplayRotation(texCoords0, displayRotation_, outputTexCoords);

  if (r == TANGO_SUCCESS)
    std::copy(std::begin(outputTexCoords), std::end(outputTexCoords), std::begin(texCoords_));
}

void EnvironmentMapOverlay::setTextureOffset(float screenWidth, float screenHeight, float imageWidth, float imageHeight) {
  if ((screenWidth / screenHeight > 1.0f) !=
      (imageWidth / imageHeight > 1.0f)) {
    // if image ratio and screen ratio don't comply to each other, we always
    // aligned things to screen ratio.
    float tmp = imageWidth;
    imageWidth = imageHeight;
    imageHeight = tmp;
  }

  float screen_ratio = screenWidth / screenHeight;
  float image_ratio = imageWidth / imageHeight;
  float zoomFactor = 1.0f;

  if (image_ratio > screen_ratio)
    zoomFactor = screenHeight / imageHeight;
  else
    zoomFactor = screenWidth / imageWidth;

  float renderWidth = imageWidth * zoomFactor;
  float renderHeight = imageHeight * zoomFactor;

  uOffset_ = ((renderWidth - screenWidth) / 2.0f) / renderWidth;
  vOffset_ = ((renderHeight - screenHeight) / 2.0f) / renderHeight;

  setDisplayRotation(displayRotation_);
}

void EnvironmentMapOverlay::render(const glm::mat4& viewMat, const glm::mat4& projMat) {
  if(!visible_)
    return;

  glUseProgram(shaderProgram_);

  if(textureLocation_ != -1) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texture_->getTextureTarget(), texture_->getTextureID());
    glUniform1i(textureLocation_, 0);
  }

  glm::mat4 modelMat = transform_.getTransformationMatrix();

  glm::mat4 mvpMat = projMat * viewMat * modelMat;
  if(mvpLocation_ != -1)
    glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, glm::value_ptr(mvpMat));

  // Bind vertices buffer.
  if(verticesLocation_ != -1) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers_[0]);
    glEnableVertexAttribArray(verticesLocation_);
    glVertexAttribPointer(verticesLocation_, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  if(texCoordLocation_ != -1) {
    glEnableVertexAttribArray(texCoordLocation_);
    glVertexAttribPointer(texCoordLocation_, 2, GL_FLOAT, GL_FALSE, 0, texCoords_.data());
  }

  // Bind element array buffer.
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBuffers_[1]);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
  util::checkGlError("glDrawElements");
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glUseProgram(0);

  util::checkGlError("glUseProgram()");
}

void EnvironmentMapOverlay::updateImage(const std::shared_ptr<io::Image>& img) {
  texture_->updateTexture(img);
}

void EnvironmentMapOverlay::updateTexture(std::shared_ptr<Texture> texture) {
  texture_ = texture;
}
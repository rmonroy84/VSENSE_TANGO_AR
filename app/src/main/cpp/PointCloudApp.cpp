/*
 * Copyright 2016 Google Inc. All Rights Reserved.
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

#include "PointCloudApp.h"

#include <vsense/gl/Camera.h>
#include <vsense/gl/GizmoObject.h>
#include <vsense/gl/MeshObject.h>
#include <vsense/gl/SHMeshDotObject.h>
#include <vsense/gl/SHMeshPlaneDotObject.h>
#include <vsense/gl/PointCloudObject.h>
#include <vsense/gl/VideoOverlay.h>
#include <vsense/gl/EnvironmentMapOverlay.h>
#include <vsense/gl/StaticMesh.h>
#include <vsense/gl/Texture.h>
#include <vsense/io/Image.h>
#include <vsense/io/ObjReader.h>
#include <vsense/sh/SphericalHarmonics.h>
#include <vsense/depth/DepthMap.h>
#include <vsense/em/EnvironmentMap.h>
#include <vsense/em/Process.h>

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/quaternion.hpp>

#include <fstream>
#include <string>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace vsense;
using namespace vsense::ar;

const std::string StorageFolder = "/sdcard/TCD/";
const std::string DataFolder = StorageFolder + "data/";
const std::string MeshFolder = StorageFolder + "mesh/";
const std::string SphericalHarmonicsFolder = StorageFolder + "sh/";

const std::string BunnyBasename = "bunny";
const std::string RafaBasename = "rafa";
const std::string SpotBasename = "spot";
const std::string NefertitiBasename = "nefertiti";
const std::string DragonBasename = "dragon";
const std::string MortyBasename = "morty";

const std::string MeshSOFile = "_Coeffs.msh";
const std::string ShadowFile = "_PlaneCoeffsWith.msh";
const std::string PlaneFile = "_PlaneCoeffs.msh";

const unsigned char HasPointCloud = 0x01;
const unsigned char HasImage      = 0x02;
const unsigned char HasBoth       = HasPointCloud | HasImage;

const float ObjectOffset = 0.001f;

const float AnimationRadius = 0.2f;

const uint16_t MaxMissingFrames = 10;

const float LimitSHCalculation = 5.f; // Recalculate SH every 10degs

const std::string Perf_EM    = "EM";
const std::string Perf_MSE    = "MSE";

namespace {
// The minimum Tango Core version required from this application.
  constexpr int kTangoCoreMinimumVersion = 9377;

  void onPointCloudAvailableRouter(void* context, const TangoPointCloud* point_cloud) {
    PointCloudApp* app = static_cast<PointCloudApp*>(context);
    app->onPointCloudAvailable(point_cloud);
  }

  void onFrameAvailableRouter(void* context, TangoCameraId, const TangoImageBuffer* buffer) {
    PointCloudApp* app = static_cast<PointCloudApp*>(context);
    app->onFrameAvailable(buffer);
  }

/**
 * Create an OpenGL perspective matrix from window size, camera intrinsics, and clip settings.
 */
  glm::mat4 projectionMatrixForCameraIntrinsics(const TangoCameraIntrinsics& intrinsics, float near, float far) {
    // Adjust camera intrinsics according to rotation
    double cx = intrinsics.cx;
    double cy = intrinsics.cy;
    double width = intrinsics.width;
    double height = intrinsics.height;
    double fx = intrinsics.fx;
    double fy = intrinsics.fy;

    return gl::Camera::projectionMatrixForCameraIntrinsics(width, height, fx, fy, cx, cy, near, far);
  }
}

enum {
  BunnyIdx = 0,
  RafaIdx,
  SpotIdx,
  NefertitiIdx,
  DragonIdx,
  MortyIdx
};

void TimeStats::printStats() {
  LOGI("V_SENSE_DEBUG: Frames: %d", nbrFrames);
  for (int i = 0; i < NbrStats; i++)
    LOGI("V_SENSE_DEBUG: %d: %fms", i, timeMS[i]);
}

PointCloudApp::PointCloudApp() : screenWidth_(0.0f), screenHeight_(0.0f), lastColorTimestamp_(0.0), isServiceConnected_(false), saveFiles_(false), renderBaseColor_(true), missingFrames_(0),
                                 isGLInitialized_(false), recording_(false), isSceneCameraConfigured_(false), availableFlags_(0), displayRotation_(TangoSupportRotation::ROTATION_IGNORED) {
  objIdx_ = 0;
}

PointCloudApp::~PointCloudApp() {
  TangoConfig_free(tangoConfig_);
  TangoSupport_freePointCloudManager(pointCloudManager_);
  pointCloudManager_ = nullptr;
  TangoSupport_freeImageBufferManager(imageBufferManager_);
  imageBufferManager_ = nullptr;
}

void PointCloudApp::init(AAssetManager *assetManager) {
  assetManager_ = assetManager;
}

void PointCloudApp::onCreate(JNIEnv *env, jobject activity) {
  // Check the installed version of the TangoCore.  If it is too old, then it will not support the most up to date features.
  int version;
  TangoErrorType err = TangoSupport_GetTangoVersion(env, activity, &version);

  if (err != TANGO_SUCCESS || version < kTangoCoreMinimumVersion) {
    LOGE("PointToPointApplication::onCreate, Tango Core version is out of date.");
    std::terminate();
  }
}

void PointCloudApp::onTangoServiceConnected(JNIEnv *env, jobject binder) {
  TangoErrorType ret = TangoService_setBinder(env, binder);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication: Failed to initialize Tango service with error code: %d", ret);
    std::terminate();
  }

  tangoSetupConfig();
  tangoConnectCallbacks();
  tangoConnect();
  isServiceConnected_ = true;
}

void PointCloudApp::onPointCloudAvailable(const TangoPointCloud *point_cloud) {
  if(!newFrameMutex_.try_lock()) {
    TangoSupport_updatePointCloud(pointCloudManager_, NULL);
    return;
  }

  TangoSupport_updatePointCloud(pointCloudManager_, point_cloud);

  missingFrames_++;
  availableFlags_ |= HasPointCloud;

  if(missingFrames_ > MaxMissingFrames) {
    LOGE("Missing frames: %d", missingFrames_);
  }

  newFrameMutex_.unlock();
}

void PointCloudApp::onFrameAvailable(const TangoImageBuffer *buffer) {
  if(!newFrameMutex_.try_lock()) {
    TangoSupport_updateImageBuffer(imageBufferManager_, NULL);
    return;
  }

  TangoSupport_updateImageBuffer(imageBufferManager_, buffer);

  missingFrames_ = 0;
  availableFlags_ |= HasImage;

  newFrameMutex_.unlock();
}

void PointCloudApp::tangoSetupConfig() {
  tangoConfig_ = TangoService_getConfig(TANGO_CONFIG_DEFAULT);
  if (tangoConfig_ == nullptr) {
    LOGE("PointToPointApplication::tangoSetupConfig, Unable to get tango config");
    std::terminate();
  }

  TangoErrorType ret;
  ret = TangoConfig_setBool(tangoConfig_, "config_enable_depth", true);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication::tangoSetupConfig, Failed to enable depth.");
    std::terminate();
  }

  ret = TangoConfig_setInt32(tangoConfig_, "config_depth_mode", TANGO_POINTCLOUD_XYZC);
  if (ret != TANGO_SUCCESS) {
    LOGE("Failed to set 'depth_mode' configuration flag with error code: %d", ret);
    std::terminate();
  }

  ret = TangoConfig_setBool(tangoConfig_, "config_enable_color_camera", true);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication::tangoSetupConfig, Failed to enable color camera.");
    std::terminate();
  }

  ret = TangoConfig_setBool(tangoConfig_, "config_enable_drift_correction", true);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication::tangoSetupConfig, Fail to enable drift correction mode");
    std::terminate();
  }

  ret = TangoConfig_setBool(tangoConfig_, "config_enable_low_latency_imu_integration", true);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication::tangoSetupConfig, Failed to enable low latency imu integration.");
    std::terminate();
  }
}

void PointCloudApp::tangoConnectCallbacks() {
  if (pointCloudManager_ == nullptr) {
    int32_t max_point_cloud_elements;
    TangoErrorType ret = TangoConfig_getInt32(tangoConfig_, "max_point_cloud_elements", &max_point_cloud_elements);
    if (ret != TANGO_SUCCESS) {
      LOGE("PointToPointApplication::tangoConnectCallbacks, Failed to query maximum number of point cloud elements.");
      std::terminate();
    }

    ret = TangoSupport_createPointCloudManager(max_point_cloud_elements, &pointCloudManager_);
    if (ret != TANGO_SUCCESS) {
      LOGE("Failed to create point cloud manager.");
      std::terminate();
    }
  }

  TangoErrorType ret = TangoService_connectOnPointCloudAvailable(onPointCloudAvailableRouter);
  if (ret != TANGO_SUCCESS) {
    LOGE("Failed to connected to depth callback.");
    std::terminate();
  }

  // Connect to color camera.
  ret = TangoService_connectOnTextureAvailable(TANGO_CAMERA_COLOR, this, nullptr);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication: Failed to initialize the video overlay");
    std::terminate();
  }
}

void PointCloudApp::tangoConnect() {
  TangoErrorType ret = TangoService_connect(this, tangoConfig_);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication: Failed to connect to the Tango service.");
    std::terminate();
  }

  ret = TangoService_getCameraIntrinsics(TANGO_CAMERA_DEPTH, &depthCameraIntrinsics_);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication: Failed to get the intrinsics for the depth camera.");
    std::terminate();
  }

  ret = TangoService_getCameraIntrinsics(TANGO_CAMERA_COLOR, &colorCameraIntrinsics_);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication: Failed to get the intrinsics for the color camera.");
    std::terminate();
  }

  ret = TangoService_connectOnFrameAvailable(TANGO_CAMERA_COLOR, this, onFrameAvailableRouter);
  if (ret != TANGO_SUCCESS) {
    LOGE("PointToPointApplication: Error connecting color frame %d", ret);
    std::terminate();
  }

  TangoSupport_initializeLibrary();

  if (!imageBufferManager_) {
    ret = TangoSupport_createImageBufferManager(TANGO_HAL_PIXEL_FORMAT_YCrCb_420_SP, colorCameraIntrinsics_.width, colorCameraIntrinsics_.height, &imageBufferManager_);

    if (ret != TANGO_SUCCESS) {
      LOGE("PointToPointApplication: Failed to create image buffer manager");
      std::terminate();
    }
  }
}

void PointCloudApp::onPause() {
  isServiceConnected_ = false;
  isGLInitialized_ = false;
  tangoDisconnect();
  deleteResources();
}

void PointCloudApp::tangoDisconnect() {
  TangoService_disconnect();
}

void PointCloudApp::onSurfaceCreated(AAssetManager* assetManager) {
  videoOverlay_.reset(new gl::VideoOverlay(assetManager));
  videoOverlay_->setDisplayRotation(displayRotation_);

  emOverlay_.reset(new gl::EnvironmentMapOverlay(assetManager));
  emOverlay_->setDisplayRotation(displayRotation_);

  camera_.reset(new gl::Camera());

  gizmoObject_.reset(new gl::GizmoObject(assetManager));

  std::shared_ptr<gl::StaticMesh> virtualMesh(new gl::StaticMesh);
  std::string basenameStr;
  float scale = 1.f;
  maxMeshOrder_ = 4;

  if(objIdx_ == BunnyIdx)
    basenameStr = BunnyBasename;
  else if(objIdx_ == RafaIdx)
    basenameStr = RafaBasename;
  else if(objIdx_ == SpotIdx)
    basenameStr = SpotBasename;
  else if(objIdx_ == NefertitiIdx)
    basenameStr = NefertitiBasename;
  else if(objIdx_ == DragonIdx)
    basenameStr = DragonBasename;
  else {
    basenameStr = MortyBasename;
    scale = 0.02f;
    maxMeshOrder_ = 4;
  }

  io::ObjReader::loadFromFile(MeshFolder + basenameStr + ".obj", virtualMesh, scale);
  virtualObject_.reset(new gl::SHMeshDotObject(assetManager_, virtualMesh));
  virtualObject_->transform(glm::vec3(0.f, 0.f, 0.f), glm::quat());
  virtualObject_->updateCoefficients(SphericalHarmonicsFolder + basenameStr + MeshSOFile);
  virtualObject_->updateRenderCoefficientsNumber(nbrSHNbrCoeffs_);

  if(objIdx_ == MortyIdx) {
    std::shared_ptr<gl::Texture> meshTexture;
    meshTexture.reset(new gl::Texture(MeshFolder + MortyBasename + ".bin"));

    virtualObject_->setDiffuseTexture(meshTexture);
  }

  if(objIdx_ == RafaIdx)
    virtualObject_->updateBaseColor(glm::vec3(0.404f, 0.094f, 0.028f));
  else if(objIdx_ == MortyIdx)
    virtualObject_->updateBaseColor(glm::vec3(0.f, 0.f, 0.f));
  else
    virtualObject_->updateBaseColor(glm::vec3(0.1f, 0.1f, 0.1f));

  virtualPlaneObject_.reset(new gl::SHMeshPlaneDotObject(assetManager_, 0.02f));
  virtualPlaneObject_->transform(glm::vec3(0.f, 0.f, 0.f), glm::quat());
  virtualPlaneObject_->updateCoefficients(SphericalHarmonicsFolder + basenameStr + ShadowFile, SphericalHarmonicsFolder + basenameStr + PlaneFile);
  virtualPlaneObject_->updateRenderCoefficientsNumber(nbrSHNbrCoeffs_);

  emProcess_.reset(new em::Process(assetManager));
  emProcess_->updateMaxError(maxMSE_);
  emProcess_->setMaxSHOrder(std::min(maxSHOrder_, maxMeshOrder_));

  emOverlay_->updateTexture(emProcess_->getEnvironmentMap());

  gizmoObject_->setVisibility(false);

  isGLInitialized_ = true;
}

void PointCloudApp::onSurfaceChanged(int width, int height) {
  screenWidth_ = static_cast<float>(width);
  screenHeight_ = static_cast<float>(height);
  isSceneCameraConfigured_ = false;
}

void PointCloudApp::setViewportAndProjectionGLThread() {
  if (!isGLInitialized_ || !isServiceConnected_|| !videoOverlay_)
    return;

  videoOverlay_->setTextureOffset(screenWidth_, screenHeight_, static_cast<float>(colorCameraIntrinsics_.width), static_cast<float>(colorCameraIntrinsics_.height));
  emOverlay_->setTextureOffset(screenWidth_, screenHeight_, static_cast<float>(colorCameraIntrinsics_.width), static_cast<float>(colorCameraIntrinsics_.height));

  glViewport(0, 0, screenWidth_, screenHeight_);

  constexpr float kNearPlane = 0.1f;
  constexpr float kFarPlane = 100.0f;
  projectionMtxAR_ = projectionMatrixForCameraIntrinsics(colorCameraIntrinsics_, kNearPlane, kFarPlane);
}

void PointCloudApp::onUpdateRenderData() {
  std::lock_guard<std::mutex> lock(newFrameMutex_);

  if (isAnimated_) {
    glm::vec3 pos(AnimationRadius, 0.f, 0.f);
    if (virtualObjPos_ == virtualObjAnimPos_) {
      pos = virtualObjRotationMtx_ * pos + virtualObjPos_;
      curAnimationAngle_ = 0.f;
      lastCalculatedSHAngle_ = curAnimationAngle_ - 10.f;
    }else {
      glm::quat animationRot = glm::angleAxis(glm::radians(curAnimationAngle_), glm::vec3(0.f, 1.f, 0.f));
      pos = animationRot*pos;
      pos = virtualObjRotationMtx_ * pos + virtualObjPos_;

      curAnimationAngle_ -= 1.f;
      if(curAnimationAngle_ < 0.f)
        curAnimationAngle_ = 360.f;
    }

    if (std::abs(lastCalculatedSHAngle_ - curAnimationAngle_) >= LimitSHCalculation) {
      lastCalculatedSHAngle_ = curAnimationAngle_;


      emProcess_->updateEMOrigin(pos + glm::vec3(0.f, 0.05f, 0.f), false);
      emProcess_->translateEM();
      emProcess_->updateSHCoefficients();

      emOverlay_->updateTexture(emProcess_->getEnvironmentMapAlt());

      virtualObject_->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
      virtualPlaneObject_->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
    }

    virtualObjAnimPos_ = pos;
    virtualObject_->transform(virtualObjAnimPos_, glm::quat());
    virtualPlaneObject_->transform(virtualObjAnimPos_, glm::quat());
  } else if(emProcess_->needsTranslateEM()) {
    emProcess_->translateEM();
    emProcess_->updateSHCoefficients();
    emOverlay_->updateTexture(emProcess_->getEnvironmentMap());

    virtualObject_->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
    virtualPlaneObject_->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
  }

  if (availableFlags_ == HasBoth)
    addCurrentFrame();
}

void PointCloudApp::onDrawFrame() {
  if (!isGLInitialized_ || !isServiceConnected_) {
    LOGE("VSENSE_AR_DEBUG_C: Not initialized or service not connected.");
    return;
  }

  if (!isSceneCameraConfigured_) {
    setViewportAndProjectionGLThread();
    isSceneCameraConfigured_ = true;
  }

  // Update the texture associated with the color camera.
  double lastGPUTimestamp;
  if (TangoService_updateTextureExternalOes(TANGO_CAMERA_COLOR, videoOverlay_->getTextureId(), &lastGPUTimestamp) != TANGO_SUCCESS) {
    LOGE("VSENSE_AR_DEBUG_C: Failed to get a color image.");
    return;
  }

  TangoMatrixTransformData matrixTransform;
  TangoSupport_getMatrixTransformAtTime(lastGPUTimestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                                        TANGO_COORDINATE_FRAME_CAMERA_COLOR, TANGO_SUPPORT_ENGINE_OPENGL,
                                        TANGO_SUPPORT_ENGINE_OPENGL, displayRotation_, &matrixTransform);

  if (matrixTransform.status_code != TANGO_POSE_VALID) {
    LOGE("VSENSE_AR_DEBUG_C: Failed to get a transformation.");
    return;
  } else {
    const glm::mat4 frame_area_desc_T_color_camera = glm::make_mat4(matrixTransform.matrix);

    renderGL(frame_area_desc_T_color_camera);
  }
}

void PointCloudApp::renderGL(const glm::mat4 &frame_area_desc_T_color_camera) {
  glEnable(GL_CULL_FACE);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  if(videoOverlay_)
    videoOverlay_->render(glm::mat4(1.0), glm::mat4(1.0));
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  setupCamera(projectionMtxAR_, frame_area_desc_T_color_camera);

  if (gizmoObject_)
    gizmoObject_->render(camera_.get());

  if (virtualObject_->isVisible())
    virtualObject_->render(camera_.get());
  if (virtualPlaneObject_)
    virtualPlaneObject_->render(camera_.get());

  if (emOverlay_) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    emOverlay_->render(glm::mat4(1.0), glm::mat4(1.0));
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
  }
}

void PointCloudApp::deleteResources() {
  //envMap_.reset();
}


void PointCloudApp::setupCamera(const glm::mat4& projection_matrix, const glm::mat4& transformation_matrix) {
  if (camera_) {
    camera_->setProjectionMatrix(projection_matrix);
    camera_->setTransformationMatrix(transformation_matrix);
  }
}

void PointCloudApp::onDisplayChanged(int display_rotation) {
  displayRotation_ = static_cast<TangoSupportRotation>(display_rotation);
  isSceneCameraConfigured_ = false;
}

void PointCloudApp::addCurrentFrame() {
  availableFlags_ = 0;

  if (!isGLInitialized_ || !isServiceConnected_)
    return;

  // Get the latest point cloud
  TangoPointCloud* pointCloud = nullptr;
  TangoPoseData posePC;
  TangoErrorType ret = TangoSupport_getLatestPointCloudWithPose(pointCloudManager_, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION, TANGO_SUPPORT_ENGINE_OPENGL, TANGO_SUPPORT_ENGINE_TANGO, ROTATION_IGNORED, &pointCloud, &posePC);

  if (ret != TANGO_SUCCESS || pointCloud == nullptr)
    return;
  if (posePC.status_code != TANGO_POSE_VALID)
    return;

  // Get the latest color image
  TangoImageBuffer* imgBuffer = nullptr;
  TangoPoseData poseIM;
  TangoSupport_getLatestImageBuffer(imageBufferManager_, &imgBuffer);
  ret = TangoSupport_calculateRelativePose(imgBuffer->timestamp, TANGO_COORDINATE_FRAME_CAMERA_COLOR, pointCloud->timestamp, TANGO_COORDINATE_FRAME_CAMERA_DEPTH, &poseIM);
  if(ret != TANGO_SUCCESS)
    return;

  lastColorTimestamp_ = imgBuffer->timestamp;

  if(saveFiles_ && recording_)
    saveFiles(pointCloud, posePC, imgBuffer, poseIM);

  clock_t t = clock();

  t = clock() - t;
  emProcess_->addFrame(pointCloud, &posePC, &depthCameraIntrinsics_, imgBuffer, &poseIM, &colorCameraIntrinsics_, minConfidence_, recording_, !isAnimated_);
  perf_[Perf_EM] = (float)t/CLOCKS_PER_SEC;
  perf_[Perf_MSE] = emProcess_->getLastCorrectionMatrixError();

  virtualObject_->updateColorCorrectionMtx(emProcess_->getLastInvCorrectionMatrix());
  virtualPlaneObject_->updateColorCorrectionMtx(emProcess_->getLastInvCorrectionMatrix());

  if(!isAnimated_) {
    if(recording_)
      emOverlay_->updateTexture(emProcess_->getEnvironmentMap());

    virtualObject_->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
    virtualPlaneObject_->updateAmbientCoefficients(emProcess_->getSHCoefficientsTexture());
  }
}

void PointCloudApp::saveFiles(const TangoPointCloud* pointCloud, const TangoPoseData& posePC, const TangoImageBuffer* image, const TangoPoseData& poseIM) {
  unsigned int curFrame = status_.incrementFrameCount() - 1;
  char buffer[20];
  sprintf(buffer, "%d", curFrame);
  std::string frameStr = std::string(buffer);

  // Save point cloud
  std::ofstream pointCloudFile;
  pointCloudFile.open(curSaveFolder_ + "/PointCloud" + frameStr + ".pc");
  pointCloudFile.write((const char*)&depthCameraIntrinsics_.width, sizeof(uint32_t));
  pointCloudFile.write((const char*)&depthCameraIntrinsics_.height, sizeof(uint32_t));
  pointCloudFile.write((const char*)&depthCameraIntrinsics_.fx, sizeof(double));
  pointCloudFile.write((const char*)&depthCameraIntrinsics_.fy, sizeof(double));
  pointCloudFile.write((const char*)&depthCameraIntrinsics_.cx, sizeof(double));
  pointCloudFile.write((const char*)&depthCameraIntrinsics_.cy, sizeof(double));
  pointCloudFile.write((const char*)&depthCameraIntrinsics_.distortion, sizeof(double)*5);
  pointCloudFile.write((const char*)&pointCloud->num_points, sizeof(uint32_t));
  pointCloudFile.write((const char*)&pointCloud->timestamp, sizeof(double));
  pointCloudFile.write((const char*)posePC.translation, sizeof(double)*3);
  pointCloudFile.write((const char*)posePC.orientation, sizeof(double)*4);
  pointCloudFile.write((const char*)&posePC.accuracy, sizeof(float));
  pointCloudFile.write((const char*)pointCloud->points, sizeof(float)*4*pointCloud->num_points);
  pointCloudFile.close();

  // Save image
  int nbrPixels = image->stride*image->height;
  std::ofstream imageFile;
  imageFile.open(curSaveFolder_ + "/PointCloud" + frameStr + ".im");
  imageFile.write((const char*)&image->width, sizeof(uint32_t));
  imageFile.write((const char*)&image->height, sizeof(uint32_t));
  imageFile.write((const char*)&image->exposure_duration_ns, sizeof(int64_t));
  imageFile.write((const char*)&image->timestamp, sizeof(double));
  imageFile.write((const char*)&colorCameraIntrinsics_.fx, sizeof(double));
  imageFile.write((const char*)&colorCameraIntrinsics_.fy, sizeof(double));
  imageFile.write((const char*)&colorCameraIntrinsics_.cx, sizeof(double));
  imageFile.write((const char*)&colorCameraIntrinsics_.cy, sizeof(double));
  imageFile.write((const char*)&colorCameraIntrinsics_.distortion, sizeof(double)*5);
  imageFile.write((const char*)poseIM.translation, sizeof(double)*3);
  imageFile.write((const char*)poseIM.orientation, sizeof(double)*4);
  imageFile.write((const char*)&poseIM.accuracy, sizeof(float));
  imageFile.write((const char*)image->data, sizeof(unsigned char)*(nbrPixels + nbrPixels/2));
  imageFile.close();
}

void PointCloudApp::onTouchEvent(float x, float y) {
  if(!isGLInitialized_ || !isServiceConnected_)
    return;

  //std::lock_guard<std::mutex> lock(newFrameMutex_);

  // Get the latest point cloud
  TangoPointCloud* pointCloud = nullptr;
  TangoSupport_getLatestPointCloud(pointCloudManager_, &pointCloud);
  if (pointCloud == nullptr)
    return;


  // Get the pose transforms from depth and color camera to opengl frame.
  TangoPoseData posePointCloud, poseColorCamera;
  TangoErrorType ret;

  ret = TangoSupport_getPoseAtTime(pointCloud->timestamp, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                                   TANGO_COORDINATE_FRAME_CAMERA_DEPTH, TANGO_SUPPORT_ENGINE_OPENGL,
                                   TANGO_SUPPORT_ENGINE_TANGO, ROTATION_IGNORED, &posePointCloud);
  if (ret != TANGO_SUCCESS) {
    LOGE("%s: could not get openglTdepth pose for timestamp %f.", __func__, pointCloud->timestamp);
    return;
  }
  ret = TangoSupport_getPoseAtTime(lastColorTimestamp_, TANGO_COORDINATE_FRAME_AREA_DESCRIPTION,
                                   TANGO_COORDINATE_FRAME_CAMERA_COLOR, TANGO_SUPPORT_ENGINE_OPENGL,
                                   TANGO_SUPPORT_ENGINE_TANGO, ROTATION_IGNORED, &poseColorCamera);
  if (ret != TANGO_SUCCESS) {
    LOGE("%s: could not get openglTcolor pose for last_gpu_timestamp_ %f.", __func__, lastColorTimestamp_);
    return;
  }

  // Touch location in [0,1] range.
  glm::vec2 uv(x / screenWidth_, y / screenHeight_);

  // Fit plane model.
  TangoSupportPlane* planes;
  int nbrPlanes;
  if (TangoSupport_fitMultiplePlaneModelsNearPoint(pointCloud, posePointCloud.translation,
                                                   posePointCloud.orientation, glm::value_ptr(uv), displayRotation_,
                                                   poseColorCamera.translation, poseColorCamera.orientation, &planes, &nbrPlanes) != TANGO_SUCCESS) {
    return;
  }

  // Cast down to float for other functions.
  const glm::vec3 depthPosition = glm::vec3(planes[0].intersection_point[0],
                                            planes[0].intersection_point[1],
                                            planes[0].intersection_point[2]);

  // Use world up as the second vector unless they are nearly parallel, in
  // which case use world +Z.
  const glm::vec3 planeNormal(planes[0].plane_equation[0],
                              planes[0].plane_equation[1],
                              planes[0].plane_equation[2]);

  // Free the planes returned by the plane fit call
  TangoSupport_freePlaneList(&planes);

  glm::vec3 normalX = glm::vec3(1.f, 0.f, 0.f);  // also world up vector

  const float worldUpThreshold = 0.5f;
  if (glm::dot(planeNormal, normalX) > worldUpThreshold)
    normalX = glm::vec3(0.0f, 0.0f, 1.0f);

  const glm::vec3 normalZ = glm::normalize(glm::cross(planeNormal, normalX));
  normalX = glm::normalize(glm::cross(normalZ, planeNormal));

  // Set position/rotation of cube visualization.
  virtualObjRotationMtx_[0] = normalX;
  virtualObjRotationMtx_[1] = planeNormal;
  virtualObjRotationMtx_[2] = normalZ;

  glm::mat3 tmp;
  {
    glm::vec3 normalY = glm::vec3(0.f, 1.f, 0.f);  // also world up vector

    const float worldUpThreshold = 0.5f;
    if (glm::dot(planeNormal, normalY) > worldUpThreshold)
      normalY = glm::vec3(0.0f, 0.0f, 1.0f);

    const glm::vec3 normalZ = glm::normalize(glm::cross(planeNormal, normalY));
    normalY = glm::normalize(glm::cross(normalZ, planeNormal));

    // Set position/rotation of cube visualization.
    tmp[0] = planeNormal;
    tmp[1] = normalY;
    tmp[2] = normalZ;
  }

  glm::vec3 pos = depthPosition + planeNormal*ObjectOffset;
  emProcess_->updateEMOrigin(pos + glm::vec3(0.f, 0.05f, 0.f), true);
  virtualObject_->transform(pos, glm::quat());
  virtualPlaneObject_->transform(pos, glm::quat());

  LOGI("VSENSE_DEBUG Pos: %f, %f, %f", pos.x, pos.y, pos.z);
}

void PointCloudApp::onStartRecord() {
  recording_ = true;
}

void PointCloudApp::onPauseRecord() {
  recording_ = false;

  /*if(emProcess_)
    emProcess_->printStats();*/
}

void PointCloudApp::onDeleteRecord() {
  status_.updateFrameCount(0);
  sphImage_.reset();
}

void PointCloudApp::onUpdateSave(bool save) {
  if(save) {
    int curFolderNbr = 0;
    std::string curFolder;
    struct stat info;

    char buffer[20];

    while(true) {
      sprintf(buffer, "%d", curFolderNbr);
      std::string curFolderNbrStr = std::string(buffer);
      curFolder = DataFolder + curFolderNbrStr;

      if(stat(curFolder.c_str(), &info) == -1)
        break;
      else
        curFolderNbr++;
    }

    mkdir(curFolder.c_str(), 0770);

    curSaveFolder_ = curFolder;
  }

  saveFiles_ = save;
}

void PointCloudApp::onUpdateColorCorrection(bool colorCorrection) {
  if(virtualObject_)
    virtualObject_->updateColorCorrection(colorCorrection);
  if(virtualPlaneObject_)
    virtualPlaneObject_->updateColorCorrection(colorCorrection);
  if(emProcess_)
    emProcess_->updateDoColorCorrection(colorCorrection);
}

void PointCloudApp::onUpdateAnimation(bool animate) {
  isAnimated_ = animate;

  if(isAnimated_)
    virtualObjAnimPos_ = virtualObjPos_ = virtualObject_->getTranslation();
  else {
    virtualObject_->transform(virtualObjPos_, glm::quat());
    virtualPlaneObject_->transform(virtualObjPos_, glm::quat());

    emOverlay_->updateTexture(emProcess_->getEnvironmentMap());
  }
}

void PointCloudApp::onUpdateGizmo(bool vis) {
  //gizmoObject_->setVisibility(vis);
  if(emOverlay_)
    emOverlay_->setVisibility(vis);
}

void PointCloudApp::onUpdateBunny(bool vis) {
  if(virtualObject_)
    virtualObject_->setVisibility(vis);
}

void PointCloudApp::onUpdateShadow(bool vis) {
  if(virtualPlaneObject_)
    virtualPlaneObject_->setVisibility(vis);
}

void PointCloudApp::onUpdateVideo(bool vis) {
  if(videoOverlay_)
    videoOverlay_->setVisibility(vis);
}

std::string PointCloudApp::getStateString() {
  std::string status;

  std::ostringstream ss;
  ss.precision(3);
  if(perf_.find(Perf_EM) != perf_.end())
    ss << Perf_EM << ": " << perf_[Perf_EM] << std::endl;
  if(perf_.find(Perf_MSE) != perf_.end())
    ss << Perf_MSE << ": " << perf_[Perf_MSE] << std::endl;

  if(emProcess_) {
    glm::vec3 emOrigin = emProcess_->getOrigin();
    ss << "EM Origin: " << emOrigin.x << ", " << emOrigin.y << ", " << emOrigin.z << std::endl;
  }

  status += ss.str();

  return status;
}

void PointCloudApp::onStopProcess() {
}

void PointCloudApp::updateTexture() {
  if (!emOverlay_) {
    status_.updateState(common::Status::Idle);
    return;
  }

  status_.updateState(common::Status::Idle);
}

void PointCloudApp::onSetDepthMapConfidence(float confidence) {
  minConfidence_ = confidence;
}

void PointCloudApp::onSetDepthMapEnableFillHoles(bool enable) {
  enableFillHoles_ = enable;
  depth::DepthMap::setEnableFillHoles(enable);
}

void PointCloudApp::onSetDepthMapEnableFillHolesWithMax(bool enable) {
  depth::DepthMap::setEnableFillWithMax(enable);
}

void PointCloudApp::onSetColorCorrectionEnable(bool enable) {
  em::EnvironmentMap::setColorCorrectionEnabled(enable);
}

void PointCloudApp::onSetColorCorrectionMaxError(float error) {
  maxMSE_ = error;
  em::EnvironmentMap::setColorCorrectionMaxError(error);
}

void PointCloudApp::onSetColorCorrectionMinPoints(int pointsNbr) {
  em::EnvironmentMap::setColorCorrectionMinPoints(pointsNbr);
}

void PointCloudApp::onSetRenderNumberSamples(int samplesNbr) {
  nbrSHSamples_ = samplesNbr;
}

void PointCloudApp::onSetRenderNumberOrder(int orderNbr) {
  maxSHOrder_ = orderNbr;
  nbrSHNbrCoeffs_ = (orderNbr + 1)*(orderNbr + 1);

  if(emProcess_)
    emProcess_->setMaxSHOrder(std::min(maxSHOrder_, maxMeshOrder_));
  if(virtualObject_)
    virtualObject_->updateRenderCoefficientsNumber(nbrSHNbrCoeffs_);
  if(virtualPlaneObject_)
    virtualPlaneObject_->updateRenderCoefficientsNumber(nbrSHNbrCoeffs_);
}

void PointCloudApp::onSetRenderObject(int objIdx) {
  objIdx_ = objIdx;
}

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

#ifndef VSENSE_AR_POINTCLOUDAPP_H_
#define VSENSE_AR_POINTCLOUDAPP_H_

#include <atomic>
#include <jni.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <time.h>

#include <tango_client_api.h>
#include <tango_support_api.h>

#include <vsense/gl/Util.h>
#include <vsense/common/Status.h>

struct AAssetManager;

namespace vsense {

namespace em {
  //class EnvironmentMap;
  class Process;
}

namespace io {
  class Image;
}

namespace tree{
  class PointCloudOctree;
}

namespace gl {
  class Camera;
  class GizmoObject;
  class PointCloudObject;
  class MeshObject;
  class SHMeshObject;
  class SHMeshDotObject;
  class SHMeshPlaneDotObject;
  class VideoOverlay;
  class EnvironmentMapOverlay;
}

namespace depth {
  class DepthMap;
}

namespace ar {

const int NbrStats = 5;

struct TimeStats {
  TimeStats() {
    for (int i = 0; i < NbrStats; i++)
      timeMS[i] = 0.0;

    nbrFrames = 0;
  }

  void addTimeStats(const TimeStats& ts) {
    for (int i = 0; i < NbrStats; i++)
      timeMS[i] *= nbrFrames;

    nbrFrames++;
    for (int i = 0; i < NbrStats; i++) {
      timeMS[i] += ts[i];
      timeMS[i] /= nbrFrames;
    }
  }

  void start(int idx) {
    t[idx] = clock();
  }

  void stop(int idx) {
    t[idx] = clock() - t[idx];
    timeMS[idx] = 1000.0 * (double)t[idx] / CLOCKS_PER_SEC;
  }

  const double& operator[](int idx) const {
    return timeMS[idx];
  }

  void printStats();

  clock_t  t[NbrStats];
  double   timeMS[NbrStats];
  uint32_t nbrFrames;
};

typedef std::map<std::string, float> Performances;

struct SavedPose {
  SavedPose(const double* o, const double* t) {
    memcpy(orientation, o, sizeof(double)*4);
    memcpy(translation, t, sizeof(double)*3);
  }

  double orientation[4];
  double translation[3];
};

/**
 * This class is the main application for PointToPoint. It can be instantiated
 * in the JNI layer and use to pass information back and forth between Java. The
 * class also manages the application's lifecycle and interaction with the Tango
 * service. Primarily, this involves registering for callbacks and passing on
 * the necessary information to stored objects.
 */
class PointCloudApp {
public:
  PointCloudApp();

  ~PointCloudApp();

  /**
   * Initialize the application.
   * @param assetManager Pointer to the asset manager.
   */
  void init(AAssetManager* assetManager);

  /**
   * Callback called when the Android application OnCreate function is called from
   * the UI thread. Tango Core's version is checked here.
   * @param env Pointer to the JNI context of the native activity.
   * @param caller_activity Native activity object handle of the calling native activity.
   */
  void onCreate(JNIEnv *env, jobject caller_activity);

  /**
   * Callback called when the Android application OnPause function is
   * called from the UI thread. The Tango Service is disconnected and
   * the configuration file is released. If not done, other application
   * won't be able to connect to the Tango service.
   */
  void onPause();

  /**
   * Callback called when the Tango service has connected successfully.
   * @param env Pointer to the JNI context of the native activity.
   * @param binder Binder object received after binding to the Tango service.
   */
  void onTangoServiceConnected(JNIEnv *env, jobject binder);

  /**
   * Creates the OpenGL state and connect to the color camera texture.
   */
  void onSurfaceCreated(AAssetManager* assetManager);

  /**
   * Configures the viewport of the GL view.
   * @param width Viewport's width.
   * @param height Viweport's height.
   */
  void onSurfaceChanged(int width, int height);

  void onUpdateRenderData();

  /**
   * Gets the current position of the camera and renders it.
   */
  void onDrawFrame();

  /**
   * Callback called whenever a point cloud is available.
   * @param point_cloud Pointer to the structure containing the point cloud.
   */
  void onPointCloudAvailable(const TangoPointCloud *point_cloud);

  /**
   * Callback called whenever a color image is available.
   * @param buffer Pointer to the image buffer.
   */
  void onFrameAvailable(const TangoImageBuffer *buffer);

  /**
   * Callback called whenever there is a display change event. It's used to detect
   * the screen orientation.
   * @param display_rotation Rotation index of the display. Same as the Android display enum value.
   */
  void onDisplayChanged(int display_rotation);

  /**
   * Callback called whenever a touch event is triggered.
   * @param x Requested x coordinate in screen space.
   * @param y Requested y coordinate in screen space.
   */
  void onTouchEvent(float x, float y);

  std::string getStateString();

  /*
   * Pauses the frame acquisition.
   */
  void onPauseRecord();

  /*
   * Starts the frame acquisition.
   */
  void onStartRecord();

  /*
   * Deletes saved frames.
   */
  void onDeleteRecord();

  void onUpdateSave(bool save);

  void onUpdateColorCorrection(bool colorCorrection);

  void onUpdateAnimation(bool animate);

  void onUpdateGizmo(bool vis);

  void onUpdateVideo(bool vis);

  void onUpdateBunny(bool vis);

  void onUpdateShadow(bool vis);

  void onStopProcess();

  void onSetSaving(bool saving) { saveFiles_ = saving; }

  void onSetDepthMapConfidence(float confidence);

  void onSetDepthMapEnableFillHoles(bool enable);

  void onSetDepthMapEnableFillHolesWithMax(bool enable);

  void onSetOctreeResolution(float resolution);

  void onSetOctreeMaxDepth(int depth);

  void onSetOctreePointsPerLeaf(int pointsNbr);

  void onSetOctreeEnableDynamic(bool enable);

  void onSetColorCorrectionEnable(bool enable);

  void onSetColorCorrectionMaxError(float error);

  void onSetColorCorrectionMinPoints(int pointsNbr);

  void onSetRenderNumberSamples(int samplesNbr);

  void onSetRenderNumberOrder(int orderNbr);

  void onSetRenderObject(int objIdx);

private:
  /**
   * Rendering process after the transforms are determined.
   * @param frame_area_desc_T_color_camera Matrix transformation from the ADF to the color camera.
   */
  void renderGL(const glm::mat4 &frame_area_desc_T_color_camera);

  /**
   * Sets up the configuration file for the Tango service.
   */
  void tangoSetupConfig();

  /**
   * Connect the OnTextureAvailable and OnTangoEvent callbacks.
   */
  void tangoConnectCallbacks();

  /**
   * Connects to Tango service and starts the Motion Tracking.
   */
  void tangoConnect();

  /**
   * Disconnect from the Tango service.
   */
  void tangoDisconnect();

  /**
   * Delete the allocated resources.
   */
  void deleteResources();

  /**
   * Sets the viewport and projection matrix.
   */
  void setViewportAndProjectionGLThread();

  void setupCamera(const glm::mat4& projection_matrix, const glm::mat4& transformation_matrix);

  void addCurrentFrame();

  /**
   * Saves the point cloud and color image.
   */
  void saveFiles(const TangoPointCloud* pointCloud, const TangoPoseData& posePC, const TangoImageBuffer* image, const TangoPoseData& poseIM);

  void updateTexture();

  AAssetManager*    assetManager_; /*!< Pointer to the asset manager in the project. */

  int nbrSHSamples_; /*!< Number of samples used to calculate the SH coefficients. */
  int nbrSHNbrCoeffs_; /*!< Number of order used to calculate the SH coefficients. */
  int maxSHOrder_;
  int maxMeshOrder_;
  bool renderBaseColor_; /*!< True if base color of virtual object is to be rendered. */
  float minConfidence_; /*!< Minimum confidence allowed to be used as trusted in a depth map. */

  common::Status status_;       /*!< Current process status. */

  std::mutex newFrameMutex_;

  //std::mutex renderMutex_;

  float screenWidth_;                    /*!< Width of the render window. */
  float screenHeight_;                   /*!< Height of the render window. */
  TangoSupportRotation displayRotation_; /*!< Orientation  used for handling display rotation. */
  glm::mat4   projectionMtxAR_;          /*!< Cached transforms OpenGL projection matrix. */
  std::shared_ptr<gl::Camera> camera_;   /*!< Camera object used to render virtual content. */

  // Renderable objects
  std::shared_ptr<gl::VideoOverlay>          videoOverlay_;    /*!< Overlay showing the live-stream video. */
  std::shared_ptr<gl::EnvironmentMapOverlay> emOverlay_;
  std::shared_ptr<gl::GizmoObject>           gizmoObject_;

  std::shared_ptr<gl::SHMeshDotObject>       virtualObject_;
  std::shared_ptr<gl::SHMeshPlaneDotObject>  virtualPlaneObject_;

  std::shared_ptr<io::Image>            sphImage_;

  //std::shared_ptr<em::EnvironmentMap>   envMap_;
  std::shared_ptr<em::Process> emProcess_;

  std::shared_ptr<depth::DepthMap>      dm_;

  TangoSupportPointCloudManager*  pointCloudManager_;  /*!< Point cloud data manager. */
  TangoSupportImageBufferManager* imageBufferManager_; /*!< Image (color) data manager */

  std::atomic<bool> isAnimated_;
  std::atomic<bool> isServiceConnected_;
  std::atomic<bool> isGLInitialized_;
  std::atomic<bool> isSceneCameraConfigured_;
  std::atomic<bool> saveFiles_;                /*!< Flag to indicate if files are to be save. */
  std::atomic<bool> recording_;                /*!< Allowed values: -1 (continuous recording), 0 (stop recording), 1 (single shot) */

  std::string curSaveFolder_;

  int objIdx_;

  float maxMSE_;

  bool  enableFillHoles_;

  TangoConfig           tangoConfig_;             /*!< Configuration object for Tango. */
  TangoCameraIntrinsics colorCameraIntrinsics_;   /*!< Intrinsics for the color camera. */
  TangoCameraIntrinsics depthCameraIntrinsics_;   /*!< Intrinsics for the depth camera. */

  std::atomic<unsigned char> availableFlags_;  /*!< Flags indicating the availability of a Point Cloud and its corresponding image */
  double                     lastColorTimestamp_;

  Performances               perf_;

  uint16_t                   missingFrames_;

  glm::mat3                  virtualObjRotationMtx_;
  glm::vec3                  virtualObjPos_;
  glm::vec3                  virtualObjAnimPos_;
  float                      curAnimationAngle_;
  float                      lastCalculatedSHAngle_;
};

} }

#endif

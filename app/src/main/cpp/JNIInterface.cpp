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

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <jni.h>

#include <thread>

#include "PointCloudApp.h"

static vsense::ar::PointCloudApp app;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_init(
  JNIEnv *env, jclass type, jobject assetManager) {

  (void)type;
  AAssetManager* nativeAssetManager = AAssetManager_fromJava(env, assetManager);
  app.init(nativeAssetManager);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onCreate(
    JNIEnv* env, jobject, jobject activity) {
  app.onCreate(env, activity);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onPause(
    JNIEnv*, jobject) {
  app.onPause();
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onTangoServiceConnected(
    JNIEnv* env, jobject /*obj*/, jobject binder) {
  app.onTangoServiceConnected(env, binder);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onGlSurfaceCreated(
  JNIEnv* env, jobject, jobject j_asset_manager) {
  AAssetManager* aasset_manager = AAssetManager_fromJava(env, j_asset_manager);
  app.onSurfaceCreated(aasset_manager);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onGlSurfaceChanged(
    JNIEnv* /*env*/, jobject /*obj*/, jint width, jint height) {
  app.onSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateRenderData(
  JNIEnv* /*env*/, jobject /*obj*/) {
  app.onUpdateRenderData();
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onGlSurfaceDrawFrame(
    JNIEnv* /*env*/, jobject /*obj*/) {
  app.onDrawFrame();
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onDisplayChanged(
    JNIEnv* /*env*/, jobject /*obj*/, jint display_rotation) {
  app.onDisplayChanged(display_rotation);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onTouchEvent(
  JNIEnv* /*env*/, jobject  /*obj*/, jfloat x, jfloat y) {

  app.onTouchEvent(x, y);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onPauseRecord (
  JNIEnv* /*env*/, jobject  /*obj*/) {
  app.onPauseRecord();
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onStartRecord (
  JNIEnv* /*env*/, jobject  /*obj*/) {
  app.onStartRecord();
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onDeleteRecord (
  JNIEnv* /*env*/, jobject  /*obj*/) {
  app.onDeleteRecord();
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateSave (
  JNIEnv* /*env*/, jobject  /*obj*/, jboolean save) {
  app.onUpdateSave(save);
}

JNIEXPORT jstring JNICALL
Java_vsense_ar_TangoJNINative_getStateString(
  JNIEnv* env, jobject) {
  return (env)->NewStringUTF(app.getStateString().c_str());
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateColorCorrection(
  JNIEnv* /*env*/, jobject  /*obj*/, jboolean colorCorrection) {
  app.onUpdateColorCorrection(colorCorrection);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateAnimation(
  JNIEnv* /*env*/, jobject  /*obj*/, jboolean animate) {
  app.onUpdateAnimation(animate);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateGizmo(
  JNIEnv* /*env*/, jobject  /*obj*/, jboolean vis) {
  app.onUpdateGizmo(vis);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateVideo(
  JNIEnv* /*env*/, jobject  /*obj*/, jboolean vis) {
  app.onUpdateVideo(vis);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateBunny(
  JNIEnv* /*env*/, jobject  /*obj*/, jboolean vis) {
  app.onUpdateBunny(vis);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onUpdateShadow(
  JNIEnv* /*env*/, jobject  /*obj*/, jboolean vis) {
  app.onUpdateShadow(vis);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetRenderObject(
  JNIEnv* /*env*/, jobject  /*obj*/, jint objectIdx) {
  app.onSetRenderObject(objectIdx);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onStopProcess(
  JNIEnv* /*env*/, jobject  /*obj*/) {

  std::thread t(&vsense::ar::PointCloudApp::onStopProcess, &app);
  //app.onStopProcess(x, y);

  t.join();
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetDepthMapConfidence(JNIEnv* /*env*/, jobject /*obj*/, jfloat confidence) {
  app.onSetDepthMapConfidence(confidence);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetDepthMapEnableFillHoles(JNIEnv* /*env*/, jobject  /*obj*/, jboolean enable) {
  app.onSetDepthMapEnableFillHoles(enable);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetDepthMapEnableFillHolesWithMax(JNIEnv* /*env*/, jobject  /*obj*/, jboolean enable) {
  app.onSetDepthMapEnableFillHolesWithMax(enable);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetColorCorrectionEnable(JNIEnv* /*env*/, jobject  /*obj*/, jboolean enable) {
  app.onSetColorCorrectionEnable(enable);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetColorCorrectionMaxError(JNIEnv* /*env*/, jobject /*obj*/, jfloat error) {
  app.onSetColorCorrectionMaxError(error);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetColorCorrectionMinPoints(JNIEnv* /*env*/, jobject /*obj*/, jint pointsNbr) {
  app.onSetColorCorrectionMinPoints(pointsNbr);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetRenderNumberSamples(JNIEnv* /*env*/, jobject /*obj*/, jint samplesNbr) {
  app.onSetRenderNumberSamples(samplesNbr);
}

JNIEXPORT void JNICALL
Java_vsense_ar_TangoJNINative_onSetRenderNumberOrder(JNIEnv* /*env*/, jobject /*obj*/, jint orderNbr) {
  app.onSetRenderNumberOrder(orderNbr);
}

#ifdef __cplusplus
}
#endif

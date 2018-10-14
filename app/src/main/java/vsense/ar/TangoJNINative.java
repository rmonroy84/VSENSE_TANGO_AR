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

package vsense.ar;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.IBinder;
import android.util.Log;

import vsense.ar.TangoInitializationHelper;

/**
 * Interfaces between native C++ code and Java code.
 */
public class TangoJNINative {
    static {
        // This project depends on tango_client_api, so we need to make sure we load
        // the correct library first.
        if (TangoInitializationHelper.loadTangoSharedLibrary() ==
                TangoInitializationHelper.ARCH_ERROR) {
            Log.e("TangoJNINative", "ERROR! Unable to load libtango_client_api.so!");
        }
        System.loadLibrary("vsense_ar");
    }

    /**
     * Initialize the object from the assets in the project.
     * @param assetManager
     */
    public static native void init(AssetManager assetManager);

    /**
     * Interfaces to native onCreate function.
     *
     * @param callerActivity the caller activity of this function.
     */
    public static native void onCreate(Activity callerActivity);

    /**
     * Called when the Tango service is connected successfully.
     *
     * @param nativeTangoServiceBinder The native binder object.
     */
    public static native void onTangoServiceConnected(IBinder nativeTangoServiceBinder);

    /**
     * Interfaces to native onPause function.
     */
    public static native void onPause();

    // Allocate OpenGL resources for rendering and register the color
    // camera texture.
    public static native void onGlSurfaceCreated(AssetManager assetManager);

    // Setup the view port width and height.
    public static native void onGlSurfaceChanged(int width, int height);

    public static native void onUpdateRenderData();

    // Main render loop.
    public static native void onGlSurfaceDrawFrame();

    // Respond to a display change.
    public static native void onDisplayChanged(int displayRotation);

    // User touched the screen
    public static native void onTouchEvent(float x, float y);

    // User requested pausing frame acquisition
    public static native void onPauseRecord();

    // User requested starting frame acquisition
    public static native void onStartRecord();

    // User requested to erase all saved frames
    public static native void onDeleteRecord();

    public static native void onUpdateSave(boolean save);

    // Retrieves the current state as a string
    public static native String getStateString();

    // User requested a change in the animatino state
    public static native void onUpdateAnimation(boolean animate);

    // User requested to toggle the visibility of the gizmo
    public static native void onUpdateGizmo(boolean vis);

    // User requested to toggle the visibility of the video stream
    public static native void onUpdateVideo(boolean vis);

    public static native void onUpdateBunny(boolean vis);

    public static native void onUpdateShadow(boolean vis);

    public static native void onUpdateColorCorrection(boolean colorCorrection);

    // User requested to stop the current process
    public static native void onStopProcess();

    public static native void onSetDepthMapConfidence(float confidence);

    public static native void onSetDepthMapEnableFillHoles(boolean enable);

    public static native void onSetDepthMapEnableFillHolesWithMax(boolean enable);

    public static native void onSetColorCorrectionEnable(boolean enable);

    public static native void onSetColorCorrectionMaxError(float error);

    public static native void onSetColorCorrectionMinPoints(int pointsNbr);

    public static native void onSetRenderNumberSamples(int samplesNbr);

    public static native void onSetRenderNumberOrder(int orderNbr);

    public static native void onSetRenderObject(int objectIdx);
}

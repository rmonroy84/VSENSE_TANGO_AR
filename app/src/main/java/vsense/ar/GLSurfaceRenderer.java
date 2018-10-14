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

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.opengl.GLES30;
import android.opengl.GLES31;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.IntBuffer;
import java.util.Date;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Renders graphic content.
 */
public class GLSurfaceRenderer implements GLSurfaceView.Renderer {
    private static final String TAG = GLSurfaceRenderer.class.getSimpleName();

    private MainActivity mMainActivity;
    private AssetManager mAssetManager;
    private boolean printScreen = false;
    private int surfaceWidth;
    private int surfaceHeight;
    private long mNbrFrame;
    private File timingsFile;
    private FileOutputStream timingsFileStream;

    private boolean mIsRecording;
    private boolean mIsColorCorrecting;

    public GLSurfaceRenderer(MainActivity mainActivity) {
        mMainActivity = mainActivity;
        mAssetManager = mMainActivity.getAssets();
        mIsRecording = false;
        mIsColorCorrecting = true;
        mNbrFrame = 0;

        /*try {
            timingsFile = new File("/sdcard/TCD/out/", "TimingsJ.csv");
            timingsFileStream = new FileOutputStream(timingsFile);
        } catch(java.io.FileNotFoundException e) {

        }*/
    }

    // Render loop of the GL context.
    public void onDrawFrame(GL10 gl) {
        long t0 = System.nanoTime();
        TangoJNINative.onUpdateRenderData();
        long t1 = System.nanoTime();
        TangoJNINative.onGlSurfaceDrawFrame();
        long t2 = System.nanoTime();

        /*double emTiming = (t1 - t0)/1000000.0;
        double allTiming = (t2 - t1)/1000000.0;

        try {
            int flags = 0;
            if(mIsRecording)
                flags += 1;
            if(mIsColorCorrecting)
                flags += 2;
            timingsFileStream.write((flags + ", " + emTiming + " " + allTiming +  "\n").getBytes());
        } catch(java.io.IOException e) {

        }*/

        mNbrFrame++;

        if(printScreen)
            saveScreenshot();
    }

    public void closeTimingsFile() {
        /*try {
            timingsFileStream.close();
        } catch(java.io.IOException e) {

        }*/
    }

    public long getCurrentFrameNbr() {
        return mNbrFrame;
    }

    public void updateRecording(boolean recording){
        mIsRecording = recording;
    }

    public void updateColorCorrecting(boolean correcting) { mIsColorCorrecting = correcting; }

    private void saveScreenshot() {
        printScreen = false;

        int b[]= new int[(int) (surfaceWidth*surfaceHeight)];
        int bt[]= new int[(int) (surfaceWidth*surfaceHeight)];
        IntBuffer buffer = IntBuffer.wrap(b);
        buffer.position(0);
        GLES31.glReadPixels(0, 0, surfaceWidth, surfaceHeight,GLES31.GL_RGBA,GLES31.GL_UNSIGNED_BYTE, buffer);
        int curSrc = 0;
        for(int i = 0; i < surfaceHeight; i++) {
            int curDst = (surfaceHeight - i - 1)*surfaceWidth;
            for(int j = 0; j < surfaceWidth; j++) {
                int pix = b[curSrc];
                int pb = (pix >> 16)&0x000000ff;
                int pr = (pix << 16)&0x00ff0000;
                int pix1 = (pix & 0xff00ff00) | pr | pb;
                bt[curDst] = pix1;

                curSrc++;
                curDst++;
            }
        }

        Bitmap inBitmap = Bitmap.createBitmap(bt, surfaceWidth, surfaceHeight, Bitmap.Config.ARGB_8888);

        Date now = new Date();
        android.text.format.DateFormat.format("yyyy-MM-dd_hh:mm:ss", now);
        String mPath = Environment.getExternalStorageDirectory().toString() + "/TCD/screenshot/" + now + ".jpg";

        File imageFile = new File(mPath);

        try {
            FileOutputStream outputStream = new FileOutputStream(imageFile);
            inBitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream);
            outputStream.flush();
            outputStream.close();
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    public void triggerScreenshot() {
        printScreen = true;
    }

    // Called when the surface size changes.
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        TangoJNINative.onGlSurfaceChanged(width, height);

        surfaceWidth = width;
        surfaceHeight = height;
    }

    // Called when the surface is created or recreated.
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        TangoJNINative.onGlSurfaceCreated(mAssetManager);
    }
}

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
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.hardware.display.DisplayManager;
import android.opengl.GLES30;
import android.opengl.GLES31;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import org.apache.commons.io.FileUtils;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Date;

class Settings {
    public float   dmConfidence;
    public boolean dmFillHoles;
    public boolean dmFillHolesWithMax;
    public boolean ccEnable;
    public float   ccMaxVariance;
    public float   ccMaxError;
    public int     ccMinPoints;
    public int     rnNbrSamples;
    public int     rnNbrOrder;
    public boolean rnSkipEmpty;
    public boolean rnRenderBaseColor;
    public int     rnObject;
    public int     rnCCMode;
}

/**
 * Primary activity of the example.
 */
public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    static final int CHANGE_SETTINGS = 1001;
    static final int MAX_ORDER = 9;
    static final int MIN_ORDER = 0;

    private static final String TAG = MainActivity.class.getSimpleName();
    private static final int UPDATE_UI_INTERVAL_MS = 10;

    // GLSurfaceView and renderer, all of the graphic content is rendered
    // through OpenGL ES 2.0 in native code.
    private GLSurfaceView mGLView;
    private GLSurfaceRenderer mRenderer;

    private ImageButton mPauseRecord;
    private ImageButton mStartRecord;
    private ImageButton mToggleAnimation;
    private ImageButton mScreenshot;
    private ImageButton mSave;
    private ImageButton mColorCorrection;
    private ImageButton mToggleOptions;
    private ImageButton mSettings;
    private ImageButton mToggleGizmo;
    private ImageButton mToggleVideo;
    private ImageButton mToggleBunny;
    private ImageButton mToggleShadow;
    private ImageButton mToggleNoShadow;
    private ImageButton mIncreaseOrder;
    private ImageButton mDecreaseOrder;

    private TextView mStatus;
    private Handler mHandler = new Handler();

    private boolean mOptionsVisible = true;
    private boolean mIsRecording = false;
    private boolean mIsSaving = false;
    private boolean mDoColorCorrection = true;
    private boolean mShowSettings = false;
    private boolean mShowGizmo = true;
    private boolean mShowVideo = true;
    private boolean mShowBunny = true;
    private boolean mShowShadow = true;
    private boolean mAnimate = false;
    private int mSHOrder = 0;

    private Settings mAppSettings = new Settings();

    private enum ProcessStatus {
        IDLE,
        WAITING_START,
        WAITING_STOP,
        PROCESSING
    }

    private ProcessStatus mProcessStatus = ProcessStatus.IDLE;

    // Tango Service connection.
    ServiceConnection mTangoServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName name, IBinder service) {
            TangoJNINative.onTangoServiceConnected(service);
            setDisplayRotation();
        }

        public void onServiceDisconnected(ComponentName name) {
            // Handle this if you need to gracefully shutdown/retry
            // in the event that Tango itself crashes/gets upgraded while running.
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        // Register for display orientation change updates.
        DisplayManager displayManager = (DisplayManager) getSystemService(DISPLAY_SERVICE);
        if (displayManager != null) {
            displayManager.registerDisplayListener(new DisplayManager.DisplayListener() {
                @Override
                public void onDisplayAdded(int displayId) {}

                @Override
                public void onDisplayChanged(int displayId) {
                    synchronized (this) {
                        setDisplayRotation();
                    }
                }

                @Override
                public void onDisplayRemoved(int displayId) {}
            }, null);
        }

        TangoJNINative.onCreate(this);

        setContentView(R.layout.activity_pointcloud);

        mPauseRecord = (ImageButton)findViewById(R.id.pause_record);
        mStartRecord = (ImageButton)findViewById(R.id.start_record);
        mToggleAnimation = (ImageButton)findViewById(R.id.toggle_animate);
        mScreenshot = (ImageButton)findViewById(R.id.screenshot);
        mSave = (ImageButton)findViewById(R.id.save);
        mColorCorrection = (ImageButton)findViewById(R.id.color_correction);
        mToggleOptions = (ImageButton)findViewById(R.id.options);
        mSettings = (ImageButton)findViewById(R.id.settings);
        mToggleGizmo = (ImageButton)findViewById(R.id.toggle_gizmo);
        mToggleVideo = (ImageButton)findViewById(R.id.toggle_video);
        mToggleBunny = (ImageButton)findViewById(R.id.toggle_bunny);
        mToggleShadow = (ImageButton)findViewById(R.id.toggle_shadow);
        mToggleNoShadow = (ImageButton)findViewById(R.id.toggle_noshadow);
        mIncreaseOrder = (ImageButton)findViewById(R.id.increase_order);
        mDecreaseOrder = (ImageButton)findViewById(R.id.decrease_order);
        mStatus = (TextView)findViewById(R.id.status);

        mPauseRecord.setOnClickListener(this);
        mStartRecord.setOnClickListener(this);
        mToggleAnimation.setOnClickListener(this);
        mScreenshot.setOnClickListener(this);
        mSave.setOnClickListener(this);
        mColorCorrection.setOnClickListener(this);
        mToggleOptions.setOnClickListener(this);
        mSettings.setOnClickListener(this);
        mToggleGizmo.setOnClickListener(this);
        mToggleVideo.setOnClickListener(this);
        mToggleBunny.setOnClickListener(this);
        mToggleShadow.setOnClickListener(this);
        mToggleNoShadow.setOnClickListener(this);
        mIncreaseOrder.setOnClickListener(this);
        mDecreaseOrder.setOnClickListener(this);

        configureGlSurfaceView();
    }

    // Pass device's camera sensor rotation and display rotation to native layer.
    // These two parameter are important for Tango to render video overlay and
    // virtual objects in the correct device orientation.
    private void setDisplayRotation() {
        Display display = getWindowManager().getDefaultDisplay();

        TangoJNINative.onDisplayChanged(display.getRotation());
    }

    private void configureGlSurfaceView() {
        // OpenGL view where all of the graphics are drawn.
        mGLView = (GLSurfaceView) findViewById(R.id.gl_surface_view);

        mGLView.setEGLContextClientVersion(3);
        // Configure the OpenGL renderer.
        mRenderer = new GLSurfaceRenderer(this);
        mGLView.setRenderer(mRenderer);
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.init(getAssets());
            }
        });

        updateSettings(true);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mGLView.onResume();

        mHandler.post(mUpdateUILoopRunnable);

        TangoInitializationHelper.bindTangoService(this, mTangoServiceConnection);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mGLView.onPause();
        TangoJNINative.onPause();
        unbindService(mTangoServiceConnection);
        mHandler.removeCallbacksAndMessages(null);
        mRenderer.closeTimingsFile();
    }

    @Override
    public void onClick(View v) {
        mSave = (ImageButton) findViewById(R.id.save);

        switch (v.getId()) {
            case R.id.pause_record:
                mGLView.queueEvent(new Runnable() {
                    @Override
                    public void run() {
                        TangoJNINative.onPauseRecord();
                    }
                });
                mPauseRecord.setVisibility(View.GONE);
                mStartRecord.setVisibility(View.VISIBLE);
                mIsRecording = false;
                mRenderer.updateRecording(mIsRecording);
                break;
            case R.id.start_record:
                mGLView.queueEvent(new Runnable() {
                    @Override
                    public void run() {
                        TangoJNINative.onStartRecord();
                    }
                });
                mStartRecord.setVisibility(View.GONE);
                mPauseRecord.setVisibility(View.VISIBLE);
                mIsRecording = true;
                mRenderer.updateRecording(mIsRecording);
                break;
            case R.id.toggle_animate:
                mAnimate = !mAnimate;
                updateAnimation();
                break;
            case R.id.screenshot:
                captureScreen();
                break;
            case R.id.save:
                mIsSaving = !mIsSaving;
                updateSaving();
                break;
            case R.id.options:
                mOptionsVisible = !mOptionsVisible;
                updateOptions(mOptionsVisible);
                break;
            case R.id.color_correction:
                mDoColorCorrection = !mDoColorCorrection;
                mRenderer.updateColorCorrecting(mDoColorCorrection);
                updateColorCorrection();
                break;
            case R.id.settings:
                mShowSettings = !mShowSettings;
                showSettings(mShowSettings);
                break;
            case R.id.toggle_gizmo:
                mShowGizmo = !mShowGizmo;
                updateGizmo();
                break;
            case R.id.toggle_bunny:
                mShowBunny = !mShowBunny;
                updateBunny();
                break;
            case R.id.toggle_shadow:
                mShowShadow = true;
                mToggleShadow.setVisibility(View.GONE);
                mToggleNoShadow.setVisibility(View.VISIBLE);
                updateShadow();
                break;
            case R.id.toggle_noshadow:
                mShowShadow = false;
                mToggleShadow.setVisibility(View.VISIBLE);
                mToggleNoShadow.setVisibility(View.GONE);
                updateShadow();
                break;
            case R.id.toggle_video:
                mShowVideo = !mShowVideo;
                updateVideo();
                break;
            case R.id.increase_order:
            {
                int newOrder = mSHOrder + 1;
                if(newOrder > MAX_ORDER)
                    newOrder = MAX_ORDER;

                if(newOrder != mSHOrder) {
                    mSHOrder = newOrder;

                    mGLView.queueEvent(new Runnable() {
                        @Override
                        public void run() {
                            TangoJNINative.onSetRenderNumberOrder(mSHOrder);
                        }
                    });
                }
            }
                break;
            case R.id.decrease_order:
                int newOrder = mSHOrder - 1;
                if(newOrder < MIN_ORDER)
                    newOrder = MIN_ORDER;

                if(newOrder != mSHOrder) {
                    mSHOrder = newOrder;

                    mGLView.queueEvent(new Runnable() {
                        @Override
                        public void run() {
                            TangoJNINative.onSetRenderNumberOrder(mSHOrder);
                        }
                    });
                }
                break;
        }
    }

    @Override
    public boolean onTouchEvent(final MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            mProcessStatus = ProcessStatus.WAITING_START;
            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onTouchEvent(event.getX(), event.getY());
                }
            });
        }

        return super.onTouchEvent(event);
    }

    private Runnable mUpdateUILoopRunnable = new Runnable() {
        @Override
        public void run() {
            updateUI();
            mHandler.postDelayed(this, UPDATE_UI_INTERVAL_MS);
        }
    };

    private void updateUI() {
        try {
            String status = TangoJNINative.getStateString();

            status = status + "SH Order: " + mSHOrder + "\n";
            status = status + "Frame: " + mRenderer.getCurrentFrameNbr();

            mStatus.setText(status);
        } catch(Exception e) {
            e.printStackTrace();
            Log.e(TAG, "Exception updating UI elements");
        }
    }

    private void showSettings(boolean visible) {
        Intent modifySettings = new Intent(MainActivity.this, SettingsActivity.class);
        startActivityForResult(modifySettings, CHANGE_SETTINGS);
    }

    private void updateSettings(boolean firstTime) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(MainActivity.this);

        float dmConfidence = prefs.getFloat("dm_confidence", 1.0f);
        if(mAppSettings.dmConfidence != dmConfidence || firstTime) {
            mAppSettings.dmConfidence = dmConfidence;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetDepthMapConfidence(mAppSettings.dmConfidence);
                }
            });
        }

        boolean dmFillHoles = prefs.getBoolean("dm_fill_holes", true);
        if(mAppSettings.dmFillHoles != dmFillHoles || firstTime) {
            mAppSettings.dmFillHoles = dmFillHoles;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetDepthMapEnableFillHoles(mAppSettings.dmFillHoles);
                }
            });
        }

        boolean dmFillHolesWithMax = prefs.getBoolean("dm_fill_with_max", false);
        if(mAppSettings.dmFillHolesWithMax != dmFillHolesWithMax || firstTime) {
            mAppSettings.dmFillHolesWithMax = dmFillHolesWithMax;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetDepthMapEnableFillHolesWithMax(mAppSettings.dmFillHolesWithMax);
                }
            });
        }

        boolean ccEnable = prefs.getBoolean("cc_enable", true);
        if(mAppSettings.ccEnable != ccEnable || firstTime) {
            mAppSettings.ccEnable = ccEnable;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetColorCorrectionEnable(mAppSettings.ccEnable);
                }
            });
        }

        float ccMaxError = prefs.getFloat("cc_max_error", 0.1f);
        if(mAppSettings.ccMaxError != ccMaxError || firstTime) {
            mAppSettings.ccMaxError = ccMaxError;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetColorCorrectionMaxError(mAppSettings.ccMaxError);
                }
            });
        }

        int ccMinPoints = prefs.getInt("cc_min_pts", 4000);
        if(mAppSettings.ccMinPoints != ccMinPoints || firstTime) {
            mAppSettings.ccMinPoints = ccMinPoints;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetColorCorrectionMinPoints(mAppSettings.ccMinPoints);
                }
            });
        }

        int rnNbrSamples = prefs.getInt("rn_nbr_samples", 1000);
        if(mAppSettings.rnNbrSamples != rnNbrSamples || firstTime) {
            mAppSettings.rnNbrSamples = rnNbrSamples;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetRenderNumberSamples(mAppSettings.rnNbrSamples);
                }
            });
        }

        int rnNbrOrder = prefs.getInt("rn_nbr_order", 9);
        mSHOrder = rnNbrOrder;
        if(mAppSettings.rnNbrOrder != rnNbrOrder || firstTime) {
            mAppSettings.rnNbrOrder = rnNbrOrder;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetRenderNumberOrder(mAppSettings.rnNbrOrder);
                }
            });
        }

        String rnObjectStr = prefs.getString("render_object", "bunny");
        int rnObject = rnObjectStr.compareTo("bunny") == 0 ? 0 : -1;
        if(rnObject < 0)
            rnObject = rnObjectStr.compareTo("rafa") == 0 ? 1 : -1;
        if(rnObject < 0)
            rnObject = rnObjectStr.compareTo("spot") == 0 ? 2 : -1;
        if(rnObject < 0)
            rnObject = rnObjectStr.compareTo("nefertiti") == 0 ? 3 : -1;
        if(rnObject < 0)
            rnObject = rnObjectStr.compareTo("dragon") == 0 ? 4 : -1;
        if(rnObject < 0)
            rnObject = 5;

        if(mAppSettings.rnObject != rnObject || firstTime) {
            mAppSettings.rnObject = rnObject;

            mGLView.queueEvent(new Runnable() {
                @Override
                public void run() {
                    TangoJNINative.onSetRenderObject(mAppSettings.rnObject);
                }
            });
        }

        if(!firstTime)
            mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onDeleteRecord();
            }
        });
    }

    private void updateOptions(boolean visible) {
        mSave.setVisibility(visible ? View.VISIBLE : View.GONE);
        mSettings.setVisibility(visible ? View.VISIBLE : View.GONE);
        mColorCorrection.setVisibility(visible ? View.VISIBLE : View.GONE);
        mToggleGizmo.setVisibility(visible ? View.VISIBLE : View.GONE);
        mToggleVideo.setVisibility(visible ? View.VISIBLE : View.GONE);
        mStatus.setVisibility(visible ? View.VISIBLE : View.GONE);
        mToggleBunny.setVisibility(visible ? View.VISIBLE : View.GONE);
        mIncreaseOrder.setVisibility(visible ? View.VISIBLE : View.GONE);
        mDecreaseOrder.setVisibility(visible ? View.VISIBLE : View.GONE);

        if(!visible) {
            mStartRecord.setVisibility(View.GONE);
            mPauseRecord.setVisibility(View.GONE);

            mToggleShadow.setVisibility(View.GONE);
            mToggleNoShadow.setVisibility(View.GONE);
        } else {
            mStartRecord.setVisibility(mIsRecording ? View.GONE : View.VISIBLE);
            mPauseRecord.setVisibility(mIsRecording ? View.VISIBLE : View.GONE);

            mToggleShadow.setVisibility(mShowShadow ? View.GONE : View.VISIBLE);
            mToggleNoShadow.setVisibility(mShowShadow ? View.VISIBLE : View.GONE);
        }
    }

    private void updateAnimation() {
        mToggleAnimation.setAlpha(mAnimate ? 0.3f : 1.f);
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onUpdateAnimation(mAnimate);
            }
        });
    }

    private void updateGizmo() {
        mToggleGizmo.setAlpha(mShowGizmo ? 1.f : 0.3f);
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onUpdateGizmo(mShowGizmo);
            }
        });
    }

    private void updateColorCorrection() {
        mColorCorrection.setAlpha(mDoColorCorrection ? 1.f : 0.3f);
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onUpdateColorCorrection(mDoColorCorrection);
            }
        });
    }

    private void updateVideo() {
        mToggleVideo.setAlpha(mShowVideo ? 1.f : 0.3f);
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onUpdateVideo(mShowVideo);
            }
        });
    }

    private void updateBunny() {
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onUpdateBunny(mShowBunny);
            }
        });
    }

    private void updateShadow() {
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onUpdateShadow(mShowShadow);
            }
        });
    }

    private void updateSaving() {
        mSave.setAlpha(mIsSaving ? 0.3f : 1.f);
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                TangoJNINative.onUpdateSave(mIsSaving);
            }
        });
    }

    private void captureScreen() {
        mRenderer.triggerScreenshot();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(requestCode == CHANGE_SETTINGS)
           updateSettings(false);
    }
}

package vsense.ar;

import android.content.Context;
import android.content.res.TypedArray;
import android.preference.DialogPreference;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;
import android.widget.NumberPicker;
import android.widget.TextView;

/**
 * Created by monroyrr on 12/7/2017.
 */

public class DecimalNumberPickerPreference extends DialogPreference {
    private NumberPicker mPicker;
    private float mNumber = 0;
    private float mMinValue = 0;
    private float mMaxValue = 0;
    private float mStep = 1;
    private int mLength = 4;

    public DecimalNumberPickerPreference(Context context) {
        this(context, null, 0);
    }

    public DecimalNumberPickerPreference(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        updateFromAttributes(attrs);
    }

    public DecimalNumberPickerPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setPositiveButtonText(android.R.string.ok);
        setNegativeButtonText(android.R.string.cancel);
        updateFromAttributes(attrs);
    }

    private void updateFromAttributes(AttributeSet attrs) {
        mMinValue = attrs.getAttributeFloatValue(null, "min", 0);
        mMaxValue = attrs.getAttributeFloatValue(null, "max", 0);
        mStep = attrs.getAttributeFloatValue(null, "step", 1);
        mLength = attrs.getAttributeIntValue(null, "length", 4);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        TextView title = (TextView) view.findViewById(android.R.id.title);
        if (title != null) {
            title.setSingleLine(false);
            title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        }
    }

    @Override
    protected View onCreateDialogView() {
        mPicker = new NumberPicker(getContext());

        int nbrElements = (int)((mMaxValue - mMinValue)/mStep) + 1;

        mPicker.setMinValue(0);
        mPicker.setMaxValue(nbrElements - 1);

        String[] values = new String[nbrElements];
        float curVal = mMinValue;
        int initPos = 0;
        for (int i = 0; i < nbrElements; i++) {
            String number = String.format("%f", curVal);
            if(number.length() < mLength) {
                while(number.length() < mLength)
                    number += "0";
            }else if(number.length() > mLength)
                number = number.substring(0, mLength);

            values[i] = number;

            if(Math.abs(curVal - mNumber) < (mStep / 4))
                initPos = i;

            curVal += mStep;
        }
        mPicker.setDisplayedValues(values);

        mPicker.setValue(initPos);

        return mPicker;
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        if (positiveResult) {
            mPicker.clearFocus();
            setValue(getFloatFromValue(mPicker.getValue()));
        }
    }

    @Override
    protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {
        setValue(restoreValue ? getPersistedFloat(mNumber) : (Float) defaultValue);
    }

    public void setValue(float value) {
        if (shouldPersist()) {
            persistFloat(value);
        }

        if (value != mNumber) {
            mNumber = value;
            notifyChanged();
        }
    }

    private float getFloatFromValue(int value) {
        float result = value*mStep + mMinValue;
        return result;
    }

    @Override
    protected Object onGetDefaultValue(TypedArray a, int index) {
        return a.getFloat(index, 0);
    }
}

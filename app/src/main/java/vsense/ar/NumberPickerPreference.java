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

public class NumberPickerPreference extends DialogPreference {
    private NumberPicker mPicker;
    private int mNumber = 0;
    private int mMinValue = 0;
    private int mMaxValue = 0;
    private int mStep = 1;

    public NumberPickerPreference(Context context) {
        this(context, null, 0);
    }

    public NumberPickerPreference(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
        updateFromAttributes(attrs);
    }

    public NumberPickerPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setPositiveButtonText(android.R.string.ok);
        setNegativeButtonText(android.R.string.cancel);
        updateFromAttributes(attrs);
    }

    private void updateFromAttributes(AttributeSet attrs) {
        mMinValue = attrs.getAttributeIntValue(null, "min", 0);
        mMaxValue = attrs.getAttributeIntValue(null, "max", 0);
        mStep = attrs.getAttributeIntValue(null, "step", 1);
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

        int nbrElements = (mMaxValue - mMinValue)/mStep + 1;

        mPicker.setMinValue(0);
        mPicker.setMaxValue(nbrElements - 1);

        String[] values = new String[nbrElements];
        int curVal = mMinValue;
        int initPos = 0;
        for (int i = 0; i < nbrElements; i++) {
            String number = Integer.toString(curVal);
            values[i] = number;

            if(curVal == mNumber)
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
            setValue(getIntegerFromValue(mPicker.getValue()));
        }
    }

    @Override
    protected void onSetInitialValue(boolean restoreValue, Object defaultValue) {
        setValue(restoreValue ? getPersistedInt(mNumber) : (Integer) defaultValue);
    }

    public void setValue(int value) {
        if (shouldPersist()) {
            persistInt(value);
        }

        if (value != mNumber) {
            mNumber = value;
            notifyChanged();
        }
    }

    private int getIntegerFromValue(int value) {
        int result = value*mStep + mMinValue;
        return result;
    }

    @Override
    protected Object onGetDefaultValue(TypedArray a, int index) {
        return a.getInt(index, 0);
    }
}

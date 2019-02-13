package com.google.gables;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.graphics.PorterDuff;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import java.io.File;

import static com.google.gables.Roofline.GPU_Finish;
import static com.google.gables.Roofline.GPU_Initialize;
import static com.google.gables.Roofline.GPU_MaxWorkGroupCount;
import static com.google.gables.Roofline.GPU_MaxWorkGroupSize;
import static com.google.gables.Roofline.SOC_Mixer;
import static com.google.gables.Utils.ExtStoragePermissions_t.EXT_STORAGE_RW;
import static com.google.gables.Utils.MiB;
import static com.google.gables.Utils.deleteDirectory;
import static com.google.gables.Utils.getNumberOfCores;
import static com.google.gables.Utils.isStorageAvailable;
import static com.google.gables.Utils.log;
import static com.google.gables.Utils.writeToSDFile;

public class SOCRoofline extends Fragment {
    private static final String TAG = SOCRoofline.class.getName();

    private ProgressDialog gProcessDialog;
    private String gResultsDir = "SOCRoofline";

    private void setupMemorySlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_totalMem);

        TextView textView = rootView.findViewById(R.id.txtBox_totalMem);
        textView.setText("Total Memory Alloc. (" + Long.toString(1 << mSeekBar.getProgress()) + " MB)");

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runMixer);

        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                TextView textView = rootView.findViewById(R.id.txtBox_totalMem);
                textView.setText("Total Memory Alloc. (" + Long.toString(1 << mSeekBar.getProgress()) + " MB)");

                // need to update associated text
                setupWorkFractionSlider(rootView);
            }
        });
    }

    private void setupMaxWorkGroupSizeSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxGPUWorkgroupSize);

        // determine the maximum number of threads per work group
        GPU_Initialize();
        int maxThreads = GPU_MaxWorkGroupCount(0);
        GPU_Finish();

        mSeekBar.setMax((int) Math.ceil(log(maxThreads, 2)));

        mSeekBar.setProgress((int) Math.ceil(log(1024, 2))); //fixme: remove hardcode

        TextView textView = rootView.findViewById(R.id.txtBox_maxThreads6);
        maxThreads = (int) Math.pow(2, mSeekBar.getProgress());
        textView.setText("GPU Work Group Size (" + Integer.toString(1 << mSeekBar.getProgress()) + ")"); // increment by 1 since bar starts from 0.

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runMixer);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                TextView textView = rootView.findViewById(R.id.txtBox_maxThreads6);
                int currSize = (int) Math.pow(2, mSeekBar.getProgress());
                textView.setText("GPU Work Group Size (" + Integer.toString(currSize) + ")");
            }
        });
    }

    private void setupMaxWorkGroupItemsSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxGPUWorkItems);

        GPU_Initialize();
        int maxGroups = GPU_MaxWorkGroupSize(0);
        GPU_Finish();

        mSeekBar.setMax((int) Math.ceil(log(maxGroups, 2)));
        mSeekBar.setProgress((int) Math.ceil(log(256, 2)));

        TextView textView = rootView.findViewById(R.id.txtBox_maxThreads4);
        textView.setText("GPU Work Items (" + Integer.toString(1 << mSeekBar.getProgress()) + ")"); // increment by 1 since bar starts from 0.

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runMixer);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                TextView textView = rootView.findViewById(R.id.txtBox_maxThreads4);
                int currSize = (int) Math.pow(2, mSeekBar.getProgress());
                textView.setText("GPU Work Items (" + Integer.toString(currSize) + ")");
            }
        });
    }

    private void setupWorkFractionSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_workFraction);
        final SeekBar seekBarTotalMem = rootView.findViewById(R.id.seekBar_totalMem);

        mSeekBar.getProgressDrawable().setColorFilter(Color.YELLOW, PorterDuff.Mode.MULTIPLY);
        mSeekBar.getThumb().setColorFilter(Color.LTGRAY, PorterDuff.Mode.MULTIPLY);

        final TextView cpuTextView = rootView.findViewById(R.id.txtBox_workFractionCPU);
        final TextView gpuTextView = rootView.findViewById(R.id.txtBox_workFractionGPU);
        final TextView wrkTextView = rootView.findViewById(R.id.txtBox_workFraction);

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runMixer);

        float cpu_frac = (float) mSeekBar.getProgress() / mSeekBar.getMax();
        int cpu_mb = (int) ((1 << seekBarTotalMem.getProgress()) * cpu_frac);
        int gpu_mb = (int) ((1 << seekBarTotalMem.getProgress()) * (1.0 - cpu_frac));

        cpuTextView.setText("CPU (" + cpu_mb + " MB)");
        gpuTextView.setText("GPU (" + gpu_mb + " MB)");
        wrkTextView.setText("Workload Offload Fraction (f): " + (int) ((1.0 - cpu_frac) * 100.0) + "%");

        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                float cpu_frac = (float) mSeekBar.getProgress() / mSeekBar.getMax();
                int cpu_mb = (int) ((1 << seekBarTotalMem.getProgress()) * cpu_frac);
                int gpu_mb = (int) ((1 << seekBarTotalMem.getProgress()) * (1.0 - cpu_frac));

                cpuTextView.setText("CPU (" + Long.toString(cpu_mb) + " MB)");
                gpuTextView.setText("GPU (" + Long.toString(gpu_mb) + " MB)");
                wrkTextView.setText("Workload Offload Fraction (f): " + String.format("%.2f", (1.0 - cpu_frac) * 100.0) + "%");
            }
        });
    }

    private void setupFlopsSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_nFlops);

        TextView textView = rootView.findViewById(R.id.txtBox_nFlops);
        textView.setText("Ops (" + Long.toString(1 << mSeekBar.getProgress()) + " FLOPS/byte)"); // increment by 1 since bar starts from 0.

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runMixer);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                TextView textView = rootView.findViewById(R.id.txtBox_nFlops);
                textView.setText("Ops (" + Long.toString(1 << mSeekBar.getProgress()) + " FLOPS/byte)");
            }
        });
    }

    private void setupMaxCPUThreadSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxCPUThreads);

        int nCores = getNumberOfCores();
        mSeekBar.setProgress(nCores);
        mSeekBar.setMax(nCores - 1); // we start from 0, but we display +1

        TextView textView = rootView.findViewById(R.id.txtBox_maxThreads2);
        textView.setText("CPU Thread Count (" + Long.toString(mSeekBar.getProgress() + 1) + " Cores)"); // increment by 1 since bar starts from 0.

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runMixer);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                toggle.setChecked(false);
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                TextView textView = rootView.findViewById(R.id.txtBox_maxThreads2);
                textView.setText("CPU Thread Count (" + Long.toString(mSeekBar.getProgress() + 1) + " Cores)");
            }
        });
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_mixer_roofline, container, false);

        setupMemorySlider(rootView);
        setupMaxCPUThreadSlider(rootView);
        setupMaxWorkGroupSizeSlider(rootView);
        setupMaxWorkGroupItemsSlider(rootView);
        setupWorkFractionSlider(rootView);
        setupFlopsSlider(rootView);
        setupButton(rootView);

        return rootView;
    }

    void setupButton(final View rootView) {
        final ToggleButton button = rootView.findViewById(R.id.toggle_runMixer);
        button.setChecked(false); // set the current state of a toggle button

        button.setTextOn("Busy!");
        button.setTextOff("Run");

        button.setChecked(false);

        button.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

                if (isChecked) { // enabled
                    SeekBar seekBar_split = rootView.findViewById(R.id.seekBar_workFraction);
                    SeekBar seekBar_mem = rootView.findViewById(R.id.seekBar_totalMem);
                    SeekBar seekBar_cpu = rootView.findViewById(R.id.seekBar_maxCPUThreads);
                    SeekBar seekBar_gpuWorkGroupCount = rootView.findViewById(R.id.seekBar_maxGPUWorkgroupSize);
                    SeekBar seekBar_gpuWorkGroupSize = rootView.findViewById(R.id.seekBar_maxGPUWorkItems);
                    SeekBar seekBar_flops = rootView.findViewById(R.id.seekBar_nFlops);

                    int memTotal = (1 << seekBar_mem.getProgress()) * MiB; // converting to bytes
                    int gpuWorkGroupSize = 1 << seekBar_gpuWorkGroupCount.getProgress();
                    int gpuWorkItemSize = 1 << seekBar_gpuWorkGroupSize.getProgress();
                    int nFlops = 1 << seekBar_flops.getProgress();
                    int cpuThreads = seekBar_cpu.getProgress() + 1; // seekbar starts at 0

                    float cpuWorkFraction = ((float) seekBar_split.getProgress() / seekBar_split.getMax());

                    Mixer(memTotal, cpuWorkFraction, nFlops, cpuThreads, gpuWorkGroupSize, gpuWorkItemSize);

                    button.setChecked(false);
                }
            }
        });
    }

    public void Mixer(int memTotal, float cpuWorkFraction, int nFlops, int cpuThreads, int gpuWorkGroupSize, int gpuWorkItemSize) {
        SOCRoofline.MixerAsync mixerATask = new SOCRoofline.MixerAsync(getActivity(), memTotal, cpuWorkFraction, nFlops, cpuThreads, gpuWorkGroupSize, gpuWorkItemSize);

        mixerATask.execute();
    }

    class MixerAsync extends AsyncTask<Void, String, String> {
        final int memTotal, cpuThreads, gpuWorkGroupSize, gpuWorkItemSize, flops;
        final float cpuWorkFraction;

        private AsyncTask<Void, String, String> currTask = null;

        public MixerAsync(Activity activity, int memTotal, float cpuWorkFraction, int nFlops, int cpuThreads, int gpuWorkGroupSize, int gpuWorkItemSize) {
            super();

            this.flops = nFlops;
            this.memTotal = memTotal;
            this.cpuWorkFraction = cpuWorkFraction;
            this.cpuThreads = cpuThreads;
            this.gpuWorkGroupSize = gpuWorkGroupSize;
            this.gpuWorkItemSize = gpuWorkItemSize;

            gProcessDialog = new ProgressDialog(activity);
        }

        @Override
        protected void onPreExecute() {
            super.onPreExecute();

            currTask = this;

            gProcessDialog.setMax(100);
            gProcessDialog.setProgress(0);
            gProcessDialog.setTitle("Mixing in progress");
            gProcessDialog.setMessage("Starting up... ");
            gProcessDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            gProcessDialog.setCancelable(false);
            gProcessDialog.setButton(DialogInterface.BUTTON_NEGATIVE, "Cancel", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                    currTask.cancel(true);
                }
            });

            gProcessDialog.show();
        }

        @Override
        protected String doInBackground(Void... params) {
            File root = android.os.Environment.getExternalStorageDirectory();
            File odir = new File(root.getAbsolutePath() + "/" + gResultsDir + "/");
            deleteDirectory(odir);
            Log.i(TAG, "Deleting the " + odir + " output directory.");

            String output = SOC_Mixer(memTotal, flops, cpuWorkFraction, cpuThreads, gpuWorkGroupSize, gpuWorkItemSize);
            Log.i(TAG, "Finished with mixing async task.");

            if (isStorageAvailable(EXT_STORAGE_RW)) {
                String ofilename = "mixer-CPUxGPU.txt";
                writeToSDFile(odir, ofilename, output);
            } else {
                Log.e(TAG, "Storage is not available for RW or is unavailable.");
            }

            return "All done with MixerTask!";
        }

        @Override
        protected void onProgressUpdate(String... msg) {
            super.onProgressUpdate(msg);
            gProcessDialog.setProgress(Integer.parseInt(msg[0]));
            gProcessDialog.setMessage(msg[1]);
        }

        // onCancelled() is called when the async task is cancelled.
        @Override
        protected void onCancelled(String result) {
        }

        protected void onPostExecute(String param) {

            gProcessDialog.setMessage("Finished processing data.");

            if (gProcessDialog.isShowing()) {
                gProcessDialog.dismiss();
            }
        }
    }
}

package com.google.gables;


import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GridLabelRenderer;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import static com.google.gables.Roofline.DSP_Execute;
import static com.google.gables.Roofline.DSP_NumThreads;
import static com.google.gables.Utils.MiB;
import static com.google.gables.Utils.deleteDirectory;

public class DSPRoofline extends Fragment {
    private static final String TAG = DSPRoofline.class.getName();

    private ProgressDialog gProcessDialog;
    private String gResultsDir = "DSPRoofline";

    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

    }

    void drawGraph(View rootView) {
        // Setup a graph view
        GraphView graphview = rootView.findViewById(R.id.graph);

        // activate horizontal zooming and scrolling
        graphview.getViewport().setScalable(false);
        graphview.getViewport().setScrollable(true);
        graphview.getViewport().setScalableY(false);
        graphview.getViewport().setScrollableY(true);

        LineGraphSeries<DataPoint> series = new LineGraphSeries<>(new DataPoint[]{
                new DataPoint(0, 1),
                new DataPoint(1, 2),
                new DataPoint(2, 3),
                new DataPoint(3, 4),
                new DataPoint(4, 4),
                new DataPoint(5, 4)
        });

        GridLabelRenderer gridLabel = graphview.getGridLabelRenderer();
        gridLabel.setHorizontalAxisTitle("Operational Intensity (FLOPS/byte)");
        gridLabel.setVerticalAxisTitle("Performance (GFLOPS)");

        graphview.setTitle("DSP Roofline");

        series.setTitle("DSP Roofline");
        series.setBackgroundColor(Color.GRAY);
        series.setColor(Color.BLACK);
        series.setDrawDataPoints(false);
        series.setDataPointsRadius(10);
        series.setThickness(7);

        graphview.addSeries(series);
    }

    /**
     * Setup a button to trigger the activation of the roofline process
     *
     * @param rootView
     */
    void setupButton(final View rootView) {
        // Register a callback on the toggle button to turn ON/OFF DSP activity
        final ToggleButton button = rootView.findViewById(R.id.toggle_runRoofline);
        button.setTextOn("Busy!");
        button.setTextOff("Run");
        button.setChecked(false); // set the current state of a toggle button

        button.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

                if (isChecked) { // enabled
                    final SeekBar seekBar_mem = rootView.findViewById(R.id.seekBar_threadMem);
                    final SeekBar seekBar_DSP = rootView.findViewById(R.id.seekBar_maxThreads);
                    final SeekBar seekBar_opi = rootView.findViewById(R.id.seekBar_opIntensity);

                    final int maxMemory = (1 << seekBar_mem.getProgress()) * MiB; // converting to bytes
                    final int nThreads = seekBar_DSP.getProgress() + 1; // remember that seekbar starts at 0
                    final int nFlops = (1 << seekBar_opi.getProgress()); // remember that seekbar starts at 0

                    final Switch hvxSwitch = rootView.findViewById(R.id.switch_hvxMode);
                    final boolean hvxMode = hvxSwitch.isChecked();

                    DSPRoofline(nThreads, maxMemory, nFlops, hvxMode);

                    button.setChecked(false);
                }
            }
        });
    }

    private void setupThreadCountSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxThreads);

        final Switch toggle = rootView.findViewById(R.id.switch_hvxMode);

        int nCores = DSP_NumThreads(toggle.isChecked());
        mSeekBar.setProgress(nCores);
        mSeekBar.setMax(nCores - 1); // we start from 0, but we display +1

        TextView textView = rootView.findViewById(R.id.txtBox_maxThreads);
        textView.setText("Thread Count (" + Long.toString(mSeekBar.getProgress() + 1) + " Cores)"); // increment by 1 since bar starts from 0.

        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            public void onStopTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                TextView textView = rootView.findViewById(R.id.txtBox_maxThreads);
                textView.setText("Thread Count (" + Long.toString(mSeekBar.getProgress() + 1) + " Cores)");
            }
        });
    }

    private void setupOpIntensity(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_opIntensity);
        mSeekBar.setProgress(4); //fixme debug version

        TextView textView = rootView.findViewById(R.id.txtBox_opIntensity);
        textView.setText("Ops (" + Long.toString(1 << mSeekBar.getProgress()) + " FLOPS/byte)");

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runRoofline);
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
                TextView textView = rootView.findViewById(R.id.txtBox_opIntensity);
                textView.setText("Ops (" + Long.toString(1 << mSeekBar.getProgress()) + " FLOPS/byte)");
            }
        });
    }

    private void setupMemorySlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_threadMem);

        TextView textView = rootView.findViewById(R.id.txtBox_threadMem);
        textView.setText("Memory Alloc. (" + Long.toString(1 << mSeekBar.getProgress()) + " MB)");

        final ToggleButton toggle = rootView.findViewById(R.id.toggle_runRoofline);

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
                TextView textView = rootView.findViewById(R.id.txtBox_threadMem);
                textView.setText("Memory Alloc. (" + Long.toString(1 << mSeekBar.getProgress()) + " MB)");
            }
        });
    }

    private void setupSwitch(final View rootView) {
        final Switch hvxSwitch = rootView.findViewById(R.id.switch_hvxMode);

        hvxSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                TextView textView = rootView.findViewById(R.id.txtBox_hvxSwitch);
                if (isChecked) {
                    textView.setText("HVX Vectorization (Enabled)");
                } else {
                    textView.setText("HVX Vectorization (Disabled)");
                }

                SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxThreads);
                int nCores = DSP_NumThreads(isChecked);
                mSeekBar.setMax(nCores - 1); // we start from 0, but we display +1
                mSeekBar.setProgress(nCores - 1);

                TextView textView2 = rootView.findViewById(R.id.txtBox_maxThreads);
                textView2.setText("Thread Count (" + Long.toString(mSeekBar.getProgress() + 1) + " Cores)"); // increment by 1 since bar starts from 0.
            }
        });
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_dsp_roofline, container, false);

        drawGraph(rootView);

        setupMemorySlider(rootView);
        setupThreadCountSlider(rootView);
        setupOpIntensity(rootView);

        setupSwitch(rootView);

        setupButton(rootView);

        return rootView;
    }

    private void writeToSDFile(File dir, String filename, String ostr) {

        boolean success = true;
        if (!dir.exists()) {
            success = dir.mkdir(); // FIXME: if this fails, then you might have to request permission in app settings.
        }
        if (success) {
            Log.i(TAG, "Output directory created.");
        } else {
            Log.i(TAG, "Output directory NOT created!");
        }

        File file = new File(dir, filename);

        try {
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(ostr.getBytes());
            fos.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            Log.i(TAG, "******* File not found. Did you" +
                    " add a WRITE_EXTERNAL_STORAGE permission to the   manifest?");
        } catch (IOException e) {
            e.printStackTrace();
        }
        Log.i(TAG, "\n\nFile written to " + file);
    }

    public void DSPRoofline(int nThreads, int maxMemory, int nFlops, boolean hvx) {
        DSPRooflineAsync rooflineATask = new DSPRooflineAsync(getActivity(),
                nThreads, maxMemory, nFlops, hvx);

        rooflineATask.execute();
    }

    class DSPRooflineAsync extends AsyncTask<Void, String, String> {
        final int threads, mem, flops;
        final boolean hvx;
        private AsyncTask<Void, String, String> currTask = null;

        public DSPRooflineAsync(Activity activity, int nThreads, int threadMem, int maxFlops, boolean hvx) {
            super();

            this.threads = nThreads;
            this.mem = threadMem;
            this.flops = maxFlops;
            this.hvx = hvx;

            gProcessDialog = new ProgressDialog(activity);
        }

        @Override
        protected void onPreExecute() {
            super.onPreExecute();

            currTask = this;

            gProcessDialog.setMax(100);
            gProcessDialog.setProgress(0);
            gProcessDialog.setTitle("Roofline in progress");
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

            // call some function that runs on the DSP
            DSP_Execute(mem, threads, flops, hvx);

            return "All done with AsyncTask!";
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

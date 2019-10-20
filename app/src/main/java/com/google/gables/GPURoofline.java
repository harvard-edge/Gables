package com.google.gables;


import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.opengl.GLES31;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GridLabelRenderer;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;
import com.synnapps.carouselview.CarouselView;
import com.synnapps.carouselview.ImageListener;

import java.io.File;
import java.nio.IntBuffer;

import gables.gables_processor.GablesPython;

import static android.opengl.GLES31.GL_MAX_COMPUTE_WORK_GROUP_COUNT;
import static android.opengl.GLES31.GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS;
import static android.opengl.GLES31.GL_MAX_COMPUTE_WORK_GROUP_SIZE;
import static com.google.gables.Roofline.GPU_Execute;
import static com.google.gables.Roofline.GPU_Finish;
import static com.google.gables.Roofline.GPU_Initialize;
import static com.google.gables.Roofline.GPU_MaxWorkGroupCount;
import static com.google.gables.Roofline.GPU_MaxWorkGroupSize;
import static com.google.gables.Utils.ExtStoragePermissions_t.EXT_STORAGE_RW;
import static com.google.gables.Utils.MiB;
import static com.google.gables.Utils.deleteDirectory;
import static com.google.gables.Utils.isStorageAvailable;
import static com.google.gables.Utils.log;
import static com.google.gables.Utils.writeToSDFile;

public class GPURoofline extends Fragment {
    private static final String TAG = GPURoofline.class.getName();

    private ProgressDialog gProcessDialog;
    private String gResultsDir = "GPURoofline";
    private CarouselView slider;
    private View slidePrompt, edgeLogo;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

//    void drawGraph(View rootView) {
//        // Setup a graph view
//        GraphView graphview = rootView.findViewById(R.id.graph);
//
//        // activate horizontal zooming and scrolling
//        graphview.getViewport().setScalable(false);
//        graphview.getViewport().setScrollable(true);
//        graphview.getViewport().setScalableY(false);
//        graphview.getViewport().setScrollableY(true);
//
//        LineGraphSeries<DataPoint> series = new LineGraphSeries<>(new DataPoint[]{
//                new DataPoint(0, 1),
//                new DataPoint(1, 2),
//                new DataPoint(2, 3)
//        });
//
//        GridLabelRenderer gridLabel = graphview.getGridLabelRenderer();
//        gridLabel.setHorizontalAxisTitle("Operational Intensity (FLOPS/byte)");
//        gridLabel.setVerticalAxisTitle("Performance (GFLOPS)");
//
//        graphview.setTitle("GPU Roofline");
//
//        series.setTitle("GPU Roofline");
//        series.setBackgroundColor(Color.GRAY);
//        series.setColor(Color.BLACK);
//        series.setDrawDataPoints(false);
//        series.setDataPointsRadius(10);
//        series.setThickness(7);
//
//        graphview.addSeries(series);
//
//        Log.d(TAG, "Finished drawing graphview for GPU Roofline");
//    }

    void printOpenGLInfo() { //fixme, if this works i can get rid of the GPU JNI calls.
        IntBuffer work_grp_info = IntBuffer.allocate(3);

        // maximum global work group (total work in a dispatch)
        GLES31.glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_COUNT, work_grp_info);
        Log.i(TAG, "max global (total) work group size  x:" + work_grp_info.get(0)
                + " y: " + work_grp_info.get(1)
                + " z: " + work_grp_info.get(2));

        // maximum local work group (one shader's slice)
        GLES31.glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_SIZE, work_grp_info);
        Log.i(TAG, "max local (in one shader) work group sizes x:" + work_grp_info.get(0)
                + " y: " + work_grp_info.get(1)
                + " z: " + work_grp_info.get(2));

        IntBuffer work_grp_invo = IntBuffer.allocate(1);
        // maximum compute shader invocations (x * y * z)
        GLES31.glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, work_grp_invo);
        Log.i(TAG, "max computer shader invocations: " + work_grp_invo.get(0));
    }

    /**
     * Setup a button to trigger the activation of the roofline process
     *
     * @param rootView
     */
    void setupButton(final View rootView) {
        // Register a callback on the toggle button to turn ON/OFF cpu activity
        final ToggleButton button = rootView.findViewById(R.id.toggle_runRoofline);
        button.setTextOn("Busy!");
        button.setTextOff("Run");
        button.setChecked(false); // set the current state of a toggle button

        button.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {

            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {

                if (isChecked) { // enabled
                    SeekBar seekBar_maxWorkGroupCount = rootView.findViewById(R.id.seekBar_maxWorkGroupCount);
                    SeekBar seekBar_maxWorkGroupSize = rootView.findViewById(R.id.seekBar_maxWorkGroupSize);
                    SeekBar seekBar_opIntensity = rootView.findViewById(R.id.seekBar_opIntensity);
                    SeekBar seekBar_memSize = rootView.findViewById(R.id.seekBar_maxMemory);

                    int nGroups = 1 << seekBar_maxWorkGroupCount.getProgress();   // 2^maxGroups
                    int nThreads = 1 << seekBar_maxWorkGroupSize.getProgress(); // 2^maxThreads
                    int nFlops = 1 << seekBar_opIntensity.getProgress();          // 2^flops
                    int memSize = (1 << seekBar_memSize.getProgress()) * MiB; // MB

                    GPURooflineAsync(nGroups, nThreads, nFlops, memSize);

                    button.setChecked(false);
                }
            }
        });
    }

    public void GPURooflineAsync(int nGroups, int nThreads, int nFlops, int memSize) {
        GPURooflineAsyncTask rooflineATask = new GPURooflineAsyncTask(getActivity(),
                nGroups, nThreads, nFlops, memSize);
        rooflineATask.execute();
    }

    private void setupMemAllocSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxMemory);

        TextView textView = rootView.findViewById(R.id.textView_maxMemory);
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
                TextView textView = rootView.findViewById(R.id.textView_maxMemory);
                textView.setText("Memory Alloc. (" + Long.toString(1 << mSeekBar.getProgress()) + " MB)");
            }
        });
    }

    private void setupOpIntensitySlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_opIntensity);

        mSeekBar.setProgress(mSeekBar.getMax()); //fixme: use setMin() but that's not available till API level 26

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

    private void setupMaxWorkGroupSizeSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxWorkGroupSize);

        // determine the maximum number of threads per work group
        GPU_Initialize();
        int maxThreads = GPU_MaxWorkGroupSize(0);
        GPU_Finish();

        mSeekBar.setMax((int) Math.ceil(log(maxThreads, 2)));
        mSeekBar.setProgress((int) Math.ceil(log(256, 2)));

        TextView textView = rootView.findViewById(R.id.txtBox_maxThreads);
        textView.setText("Work Group Size (" + Integer.toString(1 << mSeekBar.getProgress()) + ")"); // increment by 1 since bar starts from 0.

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
                TextView textView = rootView.findViewById(R.id.txtBox_maxThreads);
                int maxThreads = (int) Math.pow(2, mSeekBar.getProgress());
                textView.setText("Work Group Size (" + Integer.toString(maxThreads) + ")");
            }
        });
    }

    private void setupMaxWorkGroupCountSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxWorkGroupCount);

        GPU_Initialize();
        int maxGroups = GPU_MaxWorkGroupCount(0);
        GPU_Finish();

        mSeekBar.setMax((int) Math.ceil(log(maxGroups, 2)));
        mSeekBar.setProgress((int) Math.ceil(log(1024, 2)));

        TextView textView = rootView.findViewById(R.id.txtBox_maxWorkGroups);
        textView.setText("Work Group Count (" + Integer.toString(1 << mSeekBar.getProgress()) + ")"); // increment by 1 since bar starts from 0.

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
                TextView textView = rootView.findViewById(R.id.txtBox_maxWorkGroups);
                int maxGroups = (int) Math.pow(2, mSeekBar.getProgress());
                textView.setText("Work Group Count (" + Integer.toString(maxGroups) + ")");
            }
        });
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        Log.d(TAG, "onDestryView being called to terminate openGL.");
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_gpu_roofline, container, false);

//        drawGraph(rootView);

        setupMaxWorkGroupCountSlider(rootView);
        setupMaxWorkGroupSizeSlider(rootView);
        setupOpIntensitySlider(rootView);
        setupMemAllocSlider(rootView);

        setupButton(rootView);

        slider = rootView.findViewById(R.id.carouselView);
        slidePrompt = rootView.findViewById(R.id.swipe_prompt);
        edgeLogo = rootView.findViewById(R.id.edge_logo);
        return rootView;
    }

    private void setupSlider() {
        ImageListener imageListener = new ImageListener() {
            @Override
            public void setImageForPosition(int position, ImageView imageView) {
                String filename = "";
                switch (position) {
                    case 0:
                    default:
                        filename = "/sdcard/GPURoofline/roofline.png";
                        break;
                    case 1:
                        filename = "/sdcard/GPURoofline/bandwidth.png";
                        break;
                }
                displayGraph(filename, imageView);
            }
        };
        slider.setImageListener(imageListener);
        slider.setPageCount(2);
        slider.setVisibility(View.VISIBLE);
        slidePrompt.setVisibility(View.VISIBLE);
        edgeLogo.setVisibility(View.GONE);
    }

    public void displayGraph(String filename, ImageView view) {
        File file = new File(filename);
        if (file.exists()) {
            Bitmap myBitmap = BitmapFactory.decodeFile(file.getAbsolutePath());
            view.setImageBitmap(myBitmap);
        }
    }
    class GPURooflineAsyncTask extends AsyncTask<Void, String, String> {
        final int threads, groups, flops, mem;
        AssetManager mgr;
        private AsyncTask<Void, String, String> currTask = null;


        public GPURooflineAsyncTask(Activity activity, int nGroups, int nThreads, int maxFlops, int memSize) {
            super();

            this.groups = nGroups;
            this.threads = nThreads;
            this.flops = maxFlops;
            this.mgr = activity.getAssets();
            this.mem = memSize;

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
            File root = Environment.getExternalStorageDirectory();
            File odir = new File(root.getAbsolutePath() + "/" + gResultsDir + "/");
            deleteDirectory(odir);
            Log.i(TAG, "Deleting the " + odir + " output directory.");

            // storage for updating progress of the work to progress bar
            int progressCounter = 0;
            final int progressTotal = (log(flops, 2) + 1)
                    * (log(threads, 2) + 1)
                    * (log(groups, 2) + 1);

            int g = groups;
            int f = flops;
            int t = threads;
            GPU_Initialize();
            if (g == 65536) { // fixme: make this more generic.
                g--;
            }
            String strCurrProgress = String.valueOf(Math.round(100.0 * progressCounter / progressTotal));
            String message = "Running " + f + " FLOPS/byte with " + "<" + g + ", " + t + ">.";
            publishProgress(strCurrProgress, message);
            Log.i(TAG, message);

            String output = GPU_Execute(mgr, g, t, f, mem);
            Log.d(TAG, "Finished " + message);

            if (isStorageAvailable(EXT_STORAGE_RW)) {
                message = "Writing results to storage.";
                publishProgress(strCurrProgress, message);

                String ofilename = "groups-" + g + "_threads-" + t + "_flops-" + f + ".gables";
                writeToSDFile(odir, ofilename, output);
            } else {
                Log.e(TAG, "Storage is not available for RW or is unavailable.");
            }
//            // we've made progress so update
//            progressCounter++;
            GPU_Finish();
            Log.i(TAG, "Finished doing all the sweeps.");

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
            new GablesPython().processGPURoofline();
            setupSlider();
        }
    }
}

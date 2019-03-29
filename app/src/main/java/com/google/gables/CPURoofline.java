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

import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.AxisBase;
import com.github.mikephil.charting.components.Description;
import com.github.mikephil.charting.components.XAxis;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import com.github.mikephil.charting.formatter.IAxisValueFormatter;
import com.github.mikephil.charting.formatter.IndexAxisValueFormatter;
import com.github.mikephil.charting.formatter.ValueFormatter;
import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.GridLabelRenderer;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import gables.gables_processor.GablesPython;

import static com.google.gables.Roofline.CPU_Execute;
import static com.google.gables.Utils.ExtStoragePermissions_t.EXT_STORAGE_RW;
import static com.google.gables.Utils.MiB;
import static com.google.gables.Utils.deleteDirectory;
import static com.google.gables.Utils.getNumberOfCores;
import static com.google.gables.Utils.isStorageAvailable;
import static com.google.gables.Utils.log;

public class CPURoofline extends Fragment {
    private static final String TAG = CPURoofline.class.getName();
    private LineChart chart;
    private ProgressDialog gProcessDialog;
    private String gResultsDir = "CPURoofline";

    @Override
    public void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
    }

    void drawGraph(float[][] data) {
        LineData lineData = new LineData();
        float[] x = data[2];
        for(int i = 2; i <= 4; i++) {
            float[] ys = data[i];
            List<Entry> entries = new ArrayList<Entry>();

            for (int j = 0; j < ys.length; j++) {
                entries.add(new Entry(scaleCbr(x[j]), scaleCbr(ys[j])));
            }
            LineDataSet dataSet = null;
            switch (i) {
                case 2:
                    // DRAM
                    dataSet = new LineDataSet(entries, "DRAM");
                    dataSet.setColor(Color.RED);
                    break;

                case 3:
                    // L1
                    dataSet = new LineDataSet(entries, "L1");
                    dataSet.setColor(Color.GREEN);
                    break;

                case 4:
                    // L2
                    dataSet = new LineDataSet(entries, "L2");
                    dataSet.setColor(Color.BLUE);
                    break;
            }
            dataSet.setDrawCircles(false);
            lineData.addDataSet(dataSet);
        }
        Log.d("GRAPH", "Drawing graph");
        chart.getAxisRight().setEnabled(false);
        chart.getXAxis().setPosition(XAxis.XAxisPosition.BOTTOM);
//        chart.getAxisLeft().setValueFormatter(new IAxisValueFormatter() {
//            @Override
//            public String getFormattedValue(float value, AxisBase axis) {
//                DecimalFormat mFormat;
//                mFormat = new DecimalFormat("##.###");
//                return "10^" + mFormat.format(unScaleCbr(value));
//            }
//        });
//        chart.getXAxis().setValueFormatter(new ValueFormatter() {
//            @Override
//            public String getFormattedValue(float value, AxisBase axis) {
//                DecimalFormat mFormat;
//                mFormat = new DecimalFormat("##.###");
//                return "10^" + mFormat.format(unScaleCbr(value));
//            }
//        });
        chart.setDescription(new Description());
        chart.setData(lineData);
        chart.invalidate();
    }

    private float scaleCbr(double cbr) {
        return (float)(Math.log10(cbr));
    }

    private float unScaleCbr(double cbr) {
        double calcVal = Math.pow(10, cbr);
        return (float)(calcVal);
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
                    final SeekBar seekBar_mem = rootView.findViewById(R.id.seekBar_threadMem);
                    final SeekBar seekBar_cpu = rootView.findViewById(R.id.seekBar_maxThreads);
                    final SeekBar seekBar_opi = rootView.findViewById(R.id.seekBar_opIntensity);

                    final int maxMemory = (1 << seekBar_mem.getProgress()) * MiB; // converting to bytes
                    final int nThreads = seekBar_cpu.getProgress() + 1; // remember that seekbar starts at 0
                    final int nFlops = (1 << seekBar_opi.getProgress()); // remember that seekbar starts at 0

                    final Switch neonSwitch = rootView.findViewById(R.id.switch_neonMode);
                    final boolean neonMode = neonSwitch.isChecked();

                    CPURoofline(nThreads, maxMemory, nFlops, neonMode);

                    button.setChecked(false);
                }
            }
        });
    }

    public void RooflineCmdline() {
        File outputFile = new File("/data/data/com.google.gables/pixel.gables");
        try {
            ProcessBuilder pb = new ProcessBuilder("/data/data/com.google.gables/gables");
            pb.redirectOutput(outputFile);

            Log.i(TAG, "native started...");
            Process process = pb.start();
            BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line;
            while ((line = reader.readLine()) != null) {
                Log.i(TAG, line);
            }
            process.waitFor();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        Log.i(TAG, "native finished...");
    }

    private void setupCPUSlider(final View rootView) {
        final SeekBar mSeekBar = rootView.findViewById(R.id.seekBar_maxThreads);

        int nCores = getNumberOfCores();
        mSeekBar.setProgress(nCores);
        mSeekBar.setMax(nCores - 1); // we start from 0, but we display +1

        TextView textView = rootView.findViewById(R.id.txtBox_maxThreads);
        textView.setText("Thread Count (" + Long.toString(mSeekBar.getProgress() + 1) + " Cores)"); // increment by 1 since bar starts from 0.

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
        final Switch neonSwitch = rootView.findViewById(R.id.switch_neonMode);
        final boolean neonMode = neonSwitch.isChecked();

        neonSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                TextView textView = rootView.findViewById(R.id.txtBox_neonSwitch);
                if (isChecked) {
                    textView.setText("NEON Vectorization (Enabled)");
                } else {
                    //fixme: check to see how we can disable this for hardcore real.
                    textView.setText("NEON Vectorization (Disabled)");
                }
            }
        });
    }

    private void setupSliders(View rootView) {
        setupMemorySlider(rootView);
        setupCPUSlider(rootView);
        setupOpIntensity(rootView);
        setupSwitch(rootView);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View rootView = inflater.inflate(R.layout.fragment_cpu_roofline, container, false);
        chart = rootView.findViewById(R.id.graph);
        setupSliders(rootView);
        setupButton(rootView);
        drawGraph(new GablesPython().processCPURoofline());
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

    public void CPURoofline(int nThreads, int maxMemory, int nFlops, boolean neon) {
        CPURooflineAsync rooflineATask = new CPURooflineAsync(getActivity(),
                nThreads, maxMemory, nFlops, neon);

        rooflineATask.execute();
    }

    class CPURooflineAsync extends AsyncTask<Void, String, String> {
        final int threads, mem, flops;
        final boolean neon;
        private AsyncTask<Void, String, String> currTask = null;

        public CPURooflineAsync(Activity activity, int nThreads, int threadMem, int maxFlops, boolean neon) {
            super();

            this.threads = nThreads;
            this.mem = threadMem;
            this.flops = maxFlops;
            this.neon = neon;

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

            // storage for updating progress of the work to progress bar
            int progressCounter = 0;
            final int progressTotal = (log(flops, 2) + 1) * threads;

            stop:
            // label to break out of inner loop if "cancel" was hit on progress bar.
            for (int i = 1; i <= flops; i = i * 2) {
                for (int j = threads; j <= threads; j++) { //FIXME: Fix this debug loop
                    String strCurrProgress = String.valueOf(Math.round(100.0 * progressCounter / progressTotal));
                    String message = "Running " + i + " FLOPS/byte with " + j + " threads.";
                    publishProgress(strCurrProgress, message);
                    publishProgress(strCurrProgress, message);
                    Log.i(TAG, "Starting " + i + " FLOPS/byte with " + j + " threads.");

                    String output = CPU_Execute(mem, j, i, neon);

                    generatePlotData(processRawData(output));

                    if (isCancelled()) {
                        message = "Cancelling...";
                        publishProgress(strCurrProgress, message);
                        Log.i(TAG, "Terminating out of the loop.");

                        break stop;
                    }

                    if (isStorageAvailable(EXT_STORAGE_RW)) {
                        message = "Writing results to storage.";
                        publishProgress(strCurrProgress, message);

                        String ofilename = "threads-" + j + "_flops-" + i + "_neon-" + neon + ".gables";
                        writeToSDFile(odir, ofilename, output);
                    } else {
                        Log.e(TAG, "Storage is not available for RW or is unavailable.");
                    }

                    // we've made progress so update
                    progressCounter++;
                }
            }

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
            drawGraph(new GablesPython().processCPURoofline());
        }

        void generatePlotData(List<CPUDataPoint> values) {
            List<Double> gflops = new ArrayList<>();
            List<Double> flopsPerByte = new ArrayList<>();
            for (CPUDataPoint value : values) {
                gflops.add(value.getTotalFlops()/value.getTotalSeconds());
                flopsPerByte.add(value.getTotalFlops() / value.getTotalBytes());
            }
            System.out.println(gflops.toString());
            System.out.println(flopsPerByte.toString());
        }

        List<CPUDataPoint> processRawData(String data) {
            List<String> rows = Arrays.asList(data.trim().split("\\s*\\n\\s*"));
            List<CPUDataPoint> points = new ArrayList<>();
            for (String row : rows) {
                if (row.contains("META_DATA")) {
                    break;
                } else {
                    points.add(CPUDataPoint.parseCPUDataPoint(row));
                }
            }
            return points;
        }
    }

    static class CPUDataPoint {

        double distinctBytes;
        double nTrials;
        double totalSeconds;
        double totalBytes;
        double totalFlops;

        public static CPUDataPoint parseCPUDataPoint(String input) {
            String[] values = input.split("\\s+");
            CPUDataPoint point = new CPUDataPoint();
            point.setDistinctBytes(Double.valueOf(values[0]));
            point.setnTrials(Double.valueOf(values[1]));
            point.setTotalSeconds(Double.valueOf(values[2]));
            point.setTotalBytes(Double.valueOf(values[3]));
            point.setTotalFlops(Double.valueOf(values[4]));
            return point;
        }

        public double getDistinctBytes() {
            return distinctBytes;
        }

        public void setDistinctBytes(double distinctBytes) {
            this.distinctBytes = distinctBytes;
        }

        public double getnTrials() {
            return nTrials;
        }

        public void setnTrials(double nTrials) {
            this.nTrials = nTrials;
        }

        public double getTotalSeconds() {
            return totalSeconds;
        }

        public void setTotalSeconds(double totalSeconds) {
            this.totalSeconds = totalSeconds;
        }

        public double getTotalBytes() {
            return totalBytes;
        }

        public void setTotalBytes(double totalBytes) {
            this.totalBytes = totalBytes;
        }

        public double getTotalFlops() {
            return totalFlops;
        }

        public void setTotalFlops(double totalFlops) {
            this.totalFlops = totalFlops;
        }
    }
}

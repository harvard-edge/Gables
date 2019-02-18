# Step by step instructions on generating roofline plots

## Prerequisites

Ensure that you have followed the steps in the [README.md](README.md) to build and install the APK onto a device of your choosing. You should now be able to open the Gables app.

In this example we will be generating a CPU rooline plot, but the process for generating a GPU roofline is very similar.

## Collecting data

Open the Gables app and if necessary select yes on the prompt asking for permission to read and write files. Naviate to the `CPU ROOFLINE` tab of the app and select the test parameters. Once this has been done press run to perform the test. Once the test completes a number of files will have been written to a folder called `CPURoofline` in the `Internal Storage` directory of the device.

## Extracting Data

You should transfer all of the files from the `CPURoofline` folder on the phone to a computer. You can do this in a couple of ways:

1. Use a file manager app to browse to the folder on the phone, highlight the files and then press `share` to share them with yourself.
2. Use [adb](https://developer.android.com/studio/command-line/adb) to extract the files. If you already have an Android device connected with USB debugging enabled, you can use the `abd` tool (located in the Android SDK) to extract the files to a folder on your computer called CPURoofline using the following command: `adb pull /sdcard/CPURoofline/ .`

## Processing Data

The app currently uses a separate Python2 script to process the files generated when running a test in order to produce the roofline plots. The path to this Python script is `Gables/app/utils/plotting/scripts/gables.py`. In order to generate a plot from the files extracted in the previous section you can use the script as follows:

`python gables.py -d [DIRECTORY_CONTAINING_EXTRACTED_FILES]`

This will produce a file `roofline.gnu` in the output directory, which can be viewed using the [gnuplot](http://www.gnuplot.info/) tool using the following command:

`gnuplot gnuplots/roofline.gnu -p`

package com.google.gables;

import android.os.Build;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import static com.google.gables.Utils.ExtStoragePermissions_t.EXT_STORAGE_AV;
import static com.google.gables.Utils.ExtStoragePermissions_t.EXT_STORAGE_RD;
import static com.google.gables.Utils.ExtStoragePermissions_t.EXT_STORAGE_RW;

public class Utils {

    public static final int KiB = 1024;
    public static final int MiB = KiB * 1024;
    public static final int GiB = MiB * 1024;
    private static final String TAG = CPURoofline.class.getName();

    public static int getNumberOfCores() {
        int nCores = -1;
        if (Build.VERSION.SDK_INT >= 17) {
            nCores = Runtime.getRuntime().availableProcessors();
        } else {
            throw new UnsupportedOperationException("Not yet implemented");
        }

        return nCores;
    }

    public static int log(final int x, final int base) {
        return (int) (Math.log(x) / Math.log(base));
    }

    public static Boolean isStorageAvailable(ExtStoragePermissions_t typeCheck) {
        boolean mExternalStorageAvailable = false;
        boolean mExternalStorageWriteable = false;
        String state = Environment.getExternalStorageState();

        if (Environment.MEDIA_MOUNTED.equals(state)) {
            // Can read and write the media
            mExternalStorageAvailable = mExternalStorageWriteable = true;
        } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
            // Can only read the media
            mExternalStorageAvailable = true;
            mExternalStorageWriteable = false;
        } else {
            // Can't read or write
            mExternalStorageAvailable = mExternalStorageWriteable = false;
        }
        Log.i("", "\n\nExternal Media: readable="
                + mExternalStorageAvailable + " writable=" + mExternalStorageWriteable);

        if (typeCheck == EXT_STORAGE_RW)
            return (mExternalStorageAvailable && mExternalStorageWriteable);
        else if (typeCheck == EXT_STORAGE_RD)
            return (mExternalStorageAvailable && !mExternalStorageWriteable);
        else if (typeCheck == EXT_STORAGE_AV)
            return mExternalStorageAvailable;

        return false;
    }

    public static boolean deleteDirectory(File path) {
        if (path.exists()) {
            File[] files = path.listFiles();
            if (files == null) {
                return true;
            }
            for (int i = 0; i < files.length; i++) {
                if (files[i].isDirectory()) {
                    deleteDirectory(files[i]);
                } else {
                    files[i].delete();
                }
            }
        }
        return (path.delete());
    }

    public static void writeToSDFile(File dir, String filename, String ostr) {

        boolean success = true;
        if (!dir.exists()) {
            success = dir.mkdir(); // FIXME: if this fails, then you might have to request permission in app settings.
        }
        if (success) {
            Log.i("writeToSDFile", "Output directory created.");
        } else {
            Log.i("writeToSDFile", "Output directory NOT created!");
        }

        File file = new File(dir, filename);

        try {
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(ostr.getBytes());
            fos.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            Log.i("writeToSDFile", "******* File not found. Did you" +
                    " add a WRITE_EXTERNAL_STORAGE permission to the   manifest?");
        } catch (IOException e) {
            e.printStackTrace();
        }
        Log.i("writeToSDFile", "\n\nFile written to " + file);
    }

    public enum ExtStoragePermissions_t {
        EXT_STORAGE_RD,
        EXT_STORAGE_RW,
        EXT_STORAGE_AV
    }
}
package com.example.hellojni;

import android.app.Activity;
import android.content.res.AssetManager;
import android.util.Log;
import android.widget.TextView;
import android.os.Bundle;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;

public class HelloJni extends Activity {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        String path = this.getApplicationContext().getApplicationInfo().dataDir + "/" + "cacert.pem";
        AssetManager assetManager = this.getAssets();
        try {
            InputStream in = assetManager.open("cacert.pem");
            OutputStream out = new FileOutputStream(path);
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            in.close();
            out.flush();
            out.close();
            Log.v("APP", "Cert copied to \"" + path + "\"");

            this.testCURL(path);
        } catch(Exception e) {
            Log.e("APP", e.getMessage());
        }

        TextView tv = new TextView(this);
        try {
            tv.setText(stringFromJNI("POL".getBytes("UTF-8")));
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        setContentView(tv);
    }

    public static void log(String s){
        Log.i("NATIVE", s);
    }

    // Implemented in JNI
    public native String stringFromJNI(byte[] arg);
    public native void testCURL(String s);

    static {
        System.loadLibrary("hello-jni");
    }
}

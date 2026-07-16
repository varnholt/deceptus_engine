package com.deceptus.webview;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;

import org.mozilla.geckoview.GeckoRuntime;
import org.mozilla.geckoview.GeckoRuntimeSettings;
import org.mozilla.geckoview.GeckoSession;
import org.mozilla.geckoview.GeckoView;

import java.io.IOException;

import fi.iki.elonen.NanoHTTPD;

/**
 * Full-screen, landscape host for the Deceptus WASM build running inside GeckoView.
 *
 * GeckoView (Mozilla's Firefox engine) supports crossOriginIsolated / SharedArrayBuffer, unlike the
 * stock Android WebView, so the pthreads WASM build can run unchanged. The game is served by an
 * in-app loopback HTTP server ({@link AssetHttpServer}) that sends the required cross-origin
 * isolation headers; GeckoView loads it from http://127.0.0.1:PORT/ (a secure context).
 */
public class MainActivity extends Activity {

    private static final String TAG = "DeceptusWebView";
    private static final int SERVER_PORT = 8787;

    private AssetHttpServer _assetHttpServer;
    private GeckoSession _geckoSession;
    private GeckoView _geckoView;

    // GeckoRuntime is a process-wide singleton; create it once.
    private static GeckoRuntime _geckoRuntime;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        enableImmersiveMode();

        // Start the loopback asset server before loading anything.
        _assetHttpServer = new AssetHttpServer(getAssets(), SERVER_PORT);
        try {
            _assetHttpServer.start(NanoHTTPD.SOCKET_READ_TIMEOUT, false);
        } catch (IOException exception) {
            Log.e(TAG, "failed to start asset http server", exception);
        }

        if (_geckoRuntime == null) {
            final GeckoRuntimeSettings settings = new GeckoRuntimeSettings.Builder()
                    .consoleOutput(true)          // pipe page console.* to logcat (tag "GeckoConsole")
                    .remoteDebuggingEnabled(true) // allow about:debugging from desktop Firefox
                    .build();
            _geckoRuntime = GeckoRuntime.create(this, settings);
        }

        _geckoSession = new GeckoSession();
        _geckoSession.open(_geckoRuntime);

        _geckoView = new GeckoView(this);
        _geckoView.setSession(_geckoSession);
        setContentView(_geckoView);

        _geckoSession.loadUri("http://127.0.0.1:" + SERVER_PORT + "/index.html");
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            enableImmersiveMode();
        }
    }

    @Override
    protected void onDestroy() {
        if (_geckoSession != null) {
            _geckoSession.close();
            _geckoSession = null;
        }
        if (_assetHttpServer != null) {
            _assetHttpServer.stop();
            _assetHttpServer = null;
        }
        super.onDestroy();
    }

    private void enableImmersiveMode() {
        final View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
    }
}

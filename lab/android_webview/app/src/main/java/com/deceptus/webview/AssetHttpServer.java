package com.deceptus.webview;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

import fi.iki.elonen.NanoHTTPD;

/**
 * Minimal loopback HTTP server that streams the bundled Emscripten build (deceptus.js / .wasm /
 * .data + the touch shell) out of app assets.
 *
 * It exists so GeckoView can load the game from http://127.0.0.1:PORT/ — a loopback origin, which
 * Gecko treats as a secure context — while every response carries the real
 * Cross-Origin-Opener-Policy / Cross-Origin-Embedder-Policy / Cross-Origin-Resource-Policy headers.
 * Those headers, delivered from a genuine HTTP response, make Gecko report crossOriginIsolated ===
 * true, which the SharedArrayBuffer/pthreads WASM build requires. (Android's WebView refuses
 * cross-origin isolation regardless of how the headers are delivered, which is why this lab uses
 * GeckoView instead.)
 */
public class AssetHttpServer extends NanoHTTPD {

    private final AssetManager _assetManager;

    public AssetHttpServer(AssetManager assetManager, int port) {
        // Bind to loopback only — nothing outside the device can reach it.
        super("127.0.0.1", port);
        _assetManager = assetManager;
    }

    @Override
    public Response serve(IHTTPSession session) {
        String requestedPath = session.getUri();
        if (requestedPath == null || requestedPath.isEmpty() || requestedPath.equals("/")) {
            requestedPath = "/index.html";
        }
        // Strip the leading slash and map onto assets/game/.
        final String assetPath = "game/" + requestedPath.substring(1);

        try {
            final String mimeType = mimeTypeFor(assetPath);
            InputStream assetStream;
            long contentLength;

            try {
                // Uncompressed assets (deceptus.data / .wasm — see noCompress in build.gradle) can be
                // streamed straight from the APK with a known length, avoiding loading 117 MB into RAM.
                final AssetFileDescriptor assetFileDescriptor = _assetManager.openFd(assetPath);
                contentLength = assetFileDescriptor.getLength();
                assetStream = assetFileDescriptor.createInputStream();
            } catch (IOException compressedAsset) {
                // Compressed assets (index.html / deceptus.js) can't be opened by fd; read them fully.
                final byte[] assetBytes = readAll(_assetManager.open(assetPath));
                contentLength = assetBytes.length;
                assetStream = new ByteArrayInputStream(assetBytes);
            }

            final Response response = newFixedLengthResponse(Response.Status.OK, mimeType, assetStream, contentLength);
            addCrossOriginIsolationHeaders(response);
            return response;
        } catch (IOException notFound) {
            final Response response = newFixedLengthResponse(
                    Response.Status.NOT_FOUND, "text/plain", "not found: " + requestedPath);
            addCrossOriginIsolationHeaders(response);
            return response;
        }
    }

    private static void addCrossOriginIsolationHeaders(Response response) {
        response.addHeader("Cross-Origin-Opener-Policy", "same-origin");
        response.addHeader("Cross-Origin-Embedder-Policy", "require-corp");
        response.addHeader("Cross-Origin-Resource-Policy", "same-origin");
        // The APK is the source of truth; never let a stale artifact be cached.
        response.addHeader("Cache-Control", "no-store");
    }

    private static byte[] readAll(InputStream inputStream) throws IOException {
        try (InputStream stream = inputStream) {
            final ByteArrayOutputStream buffer = new ByteArrayOutputStream();
            final byte[] chunk = new byte[16384];
            int bytesRead;
            while ((bytesRead = stream.read(chunk)) != -1) {
                buffer.write(chunk, 0, bytesRead);
            }
            return buffer.toByteArray();
        }
    }

    private static String mimeTypeFor(String path) {
        final String lowerCasePath = path.toLowerCase(Locale.ROOT);
        if (lowerCasePath.endsWith(".html")) {
            return "text/html";
        }
        if (lowerCasePath.endsWith(".js")) {
            return "text/javascript";
        }
        if (lowerCasePath.endsWith(".wasm")) {
            // application/wasm is required for WebAssembly.instantiateStreaming to accept the module.
            return "application/wasm";
        }
        if (lowerCasePath.endsWith(".css")) {
            return "text/css";
        }
        if (lowerCasePath.endsWith(".json")) {
            return "application/json";
        }
        // .data and everything else stream as opaque binary.
        return "application/octet-stream";
    }
}

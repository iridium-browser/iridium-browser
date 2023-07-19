/*
 *  Copyright 2022 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package org.webrtc;

import android.os.Handler;
import android.os.HandlerThread;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import org.webrtc.EglBase.EglConnection;

/** EGL graphics thread that allows multiple clients to share the same underlying EGLContext. */
public class EglThread {
  /** Callback for externally managed reference count. */
  public interface ReleaseMonitor {
    /**
     * Called by EglThread when a client releases its reference. Returns true when there are no more
     * references and resources should be released.
     */
    boolean onRelease(EglThread eglThread);
  }

  public static EglThread create(@Nullable ReleaseMonitor releaseMonitor,
      @Nullable final EglBase.Context sharedContext, final int[] configAttributes) {
    final HandlerThread renderThread = new HandlerThread("EglThread");
    renderThread.start();
    Handler handler = new Handler(renderThread.getLooper());

    // If sharedContext is null, then texture frames are disabled. This is typically for old
    // devices that might not be fully spec compliant, so force EGL 1.0 since EGL 1.4 has
    // caused trouble on some weird devices.
    EglConnection eglConnection;
    if (sharedContext == null) {
      eglConnection = EglConnection.createEgl10(configAttributes);
    } else {
      eglConnection = EglConnection.create(sharedContext, configAttributes);
    }

    return new EglThread(
        releaseMonitor != null ? releaseMonitor : eglThread -> true, handler, eglConnection);
  }

  private final ReleaseMonitor releaseMonitor;
  private final Handler handler;
  private final EglConnection eglConnection;

  @VisibleForTesting
  EglThread(ReleaseMonitor releaseMonitor, Handler handler, EglConnection eglConnection) {
    this.releaseMonitor = releaseMonitor;
    this.handler = handler;
    this.eglConnection = eglConnection;
  }

  public void release() {
    if (!releaseMonitor.onRelease(this)) {
      // Thread is still in use, do not release yet.
      return;
    }

    handler.post(eglConnection::release);
    handler.getLooper().quitSafely();
  }

  /**
   * Creates an EglBase instance with the EglThread's EglConnection. This method can be called on
   * any thread, but the returned EglBase instance should only be used on this EglThread's Handler.
   */
  public EglBase createEglBaseWithSharedConnection() {
    return EglBase.create(eglConnection);
  }

  /**
   * Returns the Handler to interact with Gl/EGL on. Callers need to make sure that their own
   * EglBase is current on the handler before running any graphics operations since the EglThread
   * can be shared by multiple clients.
   */
  public Handler getHandler() {
    return handler;
  }
}

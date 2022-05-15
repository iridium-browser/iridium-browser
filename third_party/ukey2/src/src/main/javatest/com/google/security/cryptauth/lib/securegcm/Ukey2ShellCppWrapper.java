// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.security.cryptauth.lib.securegcm;

import com.google.common.io.BaseEncoding;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ProcessBuilder.Redirect;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import javax.annotation.Nullable;

/**
 * A wrapper to execute and interact with the //security/cryptauth/lib/securegcm:ukey2_shell binary.
 *
 * <p>This binary is a shell over the C++ implementation of the UKEY2 protocol, so this wrapper is
 * used to test compatibility between the C++ and Java implementations.
 *
 * <p>The ukey2_shell is invoked as follows:
 *
 * <pre>{@code
 * ukey2_shell --mode=<mode> --verification_string_length=<length>
 * }</pre>
 *
 * where {@code mode={initiator, responder}} and {@code verification_string_length} is a positive
 * integer.
 */
public class Ukey2ShellCppWrapper {
  // The path the the ukey2_shell binary.
  private static final String BINARY_PATH = "build/src/main/cpp/src/securegcm/ukey2_shell";

  // The time to wait before timing out a read or write operation to the shell.
  @SuppressWarnings("GoodTime") // TODO(b/147378611): store a java.time.Duration instead
  private static final long IO_TIMEOUT_MILLIS = 5000;

  public enum Mode {
    INITIATOR,
    RESPONDER
  }

  private final Mode mode;
  private final int verificationStringLength;
  private final ExecutorService executorService;

  @Nullable private Process shellProcess;
  private boolean secureContextEstablished;

  /**
   * @param mode The mode to run the shell in (initiator or responder).
   * @param verificationStringLength The length of the verification string used in the handshake.
   */
  public Ukey2ShellCppWrapper(Mode mode, int verificationStringLength) {
    this.mode = mode;
    this.verificationStringLength = verificationStringLength;
    this.executorService = Executors.newSingleThreadExecutor();
  }

  /**
   * Begins execution of the ukey2_shell binary.
   *
   * @throws IOException
   */
  public void startShell() throws IOException {
    if (shellProcess != null) {
      throw new IllegalStateException("Shell already started.");
    }

    String modeArg = "--mode=" + getModeString();
    String verificationStringLengthArg = "--verification_string_length=" + verificationStringLength;

    final ProcessBuilder builder =
        new ProcessBuilder(BINARY_PATH, modeArg, verificationStringLengthArg);

    // Merge the shell's stderr with the stderr of the current process.
    builder.redirectError(Redirect.INHERIT);

    shellProcess = builder.start();
  }

  /**
   * Stops execution of the ukey2_shell binary.
   *
   * @throws IOException
   */
  public void stopShell() {
    if (shellProcess == null) {
      throw new IllegalStateException("Shell not started.");
    }
    shellProcess.destroy();
  }

  /**
   * @return the handshake message read from the shell.
   * @throws IOException
   */
  public byte[] readHandshakeMessage() throws IOException {
    return readFrameWithTimeout();
  }

  /**
   * Sends the handshake message to the shell.
   *
   * @param message
   * @throws IOException
   */
  public void writeHandshakeMessage(byte[] message) throws IOException {
    writeFrameWithTimeout(message);
  }

  /**
   * Reads the auth string from the shell and compares it with {@code authString}. If verification
   * succeeds, then write "ok" back as a confirmation.
   *
   * @param authString the auth string to compare to.
   * @throws IOException
   */
  public void confirmAuthString(byte[] authString) throws IOException {
    byte[] shellAuthString = readFrameWithTimeout();
    if (!Arrays.equals(authString, shellAuthString)) {
      throw new IOException(
          String.format(
              "Unable to verify auth string: 0x%s != 0x%s",
              BaseEncoding.base16().encode(authString),
              BaseEncoding.base16().encode(shellAuthString)));
    }
    writeFrameWithTimeout("ok".getBytes());
    secureContextEstablished = true;
  }

  /**
   * Sends {@code payload} to be encrypted by the shell. This function can only be called after a
   * handshake is performed and a secure context established.
   *
   * @param payload the data to be encrypted.
   * @return the encrypted message returned by the shell.
   * @throws IOException
   */
  public byte[] sendEncryptCommand(byte[] payload) throws IOException {
    writeFrameWithTimeout(createExpression("encrypt", payload));
    return readFrameWithTimeout();
  }

  /**
   * Sends {@code message} to be decrypted by the shell. This function can only be called after a
   * handshake is performed and a secure context established.
   *
   * @param message the data to be decrypted.
   * @return the decrypted payload returned by the shell.
   * @throws IOException
   */
  public byte[] sendDecryptCommand(byte[] message) throws IOException {
    writeFrameWithTimeout(createExpression("decrypt", message));
    return readFrameWithTimeout();
  }

  /**
   * Requests the session unique value from the shell. This function can only be called after a
   * handshake is performed and a secure context established.
   *
   * @return the session unique value returned by the shell.
   * @throws IOException
   */
  public byte[] sendSessionUniqueCommand() throws IOException {
    writeFrameWithTimeout(createExpression("session_unique", null));
    return readFrameWithTimeout();
  }

  /**
   * Reads a frame from the shell's stdout with a timeout.
   *
   * @return The contents of the frame.
   * @throws IOException
   */
  private byte[] readFrameWithTimeout() throws IOException {
    Future<byte[]> future =
        executorService.submit(
            new Callable<byte[]>() {
              @Override
              public byte[] call() throws Exception {
                return readFrame();
              }
            });

    try {
      return future.get(IO_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
    } catch (InterruptedException | ExecutionException | TimeoutException e) {
      throw new IOException(e);
    }
  }

  /**
   * Writes a frame to the shell's stdin with a timeout.
   *
   * @param contents the contents of the frame.
   * @throws IOException
   */
  private void writeFrameWithTimeout(final byte[] contents) throws IOException {
    Future<?> future =
        executorService.submit(
            new Runnable() {
              @Override
              public void run() {
                try {
                  writeFrame(contents);
                } catch (IOException e) {
                  throw new RuntimeException(e);
                }
              }
            });

    try {
      future.get(IO_TIMEOUT_MILLIS, TimeUnit.MILLISECONDS);
    } catch (InterruptedException | ExecutionException | TimeoutException e) {
      throw new IOException(e);
    }
  }

  /**
   * Reads a frame from the shell's stdout, which has the format:
   *
   * <pre>{@code
   * +---------------------+-----------------+
   * | 4-bytes             | |length| bytes  |
   * +---------------------+-----------------+
   * | (unsigned) length   |     contents    |
   * +---------------------+-----------------+
   * }</pre>
   *
   * @return the contents that were read
   * @throws IOException
   */
  private byte[] readFrame() throws IOException {
    if (shellProcess == null) {
      throw new IllegalStateException("Shell not started.");
    }

    InputStream inputStream = shellProcess.getInputStream();
    byte[] lengthBytes = new byte[4];
    if (inputStream.read(lengthBytes) != lengthBytes.length) {
      throw new IOException("Failed to read length.");
    }

    int length = ByteBuffer.wrap(lengthBytes).order(ByteOrder.BIG_ENDIAN).getInt();
    if (length < 0) {
      throw new IOException("Length too large: " + Arrays.toString(lengthBytes));
    }

    byte[] contents = new byte[length];
    int bytesRead = inputStream.read(contents);
    if (bytesRead != length) {
      throw new IOException("Failed to read entire contents: " + bytesRead + " != " + length);
    }

    return contents;
  }

  /**
   * Writes a frame to the shell's stdin, which has the format:
   *
   * <pre>{@code
   * +---------------------+-----------------+
   * | 4-bytes             | |length| bytes  |
   * +---------------------+-----------------+
   * | (unsigned) length   |     contents    |
   * +---------------------+-----------------+
   * }</pre>
   *
   * @param contents the contents to send.
   * @throws IOException
   */
  private void writeFrame(byte[] contents) throws IOException {
    if (shellProcess == null) {
      throw new IllegalStateException("Shell not started.");
    }

    // The length is big-endian encoded, network byte order.
    long length = contents.length;
    byte[] lengthBytes = new byte[4];
    lengthBytes[0] = (byte) (length >> 32 & 0xFF);
    lengthBytes[1] = (byte) (length >> 16 & 0xFF);
    lengthBytes[2] = (byte) (length >> 8 & 0xFF);
    lengthBytes[3] = (byte) (length >> 0 & 0xFF);

    OutputStream outputStream = shellProcess.getOutputStream();
    outputStream.write(lengthBytes);
    outputStream.write(contents);
    outputStream.flush();
  }

  /**
   * Creates an expression to be processed when a secure connection is established, after the
   * handshake is done.
   *
   * @param command The command to send.
   * @param argument The argument of the command. Can be null.
   * @return the expression that can be sent to the shell.
   * @throws IOException.
   */
  private byte[] createExpression(String command, @Nullable byte[] argument) throws IOException {
    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
    outputStream.write(command.getBytes());
    outputStream.write(" ".getBytes());
    if (argument != null) {
      outputStream.write(argument);
    }
    return outputStream.toByteArray();
  }

  /** @return the mode string to use in the argument to start the ukey2_shell process. */
  private String getModeString() {
    switch (mode) {
      case INITIATOR:
        return "initiator";
      case RESPONDER:
        return "responder";
      default:
        throw new IllegalArgumentException("Uknown mode " + mode);
    }
  }
}

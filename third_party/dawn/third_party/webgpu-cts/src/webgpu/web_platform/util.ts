import { Fixture, SkipTestCase } from '../../common/framework/fixture.js';
import { assert, ErrorWithExtra, raceWithRejectOnTimeout } from '../../common/util/util.js';

declare global {
  interface HTMLMediaElement {
    // Add captureStream() support for HTMLMediaElement from
    // https://w3c.github.io/mediacapture-fromelement/#dom-htmlmediaelement-capturestream
    captureStream(): MediaStream;
  }

  interface HTMLVideoElement {
    requestVideoFrameCallback(
      callback: (now: DOMHighResTimeStamp, metadata: unknown) => void
    ): number;
  }
}

/**
 * Starts playing a video and waits for it to be consumable.
 * Returns a promise which resolves after `callback` (which may be async) completes.
 *
 * @param video An HTML5 Video element.
 * @param callback Function to call when video is ready.
 *
 * Adapted from https://github.com/KhronosGroup/WebGL/blob/main/sdk/tests/js/webgl-test-utils.js
 */
export function startPlayingAndWaitForVideo(
  video: HTMLVideoElement,
  callback: () => unknown | Promise<unknown>
): Promise<void> {
  return raceWithRejectOnTimeout(
    new Promise((resolve, reject) => {
      const callbackAndResolve = () =>
        void (async () => {
          try {
            await callback();
            resolve();
          } catch (ex) {
            reject();
          }
        })();
      if (video.error) {
        reject(
          new ErrorWithExtra('Video.error: ' + video.error.message, () => ({ error: video.error }))
        );
        return;
      }

      video.addEventListener(
        'error',
        event => reject(new ErrorWithExtra('Video received "error" event', () => ({ event }))),
        true
      );

      if ('requestVideoFrameCallback' in video) {
        video.requestVideoFrameCallback(() => {
          callbackAndResolve();
        });
      } else {
        // If requestVideoFrameCallback isn't available, check each frame if the video has advanced.
        const timeWatcher = () => {
          if (video.currentTime > 0) {
            callbackAndResolve();
          } else {
            requestAnimationFrame(timeWatcher);
          }
        };
        timeWatcher();
      }

      video.loop = true;
      video.muted = true;
      video.preload = 'auto';
      video.play().catch(reject);
    }),
    2000,
    'Video never became ready'
  );
}

/**
 * Fire a `callback` when the video reaches a new frame.
 * Returns a promise which resolves after `callback` (which may be async) completes.
 *
 * MAINTENANCE_TODO: Find a way to implement this for browsers without requestVideoFrameCallback as
 * well, similar to the timeWatcher path in startPlayingAndWaitForVideo. If that path is proven to
 * work well, we can consider getting rid of the requestVideoFrameCallback path.
 */
export function waitForNextFrame(
  video: HTMLVideoElement,
  callback: () => unknown | Promise<unknown>
): Promise<void> {
  const { promise, callbackAndResolve } = videoCallbackHelper(
    callback,
    'waitForNextFrame timed out'
  );

  if ('requestVideoFrameCallback' in video) {
    video.requestVideoFrameCallback(() => {
      callbackAndResolve();
    });
  } else {
    throw new SkipTestCase('waitForNextFrame currently requires requestVideoFrameCallback');
  }

  return promise;
}

export async function getVideoFrameFromVideoElement(
  test: Fixture,
  video: HTMLVideoElement
): Promise<VideoFrame> {
  if (video.captureStream === undefined) {
    test.skip('HTMLVideoElement.captureStream is not supported');
  }

  const track: MediaStreamVideoTrack = video.captureStream().getVideoTracks()[0];
  const reader = new MediaStreamTrackProcessor({ track }).readable.getReader();
  const videoFrame = (await reader.read()).value;
  assert(videoFrame !== undefined, 'unable to get a VideoFrame from track 0');
  return videoFrame;
}

/**
 * Helper for doing something inside of a (possibly async) callback (directly, not in a following
 * microtask), and returning a promise when the callback is done.
 * MAINTENANCE_TODO: Use this in startPlayingAndWaitForVideo (and make sure it works).
 */
function videoCallbackHelper(
  callback: () => unknown | Promise<unknown>,
  timeoutMessage: string
): { promise: Promise<void>; callbackAndResolve: () => void } {
  let callbackAndResolve: () => void;

  const promiseWithoutTimeout = new Promise<void>((resolve, reject) => {
    callbackAndResolve = () =>
      void (async () => {
        try {
          await callback(); // catches both exceptions and rejections
          resolve();
        } catch (ex) {
          reject(ex);
        }
      })();
  });
  const promise = raceWithRejectOnTimeout(promiseWithoutTimeout, 2000, timeoutMessage);
  return { promise, callbackAndResolve: callbackAndResolve! };
}

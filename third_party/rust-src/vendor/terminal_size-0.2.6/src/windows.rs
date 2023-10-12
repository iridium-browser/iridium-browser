use super::{Height, Width};
use std::os::windows::io::RawHandle;

/// Returns the size of the terminal defaulting to STDOUT, if available.
///
/// Note that this returns the size of the actual command window, and
/// not the overall size of the command window buffer
pub fn terminal_size() -> Option<(Width, Height)> {
    use windows_sys::Win32::System::Console::{GetStdHandle, STD_OUTPUT_HANDLE};

    let handle = unsafe { GetStdHandle(STD_OUTPUT_HANDLE) as RawHandle };

    terminal_size_using_handle(handle)
}

/// Returns the size of the terminal using the given handle, if available.
///
/// If the given handle is not a tty, returns `None`
pub fn terminal_size_using_handle(handle: RawHandle) -> Option<(Width, Height)> {
    use windows_sys::Win32::Foundation::INVALID_HANDLE_VALUE;
    use windows_sys::Win32::System::Console::{
        GetConsoleScreenBufferInfo, CONSOLE_SCREEN_BUFFER_INFO, COORD, SMALL_RECT,
    };

    // convert between windows_sys::Win32::Foundation::HANDLE and std::os::windows::raw::HANDLE
    let hand = handle as windows_sys::Win32::Foundation::HANDLE;

    if hand == INVALID_HANDLE_VALUE {
        return None;
    }

    let zc = COORD { X: 0, Y: 0 };
    let mut csbi = CONSOLE_SCREEN_BUFFER_INFO {
        dwSize: zc,
        dwCursorPosition: zc,
        wAttributes: 0,
        srWindow: SMALL_RECT {
            Left: 0,
            Top: 0,
            Right: 0,
            Bottom: 0,
        },
        dwMaximumWindowSize: zc,
    };
    if unsafe { GetConsoleScreenBufferInfo(hand, &mut csbi) } == 0 {
        return None;
    }

    let w: Width = Width((csbi.srWindow.Right - csbi.srWindow.Left + 1) as u16);
    let h: Height = Height((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) as u16);
    Some((w, h))
}

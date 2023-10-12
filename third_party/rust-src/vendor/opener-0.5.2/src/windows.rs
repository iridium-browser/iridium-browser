use crate::OpenError;
use std::ffi::OsStr;
use std::os::windows::ffi::OsStrExt;
use std::{io, ptr};
use winapi::ctypes::c_int;
use winapi::um::shellapi::ShellExecuteW;

pub(crate) fn open(path: &OsStr) -> Result<(), OpenError> {
    const SW_SHOW: c_int = 5;

    let path = convert_path(path).map_err(OpenError::Io)?;
    let operation: Vec<u16> = OsStr::new("open\0").encode_wide().collect();
    let result = unsafe {
        ShellExecuteW(
            ptr::null_mut(),
            operation.as_ptr(),
            path.as_ptr(),
            ptr::null(),
            ptr::null(),
            SW_SHOW,
        )
    };
    if result as c_int > 32 {
        Ok(())
    } else {
        Err(OpenError::Io(io::Error::last_os_error()))
    }
}

fn convert_path(path: &OsStr) -> io::Result<Vec<u16>> {
    let mut maybe_result: Vec<u16> = path.encode_wide().collect();
    if maybe_result.iter().any(|&u| u == 0) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "path contains NUL byte(s)",
        ));
    }

    maybe_result.push(0);
    Ok(maybe_result)
}

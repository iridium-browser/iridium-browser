/// This module implements the "container" file format that `measureme` uses for
/// storing things on disk. The format supports storing three independent
/// streams of data: one for events, one for string data, and one for string
/// index data (in theory it could support an arbitrary number of separate
/// streams but three is all we need). The data of each stream is split into
/// "pages", where each page has a small header designating what kind of
/// data it is (i.e. event, string data, or string index), and the length of
/// the page.
///
/// Pages of different kinds can be arbitrarily interleaved. The headers allow
/// for reconstructing each of the streams later on. An example file might thus
/// look like this:
///
/// ```ignore
/// | file header | page (events) | page (string data) | page (events) | page (string index) |
/// ```
///
/// The exact encoding of a page is:
///
/// | byte slice              | contents                                |
/// |-------------------------|-----------------------------------------|
/// | &[0 .. 1]               | page tag                                |
/// | &[1 .. 5]               | page size as little endian u32          |
/// | &[5 .. (5 + page_size)] | page contents (exactly page_size bytes) |
///
/// A page is immediately followed by the next page, without any padding.
use parking_lot::Mutex;
use rustc_hash::FxHashMap;
use std::cmp::min;
use std::convert::TryInto;
use std::error::Error;
use std::fmt::Debug;
use std::fs;
use std::io::Write;
use std::sync::Arc;

const MAX_PAGE_SIZE: usize = 256 * 1024;

/// The number of bytes we consider enough to warrant their own page when
/// deciding whether to flush a partially full buffer. Actual pages may need
/// to be smaller, e.g. when writing the tail of the data stream.
const MIN_PAGE_SIZE: usize = MAX_PAGE_SIZE / 2;

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum PageTag {
    Events = 0,
    StringData = 1,
    StringIndex = 2,
}

impl std::convert::TryFrom<u8> for PageTag {
    type Error = String;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(PageTag::Events),
            1 => Ok(PageTag::StringData),
            2 => Ok(PageTag::StringIndex),
            _ => Err(format!("Could not convert byte `{}` to PageTag.", value)),
        }
    }
}

/// An address within a data stream. Each data stream has its own address space,
/// i.e. the first piece of data written to the events stream will have
/// `Addr(0)` and the first piece of data written to the string data stream
/// will *also* have `Addr(0)`.
//
// TODO: Evaluate if it makes sense to add a type tag to `Addr` in order to
//       prevent accidental use of `Addr` values with the wrong address space.
#[derive(Clone, Copy, Eq, PartialEq, Debug)]
pub struct Addr(pub u32);

impl Addr {
    pub fn as_usize(self) -> usize {
        self.0 as usize
    }
}

#[derive(Debug)]
pub struct SerializationSink {
    shared_state: SharedState,
    data: Mutex<SerializationSinkInner>,
    page_tag: PageTag,
}

pub struct SerializationSinkBuilder(SharedState);

impl SerializationSinkBuilder {
    pub fn new_from_file(file: fs::File) -> Result<Self, Box<dyn Error + Send + Sync>> {
        Ok(Self(SharedState(Arc::new(Mutex::new(
            BackingStorage::File(file),
        )))))
    }

    pub fn new_in_memory() -> SerializationSinkBuilder {
        Self(SharedState(Arc::new(Mutex::new(BackingStorage::Memory(
            Vec::new(),
        )))))
    }

    pub fn new_sink(&self, page_tag: PageTag) -> SerializationSink {
        SerializationSink {
            data: Mutex::new(SerializationSinkInner {
                buffer: Vec::with_capacity(MAX_PAGE_SIZE),
                addr: 0,
            }),
            shared_state: self.0.clone(),
            page_tag,
        }
    }
}

/// The `BackingStorage` is what the data gets written to. Usually that is a
/// file but for testing purposes it can also be an in-memory vec of bytes.
#[derive(Debug)]
enum BackingStorage {
    File(fs::File),
    Memory(Vec<u8>),
}

impl Write for BackingStorage {
    #[inline]
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        match *self {
            BackingStorage::File(ref mut file) => file.write(buf),
            BackingStorage::Memory(ref mut vec) => vec.write(buf),
        }
    }

    fn flush(&mut self) -> std::io::Result<()> {
        match *self {
            BackingStorage::File(ref mut file) => file.flush(),
            BackingStorage::Memory(_) => {
                // Nothing to do
                Ok(())
            }
        }
    }
}

/// This struct allows to treat `SerializationSink` as `std::io::Write`.
pub struct StdWriteAdapter<'a>(&'a SerializationSink);

impl<'a> Write for StdWriteAdapter<'a> {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        self.0.write_bytes_atomic(buf);
        Ok(buf.len())
    }

    fn flush(&mut self) -> std::io::Result<()> {
        let mut data = self.0.data.lock();
        let SerializationSinkInner {
            ref mut buffer,
            addr: _,
        } = *data;

        // First flush the local buffer.
        self.0.flush(buffer);

        // Then flush the backing store.
        self.0.shared_state.0.lock().flush()?;

        Ok(())
    }
}

#[derive(Debug)]
struct SerializationSinkInner {
    buffer: Vec<u8>,
    addr: u32,
}

/// This state is shared between all `SerializationSink`s writing to the same
/// backing storage (e.g. the same file).
#[derive(Clone, Debug)]
struct SharedState(Arc<Mutex<BackingStorage>>);

impl SharedState {
    /// Copies out the contents of all pages with the given tag and
    /// concatenates them into a single byte vec. This method is only meant to
    /// be used for testing and will panic if the underlying backing storage is
    /// a file instead of in memory.
    fn copy_bytes_with_page_tag(&self, page_tag: PageTag) -> Vec<u8> {
        let data = self.0.lock();
        let data = match *data {
            BackingStorage::File(_) => panic!(),
            BackingStorage::Memory(ref data) => data,
        };

        split_streams(data).remove(&page_tag).unwrap_or(Vec::new())
    }
}

/// This function reconstructs the individual data streams from their paged
/// version.
///
/// For example, if `E` denotes the page header of an events page, `S` denotes
/// the header of a string data page, and lower case letters denote page
/// contents then a paged stream could look like:
///
/// ```ignore
/// s = Eabcd_Sopq_Eef_Eghi_Srst
/// ```
///
/// and `split_streams` would result in the following set of streams:
///
/// ```ignore
/// split_streams(s) = {
///     events: [abcdefghi],
///     string_data: [opqrst],
/// }
/// ```
pub fn split_streams(paged_data: &[u8]) -> FxHashMap<PageTag, Vec<u8>> {
    let mut result: FxHashMap<PageTag, Vec<u8>> = FxHashMap::default();

    let mut pos = 0;
    while pos < paged_data.len() {
        let tag = TryInto::try_into(paged_data[pos]).unwrap();
        let page_size =
            u32::from_le_bytes(paged_data[pos + 1..pos + 5].try_into().unwrap()) as usize;

        assert!(page_size > 0);

        result
            .entry(tag)
            .or_default()
            .extend_from_slice(&paged_data[pos + 5..pos + 5 + page_size]);

        pos += page_size + 5;
    }

    result
}

impl SerializationSink {
    /// Writes `bytes` as a single page to the shared backing storage. The
    /// method will first write the page header (consisting of the page tag and
    /// the number of bytes in the page) and then the page contents
    /// (i.e. `bytes`).
    fn write_page(&self, bytes: &[u8]) {
        if bytes.len() > 0 {
            // We explicitly don't assert `bytes.len() >= MIN_PAGE_SIZE` because
            // `MIN_PAGE_SIZE` is just a recommendation and the last page will
            // often be smaller than that.
            assert!(bytes.len() <= MAX_PAGE_SIZE);

            let mut file = self.shared_state.0.lock();

            file.write_all(&[self.page_tag as u8]).unwrap();

            let page_size: [u8; 4] = (bytes.len() as u32).to_le_bytes();
            file.write_all(&page_size).unwrap();
            file.write_all(&bytes[..]).unwrap();
        }
    }

    /// Flushes `buffer` by writing its contents as a new page to the backing
    /// storage and then clearing it.
    fn flush(&self, buffer: &mut Vec<u8>) {
        self.write_page(&buffer[..]);
        buffer.clear();
    }

    /// Creates a copy of all data written so far. This method is meant to be
    /// used for writing unit tests. It will panic if the underlying
    /// `BackingStorage` is a file.
    pub fn into_bytes(mut self) -> Vec<u8> {
        // Swap out the contains of `self` with something that can safely be
        // dropped without side effects.
        let mut data = Mutex::new(SerializationSinkInner {
            buffer: Vec::new(),
            addr: 0,
        });
        std::mem::swap(&mut self.data, &mut data);

        // Extract the data from the mutex.
        let SerializationSinkInner {
            ref mut buffer,
            addr: _,
        } = data.into_inner();

        // Make sure we write the current contents of the buffer to the
        // backing storage before proceeding.
        self.flush(buffer);

        self.shared_state.copy_bytes_with_page_tag(self.page_tag)
    }

    /// Atomically writes `num_bytes` of data to this `SerializationSink`.
    /// Atomic means the data is guaranteed to be written as a contiguous range
    /// of bytes.
    ///
    /// The buffer provided to the `write` callback is guaranteed to be of size
    /// `num_bytes` and `write` is supposed to completely fill it with the data
    /// to be written.
    ///
    /// The return value is the address of the data written and can be used to
    /// refer to the data later on.
    pub fn write_atomic<W>(&self, num_bytes: usize, write: W) -> Addr
    where
        W: FnOnce(&mut [u8]),
    {
        if num_bytes > MAX_PAGE_SIZE {
            let mut bytes = vec![0u8; num_bytes];
            write(&mut bytes[..]);
            return self.write_bytes_atomic(&bytes[..]);
        }

        let mut data = self.data.lock();
        let SerializationSinkInner {
            ref mut buffer,
            ref mut addr,
        } = *data;

        if buffer.len() + num_bytes > MAX_PAGE_SIZE {
            self.flush(buffer);
            assert!(buffer.is_empty());
        }

        let curr_addr = *addr;

        let buf_start = buffer.len();
        let buf_end = buf_start + num_bytes;
        buffer.resize(buf_end, 0u8);
        write(&mut buffer[buf_start..buf_end]);

        *addr += num_bytes as u32;

        Addr(curr_addr)
    }

    /// Atomically writes the data in `bytes` to this `SerializationSink`.
    /// Atomic means the data is guaranteed to be written as a contiguous range
    /// of bytes.
    ///
    /// This method may perform better than `write_atomic` because it may be
    /// able to skip the sink's internal buffer. Use this method if the data to
    /// be written is already available as a `&[u8]`.
    ///
    /// The return value is the address of the data written and can be used to
    /// refer to the data later on.
    pub fn write_bytes_atomic(&self, bytes: &[u8]) -> Addr {
        // For "small" data we go to the buffered version immediately.
        if bytes.len() <= 128 {
            return self.write_atomic(bytes.len(), |sink| {
                sink.copy_from_slice(bytes);
            });
        }

        let mut data = self.data.lock();
        let SerializationSinkInner {
            ref mut buffer,
            ref mut addr,
        } = *data;

        let curr_addr = Addr(*addr);
        *addr += bytes.len() as u32;

        let mut bytes_left = bytes;

        // Do we have too little data in the buffer? If so, fill up the buffer
        // to the minimum page size.
        if buffer.len() < MIN_PAGE_SIZE {
            let num_bytes_to_take = min(MIN_PAGE_SIZE - buffer.len(), bytes_left.len());
            buffer.extend_from_slice(&bytes_left[..num_bytes_to_take]);
            bytes_left = &bytes_left[num_bytes_to_take..];
        }

        if bytes_left.is_empty() {
            return curr_addr;
        }

        // Make sure we flush the buffer before writing out any other pages.
        self.flush(buffer);

        for chunk in bytes_left.chunks(MAX_PAGE_SIZE) {
            if chunk.len() == MAX_PAGE_SIZE {
                // This chunk has the maximum size. It might or might not be the
                // last one. In either case we want to write it to disk
                // immediately because there is no reason to copy it to the
                // buffer first.
                self.write_page(chunk);
            } else {
                // This chunk is less than the chunk size that we requested, so
                // it must be the last one. If it is big enough to warrant its
                // own page, we write it to disk immediately. Otherwise, we copy
                // it to the buffer.
                if chunk.len() >= MIN_PAGE_SIZE {
                    self.write_page(chunk);
                } else {
                    debug_assert!(buffer.is_empty());
                    buffer.extend_from_slice(chunk);
                }
            }
        }

        curr_addr
    }

    pub fn as_std_write<'a>(&'a self) -> impl Write + 'a {
        StdWriteAdapter(self)
    }
}

impl Drop for SerializationSink {
    fn drop(&mut self) {
        let mut data = self.data.lock();
        let SerializationSinkInner {
            ref mut buffer,
            addr: _,
        } = *data;

        self.flush(buffer);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // This function writes `chunk_count` byte-slices of size `chunk_size` to
    // three `SerializationSinks` that all map to the same underlying stream,
    // so we get interleaved pages with different tags.
    // It then extracts the data out again and asserts that it is the same as
    // has been written.
    fn test_roundtrip<W>(chunk_size: usize, chunk_count: usize, write: W)
    where
        W: Fn(&SerializationSink, &[u8]) -> Addr,
    {
        let sink_builder = SerializationSinkBuilder::new_in_memory();
        let tags = [PageTag::Events, PageTag::StringData, PageTag::StringIndex];
        let expected_chunk: Vec<u8> = (0..chunk_size).map(|x| (x % 239) as u8).collect();

        {
            let sinks: Vec<SerializationSink> =
                tags.iter().map(|&tag| sink_builder.new_sink(tag)).collect();

            for chunk_index in 0..chunk_count {
                let expected_addr = Addr((chunk_index * chunk_size) as u32);
                for sink in sinks.iter() {
                    assert_eq!(write(sink, &expected_chunk[..]), expected_addr);
                }
            }
        }

        let streams: Vec<Vec<u8>> = tags
            .iter()
            .map(|&tag| sink_builder.0.copy_bytes_with_page_tag(tag))
            .collect();

        for stream in streams {
            for chunk in stream.chunks(chunk_size) {
                assert_eq!(chunk, expected_chunk);
            }
        }
    }

    fn write_closure(sink: &SerializationSink, bytes: &[u8]) -> Addr {
        sink.write_atomic(bytes.len(), |dest| dest.copy_from_slice(bytes))
    }

    fn write_slice(sink: &SerializationSink, bytes: &[u8]) -> Addr {
        sink.write_bytes_atomic(bytes)
    }

    // Creates two roundtrip tests, one using `SerializationSink::write_atomic`
    // and one using `SerializationSink::write_bytes_atomic`.
    macro_rules! mk_roundtrip_test {
        ($name:ident, $chunk_size:expr, $chunk_count:expr) => {
            mod $name {
                use super::*;

                #[test]
                fn write_atomic() {
                    test_roundtrip($chunk_size, $chunk_count, write_closure);
                }

                #[test]
                fn write_bytes_atomic() {
                    test_roundtrip($chunk_size, $chunk_count, write_slice);
                }
            }
        };
    }

    mk_roundtrip_test!(small_data, 10, (90 * MAX_PAGE_SIZE) / 100);
    mk_roundtrip_test!(huge_data, MAX_PAGE_SIZE * 10, 5);

    mk_roundtrip_test!(exactly_max_page_size, MAX_PAGE_SIZE, 10);
    mk_roundtrip_test!(max_page_size_plus_one, MAX_PAGE_SIZE + 1, 10);
    mk_roundtrip_test!(max_page_size_minus_one, MAX_PAGE_SIZE - 1, 10);

    mk_roundtrip_test!(exactly_min_page_size, MIN_PAGE_SIZE, 10);
    mk_roundtrip_test!(min_page_size_plus_one, MIN_PAGE_SIZE + 1, 10);
    mk_roundtrip_test!(min_page_size_minus_one, MIN_PAGE_SIZE - 1, 10);
}

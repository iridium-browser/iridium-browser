use crate::event_id::EventId;
use crate::stringtable::StringId;
#[cfg(target_endian = "big")]
use std::convert::TryInto;

/// `RawEvent` is how events are stored on-disk. If you change this struct,
/// make sure that you increment `file_header::CURRENT_FILE_FORMAT_VERSION`.
#[derive(Eq, PartialEq, Debug)]
#[repr(C)]
pub struct RawEvent {
    pub event_kind: StringId,
    pub event_id: EventId,
    pub thread_id: u32,

    // The following 96 bits store the payload values, using
    // 48 bits for each.
    // Interval:
    // Payload 1 is start value and payload 2 is end value
    // SSSSSSSSSSSSSSSSEEEEEEEEEEEEEEEESSSSSSSEEEEEEEEE
    // [payload1_lower][payload2_lower][payloads_upper]
    // Instant:
    // Payload2 is 0xFFFF_FFFF_FFFF
    // VVVVVVVVVVVVVVVV1111111111111111VVVVVVV11111111
    // [payload1_lower][payload2_lower][payloads_upper]
    // Integer:
    // Payload2 is 0xFFFF_FFFF_FFFE
    // VVVVVVVVVVVVVVVV1111111111111111VVVVVVV11111110
    // [payload1_lower][payload2_lower][payloads_upper]
    pub payload1_lower: u32,
    pub payload2_lower: u32,
    pub payloads_upper: u32,
}

/// `RawEvents` that have a payload 2 value with this value are instant events.
const INSTANT_MARKER: u64 = 0xFFFF_FFFF_FFFF;
/// `RawEvents` that have a payload 2 value with this value are integer events.
const INTEGER_MARKER: u64 = INSTANT_MARKER - 1;

/// The max value we can represent with the 48 bits available.
pub const MAX_SINGLE_VALUE: u64 = 0xFFFF_FFFF_FFFF;

/// The max value we can represent with the 48 bits available.
/// The highest two values are reserved for the `INSTANT_MARKER` and `INTEGER_MARKER`.
pub const MAX_INTERVAL_VALUE: u64 = INTEGER_MARKER - 1;

impl RawEvent {
    #[inline]
    pub fn new_interval(
        event_kind: StringId,
        event_id: EventId,
        thread_id: u32,
        start: u64,
        end: u64,
    ) -> Self {
        assert!(start <= end);
        assert!(end <= MAX_INTERVAL_VALUE);

        Self::pack_values(event_kind, event_id, thread_id, start, end)
    }

    #[inline]
    pub fn new_instant(
        event_kind: StringId,
        event_id: EventId,
        thread_id: u32,
        instant: u64,
    ) -> Self {
        assert!(instant <= MAX_SINGLE_VALUE);
        Self::pack_values(event_kind, event_id, thread_id, instant, INSTANT_MARKER)
    }

    #[inline]
    pub fn new_integer(
        event_kind: StringId,
        event_id: EventId,
        thread_id: u32,
        value: u64,
    ) -> Self {
        assert!(value <= MAX_SINGLE_VALUE);
        Self::pack_values(event_kind, event_id, thread_id, value, INTEGER_MARKER)
    }

    #[inline]
    fn pack_values(
        event_kind: StringId,
        event_id: EventId,
        thread_id: u32,
        value1: u64,
        value2: u64,
    ) -> Self {
        let payload1_lower = value1 as u32;
        let payload2_lower = value2 as u32;

        let value1_upper = (value1 >> 16) as u32 & 0xFFFF_0000;
        let value2_upper = (value2 >> 32) as u32;

        let payloads_upper = value1_upper | value2_upper;

        Self {
            event_kind,
            event_id,
            thread_id,
            payload1_lower,
            payload2_lower,
            payloads_upper,
        }
    }

    /// The start value assuming self is an interval
    #[inline]
    pub fn start_value(&self) -> u64 {
        self.payload1_lower as u64 | (((self.payloads_upper & 0xFFFF_0000) as u64) << 16)
    }

    /// The end value assuming self is an interval
    #[inline]
    pub fn end_value(&self) -> u64 {
        self.payload2_lower as u64 | (((self.payloads_upper & 0x0000_FFFF) as u64) << 32)
    }

    /// The value assuming self is an interval or integer.
    #[inline]
    pub fn value(&self) -> u64 {
        self.payload1_lower as u64 | (((self.payloads_upper & 0xFFFF_0000) as u64) << 16)
    }

    #[inline]
    pub fn is_instant(&self) -> bool {
        self.end_value() == INSTANT_MARKER
    }

    #[inline]
    pub fn is_integer(&self) -> bool {
        self.end_value() == INTEGER_MARKER
    }

    #[inline]
    pub fn serialize(&self, bytes: &mut [u8]) {
        assert!(bytes.len() == std::mem::size_of::<RawEvent>());

        #[cfg(target_endian = "little")]
        {
            let raw_event_bytes: &[u8] = unsafe {
                std::slice::from_raw_parts(
                    self as *const _ as *const u8,
                    std::mem::size_of::<RawEvent>(),
                )
            };

            bytes.copy_from_slice(raw_event_bytes);
        }

        #[cfg(target_endian = "big")]
        {
            // We always emit data as little endian, which we have to do
            // manually on big endian targets.
            bytes[0..4].copy_from_slice(&self.event_kind.as_u32().to_le_bytes());
            bytes[4..8].copy_from_slice(&self.event_id.as_u32().to_le_bytes());
            bytes[8..12].copy_from_slice(&self.thread_id.to_le_bytes());
            bytes[12..16].copy_from_slice(&self.payload1_lower.to_le_bytes());
            bytes[16..20].copy_from_slice(&self.payload2_lower.to_le_bytes());
            bytes[20..24].copy_from_slice(&self.payloads_upper.to_le_bytes());
        }
    }

    #[inline]
    pub fn deserialize(bytes: &[u8]) -> RawEvent {
        assert!(bytes.len() == std::mem::size_of::<RawEvent>());

        #[cfg(target_endian = "little")]
        {
            let mut raw_event = RawEvent::default();
            unsafe {
                let raw_event = std::slice::from_raw_parts_mut(
                    &mut raw_event as *mut RawEvent as *mut u8,
                    std::mem::size_of::<RawEvent>(),
                );
                raw_event.copy_from_slice(bytes);
            };
            raw_event
        }

        #[cfg(target_endian = "big")]
        {
            RawEvent {
                event_kind: StringId::new(u32::from_le_bytes(bytes[0..4].try_into().unwrap())),
                event_id: EventId::from_u32(u32::from_le_bytes(bytes[4..8].try_into().unwrap())),
                thread_id: u32::from_le_bytes(bytes[8..12].try_into().unwrap()),
                payload1_lower: u32::from_le_bytes(bytes[12..16].try_into().unwrap()),
                payload2_lower: u32::from_le_bytes(bytes[16..20].try_into().unwrap()),
                payloads_upper: u32::from_le_bytes(bytes[20..24].try_into().unwrap()),
            }
        }
    }
}

impl Default for RawEvent {
    fn default() -> Self {
        RawEvent {
            event_kind: StringId::INVALID,
            event_id: EventId::INVALID,
            thread_id: 0,
            payload1_lower: 0,
            payload2_lower: 0,
            payloads_upper: 0,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn raw_event_has_expected_size() {
        // A test case to prevent accidental regressions of RawEvent's size.
        assert_eq!(std::mem::size_of::<RawEvent>(), 24);
    }

    #[test]
    fn is_instant() {
        assert!(RawEvent::new_instant(StringId::INVALID, EventId::INVALID, 987, 0,).is_instant());

        assert!(
            RawEvent::new_instant(StringId::INVALID, EventId::INVALID, 987, MAX_SINGLE_VALUE,)
                .is_instant()
        );

        assert!(!RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            987,
            0,
            MAX_INTERVAL_VALUE,
        )
        .is_instant());
    }

    #[test]
    fn is_integer() {
        let integer = RawEvent::new_integer(StringId::INVALID, EventId::INVALID, 987, 0);
        assert!(integer.is_integer());
        assert_eq!(integer.value(), 0);

        let integer = RawEvent::new_integer(StringId::INVALID, EventId::INVALID, 987, 8769);
        assert!(integer.is_integer());
        assert_eq!(integer.value(), 8769);

        assert!(
            RawEvent::new_integer(StringId::INVALID, EventId::INVALID, 987, MAX_SINGLE_VALUE,)
                .is_integer()
        );

        assert!(!RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            987,
            0,
            MAX_INTERVAL_VALUE,
        )
        .is_integer());
    }

    #[test]
    #[should_panic]
    fn invalid_instant_count() {
        let _ = RawEvent::new_instant(
            StringId::INVALID,
            EventId::INVALID,
            123,
            // count too large
            MAX_SINGLE_VALUE + 1,
        );
    }

    #[test]
    #[should_panic]
    fn invalid_start_count() {
        let _ = RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            123,
            // start count too large
            MAX_INTERVAL_VALUE + 1,
            MAX_INTERVAL_VALUE + 1,
        );
    }

    #[test]
    #[should_panic]
    fn invalid_end_count() {
        let _ = RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            123,
            0,
            // end count too large
            MAX_INTERVAL_VALUE + 3,
        );
    }

    #[test]
    #[should_panic]
    fn invalid_end_count2() {
        let _ = RawEvent::new_interval(StringId::INVALID, EventId::INVALID, 123, 0, INTEGER_MARKER);
    }

    #[test]
    #[should_panic]
    fn start_greater_than_end_count() {
        let _ = RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            123,
            // start count greater than end count
            1,
            0,
        );
    }

    #[test]
    fn start_equal_to_end_count() {
        // This is allowed, make sure we don't panic
        let _ = RawEvent::new_interval(StringId::INVALID, EventId::INVALID, 123, 1, 1);
    }

    #[test]
    fn interval_count_decoding() {
        // Check the upper limits
        let e = RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            1234,
            MAX_INTERVAL_VALUE,
            MAX_INTERVAL_VALUE,
        );

        assert_eq!(e.start_value(), MAX_INTERVAL_VALUE);
        assert_eq!(e.end_value(), MAX_INTERVAL_VALUE);

        // Check the lower limits
        let e = RawEvent::new_interval(StringId::INVALID, EventId::INVALID, 1234, 0, 0);

        assert_eq!(e.start_value(), 0);
        assert_eq!(e.end_value(), 0);

        // Check that end does not bleed into start
        let e = RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            1234,
            0,
            MAX_INTERVAL_VALUE,
        );

        assert_eq!(e.start_value(), 0);
        assert_eq!(e.end_value(), MAX_INTERVAL_VALUE);

        // Test some random values
        let e = RawEvent::new_interval(
            StringId::INVALID,
            EventId::INVALID,
            1234,
            0x1234567890,
            0x1234567890A,
        );

        assert_eq!(e.start_value(), 0x1234567890);
        assert_eq!(e.end_value(), 0x1234567890A);
    }

    #[test]
    fn instant_count_decoding() {
        assert_eq!(
            RawEvent::new_instant(StringId::INVALID, EventId::INVALID, 987, 0,).start_value(),
            0
        );

        assert_eq!(
            RawEvent::new_instant(StringId::INVALID, EventId::INVALID, 987, 42,).start_value(),
            42
        );

        assert_eq!(
            RawEvent::new_instant(StringId::INVALID, EventId::INVALID, 987, MAX_SINGLE_VALUE,)
                .start_value(),
            MAX_SINGLE_VALUE
        );
    }

    #[test]
    fn integer_decoding() {
        assert_eq!(
            RawEvent::new_integer(StringId::INVALID, EventId::INVALID, 987, 0,).start_value(),
            0
        );

        assert_eq!(
            RawEvent::new_integer(StringId::INVALID, EventId::INVALID, 987, 42,).start_value(),
            42
        );

        assert_eq!(
            RawEvent::new_integer(StringId::INVALID, EventId::INVALID, 987, MAX_SINGLE_VALUE,)
                .start_value(),
            MAX_SINGLE_VALUE
        );
    }
}

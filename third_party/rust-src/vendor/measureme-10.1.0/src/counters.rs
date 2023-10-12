//! Profiling counters and their implementation.
//!
//! # Available counters
//!
//! Name (for [`Counter::by_name()`]) | Counter                      | OSes  | CPUs
//! --------------------------------- | -------                      | ----  | ----
//! `wall-time`                       | [`WallTime`]                 | any   | any
//! `instructions:u`                  | [`Instructions`]             | Linux | `x86_64`
//! `instructions-minus-irqs:u`       | [`InstructionsMinusIrqs`]    | Linux | `x86_64`<br>- AMD (since K8)<br>- Intel (since Sandy Bridge)
//! `instructions-minus-r0420:u`      | [`InstructionsMinusRaw0420`] | Linux | `x86_64`<br>- AMD (Zen)
//!
//! *Note: `:u` suffixes for hardware performance counters come from the Linux `perf`
//! tool, and indicate that the counter is only active while userspace code executes
//! (i.e. it's paused while the kernel handles syscalls, interrupts, etc.).*
//!
//! # Limitations and caveats
//!
//! *Note: for more information, also see the GitHub PR which first implemented hardware
//! performance counter support ([#143](https://github.com/rust-lang/measureme/pull/143)).*
//!
//! The hardware performance counters (i.e. all counters other than `wall-time`) are limited to:
//! * Linux, for out-of-the-box performance counter reads from userspace
//!   * other OSes could work through custom kernel extensions/drivers, in the future
//! * `x86_64` CPUs, mostly due to lack of other available test hardware
//!   * new architectures would be easier to support (on Linux) than new OSes
//!   * easiest to add would be 32-bit `x86` (aka `i686`), which would reuse
//!     most of the `x86_64` CPU model detection logic
//! * specific (newer) CPU models, for certain non-standard counters
//!   * e.g. `instructions-minus-irqs:u` requires a "hardware interrupts" (aka "IRQs")
//!     counter, which is implemented differently between vendors / models (if at all)
//! * single-threaded programs (counters only work on the thread they were created on)
//!   * for profiling `rustc`, this means only "check mode" (`--emit=metadata`),
//!     is supported currently (`-Z no-llvm-threads` could also work)
//!   * unclear what the best approach for handling multiple threads would be
//!   * changing the API (e.g. to require per-thread profiler handles) could result
//!     in a more efficient implementation, but would also be less ergonomic
//!   * profiling data from multithreaded programs would be harder to use due to
//!     noise from synchronization mechanisms, non-deterministic work-stealing, etc.
//!
//! For ergonomic reasons, the public API doesn't vary based on `features` or target.
//! Instead, attempting to create any unsupported counter will return `Err`, just
//! like it does for any issue detected at runtime (e.g. incompatible CPU model).
//!
//! When counting instructions specifically, these factors will impact the profiling quality:
//! * high-level non-determinism (e.g. user interactions, networking)
//!   * the ideal use-case is a mostly-deterministic program, e.g. a compiler like `rustc`
//!   * if I/O can be isolated to separate profiling events, and doesn't impact
//!     execution in a more subtle way (see below), the deterministic parts of
//!     the program can still be profiled with high accuracy
//!   * intentional uses of randomness may change execution paths, though for
//!     cryptographic operations specifically, "constant time" implementations
//!     are preferred / necessary (in order to limit an external observer's
//!     ability to infer secrets), so they're not as much of a problem
//!   * even otherwise-deterministic machine-local communication (to e.g. system
//!     services or drivers) can behave unpredictably (especially under load)
//!     * while we haven't observed this in the wild yet, it's possible for
//!       file reads/writes to be split up into multiple smaller chunks
//!       (and therefore take more userspace instructions to fully read/write)
//! * low-level non-determinism (e.g. ASLR, randomized `HashMap`s, timers)
//!   * ASLR ("Address Space Layout Randomization"), may be provided by the OS for
//!     security reasons, or accidentally caused through allocations that depend on
//!     random data (even as low-entropy as e.g. the base 10 length of a process ID)
//!   * on Linux ASLR can be disabled by running the process under `setarch -R`
//!   * this impacts `rustc` and LLVM, which rely on keying `HashMap`s by addresses
//!     (typically of interned data) as an optimization, and while non-determinstic
//!     outputs are considered bugs, the instructions executed can still vary a lot,
//!     even when the externally observable behavior is perfectly repeatable
//!   * `HashMap`s are involved in one more than one way:
//!     * both the executed instructions, and the shape of the allocations depend
//!       on both the hasher state and choice of keys (as the buckets are in
//!       a flat array indexed by some of the lower bits of the key hashes)
//!     * so every `HashMap` with keys being/containing addresses will amplify
//!       ASLR and ASLR-like effects, making the entire program more sensitive
//!     * the default hasher is randomized, and while `rustc` doesn't use it,
//!       proc macros can (and will), and it's harder to disable than Linux ASLR
//!   * most ways of measuring time will inherently never perfectly align with
//!     exact points in the program's execution, making time behave like another
//!     low-entropy source of randomness - this also means timers will elapse at
//!     unpredictable points (which can further impact the rest of the execution)
//!     * this includes the common thread scheduler technique of preempting the
//!       currently executing thread with a periodic timer interrupt, so the exact
//!       interleaving of multiple threads will likely not be reproducible without
//!       special OS configuration, or tools that emulate a deterministic scheduler
//!     * `jemalloc` (the allocator used by `rustc`, at least in official releases)
//!       has a 10 second "purge timer", which can introduce an ASLR-like effect,
//!       unless disabled with `MALLOC_CONF=dirty_decay_ms:0,muzzy_decay_ms:0`
//! * hardware flaws (whether in the design or implementation)
//!   * hardware interrupts ("IRQs") and exceptions (like page faults) cause
//!     overcounting (1 instruction per interrupt, possibly the `iret` from the
//!     kernel handler back to the interrupted userspace program)
//!     * this is the reason why `instructions-minus-irqs:u` should be preferred
//!       to `instructions:u`, where the former is available
//!     * there are system-wide options (e.g. `CONFIG_NO_HZ_FULL`) for removing
//!       some interrupts from the cores used for profiling, but they're not as
//!       complete of a solution, nor easy to set up in the first place
//!   * AMD Zen CPUs have a speculative execution feature (dubbed `SpecLockMap`),
//!     which can cause non-deterministic overcounting for instructions following
//!     an atomic instruction (such as found in heap allocators, or `measureme`)
//!     * this is automatically detected, with a `log` message pointing the user
//!       to <https://github.com/mozilla/rr/wiki/Zen> for guidance on how to
//!       disable `SpecLockMap` on their system (sadly requires root access)
//!
//! Even if some of the above caveats apply for some profiling setup, as long as
//! the counters function, they can still be used, and compared with `wall-time`.
//! Chances are, they will still have less variance, as everything that impacts
//! instruction counts will also impact any time measurements.
//!
//! Also keep in mind that instruction counts do not properly reflect all kinds
//! of workloads, e.g. SIMD throughput and cache locality are unaccounted for.

use std::error::Error;
use std::time::Instant;

// HACK(eddyb) this is semantically `warn!` but uses `error!` because
// that's the only log level enabled by default - see also
// https://github.com/rust-lang/rust/issues/76824
macro_rules! really_warn {
    ($msg:literal $($rest:tt)*) => {
        error!(concat!("[WARNING] ", $msg) $($rest)*)
    }
}

pub enum Counter {
    WallTime(WallTime),
    Instructions(Instructions),
    InstructionsMinusIrqs(InstructionsMinusIrqs),
    InstructionsMinusRaw0420(InstructionsMinusRaw0420),
}

impl Counter {
    pub fn by_name(name: &str) -> Result<Self, Box<dyn Error + Send + Sync>> {
        Ok(match name {
            WallTime::NAME => Counter::WallTime(WallTime::new()),
            Instructions::NAME => Counter::Instructions(Instructions::new()?),
            InstructionsMinusIrqs::NAME => {
                Counter::InstructionsMinusIrqs(InstructionsMinusIrqs::new()?)
            }
            InstructionsMinusRaw0420::NAME => {
                Counter::InstructionsMinusRaw0420(InstructionsMinusRaw0420::new()?)
            }
            _ => return Err(format!("{:?} is not a valid counter name", name).into()),
        })
    }

    pub(super) fn describe_as_json(&self) -> String {
        let (name, units) = match self {
            Counter::WallTime(_) => (
                WallTime::NAME,
                r#"[["ns", 1], ["μs", 1000], ["ms", 1000000], ["s", 1000000000]]"#,
            ),
            Counter::Instructions(_) => (Instructions::NAME, r#"[["instructions", 1]]"#),
            Counter::InstructionsMinusIrqs(_) => {
                (InstructionsMinusIrqs::NAME, r#"[["instructions", 1]]"#)
            }
            Counter::InstructionsMinusRaw0420(_) => {
                (InstructionsMinusRaw0420::NAME, r#"[["instructions", 1]]"#)
            }
        };
        format!(r#"{{ "name": "{}", "units": {} }}"#, name, units)
    }

    #[inline]
    pub(super) fn since_start(&self) -> u64 {
        match self {
            Counter::WallTime(counter) => counter.since_start(),
            Counter::Instructions(counter) => counter.since_start(),
            Counter::InstructionsMinusIrqs(counter) => counter.since_start(),
            Counter::InstructionsMinusRaw0420(counter) => counter.since_start(),
        }
    }
}

/// "Monotonic clock" with nanosecond precision (using [`std::time::Instant`]).
///
/// Can be obtained with `Counter::by_name("wall-time")`.
pub struct WallTime {
    start: Instant,
}

impl WallTime {
    const NAME: &'static str = "wall-time";

    pub fn new() -> Self {
        WallTime {
            start: Instant::now(),
        }
    }

    #[inline]
    fn since_start(&self) -> u64 {
        self.start.elapsed().as_nanos() as u64
    }
}

/// "Instructions retired" hardware performance counter (userspace-only).
///
/// Can be obtained with `Counter::by_name("instructions:u")`.
pub struct Instructions {
    instructions: hw::Counter,
    start: u64,
}

impl Instructions {
    const NAME: &'static str = "instructions:u";

    pub fn new() -> Result<Self, Box<dyn Error + Send + Sync>> {
        let model = hw::CpuModel::detect()?;
        let instructions = hw::Counter::new(&model, HwCounterType::Instructions)?;
        let start = instructions.read();
        Ok(Instructions {
            instructions,
            start,
        })
    }

    #[inline]
    fn since_start(&self) -> u64 {
        self.instructions.read().wrapping_sub(self.start)
    }
}

/// More accurate [`Instructions`] (subtracting hardware interrupt counts).
///
/// Can be obtained with `Counter::by_name("instructions-minus-irqs:u")`.
pub struct InstructionsMinusIrqs {
    instructions: hw::Counter,
    irqs: hw::Counter,
    start: u64,
}

impl InstructionsMinusIrqs {
    const NAME: &'static str = "instructions-minus-irqs:u";

    pub fn new() -> Result<Self, Box<dyn Error + Send + Sync>> {
        let model = hw::CpuModel::detect()?;
        let instructions = hw::Counter::new(&model, HwCounterType::Instructions)?;
        let irqs = hw::Counter::new(&model, HwCounterType::Irqs)?;
        let (start_instructions, start_irqs) = (&instructions, &irqs).read();
        let start = start_instructions.wrapping_sub(start_irqs);
        Ok(InstructionsMinusIrqs {
            instructions,
            irqs,
            start,
        })
    }

    #[inline]
    fn since_start(&self) -> u64 {
        let (instructions, irqs) = (&self.instructions, &self.irqs).read();
        instructions.wrapping_sub(irqs).wrapping_sub(self.start)
    }
}

/// (Experimental) Like [`InstructionsMinusIrqs`] (but using an undocumented `r0420:u` counter).
///
/// Can be obtained with `Counter::by_name("instructions-minus-r0420:u")`.
//
// HACK(eddyb) this is a variant of `instructions-minus-irqs:u`, where `r0420`
// is subtracted, instead of the usual "hardware interrupts" (aka IRQs).
// `r0420` is an undocumented counter on AMD Zen CPUs which appears to count
// both hardware interrupts and exceptions (such as page faults), though
// it's unclear yet what exactly it's counting (could even be `iret`s).
pub struct InstructionsMinusRaw0420(InstructionsMinusIrqs);

impl InstructionsMinusRaw0420 {
    const NAME: &'static str = "instructions-minus-r0420:u";

    pub fn new() -> Result<Self, Box<dyn Error + Send + Sync>> {
        let model = hw::CpuModel::detect()?;
        let instructions = hw::Counter::new(&model, HwCounterType::Instructions)?;
        let irqs = hw::Counter::new(&model, HwCounterType::Raw0420)?;
        let (start_instructions, start_irqs) = (&instructions, &irqs).read();
        let start = start_instructions.wrapping_sub(start_irqs);
        Ok(InstructionsMinusRaw0420(InstructionsMinusIrqs {
            instructions,
            irqs,
            start,
        }))
    }

    #[inline]
    fn since_start(&self) -> u64 {
        self.0.since_start()
    }
}

trait HwCounterRead {
    type Output;
    fn read(&self) -> Self::Output;
}

enum HwCounterType {
    Instructions,
    Irqs,
    Raw0420,
}

const BUG_REPORT_MSG: &str =
    "please report this to https://github.com/rust-lang/measureme/issues/new";

/// Linux x86_64 implementation based on `perf_event_open` and `rdpmc`.
#[cfg(all(target_arch = "x86_64", target_os = "linux"))]
mod hw {
    use memmap2::{Mmap, MmapOptions};
    use perf_event_open_sys::{bindings::*, perf_event_open};
    use std::arch::asm;
    use std::convert::TryInto;
    use std::error::Error;
    use std::fs;
    use std::mem;
    use std::os::unix::io::FromRawFd;

    pub(super) struct Counter {
        mmap: Mmap,
        reg_idx: u32,
    }

    impl Counter {
        pub(super) fn new(
            model: &CpuModel,
            counter_type: super::HwCounterType,
        ) -> Result<Self, Box<dyn Error + Send + Sync>> {
            let (type_, hw_id) = match counter_type {
                super::HwCounterType::Instructions => (
                    perf_type_id_PERF_TYPE_HARDWARE,
                    perf_hw_id_PERF_COUNT_HW_INSTRUCTIONS,
                ),
                super::HwCounterType::Irqs => {
                    (perf_type_id_PERF_TYPE_RAW, model.irqs_counter_config()?)
                }
                super::HwCounterType::Raw0420 => {
                    match model {
                        CpuModel::Amd(AmdGen::Zen) => {}

                        _ => really_warn!(
                            "Counter::new: the undocumented `r0420` performance \
                             counter has only been observed on AMD Zen CPUs"
                        ),
                    }

                    (perf_type_id_PERF_TYPE_RAW, 0x04_20)
                }
            };
            Self::with_type_and_hw_id(type_, hw_id)
        }

        fn with_type_and_hw_id(
            type_: perf_type_id,
            hw_id: u32,
        ) -> Result<Self, Box<dyn Error + Send + Sync>> {
            let mut attrs = perf_event_attr {
                size: mem::size_of::<perf_event_attr>().try_into().unwrap(),
                type_,
                config: hw_id.into(),
                ..perf_event_attr::default()
            };

            // Only record same-thread, any CPUs, and only userspace (no kernel/hypervisor).
            // NOTE(eddyb) `pid = 0`, despite talking about "process id", means
            // "calling process/thread", *not* "any thread in the calling process"
            // (i.e. "process" is interchangeable with "main thread of the process")
            // FIXME(eddyb) introduce per-thread counters and/or use `inherit`
            // (and `inherit_stat`? though they might not be appropriate here)
            // to be able to read the counter on more than just the initial thread.
            let pid = 0;
            let cpu = -1;
            let group_fd = -1;
            attrs.set_exclude_kernel(1);
            attrs.set_exclude_hv(1);

            let file = unsafe {
                let fd =
                    perf_event_open(&mut attrs, pid, cpu, group_fd, PERF_FLAG_FD_CLOEXEC.into());
                if fd < 0 {
                    Err(std::io::Error::from_raw_os_error(-fd))
                } else {
                    Ok(fs::File::from_raw_fd(fd))
                }
            };
            let file = file.map_err(|e| format!("perf_event_open failed: {:?}", e))?;

            let mmap = unsafe {
                MmapOptions::new()
                    .len(mem::size_of::<perf_event_mmap_page>())
                    .map(&file)
            };
            let mmap = mmap.map_err(|e| format!("perf_event_mmap_page: mmap failed: {:?}", e))?;

            let mut counter = Counter { mmap, reg_idx: 0 };

            let (version, compat_version, caps, index, pmc_width) = counter
                .access_mmap_page_with_seqlock(|mp| {
                    (
                        mp.version,
                        mp.compat_version,
                        unsafe { mp.__bindgen_anon_1.__bindgen_anon_1 },
                        mp.index,
                        mp.pmc_width,
                    )
                });

            info!(
                "Counter::new: version={} compat_version={} index={:#x}",
                version, compat_version, index,
            );

            if caps.cap_user_rdpmc() == 0 {
                return Err(format!(
                    "perf_event_mmap_page: missing cap_user_rdpmc{}",
                    if caps.cap_bit0_is_deprecated() == 0 && caps.cap_bit0() == 1 {
                        " (ignoring legacy/broken rdpmc support)"
                    } else {
                        ""
                    }
                )
                .into());
            }

            if index == 0 {
                return Err(format!(
                    "perf_event_mmap_page: no allocated hardware register (ran out?)"
                )
                .into());
            }
            counter.reg_idx = index - 1;

            if (cfg!(not(accurate_seqlock_rdpmc)) || true) && pmc_width != 48 {
                return Err(format!(
                    "perf_event_mmap_page: {}-bit hardware counter found, only 48-bit supported",
                    pmc_width
                )
                .into());
            }

            Ok(counter)
        }

        /// Try to access the mmap page, retrying the `attempt` closure as long
        /// as the "seqlock" sequence number changes (which indicates the kernel
        /// has updated one or more fields within the mmap page).
        #[inline]
        fn access_mmap_page_with_seqlock<T>(
            &self,
            attempt: impl Fn(&perf_event_mmap_page) -> T,
        ) -> T {
            // FIXME(eddyb) it's probably UB to use regular reads, especially
            // from behind `&T`, with the only synchronization being barriers.
            // Probably needs atomic reads, and stronger ones at that, for the
            // `lock` field, than the fields (which would be `Relaxed`?).
            let mmap_page = unsafe { &*(self.mmap.as_ptr() as *const perf_event_mmap_page) };
            let barrier = || std::sync::atomic::fence(std::sync::atomic::Ordering::Acquire);

            loop {
                // Grab the "seqlock" - the kernel will update this value when it
                // updates any of the other fields that may be read in `attempt`.
                let seq_lock = mmap_page.lock;
                barrier();

                let result = attempt(mmap_page);

                // If nothing has changed, we're done. Otherwise, keep retrying.
                barrier();
                if mmap_page.lock == seq_lock {
                    return result;
                }
            }
        }
    }

    impl super::HwCounterRead for Counter {
        type Output = u64;

        #[inline]
        fn read(&self) -> u64 {
            // HACK(eddyb) keep the accurate code around while not using it,
            // to minimize overhead without losing the more complex implementation.
            let (counter, offset, pmc_width) = if cfg!(accurate_seqlock_rdpmc) && false {
                self.access_mmap_page_with_seqlock(|mp| {
                    let caps = unsafe { mp.__bindgen_anon_1.__bindgen_anon_1 };
                    assert_ne!(caps.cap_user_rdpmc(), 0);

                    (
                        rdpmc(mp.index.checked_sub(1).unwrap()),
                        mp.offset,
                        mp.pmc_width,
                    )
                })
            } else {
                (rdpmc(self.reg_idx), 0, 48)
            };

            let counter = offset + (counter as i64);

            // Sign-extend the `pmc_width`-bit value to `i64`.
            (counter << (64 - pmc_width) >> (64 - pmc_width)) as u64
        }
    }

    impl super::HwCounterRead for (&Counter, &Counter) {
        type Output = (u64, u64);

        #[inline]
        fn read(&self) -> (u64, u64) {
            // HACK(eddyb) keep the accurate code around while not using it,
            // to minimize overhead without losing the more complex implementation.
            if (cfg!(accurate_seqlock_rdpmc) || cfg!(unserialized_rdpmc)) && false {
                return (self.0.read(), self.1.read());
            }

            let pmc_width = 48;

            let (a_counter, b_counter) = rdpmc_pair(self.0.reg_idx, self.1.reg_idx);

            // Sign-extend the `pmc_width`-bit values to `i64`.
            (
                ((a_counter as i64) << (64 - pmc_width) >> (64 - pmc_width)) as u64,
                ((b_counter as i64) << (64 - pmc_width) >> (64 - pmc_width)) as u64,
            )
        }
    }

    /// Read the hardware performance counter indicated by `reg_idx`.
    ///
    /// If the counter is signed, sign extension should be performed based on
    /// the width of the register (32 to 64 bits, e.g. 48-bit seems common).
    #[inline(always)]
    fn rdpmc(reg_idx: u32) -> u64 {
        // NOTE(eddyb) below comment is outdated (the other branch uses `cpuid`).
        if cfg!(unserialized_rdpmc) && false {
            // FIXME(eddyb) the Intel and AMD manuals warn about the need for
            // "serializing instructions" before/after `rdpmc`, if avoiding any
            // reordering is desired, but do not agree on the full set of usable
            // "serializing instructions" (e.g. `mfence` isn't listed in both).
            //
            // The only usable, and guaranteed to work, "serializing instruction"
            // appears to be `cpuid`, but it doesn't seem easy to use, especially
            // due to the overlap in registers with `rdpmc` itself, and it might
            // have too high of a cost, compared to serialization benefits (if any).
            unserialized_rdpmc(reg_idx)
        } else {
            serialize_instruction_execution();
            unserialized_rdpmc(reg_idx)
        }
    }

    /// Read two hardware performance counters at once (see `rdpmc`).
    ///
    /// Should be more efficient/accurate than two `rdpmc` calls, as it
    /// only requires one "serializing instruction", rather than two.
    #[inline(always)]
    fn rdpmc_pair(a_reg_idx: u32, b_reg_idx: u32) -> (u64, u64) {
        serialize_instruction_execution();
        (unserialized_rdpmc(a_reg_idx), unserialized_rdpmc(b_reg_idx))
    }

    /// Dummy `cpuid(0)` to serialize instruction execution.
    #[inline(always)]
    fn serialize_instruction_execution() {
        unsafe {
            asm!(
                "xor %eax, %eax", // Intel syntax: "xor eax, eax"
                // LLVM sometimes reserves `ebx` for its internal use, so we need to use
                // a scratch register for it instead.
                "mov %rbx, {tmp_rbx:r}", // Intel syntax: "mov {tmp_rbx:r}, rbx"
                "cpuid",
                "mov {tmp_rbx:r}, %rbx", // Intel syntax: "mov rbx, {tmp_rbx:r}"
                tmp_rbx = lateout(reg) _,
                // `cpuid` clobbers.
                lateout("eax") _,
                lateout("edx") _,
                lateout("ecx") _,

                options(nostack),
                // Older versions of LLVM do not support modifiers in
                // Intel syntax inline asm; whenever Rust minimum LLVM version
                // supports Intel syntax inline asm, remove and replace above
                // instructions with Intel syntax version (from comments).
                options(att_syntax),
            );
        }
    }

    /// Read the hardware performance counter indicated by `reg_idx`.
    ///
    /// If the counter is signed, sign extension should be performed based on
    /// the width of the register (32 to 64 bits, e.g. 48-bit seems common).
    #[inline(always)]
    fn unserialized_rdpmc(reg_idx: u32) -> u64 {
        let (lo, hi): (u32, u32);
        unsafe {
            asm!(
                "rdpmc",
                in("ecx") reg_idx,
                lateout("eax") lo,
                lateout("edx") hi,
                options(nostack),
                // Older versions of LLVM do not support modifiers in
                // Intel syntax inline asm; whenever Rust minimum LLVM version
                // supports Intel syntax inline asm, remove and replace above
                // instructions with Intel syntax version (from comments).
                options(att_syntax),
            );
        }
        lo as u64 | (hi as u64) << 32
    }

    /// Categorization of `x86_64` CPUs, primarily based on how they
    /// support for counting "hardware interrupts" (documented or not).
    pub(super) enum CpuModel {
        Amd(AmdGen),
        Intel(IntelGen),
    }

    pub(super) enum AmdGen {
        /// K8 (Hammer) to Jaguar / Puma.
        PreZen,

        /// Zen / Zen+ / Zen 2.
        Zen,

        /// Unknown AMD CPU, contemporary to/succeeding Zen/Zen+/Zen 2,
        /// but likely similar to them.
        UnknownMaybeZenLike,
    }

    pub(super) enum IntelGen {
        /// Intel CPU predating Sandy Bridge. These are the only CPUs we
        /// can't support (more) accurate instruction counting on, as they
        /// don't (appear to) have any way to count "hardware interrupts".
        PreBridge,

        /// Sandy Bridge / Ivy Bridge:
        /// * client: Sandy Bridge (M/H) / Ivy Bridge (M/H/Gladden)
        /// * server: Sandy Bridge (E/EN/EP) / Ivy Bridge (E/EN/EP/EX)
        ///
        /// Intel doesn't document support for counting "hardware interrupts"
        /// prior to Skylake, but testing found that `HW_INTERRUPTS.RECEIVED`
        /// from Skylake has existed, with the same config, as far back as
        /// "Sandy Bridge" (but before that it mapped to a different event).
        ///
        /// These are the (pre-Skylake) *Bridge CPU models confirmed so far:
        /// * Sandy Bridge (client) Family 6 Model 42
        ///     Intel(R) Core(TM) i5-2520M CPU @ 2.50GHz (@alyssais)
        /// * Ivy Bridge (client) Family 6 Model 58
        ///     Intel(R) Core(TM) i7-3520M CPU @ 2.90GHz (@eddyb)
        ///
        /// We later found this paper, which on page 5 lists 12 counters,
        /// for each of Nehalem/Westmere, Sandy Bridge and Ivy Bridge:
        /// http://web.eece.maine.edu/~vweaver/projects/deterministic/deterministic_counters.pdf
        /// It appears that both Sandy Bridge and Ivy Bridge used to have
        /// `HW_INTERRUPTS.RECEIVED` documented, before Intel removed every
        /// mention of the counter from newer versions of their manuals.
        Bridge,

        /// Haswell / Broadwell:
        /// * client: Haswell (S/ULT/GT3e) / Broadwell (U/Y/S/H/C/W)
        /// * server: Haswell (E/EP/EX) / Broadwell (E/EP/EX/DE/Hewitt Lake)
        ///
        /// Equally as undocumented as "Sandy Bridge / Ivy Bridge" (see above).
        ///
        /// These are the (pre-Skylake) *Well CPU models confirmed so far:
        /// * Haswell (client) Family 6 Model 60
        ///     Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz (@m-ou-se)
        /// * Haswell (server) Family 6 Model 63
        ///     Intel(R) Xeon(R) CPU E5-2697 v3 @ 2.60GHz (@cuviper)
        /// * Haswell (client + GT3e) Family 6 Model 70
        ///     Intel(R) Core(TM) i7-4750HQ CPU @ 2.00GHz (@nagisa)
        ///     Intel(R) Core(TM) i7-4770HQ CPU @ 2.20GHz (@m-ou-se)
        Well,

        /// Skylake / Skylake-derived:
        /// * client: Skylake (Y/U/DT/H/S) / Kaby Lake (Y/U/DT/H/S/X) / Coffee Lake (U/S/H/E)
        /// * server: Skylake (SP/X/DE/W) / Cascade Lake (SP/X/W)
        ///
        /// Both "client" and "server" product lines have documented support
        /// for counting "hardware interrupts" (`HW_INTERRUPTS.RECEIVED`).
        ///
        /// Intel does not make it clear that future product lines, such as
        /// "Ice Lake", will continue to support this (or with what config),
        /// and even "Comet Lake" (aka "10th gen") isn't explicitly listed.
        Lake,

        /// Unknown Intel CPU, contemporary to/succeeding *Bridge/*Well/*Lake,
        /// but likely similar to them.
        UnknownMaybeLakeLike,
    }

    impl CpuModel {
        /// Detect the model of the current CPU using `cpuid`.
        pub(super) fn detect() -> Result<Self, Box<dyn Error + Send + Sync>> {
            let cpuid0 = unsafe { std::arch::x86_64::__cpuid(0) };
            let cpuid1 = unsafe { std::arch::x86_64::__cpuid(1) };
            let mut vendor = [0; 12];
            vendor[0..4].copy_from_slice(&cpuid0.ebx.to_le_bytes());
            vendor[4..8].copy_from_slice(&cpuid0.edx.to_le_bytes());
            vendor[8..12].copy_from_slice(&cpuid0.ecx.to_le_bytes());

            let vendor = std::str::from_utf8(&vendor).map_err(|_| {
                format!(
                    "cpuid returned non-UTF-8 vendor name: cpuid(0)={:?} cpuid(1)={:?}",
                    cpuid0, cpuid1
                )
            })?;

            let version = cpuid1.eax;

            let mut family = (version >> 8) & 0xf;
            if family == 15 {
                // Extended family.
                family += (version >> 20) & 0xff;
            }

            let mut model = (version >> 4) & 0xf;
            if family >= 15 || vendor == "GenuineIntel" && family == 6 {
                // Extended model.
                model += ((version >> 16) & 0xf) << 4;
            }

            info!(
                "CpuModel::detect: vendor={:?} family={} model={}",
                vendor, family, model
            );

            match vendor {
                "AuthenticAMD" => {
                    use self::AmdGen::*;

                    let (gen, name) = match (family, model) {
                        (0..=14, _) | (19, _) => {
                            return Err(format!(
                                "impossible AMD64 CPU detected (Family {} Model {}); {}",
                                family,
                                model,
                                super::BUG_REPORT_MSG
                            )
                            .into());
                        }

                        (15, _) => (PreZen, "K8 (Hammer)"),
                        (16, _) => (PreZen, "K10 (Barcelona/Shanghai/Istanbul)"),
                        (17, _) => (PreZen, "K8+K10 hybrid (Turion X2 Ultra)"),
                        (18, _) => (PreZen, "Fusion"),
                        (20, _) => (PreZen, "Bobcat"),
                        (21, _) => (PreZen, "Bulldozer / Piledriver / Steamroller / Excavator"),
                        (22, _) => (PreZen, "Jaguar / Puma"),

                        (23, 1) => (Zen, "Zen (Naples/Whitehaven/Summit Ridge/Snowy Owl)"),
                        (23, 17) => (Zen, "Zen (Raven Ridge)"),
                        (23, 24) => (Zen, "Zen (Banded Kestrel/Dali) / Zen+ (Picasso)"),
                        (23, 8) => (Zen, "Zen+ (Pinnacle Ridge)"),
                        (23, 49) => (Zen, "Zen 2 (Rome/Castle Peak)"),
                        (23, 113) => (Zen, "Zen 2 (Matisse)"),

                        (23..=0xffff_ffff, _) => {
                            really_warn!(
                                "CpuModel::detect: unknown AMD CPU (Family {} Model {}), \
                                 assuming Zen-like; {}",
                                family,
                                model,
                                super::BUG_REPORT_MSG
                            );

                            (UnknownMaybeZenLike, "")
                        }
                    };

                    if !name.is_empty() {
                        info!("CpuModel::detect: known AMD CPU: {}", name);
                    }

                    // The `SpecLockMap` (speculative atomic aka `lock` instruction
                    // execution, unclear what "Map" refers to) feature in AMD Zen CPUs
                    // causes non-deterministic overcounting of atomic instructions,
                    // presumably whenever it has to roll back the speculation
                    // (as in, the performance counters aren't rolled back).
                    // Even this this may be rare when uncontended, it adds up.
                    //
                    // There is an MSR bit (`MSRC001_1020[54]`) that's not officially
                    // documented, but which several motherboards and profiling tools
                    // set whenever IBS (Instruction-Based Sampling) is in use, and
                    // it is sometimes referred to as "disabling `SpecLockMap`"
                    // (hence having a name for the feature that speculates `lock`s).
                    //
                    // One way we could detect that the bit has been set would be to
                    // parse `uname().release` (aka `uname -r`) and look for versions
                    // which are known to include the patch suggested in this thread:
                    // https://github.com/mozilla/rr/issues/2034#issuecomment-693761247
                    //
                    // However, one may set the bit using e.g. `wrmsr`, even on older
                    // kernels, so a more reliable approach is to execute some atomics
                    // and look at the `SpecLockMapCommit` (`r0825:u`) Zen counter,
                    // which only reliably remains `0` when `SpecLockMap` is disabled.
                    if matches!(gen, Zen | UnknownMaybeZenLike) {
                        if let Ok(spec_lock_map_commit) =
                            Counter::with_type_and_hw_id(perf_type_id_PERF_TYPE_RAW, 0x08_25)
                        {
                            use super::HwCounterRead;

                            let start_spec_lock_map_commit = spec_lock_map_commit.read();

                            // Execute an atomic (`lock`) instruction, which should
                            // start speculative execution for following instructions
                            // (as long as `SpecLockMap` isn't disabled).
                            let mut atomic: u64 = 0;
                            let mut _tmp: u64 = 0;
                            unsafe {
                                asm!(
                                    // Intel syntax: "lock xadd [{atomic}], {tmp}"
                                    "lock xadd {tmp}, ({atomic})",

                                    atomic = in(reg) &mut atomic,
                                    tmp = inout(reg) _tmp,

                                    // Older versions of LLVM do not support modifiers in
                                    // Intel syntax inline asm; whenever Rust minimum LLVM
                                    // version supports Intel syntax inline asm, remove
                                    // and replace above instructions with Intel syntax
                                    // version (from comments).
                                    options(att_syntax),
                                );
                            }

                            if spec_lock_map_commit.read() != start_spec_lock_map_commit {
                                really_warn!(
                                    "CpuModel::detect: SpecLockMap detected, in AMD {} CPU; \
                                     this may add some non-deterministic noise - \
                                     for information on disabling SpecLockMap, see \
                                     https://github.com/mozilla/rr/wiki/Zen",
                                    name
                                );
                            }
                        }
                    }

                    Ok(CpuModel::Amd(gen))
                }

                "GenuineIntel" => {
                    use self::IntelGen::*;

                    let (gen, name) = match (family, model) {
                        // No need to name these, they're unsupported anyway.
                        (0..=5, _) => (PreBridge, ""),
                        (15, _) => (PreBridge, "Netburst"),
                        (6, 0..=41) => (PreBridge, ""),

                        // Older Xeon Phi CPUs, misplaced in Family 6.
                        (6, 87) => (PreBridge, "Knights Landing"),
                        (6, 133) => (PreBridge, "Knights Mill"),

                        // Older Atom CPUs, interleaved with other CPUs.
                        // FIXME(eddyb) figure out if these are like *Bridge/*Well.
                        (6, 53) | (6, 54) => (PreBridge, "Saltwell"),
                        (6, 55) | (6, 74) | (6, 77) | (6, 90) | (6, 93) => {
                            (PreBridge, "Silvermont")
                        }
                        (6, 76) => (PreBridge, "Airmont (Cherry Trail/Braswell)"),

                        // Older server CPUs, numbered out of order.
                        (6, 44) => (PreBridge, "Westmere (Gulftown/EP)"),
                        (6, 46) => (PreBridge, "Nehalem (EX)"),
                        (6, 47) => (PreBridge, "Westmere (EX)"),

                        (6, 42) => (Bridge, "Sandy Bridge (M/H)"),
                        (6, 45) => (Bridge, "Sandy Bridge (E/EN/EP)"),
                        (6, 58) => (Bridge, "Ivy Bridge (M/H/Gladden)"),
                        (6, 62) => (Bridge, "Ivy Bridge (E/EN/EP/EX)"),

                        (6, 60) => (Well, "Haswell (S)"),
                        (6, 61) => (Well, "Broadwell (U/Y/S)"),
                        (6, 63) => (Well, "Haswell (E/EP/EX)"),
                        (6, 69) => (Well, "Haswell (ULT)"),
                        (6, 70) => (Well, "Haswell (GT3e)"),
                        (6, 71) => (Well, "Broadwell (H/C/W)"),
                        (6, 79) => (Well, "Broadwell (E/EP/EX)"),
                        (6, 86) => (Well, "Broadwell (DE/Hewitt Lake)"),

                        (6, 78) => (Lake, "Skylake (Y/U)"),
                        (6, 85) => (Lake, "Skylake (SP/X/DE/W) / Cascade Lake (SP/X/W)"),
                        (6, 94) => (Lake, "Skylake (DT/H/S)"),
                        (6, 142) => (Lake, "Kaby Lake (Y/U) / Coffee Lake (U)"),
                        (6, 158) => (Lake, "Kaby Lake (DT/H/S/X) / Coffee Lake (S/H/E)"),

                        (6..=14, _) | (16..=0xffff_ffff, _) => {
                            really_warn!(
                                "CpuModel::detect: unknown Intel CPU (Family {} Model {}), \
                                 assuming Skylake-like; {}",
                                family,
                                model,
                                super::BUG_REPORT_MSG
                            );

                            (UnknownMaybeLakeLike, "")
                        }
                    };

                    if !name.is_empty() {
                        info!("CpuModel::detect: known Intel CPU: {}", name);
                    }

                    Ok(CpuModel::Intel(gen))
                }

                _ => Err(format!(
                    "cpuid returned unknown CPU vendor {:?}; version={:#x}",
                    vendor, version
                )
                .into()),
            }
        }

        /// Return the hardware performance counter configuration for
        /// counting "hardware interrupts" (documented or not).
        fn irqs_counter_config(&self) -> Result<u32, Box<dyn Error + Send + Sync>> {
            match self {
                CpuModel::Amd(model) => match model {
                    AmdGen::PreZen => Ok(0x00_cf),
                    AmdGen::Zen | AmdGen::UnknownMaybeZenLike => Ok(0x00_2c),
                },
                CpuModel::Intel(model) => match model {
                    IntelGen::PreBridge => Err(format!(
                        "counting IRQs not yet supported on Intel CPUs \
                         predating Sandy Bridge; {}",
                        super::BUG_REPORT_MSG
                    )
                    .into()),
                    IntelGen::Bridge
                    | IntelGen::Well
                    | IntelGen::Lake
                    | IntelGen::UnknownMaybeLakeLike => Ok(0x01_cb),
                },
            }
        }
    }
}

#[cfg(not(all(target_arch = "x86_64", target_os = "linux")))]
mod hw {
    use std::error::Error;

    pub(super) enum Counter {}

    impl Counter {
        pub(super) fn new(
            model: &CpuModel,
            _: super::HwCounterType,
        ) -> Result<Self, Box<dyn Error + Send + Sync>> {
            match *model {}
        }
    }

    impl super::HwCounterRead for Counter {
        type Output = u64;

        #[inline]
        fn read(&self) -> u64 {
            match *self {}
        }
    }

    impl super::HwCounterRead for (&Counter, &Counter) {
        type Output = (u64, u64);

        #[inline]
        fn read(&self) -> (u64, u64) {
            match *self.0 {}
        }
    }

    pub(super) enum CpuModel {}

    impl CpuModel {
        pub(super) fn detect() -> Result<Self, Box<dyn Error + Send + Sync>> {
            // HACK(eddyb) mark `really_warn!` (and transitively `log` macros)
            // and `BUG_REPORT_MSG` as "used" to silence warnings.
            if false {
                really_warn!("unsupported; {}", super::BUG_REPORT_MSG);
            }

            let mut msg = String::new();
            let mut add_error = |s| {
                if !msg.is_empty() {
                    msg += "; ";
                }
                msg += s;
            };

            if cfg!(not(target_arch = "x86_64")) {
                add_error("only supported architecture is x86_64");
            }

            if cfg!(not(target_os = "linux")) {
                add_error("only supported OS is Linux");
            }

            Err(msg.into())
        }
    }
}

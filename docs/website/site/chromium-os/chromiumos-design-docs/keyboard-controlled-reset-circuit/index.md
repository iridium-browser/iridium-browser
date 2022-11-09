---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/chromiumos-design-docs
  - Design Documents
page_name: keyboard-controlled-reset-circuit
title: Keyboard-Controlled Reset Circuit
---

To enable users to securely reset a Chrome device, or enter recovery and
developer modes, a hardware circuit is required. This function can be
implemented with a Silego chip, or an equivalent combination of parts and logic.

### When a user presses a combination of the power key and certain other keys, the hardware circuit monitoring the keyboard generates a reset signal to the embedded controller. The embedded controller controls power to the rest of the system, which is reset as well. The circuit contains a flip-flop that is can only be cleared by the reset signal. This flip-flop is set by a GPIO signal from the embedded controller that is sent before the embedded controller transitions from its read-only firmware to its rewriteable firmware. The output of the flip-flop goes to a GPIO on the main processor, which can use this signal to verify that the EC has been reset and is still running read-only (and hence trusted) code.

### Reset

### To reset the system, the user presses Power+F3 (Power+VolumeUp on convertible devices without a secure keyboard connection). When the embedded controller resets, it progresses from read-only firmware to rewritable firmware for operation in Normal Mode.

### Recovery mode

To enter Recovery Mode, the user holds Power+F3+ESC (Power+VolumeUp+VolumeDown
on convertible devices without a secure keyboard connection).
When the embedded controller resets in Recovery Mode, it remains in its
read-only firmware and tells the main processor to boot in Recovery Mode.

### Developer mode

Users can enter Developer Mode from Recovery Mode, usually by pressing Ctrl+D at
the recovery screen. Developer Mode can be accessed if and only if the flip-flop
indicates that the embedded controller is still in read-only mode. This
requirement ensures that the Recovery Mode firmware on the main processor can
trust the keyboard messages from the embedded controller (and thus ensures that
the keyboard messages are in fact sent by a physical user who is present at the
keyboard—not, for example, from a keyboard that has been hijacked by a remote
process).

### Optional battery cutoff

Systems with non-removable batteries may implement a battery cut-off. The
battery can be cut off by removing the power adapter while holding Power+F3
(Power+VolumeUp on convertible devices without a secure keyboard connection) for
10 seconds or more. The preferred implementation is with a control wire into the
controller/gas-gauge inside the battery pack, but an alternative is to force off
FETs in the charger circuit. This requirement allows users under direction from
support staff to completely cut off power to the device. It also enables users
to recover from situations where the power supply has locked up. For example, an
out of specification USB device causing the main power regulator to latch a
fault and shutdown until power is cycled.

[<img alt="image"
src="/chromium-os/chromiumos-design-docs/keyboard-controlled-reset-circuit/KbdReset.png">](/chromium-os/chromiumos-design-docs/keyboard-controlled-reset-circuit/KbdReset.png)

<img alt="KeyboardControlledReset.png.1353360421277.png"
src="https://lh5.googleusercontent.com/fmWa009HCo9PNdjbjeK5rpUVqax0sbm4RMc0bZpvQOM1NmhXZN867EuEZOCAWm8FMHGsIEm7ryT9kB1TITIfIvMyV32fxxw4nX9teXVWJUQ08EL5-vpybhQ9_ZMr74c55g"
height=437px; width=661px;>

This hardware circuit does the following:

*   When the Power button is pressed, it checks the keyboard matrix to
            determine whether F3 or VolumeUp (on convertible devices without a
            secure keyboard connection) is also pressed. If so, it causes a
            hard-reset of the embedded controller. This action has the side
            effect of resetting the entire system.
*   It provides an EC_IN_RW indicator for whether the embedded
            controller code has transitioned from read-only code to rewritable
            code as part of verified boot. This indicator is set by the embedded
            controller via the EC_ENTERING_RW GPIO and is reset whenever a hard
            reset is triggered. EC_IN_RW should go to a PCH/SOC GPIO line that
            can be read by the main processor. EC_IN_RW is an open drain signal
            and needs to connect to a pin with an internal pullup or have a
            pullup on the board.
*   EC_RST_L resets the embedded controller (and the rest of the
            system). This is an open drain signal and should be pulled up to the
            appropriate power rail.

The embedded controller boots into read-only (RO) code. It then normally
verifies and jumps to rewritable (RW) code. Before jumping to RW code, the RO
code must assert the EC_ENTERING_RW GPIO. This will set the flip-flop, so that
it asserts the EC_IN_RW signal to the main processor.
Once the flip-flop has been set, it can only be cleared by pressing the Reset
key combination. No action by the embedded controller can clear the flip-flop.

---
breadcrumbs:
- - /chromium-os
  - Chromium OS
- - /chromium-os/ec-development
  - Chromium Embedded Controller (EC) Development
page_name: charge-manager
title: Chrome OS EC Charge Manager
---

cros-ec boards with USB-C / USB-PD support have several independent
charge-related tasks, controlling several charge ports. These tasks use
different protocols (PD, BC1.2, type-C) to determine the voltage and input
current limit of an attached charger. Charge manager is kept up to date with
these current limits, and using its selection logic, chooses the active charge
port (there can be only one) and its current limit.

charge_manager_update() is the basic function tasks call to inform charge
manager of a power supply change.

Port / limit selection is handled by a deferred task charge_manager_refresh().
This task is queued every time a change occurs that may change port or current
limit selection.

Port Selection Logic

1.  Prefer override port over all others.
2.  Prefer higher priority supplier (ex. PD over USB-C over BC1.2) over
            lower-priority.
3.  Prefer a charger capable of supplying more power if supplier
            priority is tied between ports.
4.  Prefer the current charge port (don’t switch) in case the above are
            all tied.

Extra Selection Notes

*   Don’t charge from a dual-role partner, unless it’s the override
            port.
*   Don’t select a charge port / charge limit until all suppliers
            (tasks) / ports have reported some charge value (ex. zero, if
            nothing is plugged).

Charge Override

Override is set at the port level -- charge manager will ignore its normal port
selection logic and choose the override port as its active charge port. Also,
override supports a “don’t charge” parameter, where no port will be selected.

Override is automatically removed when a dedicated charger is plugged, or when
the charger is removed from the override port.

Delayed Override

If a dual-role device is chosen as the charge supplier via override, it may be
required to perform a power swap so that the port can switch from a source to a
sink role. The power swap takes time, and is not guaranteed to complete -- this
is known as a delayed override. Once the swap takes effect and the dual-role
device begins supplying power, the requested port will become the override port.
If the device never begins supplying power, the requested port will not become
the override port.

If the device doesn’t supply power within a timeout period (based upon
worst-case PD protocol timings), we abandon our delayed override.

Charge Ceiling

Charge manager selects the active port based upon the reported charge limit of
the attached supply. Sometimes, we may want to limit the amount of current that
we draw from a supply (ex. if we’re in the processes of negotiating to a higher
current limit) without influencing charge manager’s port selection.
charge_manager_set_ceil is used to set an artificial limit in this case. The
“charge ceiling” will be applied when assigning a current limit, but the charge
supplier limit will be used for port selection.

New Power Request

pd_set_new_power_request() is called with a port parameter when a port was
previously active and becomes non-active, or when a port was previously
non-active and becomes active. The PD task may take actions based upon port
selection, for example, switching to a higher-power PDO.

Rejected Port

The callback function board_set_active_charge_port() returns a value that
indicates whether the port change was accepted or not. A port may be rejected in
certain cases, in which case the “next best” port will be selected as active by
charge manager.

Safe Mode

To accommodate boot with low, missing or software-disconnected batteries, it's
often necessary to relax port selection and ILIM restrictions to prevent device
brown-out. On power-up, charge manager starts in safe mode, with the following
behavior changes:

1.  All chargers are considered dedicated (and thus are valid charge
            source candidates) for the purpose of port selection.
2.  Charge ceilings are ignored. Most significantly, ILIM won't drop on
            PD voltage transition.
3.  CHARGE_PORT_NONE will not be selected. Power-on default charge port
            will remain selected rather than CHARGE_PORT_NONE.

After leaving safe mode, charge_manager reverts to its normal behavior and
immediately selects charge port and current using standard rules.

Interfaces

Source:
<https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/common/charge_manager.c>

Unit tests:
<https://chromium.googlesource.com/chromiumos/platform/ec/+/HEAD/test/charge_manager.c>

Basic functionality:

charge_manager_update_charge() - Tell charge manager about a new charge source.

charge_manager_update_dualrole() - Tell charge manager about a change to the
dualrole capability of the port partner.

charge_manager_set_ceil() - Tell charge manager to set an artificial charge
limit on a port, despite its capabilities.

charge_manager_force_ceil() - A specialized version of charge_manager_set_ceil()
that causes the charge limit to go into effect immediately. It’s used only in
rare cases when we don’t have time to wait for the normal
charge_manager_refresh() to pick up the update.

charge_manager_set_override() - Tell charge manager to override its normal port
selection logic and charge from a given port, or don’t charge from any ports.

charge_manager_leave_safe_mode() - Leave safe mode (typically called when
battery is confirmed to supply sufficient power) and use standard port / ILIM
selection rules.

State checking:

charge_manager_get_override() - Returns current override status, see
charge_manager_set_override().

charge_manager_get_active_charge_port() - Returns current active charge port.

charge_manager_get_power_limit_uw() - Returns the power limit selected by charge
manager

charge_manager_get_charger_current() - Returns the ILIM selected by charge
manager

Logging:

charge_manager_save_log() - Writes a lot entry containing charge_manager state
data.

Source port control:

charge_manager_source_port() - Inform charge_manager that a port has become a PD
source, or is no longer a PD source, for controlling source current limit.

charge_manager_get_source_pdo() - Returns PDO index of the source cap to send,
given saved information about how many ports are currently sources.

Callbacks:

board_set_active_charge_port() - Called when active charge port changes.

board_set_charge_limit() - Called when active charge limit changes.

pd_set_new_power_request() - Called when a port becomes active or becomes
non-active.

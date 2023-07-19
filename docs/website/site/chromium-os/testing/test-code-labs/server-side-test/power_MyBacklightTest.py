# Copyright (c) 2013 The ChromiumOS Authors.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, subprocess, tempfile, time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import hosts
from autotest_lib.server import test
from autotest_lib.server import utils

class power_MyBacklightTest(test.test):
    """Test to check backlight brightness restored after various actions."""
    version = 1

    def _client_cmd(self, cmd):
        """Execute a command on the client.
        
        @param cmd: command to execute on client.
        @return: The output object, with stdout and stderr fields.
        """
        logging.info('Client cmd: [%s]', cmd)
        return self._client.run(cmd)


    def _client_cmd_and_wait_for_restart(self, cmd):
        boot_id = self._client.get_boot_id()
        self._client_cmd('sh -c "sync; sleep 1; %s" >/dev/null 2>&1 &' % cmd)
        self._client.wait_for_restart(old_boot_id=boot_id)


    def _get_brightness(self):
        """Get brightness in integer value. It's not a percentage value and
        may be over 100"""
        result = self._client_cmd('backlight_tool --get_brightness')
        return int(result.stdout.rstrip())


    def _set_brightness_percent(self, brightness=100):
        result = self._client_cmd('backlight_tool --set_brightness_percent %d'
                                  % brightness)


    def _check_power_status(self):
        cmd_result = self._client_cmd('status powerd')
        if 'running' not in cmd_result.stdout:
            raise error.TestError('powerd must be running.')

        result = self._client_cmd('power_supply_info | grep online')
        if 'yes' not in result.stdout:
            raise error.TestError('power must be plugged in.')


    def _do_reboot(self):
        self._client_cmd_and_wait_for_restart('reboot')


    def _do_panic(self):
        self._client_cmd_and_wait_for_restart('echo panic > /proc/breakme')


    def _do_logout(self):
        self._client_cmd('restart ui')


    def _do_suspend(self):
        # The method calls a client side test 'power_resume' to do a power
        # suspend on the DUT. It is easy yet powerful for one test to run
        # another test to implement certain action without duplicating code.
        self._client_at.run_test('power_Resume')


    def _do_power_off(self):
        # TODO(sque): Do power off thru servo.  crosbug.com/25867
        pass


    _transitions = { #'reboot': _do_reboot,
                     #'panic': _do_panic,
                     #'logout': _do_logout,
                     #'suspend': _do_suspend,
                     # 'power_off': _do_power_off,
                     }

    _als_disabled = False

    def _set_als_disable(self):
        """Turns off ALS in power manager.  Saves the old 
        has_ambient_light_sensor flag if it exists.
        """
        als_path = '/var/lib/power_manager/has_ambient_light_sensor'
        self._client_cmd('if [ -e %s ]; then mv %s %s_backup; fi' %
                         (als_path, als_path, als_path))
        self._client_cmd('echo 0 > %s' % als_path)
        self._client_cmd('restart powerd')
        self._als_disabled = True

    def _set_als_disable(self):
        """Turns off ALS in power manager.  Saves the old 
        has_ambient_light_sensor flag if it exists.
        """
        als_path = '/var/lib/power_manager/has_ambient_light_sensor'
        self._client_cmd('if [ -e %s ]; then mv %s %s_backup; fi' %
                         (als_path, als_path, als_path))
        self._client_cmd('echo 0 > %s' % als_path)
        self._client_cmd('restart powerd')
        self._als_disabled = True


    def _restore_als_disable(self):
        """Restore the has_ambient_light_sensor flag setting that was
        overwritten in _set_als_disable.
        """
        if not self._als_disabled:
            return
        als_path = '/var/lib/power_manager/has_ambient_light_sensor'
        self._client_cmd('rm %s' % als_path)
        self._client_cmd('if [ -e %s_backup ]; then mv %s_backup %s; fi' %
                         (als_path, als_path, als_path))
        self._client_cmd('restart powerd')


    def run_once(self, client_ip):
        """Run the test.

        For each system transition event in |_transitions|:
           Read the brightness.
           Trigger transition event.
           Wait for client to come back up.
           Check new brightness against previous brightness.

        @param client_ip: string of client's ip address (required)
        """
        if not client_ip:
            error.TestError("Must provide client's IP address to test")
        # Create a custom host class for this machine, which is used to execute
        # commands and other functions.
        self._client = hosts.create_host(client_ip)
        # Create an Autotest instance which you can run method like run_test,
        # which can execute another client side test to facilitate this test.
        self._client_at = autotest.Autotest(self._client)

        self._results = {}

        self._check_power_status()
        self._set_als_disable()

        # Save the original brightness, to be restored after the test ends.
        self._original_brightness = self._get_brightness()
        # Set the brightness to a random number which is different from
        # system default value.
        self._set_brightness_percent(71)

        # Run the transition event tests.
        for test_name in self._transitions:
            self._old_brightness = self._get_brightness()

            self._transitions[test_name](self)

            # Save the before and after backlight values.
            self._results[test_name] = { 'old': self._old_brightness,
                                         'new': self._get_brightness() }


    def cleanup(self):
        """Restore DUT's condition before the test starts, and check the test
        results.
        """
        self._restore_als_disable()
        self._set_brightness_percent(self._original_brightness)

        # Check results to make sure backlight levels were preserved across
        # transition events.
        num_failed = 0
        for test_name in self._results:
            old_brightness = self._results[test_name]['old']
            new_brightness = self._results[test_name]['new']

            if old_brightness == new_brightness:
                logging.info('Transition event [  PASSED  ]: %s', test_name)
            else:
                logging.info('Transition event [  FAILED  ]: %s', test_name)
                logging.info('   Brightness changed: %d -> %d',
                             old_brightness, new_brightness)
                num_failed += 1

        if num_failed > 0:
            raise error.TestFail(('Failed to preserve backlight over %d '
                                  'transition event(s).') % num_failed)

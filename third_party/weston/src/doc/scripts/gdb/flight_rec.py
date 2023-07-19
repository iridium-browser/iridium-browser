#
#  Copyright © 2019 Collabora Ltd.
#
#  Permission is hereby granted, free of charge, to any person obtaining
#  a copy of this software and associated documentation files (the
#  "Software"), to deal in the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to
#  permit persons to whom the Software is furnished to do so, subject to
#  the following conditions:
#
#  The above copyright notice and this permission notice (including the
#  next paragraph) shall be included in all copies or substantial
#  portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
#  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
#  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.
#
# Usage: source this script then 'display_flight_rec'
#

import gdb

class DisplayFlightRecorder(gdb.Command):
    def __init__(self):

        self.rb = ''
        symbol_found = False

        ring_buff =  gdb.lookup_global_symbol("weston_primary_flight_recorder_ring_buffer")
        if ring_buff == None:
            print("'weston_ring_buffer' symbol not found!")
            print("Either weston is too old or weston hasn't been loaded in memory")
        else:
            self.rb = ring_buff
            self.display_rb_data(self.rb)
            symbol_found = True

        if symbol_found:
            super(DisplayFlightRecorder, self).__init__("display_flight_rec",
                                                        gdb.COMMAND_DATA)

    def display_rb_data(self, rb):
        print("Flight recorder data found. Use 'display_flight_rec' "
                "to display its contents")
        # display this data (only) if symbol is not empty (happens if the program is not ran at all)
        if rb.value():
            print("Data at byte {append}, Size: {size}B, "
                  "Overlaped: {overlap}".format(append=rb.value()['append_pos'],
                                                size=rb.value()['size'],
                                                overlap=rb.value()['overlap']))

    # poor's man fwrite()
    def gen_contents(self, start, stop):
        _str = ''
        for j in range(start, stop):
            _str += chr(self.rb.value()['buf'][j])
        return _str

    # mirrors C version, as to make sure we're not reading other parts...
    def display_flight_rec_contents(self):

        # symbol is there but not loaded, we're not far enough
        if self.rb.value() == 0x0:
            print("Flight recorder found, but not loaded yet!")
            return
        else:
            print("Displaying flight recorder contents:")

        append_pos = self.rb.value()['append_pos']
        size = self.rb.value()['size']
        overlap = self.rb.value()['overlap']

        # if we haven't overflown and we're still at 0 means
        # we still aren't far enough to be populated
        if append_pos == 0 and not overlap:
            print("Flight recorder doesn't have anything to display right now")
            return

        # now we can print stuff
        rb_data = ''
        if not overlap:
            if append_pos:
                rb_data = self.gen_contents(0, append_pos)
            else:
                rb_data = self.gen_contents(0, size)
        else:
            rb_data = self.gen_contents(append_pos, size)
            rb_data += self.gen_contents(0, append_pos)

        print("{data}".format(data=rb_data))

    # called when invoking 'display_flight_rec'
    def invoke(self, arg, from_tty):
        self.display_flight_rec_contents()

DisplayFlightRecorder()

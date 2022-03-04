#!/usr/bin/perl
use warnings;
use strict;
$|++;

# Test all tables with lou_checktable.
#
# Copyright (C) 2010 by Swiss Library for the Blind, Visually Impaired and Print Disabled
# Copyright (C) 2012-2013 Mesar Hameed <mesar.hameed@gmail.com>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved. This file is offered as-is,
# without any warranty.

my $fail = 0;
# some tables are quite big and take some time to check, so keep the timeout reasonably long
my $timeout = 120; # seconds

# In the test suite the LOUIS_TABLEPATH is set up so that it actually
# points at the top src dir of liblouis. So to find the productive
# tables, i.e. the ones that are shipped with liblouis (and need to be
# tested) we have to check the tables subdir.
my $tablesdir = "$ENV{LOUIS_TABLEPATH}/tables";

# get all the tables from the tables directory
my @tables = glob("$tablesdir/*.{utb,ctb,tbl}");


foreach my $table (@tables) {
    if (my $pid = fork) {
	waitpid($pid, 0);
	if ($?) {
	    print STDERR "lou_checktable on $table failed or timed out\n";
	    $fail = 1;
	}
    } else {
	die "cannot fork: $!" unless defined($pid);
	alarm $timeout;
	exec ("../tools/lou_checktable $table --quiet");
	die "Exec of lou_checktable failed: $!";
    }
}

exit $fail;

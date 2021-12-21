#!/usr/bin/perl
use warnings;
use strict;
use Cwd 'abs_path';
$|++;

# Test a specific table with lou_checktable which causes an endless loop.
#
# Copyright (C) 2010,2018 by Swiss Library for the Blind, Visually Impaired and Print Disabled
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved. This file is offered as-is,
# without any warranty.

my $abs_top_srcdir = abs_path("$ENV{srcdir}/..");

my @paths = (
    "$abs_top_srcdir/tables",
    "$abs_top_srcdir/tests/tables",
    );

$ENV{"LOUIS_TABLEPATH"} = join(",", @paths);

my $table="loop.ctb";

$SIG{ALRM} = sub { die "lou_checktable on $table stuck in endless loop\n" };

alarm 5;
system("../tools/lou_checktable $table --quiet") == 0 
    or die "lou_checktable on $table failed\n";
alarm 0;

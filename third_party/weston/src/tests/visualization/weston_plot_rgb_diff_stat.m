% -- weston_plot_rgb_diff_stat (fname)
% -- weston_plot_rgb_diff_stat (fname, scale)
% -- weston_plot_rgb_diff_stat (fname, scale, x_column)
%	Plot an rgb_diff_stat dump file
%
%	Creates a new figure and draws four sub-plots: R difference,
%	G difference, B difference, and two-norm error.
%
%	Scale defaults to 255. It is used to multiply both x and y values
%	in all plots. Note that R, G and B plots will contain horizontal lines
%	at y = +/- 0.5 to help you see the optimal rounding error range for
%	the integer encoding [0, scale]. Two-norm plot contains a horizontal
%	line at y = sqrt(0.75) which represents an error sphere with the radius
%	equal to the two-norm of RGB error (0.5, 0.5, 0.5).
%
%	By default, x-axis is sample index, not multiplied by scale. If
%	x_column is given, it is a column index in the dump file to be used as
%	the x-axis values, multiplied by scale. Indices start from 1, not 0.

% SPDX-FileCopyrightText: 2022 Collabora, Ltd.
% SPDX-License-Identifier: MIT

function weston_plot_rgb_diff_stat(fname, scale, x_column);

S = load(fname);

if nargin < 2
	scale = 255;
end
if nargin < 3
	x = 1:size(S, 1);
else
	x = S(:, x_column) .* scale;
end

x_lim = [min(x) max(x)];

evec = S(:, 1) .* scale; # two-norm error
rvec = S(:, 2) .* scale; # r diff
gvec = S(:, 3) .* scale; # g diff
bvec = S(:, 4) .* scale; # b diff

figure

subplot(4, 1, 1);
plot(x, rvec, 'r');
plus_minus_half_lines(x_lim);
title(fname, "Interpreter", "none");
ylabel('R diff');
axis("tight");

subplot(4, 1, 2);
plot(x, gvec, 'g');
plus_minus_half_lines(x_lim);
ylabel('G diff');
axis("tight");

subplot(4, 1, 3);
plot(x, bvec, 'b');
plus_minus_half_lines(x_lim);
ylabel('B diff');
axis("tight");

subplot(4, 1, 4);
plot(x, evec, 'k');
hold on;
plot(x_lim, [1 1] .* sqrt(0.75), 'k:');
ylabel('Two-norm');
axis("tight");

max_abs_err = [max(abs(rvec)) max(abs(gvec)) max(abs(bvec))]

function plus_minus_half_lines(x_lim);

hold on;
plot(x_lim, [0.5 -0.5; 0.5 -0.5], 'k:');


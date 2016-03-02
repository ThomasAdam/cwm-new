#!/usr/bin/perl
#
# convert-cwmrc:  a script to convert an existing .cwmrc file into
# libconfuse format.
#
# Copyright (c) 2016, Thomas Adam <thomas@xteddu.org>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;

sub usage
{
	warn "convert-cwmrc [source-file] [destination-file]\n";
	exit 0;
}

if (@ARGV != 2) {
	usage();
}

my ($in_file, $out_file) = @ARGV;
my %orig_data = ();

sub scan_file
{
	open my $in_fh, "<", $in_file or die $!;
	while (my $line = <$in_fh>) {
		chomp $line;

		if ($line =~ /\\\s*$/) {
			$line =~ s/\\\s*$//;
			$line .= <$in_fh>;
			redo;
		}

		if ($line =~ /^\s*snapdist/i) {
			push @{ $orig_data{"screen \"*\""}->{''} }, join " = ", split /\s+/, $line, 2;
		} elsif ($line =~ /^\s*color\s+(.*?)\s+(.*?)$/i) {
			my $opt = $1;
			$opt = "fontsel" if $1 eq "selfont";
			push @{ $orig_data{"screen \"*\""}->{"groups"}->{"group \"*\""}->{"color"} }, "$opt = $2";
		} elsif ($line =~ /^\s*bind\s*(.*?)\s+(.*?)$/) {
			push @{ $orig_data{bindings}->{"key $1"} }, "command = $2";
		} elsif ($line =~ /^\s*mousebind\s*(.*?)\s+(.*?)$/) {
			push @{ $orig_data{bindings}->{"mouse $1"} }, "command = $2";
		} elsif ($line =~ /^\s*gap\s*(.*?)$/) {
			push @{ $orig_data{"screen \"*\""}->{''} }, "gap = {" . join ",",
				split /\s+/, $1 . "}";
		} elsif ($line =~ /^\s*command\s*(.*?)\s+(.*?)$/) {
			push @{ $orig_data{'menu'}->{"item $1"} }, "command = $2";
		} elsif ($line =~ /^\s*borderwidth\s+(.*?)$/i) {
			push @{ $orig_data{"screen \"*\""}->{"groups"}->{"group \"*\""}->{''} }, "borderwidth = $1";
		} elsif ($line =~ /^\s*ignore\s+(.*?)$/i) {
			push @{ $orig_data{clients}->{"client $1"} }, "ignore = true";
		} elsif ($line =~ /^\s*autogroup\s+(.*?)\s+(.*?)$/) {
			push @{ $orig_data{clients}->{"client $2"} }, "autogroup = $1";
		}
	}
	close($in_fh);

	check_bindings();
	write_file();
}

sub check_bindings
{
	# Look through the key/mouse bindings.  Where there's two entries, this
	# is invalid in the libconfuse format.  In most cases, these entries
	# will contain a command and one to unmap the binding.  In such cases,
	# delete the unmap entry, since the command will replace existing
	# bindings.

	foreach my $k (keys %{ $orig_data{'bindings'} }) {
		my @binds = @{ $orig_data{'bindings'}->{$k} };
		if (@binds >= 2) {
			my $count = 0;
			foreach my $item (@binds) {
				if ($item eq "command = unmap") {
					delete $binds[$count];
				}
				$count++;
			}
			@binds = grep {defined} @binds;
			$orig_data{'bindings'}->{$k} = \@binds;
		}
	}
}

# Recursively walk the data structure, converting the hashes to top-level
# sections, and the arrays as values therein.
my $indent = -1;
sub walk_data
{
	my ($fh, $h) = @_;
	my $tabs;

	if (ref $h eq 'HASH') {
		$indent++;
		$tabs = $indent > 0 ? "\t" x $indent : "";
		foreach my $k (sort keys %$h) {
			if ($k eq '') {
				# Special key to denote top-level options for
				# the block, but where there's no surrounding
				# block to put them in.  In such cases, we
				# just want the values from the array
				# verbatim.

				$indent--;
				walk_data($fh, $h->{$k});
				$indent++;
			} else {
				print $fh "$tabs$k {\n";
				walk_data($fh, $h->{$k});

				$tabs = $indent > 0 ? "\t" x $indent : "";
				print $fh $tabs . "}\n", "\n";
			}
		}
		$indent--;
	}

	if (ref $h eq 'ARRAY') {
		$indent++;
		$tabs = "\t" x $indent;
		foreach my $l (@$h) {
			print $fh $tabs . $l, "\n";
		}
		$indent--;
	}
};

sub write_file
{
	open my $out_fh, ">", $out_file or die $!;
	print $out_fh <<"EOF";
# cwmrc - converted to libconfuse format via 'convert-cwmrc.pl'

EOF

	walk_data($out_fh, \%orig_data);
	close($out_fh);
}

scan_file();

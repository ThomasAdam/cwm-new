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

sub write_file
{
	open my $out_fh, ">", $out_file or die $!;
	print $out_fh <<"EOF";
# cwmrc - converted to libconfuse format via 'convert-cwmrc.pl'

EOF

	foreach my $k (keys %orig_data) {
		print $out_fh "$k {\n";
		foreach my $l (sort keys %{ $orig_data{$k} }) {
			print $out_fh "\t$l {\n\t" unless $l eq '';
			if (ref $orig_data{$k}->{$l} eq 'ARRAY') {
				print $out_fh "\t" . join "\t", "$_\n"
					foreach @{ $orig_data{$k}->{$l} };
				print $out_fh "\t}\n" unless $l eq '';
			} else {
				foreach my $m (sort keys %{ $orig_data{$k}->{$l} }) {
					print $out_fh "\t\t$m {\n";

					foreach my $n (sort keys %{ $orig_data{$k}->{$l}->{$m} }) {
						print $out_fh "\t\t\t$n {\n" unless $n eq '';
						if (ref $orig_data{$k}->{$l}->{$m}->{$n} eq 'ARRAY') {
							print $out_fh "\t\t\t\t" . join "\n", "$_\n"
								foreach @{ $orig_data{$k}->{$l}->{$m}->{$n} };
							print $out_fh "\t\t\t}\n" unless $n eq '';
						}
					}
					print $out_fh "\t\t}\n";
				}
				print $out_fh "\t}\n";
			}
		}
		print $out_fh "}\n";
	}

	close($out_fh);
}

scan_file();

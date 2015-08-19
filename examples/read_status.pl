#!/usr/bin/perl
#
# A very crude example how to turn status output from CWM in to something
# that can be interpreted by dzen2.
#
# Note that there is exactly one pipe per screen which CWM will sent output
# from, so that multiple status lines per screen is easier to handle.  To
# this end, this example script will fork off one instance per screen for
# however many named pipes exist.
#
# NO ATTEMPT has been made yet to support the placement of dzen bars on a
# machine with Xinerama installed; this currently seems to work OK though.
#
# Patches improving this script is welcome; I threw it together in about
# fifteen minutes, there's no support for SIGPIPE or anything like that yet.
#
# -- Thomas Adam, Monday 23rd December 2013

use strict;
use warnings;

use JSON;

$| = 1;

my @pipes = glob("/tmp/cwm-*.fifo");

# If there are no pipes, that's OK.
exit unless @pipes;

my %scr_map;

sub query_xrandr
{
	my $opts = {
		'-p'	=> '',
		'-d'	=> '',
		'-B'	=> 'blue',
		'-u'	=> 2,
	};

	my %lb = (
		'global_monitor' => {
			screen => '',
			data => undef,
		}
	);

	open(my $fh, '-|', 'xrandr -q') or die $!;
	my $screen_num = -1;
	while (my $line = <$fh>) {
		if ($line =~ /(.*?)\s+connected\s*(.*?)\s+/) {
			my $output = $1;
			my $geom = $2;

			$screen_num++;

			# Taking the geometry string, rearrange it slightly so that it's
			# the correct width for lemonbar.  The placement part (x,y) is
			# more useful to us.
			my ($w, $h, $x, $y) = ($geom =~ /(\d+)x(\d+)\+(\d+)\+(\d+)/);

			$opts->{'-g'} = "${w}x16+${x}+${y}";

			$lb{$output} = {
				# "clone" this each time, so we have a separate copy when
				# adding it to this has.
				data	=> { %$opts },

				# In parsing this output, we assume RandR is ordering these
				# screens in a defined order!
				screen	=> $screen_num,
			}
		}
	}
	close($fh);

	return (\%lb);
}

sub format_output
{
	my ($data) = @_;
	my $msg;
	my $extra_msg = '';
	my $extra_urgent = '';
	my $screen = $data->{'screen'};

	$msg .= "%{S$scr_map{$screen}}";

	# For the list of desktops, we maintain the sort order based on the
	# group names being 0 -> 9.
	foreach my $deskname (
		sort {
			$data->{'groups'}->{$a}->{'number'} <=>
			$data->{'groups'}->{$b}->{'number'}
		} keys %{$data->{'groups'}})
	{
		my $sym_name = $data->{'groups'}->{$deskname}->{'number'};
		my $is_current = $data->{'groups'}->{$deskname}->{'is_current'};
		my $desk_count = $data->{'groups'}->{$deskname}->{'number_of_clients'};
		my $is_urgent =  $data->{'groups'}->{$deskname}->{'is_urgent'} ||= 0;
		my $is_active =  $data->{'groups'}->{$deskname}->{'is_active'} ||= 0;

		# If the window is active, give it a differnet colour.
		if ($is_current) {
				$msg .= "|%{B#39c488} $sym_name %{B-}";

				$extra_msg .= "%{B#D7C72F}[Scr:$screen][A:$desk_count]%{B-}";
				# Gather any other bits of information for the _CURRENT_
				# group we might want.
				if ($is_urgent) {
					$extra_urgent = "%{Bred}[U]%{B-}";
				}
		} else {
			if ($is_urgent) {
					$msg .= "|%{B#7c8814} $sym_name %{B-}";
			} elsif ($is_active) {
				$msg .= "|%{B#007FFF} $sym_name %{B-}";

				# If the deskname is in the active desktops lists then
				# mark it as being viewed in addition to the currently
				# active group.
			} elsif ($desk_count > 0) {
				# Highlight groups with clients on them.
				$msg .= "|%{B#004C98} $sym_name %{B-}";
			} elsif ($desk_count == 0) {
				# Don't show groups which have no clients.
				next;
			}
		}
	}

	$msg .= "%{F#FF00FF}|%{F-}$extra_msg$extra_urgent";

	if (defined $data->{'current_client'}) {
		$msg .= "%{c}%{Ugreen}%{+u}%{+o}%{B#AC59FF}%{F-}" .
			"        " . $data->{'current_client'} . "        " .
			"%{-u}%{-o}%{B-}";
	}

	return $msg;
}

sub process_line
{
	my ($fifo) = @_;
	my $msg;

	open (my $pipe_fh, '<', $fifo) or die "Cannot open $fifo: $!";
	while (my $line = <$pipe_fh>) {
		unless ($line =~ /^\{/) {
			print $line, "\n";
			next;
		}

		print format_output(from_json($line)), "\n";
	}
}

my %opts = %{ query_xrandr() };
%scr_map = map { $_ => $opts{$_}->{'screen'} } keys (%opts);

my $screen;
foreach (@pipes) {
	($screen) = ($_ =~ /cwm-(.*?)\.fifo/);
	if (fork()) {
		# XXX: Close certain filehandles here; STDERR, etc.
		my $cmd = "lemonbar " . join(" ",
			map { $_ . " $opts{$screen}->{'data'}" }
				keys(%opts));

		process_line($_);
	}
}

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

$| = 1;

my @pipes = glob("/tmp/cwm-*.fifo");

# If there are no pipes, that's OK.
exit unless @pipes;

my $preferred = 'lemonbar';

my %dzen_options = (
	'HDMI1' => {
		'-fg' => 'white',
		'-bg' => 'blue',
		'-ta' => 'l',
		'-w'  => '1024',
		'-p'  => '',
		'-xs' => 0,
	},
	'VGA1' => {
		'-fg' => 'white',
		'-bg' => 'blue',
		'-ta' => 'l',
		'-w'  => '1024',
		'-p'  => '',
		'-xs' => 1,
	},
	'global_monitor' => {
		'-fg' => 'white',
		'-bg' => 'blue',
		'-ta' => 'l',
		'-w'  => '1024',
		'-p'  => '',
	},
);

my %lemonbar_options = (
	'HDMI1' => {
		'-p'	=> '',
		'-d'	=> '',
		'-g'	=> '1920x16+0+0',
		'-B'	=> 'blue',
		'-u'	=> 2,
	},
	'VGA1' => {
		'-p'	=> '',
		'-d'	=> '',
		'-g'	=> '1920x16+1920+0',
		'-B'	=> 'blue',
		'-u'	=> 2,
	},
	'global_monitor' => {
		'-p'	=> '',
		'-d'	=> '',
		'-B'	=> 'blue',
		'-u'	=> 0,
	},
);

my %scr_map = (
	'HDMI1'	=> 0,
	'VGA1'	=> 1,
	'global_monitor' => '',
);

sub format_output
{
	my ($data) = @_;
	my $msg;
	my $extra_msg = undef;
	my $extra_urgent = undef;
	my $screen = $data->{'screen'};

	if ($preferred eq 'lemonbar') {
		$msg .= "%{S$scr_map{$screen}}";
	}

	# For the list of desktops, we maintain the sort order based on the
	# group names being 0 -> 9.
	foreach my $deskname (
		sort {
			$data->{'desktops'}->{$a}->{'sym_name'} <=>
			$data->{'desktops'}->{$b}->{'sym_name'}
		} keys %{$data->{'desktops'}})
	{
		my $sym_name = $data->{'desktops'}->{$deskname}->{'sym_name'};
		my $is_active = $deskname eq $data->{'current_desktop'};
		my $desk_count =  $data->{'desktops'}->{$deskname}->{'count'};
		# If the window is active, give it a differnet colour.
		if ($is_active) {
			if ($preferred eq 'dzen2') {
				$msg .= "|^bg(#39c488) $sym_name ^bg()";
			} else {
				$msg .= "|%{B#39c488} $sym_name %{B-}";

				# Gather any other bits of information for the _ACTIVE_
				# group we might want.
				if (grep /^$deskname$/, @{$data->{'urgency'}}) {
					$extra_urgent = "%{Bred}[U]%{B-}";
				}

				#$extra_msg .= "%{B#D7C72F} Active clients: $desk_count %{B-} ";
				$extra_msg .= "%{r}%{B#A39A45}[$desk_count]%{B-} ";
			}
		} else {
			if (grep /^$deskname$/, @{$data->{'urgency'}}) {
				if ($preferred eq 'dzen2') {
					$msg .= "|^bg(#7c8814) $sym_name ^bg()";
				} else {
					$msg .= "|%{B#7c8814} $sym_name %{B-}";
				}
			} elsif (grep /^$deskname$/, @{$data->{'active_desktops'}}) {
				$desk_count += $data->{'desktops'}->{$deskname}->{'count'};
				if ($preferred eq 'dzen2') {
					$msg .= "|^bg(#007fff) $sym_name ^bg()";
				} else {
					$msg .= "|%{B#007FFF} $sym_name %{B-}";
				}

				# If the deskname is in the active desktops lists then
				# mark it as being viewed in addition to the currently
				# active group.
			} elsif ($desk_count > 0) {
				# Highlight groups with clients on them.
				if ($preferred eq 'dzen2') {
					$msg .= "|^bg(#004c98) $sym_name ^bg()";
				} else {
					$msg .= "|%{B#004C98} $sym_name %{B-}";
				}
			} elsif ($desk_count == 0) {
				# Don't show groups which have no clients.
				next;
			}
		}
	}

	if (defined $data->{'client'}) {
		if ($preferred eq 'dzen2') {
			$msg .= "|^bg(#7f00ff)^fg()".$data->{'client'};
		} else {
			$msg .= "%{c}%{Ugreen}%{+u}%{+o}%{B#AC59FF}%{F-}" .
			"        " . $data->{'client'} . "        " . "%{-u}%{-o}%{B-}";
		}
	}

	if (defined $extra_msg) {
		$msg .= "$extra_msg$extra_urgent";
	}

	return $msg;
}

sub process_line
{
	my ($fifo) = @_;
	my %data = ();
	my $msg;

	open (my $pipe_fh, '<', $fifo) or die "Cannot open $fifo: $!";
	while (my $line = <$pipe_fh>) {
		unless ($line =~ /^screen:/) {
			print $line, "\n";
			next;
		}

		# Each element is pipe-separated.  Key/values are then
		# comma-separated, and multiple values for those are
		# comma-separated.
		%data = map {
			chomp;
			my ($k, $v) = split /:/, $_, 2;
			defined $v ? ($k => $v) : ($k => '');
		} split(/\|/, $line);

		# For those entries we know might contain more than one
		# element, convert the comma-separated list in to an
		# array.
		exists $data{$_} and $data{$_} = [split /,/, $data{$_}]
			for (qw/urgency desktops active_desktops/);

		# For the list of desktops, there's both the name and
		# the number of clients on that desk.  This can be used
		# to flag inactive groups with clients on them.
		$data{'desktops'} = {
			map {
				($1 => {sym_name => $2, count => $3})
					if /(.*?)\s+\((.*?)\)\s+\((.*?)\)/;
			} @{$data{'desktops'}}
		};

		print format_output(\%data), "\n";
	}
}

my %opts = ($preferred eq 'dzen2') ? %dzen_options : %lemonbar_options;
my $screen;
foreach (@pipes) {
	($screen) = ($_ =~ /cwm-(.*?)\.fifo/);
	if (fork()) {
		# XXX: Close certain filehandles here; STDERR, etc.
		my $cmd = "$preferred " . join(" ",
			map { $_ . " $opts{$screen}->{$_}" } keys(%{ $opts{$screen} }));

		process_line($_);
	}
}

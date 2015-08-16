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
		my $is_urgent =  $data->{'groups'}->{$deskname}->{'is_urgent'};
		my $is_active =  $data->{'groups'}->{$deskname}->{'is_active'};

		# If the window is active, give it a differnet colour.
		if ($is_current) {
				$msg .= "|%{B#39c488} $sym_name %{B-}";

				# Gather any other bits of information for the _CURRENT_
				# group we might want.
				if ($is_urgent) {
					$extra_urgent = "%{Bred}[U]%{B-}";
				}

				$extra_msg .= "%{r}%{B#A39A45}[$desk_count]%{B-} ";
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

	if (defined $data->{'client'}) {
		$msg .= "%{c}%{Ugreen}%{+u}%{+o}%{B#AC59FF}%{F-}" .
			"        " . $data->{'client'} . "        " . "%{-u}%{-o}%{B-}";
	}

	if (defined $extra_msg) {
		$msg .= "$extra_msg$extra_urgent";
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

my %opts = %lemonbar_options;
my $screen;
foreach (@pipes) {
	($screen) = ($_ =~ /cwm-(.*?)\.fifo/);
	if (fork()) {
		# XXX: Close certain filehandles here; STDERR, etc.
		my $cmd = "lemonbar " . join(" ",
			map { $_ . " $opts{$screen}->{$_}" } keys(%{ $opts{$screen} }));

		process_line($_);
	}
}

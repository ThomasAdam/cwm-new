#!/usr/bin/perl
#
# A very crude example how to turn status output from CWM in to something
# that can be interpreted by lemonbar.
#
# https://github.com/LemonBoy/bar
#
# Patches improving this script is welcome; I threw it together in about
# fifteen minutes, there's no support for SIGPIPE or anything like that yet.
#
# -- Thomas Adam

use strict;
use warnings;

use JSON;

$| = 1;

my @pipes = glob("/tmp/cwm-[0-9]*.fifo");

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
        my $skip_all_but_global = 0;

        if (exists $data->{'screens'}->{'global_monitor'} and
            (exists $scr_map{'global_monitor'} and scalar keys %scr_map >=2)) {

            # FIXME: Bug in CWM's RandR detection means it hasn't picked up a
            # legitimate output, yet xrandr(1) will have found it.  In this
            # case, CWM will be falling back to using the builtin
            # 'global_monitor' which we must therefore use.
            $skip_all_but_global = 1;
            $scr_map{'global_monitor'}->{'screen'} = 0;
        }

	foreach my $screen  (keys %{ $data->{'screens'} }) {
                next if $skip_all_but_global and $screen ne 'global_monitor';
		my $extra_msg = '';
		my $extra_urgent = '';
		my $msg = "%{S$scr_map{$screen}->{'screen'}}";

		my $scr_h = $data->{'screens'}->{$screen};

		# For the list of desktops, we maintain the sort order based on the
		# group names being 0 -> 9.
		foreach my $deskname (
			sort {
				$scr_h->{'groups'}->{$a}->{'number'} <=>
				$scr_h->{'groups'}->{$b}->{'number'}
			} keys %{$scr_h->{'groups'}})
		{
			my $sym_name   = $scr_h->{'groups'}->{$deskname}->{'number'};
			my $is_current = $scr_h->{'groups'}->{$deskname}->{'is_current'};
			my $desk_count = $scr_h->{'groups'}->{$deskname}->{'number_of_clients'};
			my $is_urgent  = $scr_h->{'groups'}->{$deskname}->{'is_urgent'} ||= 0;
			my $is_active  = $scr_h->{'groups'}->{$deskname}->{'is_active'} ||= 0;

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

		if (defined $scr_h->{'current_client'}) {
			my $cc = $scr_h->{'current_client'};
			$msg .= "%{c}%{Ugreen}%{+u}%{+o}%{B#AC59FF}%{F-}" .
				"        " . $cc . "        " .  "%{-u}%{-o}%{B-}";
		}
		print $msg, "\n";
	}
}

sub process_line
{
	my ($fifo) = @_;
	my $msg;

	open (my $pipe_fh, '<', $fifo) or die "Cannot open $fifo: $!";
	while (my $line = <$pipe_fh>) {
		chomp $line;
		if ($line =~ /^clock:/) {
			print $line, "\n";
		} else {
			format_output(from_json($line));
		}
	}
}

my %opts = %{ query_xrandr() };
%scr_map = map {
	$_ => {
		screen => $opts{$_}->{'screen'},
	} }keys (%opts);

foreach (@pipes) {
	process_line($_);
}

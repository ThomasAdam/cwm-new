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
use IO::Pipe;

$| = 1;

my @pipes = glob("/tmp/cwm-*.fifo");

# If there are no pipes, that's OK.
exit unless @pipes;

my %dzen_options = (
	'VGA-0' => {
		'-fg' => 'white',
		'-bg' => 'blue',
		'-ta' => 'l',
		'-w'  => '1024',
		'-p'  => '',
	},
	'DVI-I-1' => {
		'-fg' => 'white',
		'-bg' => 'blue',
		'-ta' => 'l',
		'-x'  => '1920',
		'-w'  => '1024',
		'-p'  => '',
	},
	'global_monitor' => {
		'-fg' => 'white',
		'-bg' => 'blue',
		'-ta' => 'l',
		'-w'  => '1024',
		'-p'  => '',
	},
);

sub send_to_dzen
{
        my ($data, $fh) = @_;
        my $msg;

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

		# If the window is active, give it a differnet colour.
		if ($is_active) {
			$msg .= "|^bg(#39c488) $sym_name ^bg()";
		} else {
			if (grep /^$deskname$/, @{$data->{'urgency'}}) {
				$msg .= "|^bg(#7c8814) $sym_name ^bg()";
			} elsif (grep /^$deskname$/, @{$data->{'active_desktops'}}) {
				$msg .= "|^bg(#007fff) $sym_name ^bg()";
			# If the deskname is in the active desktops lists then
			# mark it as being viewed in addition to the currently
			# active group.
			} elsif ($data->{'desktops'}->{$deskname}->{'count'} > 0) {
				# Highlight groups with clients on them.
				$msg .= "|^bg(#004c98) $sym_name ^bg()";
			} elsif ($data->{'desktops'}->{$deskname}->{'count'} == 0) {
				# Don't show groups which have no clients.
				next;
			}
		}
	}

        if (defined $data->{'client'}) {
                $msg .= "|^bg(#7f00ff)^fg()".$data->{'client'};
        }

        print $fh $msg, "\n";
        $fh->flush();
}

sub process_line
{
        my ($fifo, $fh) = @_;
        my %data = ();

        open (my $pipe_fh, '<', $fifo) or die "Cannot open $fifo: $!";
        while (1) {
                while (my $line = <$pipe_fh>) {
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

                        send_to_dzen(\%data, $fh);
                }
        }
        close ($pipe_fh);
}

my $screen;
foreach (@pipes) {
	($screen) = ($_ =~ /cwm-(.*?)\.fifo/);
        if (fork()) {
                # XXX: Close certain filehandles here; STDERR, etc.
                my $dzen_cmd = "dzen2 " . join(" ",
                        map { $_ . " $dzen_options{$screen}->{$_}" }
				keys(%{ $dzen_options{$screen} }));
                my $dzen_pipe = IO::Pipe->new();
                $dzen_pipe->writer($dzen_cmd);

                process_line($_, $dzen_pipe);
        }
}

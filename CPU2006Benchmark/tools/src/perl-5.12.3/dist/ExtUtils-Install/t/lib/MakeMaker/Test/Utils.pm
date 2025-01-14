package MakeMaker::Test::Utils;

use File::Spec;
use strict;
use Config;

require Exporter;
our @ISA = qw(Exporter);

our $Is_VMS   = $^O eq 'VMS';
our $Is_MacOS = $^O eq 'MacOS';

our @EXPORT = qw(which_perl perl_lib makefile_name makefile_backup
                 make make_run run make_macro calibrate_mtime
                 setup_mm_test_root
                 have_compiler slurp
                 $Is_VMS $Is_MacOS
                 run_ok
                );


# Setup the code to clean out %ENV
{
    # Environment variables which might effect our testing
    my @delete_env_keys = qw(
        PERL_MM_OPT
        PERL_MM_USE_DEFAULT
        HARNESS_TIMER
        HARNESS_OPTIONS
        HARNESS_VERBOSE
        PREFIX
        MAKEFLAGS
    );

    # Remember the ENV values because on VMS %ENV is global
    # to the user, not the process.
    my %restore_env_keys;

    sub clean_env {
        for my $key (@delete_env_keys) {
            if( exists $ENV{$key} ) {
                $restore_env_keys{$key} = delete $ENV{$key};
            }
            else {
                delete $ENV{$key};
            }
        }
    }

    END {
        while( my($key, $val) = each %restore_env_keys ) {
            $ENV{$key} = $val;
        }
    }
}
clean_env();


=head1 NAME

MakeMaker::Test::Utils - Utility routines for testing MakeMaker

=head1 SYNOPSIS

  use MakeMaker::Test::Utils;

  my $perl     = which_perl;
  perl_lib;

  my $makefile      = makefile_name;
  my $makefile_back = makefile_backup;

  my $make          = make;
  my $make_run      = make_run;
  make_macro($make, $targ, %macros);

  my $mtime         = calibrate_mtime;

  my $out           = run($cmd);

  my $have_compiler = have_compiler();

  my $text          = slurp($filename);


=head1 DESCRIPTION

A consolidation of little utility functions used through out the
MakeMaker test suite.

=head2 Functions

The following are exported by default.

=over 4

=item B<which_perl>

  my $perl = which_perl;

Returns a path to perl which is safe to use in a command line, no
matter where you chdir to.

=cut

sub which_perl {
    my $perl = $^X;
    $perl ||= 'perl';

    # VMS should have 'perl' aliased properly
    return $perl if $Is_VMS;

    $perl .= $Config{exe_ext} unless $perl =~ m/$Config{exe_ext}$/i;

    my $perlpath = File::Spec->rel2abs( $perl );
    unless( $Is_MacOS || -x $perlpath ) {
        # $^X was probably 'perl'

        # When building in the core, *don't* go off and find
        # another perl
        die "Can't find a perl to use (\$^X=$^X), (\$perlpath=$perlpath)" 
          if $ENV{PERL_CORE};

        foreach my $path (File::Spec->path) {
            $perlpath = File::Spec->catfile($path, $perl);
            last if -x $perlpath;
        }
    }

    return $perlpath;
}

=item B<perl_lib>

  perl_lib;

Sets up environment variables so perl can find its libraries.
Run this before changing directories.

=cut

my $old5lib = $ENV{SPECPERLLIB};
my $had5lib = exists $ENV{SPECPERLLIB};
sub perl_lib {
    if ($ENV{PERL_CORE}) {
	# Whilst we'll be running in perl-src/cpan/$distname/t/
	# instead of blib, our code will be copied with all the other code to
	# the top-level library.
	# $ENV{SPECPERLLIB} will be set with this, but (by default) it's a relative
	# path.
	$ENV{SPECPERLLIB} = join $Config{path_sep}, map {
	    File::Spec->rel2abs($_) } split quotemeta($Config{path_sep}), $ENV{SPECPERLLIB};
	@INC = map { File::Spec->rel2abs($_) } @INC;
    } else {
	my $lib = 'blib/lib';
	$lib = File::Spec->rel2abs($lib);
	my @libs = ($lib);
	push @libs, $ENV{SPECPERLLIB} if exists $ENV{SPECPERLLIB};
	$ENV{SPECPERLLIB} = join($Config{path_sep}, @libs);
	unshift @INC, $lib;
    }
}

END { 
    if( $had5lib ) {
        $ENV{SPECPERLLIB} = $old5lib;
    }
    else {
        delete $ENV{SPECPERLLIB};
    }
}


=item B<makefile_name>

  my $makefile = makefile_name;

MakeMaker doesn't always generate 'Makefile'.  It returns what it
should generate.

=cut

sub makefile_name {
    return $Is_VMS ? 'Descrip.MMS' : 'Makefile';
}   

=item B<makefile_backup>

  my $makefile_old = makefile_backup;

Returns the name MakeMaker will use for a backup of the current
Makefile.

=cut

sub makefile_backup {
    my $makefile = makefile_name;
    return $Is_VMS ? "$makefile".'_old' : "$makefile.old";
}

=item B<make>

  my $make = make;

Returns a good guess at the make to run.

=cut

sub make {
    my $make = $Config{make};
    $make = $ENV{MAKE} if exists $ENV{MAKE};

    return $make;
}

=item B<make_run>

  my $make_run = make_run;

Returns the make to run as with make() plus any necessary switches.

=cut

sub make_run {
    my $make = make;
    $make .= ' -nologo' if $make eq 'nmake';

    return $make;
}

=item B<make_macro>

    my $make_cmd = make_macro($make, $target, %macros);

Returns the command necessary to run $make on the given $target using
the given %macros.

  my $make_test_verbose = make_macro(make_run(), 'test', 
                                     TEST_VERBOSE => 1);

This is important because VMS's make utilities have a completely
different calling convention than Unix or Windows.

%macros is actually a list of tuples, so the order will be preserved.

=cut

sub make_macro {
    my($make, $target) = (shift, shift);

    my $is_mms = $make =~ /^MM(K|S)/i;

    my $cmd = $make;
    my $macros = '';
    while( my($key,$val) = splice(@_, 0, 2) ) {
        if( $is_mms ) {
            $macros .= qq{/macro="$key=$val"};
        }
        else {
            $macros .= qq{ $key=$val};
        }
    }

    return $is_mms ? "$make$macros $target" : "$make $target $macros";
}

=item B<calibrate_mtime>

  my $mtime = calibrate_mtime;

When building on NFS, file modification times can often lose touch
with reality.  This returns the mtime of a file which has just been
touched.

=cut

sub calibrate_mtime {
    open(FILE, ">calibrate_mtime.tmp") || die $!;
    print FILE "foo";
    close FILE;
    my($mtime) = (stat('calibrate_mtime.tmp'))[9];
    unlink 'calibrate_mtime.tmp';
    return $mtime;
}

=item B<run>

  my $out = run($command);
  my @out = run($command);

Runs the given $command as an external program returning at least STDOUT
as $out.  If possible it will return STDOUT and STDERR combined as you
would expect to see on a screen.

=cut

sub run {
    my $cmd = shift;

    use ExtUtils::MM;

    # Unix, modern Windows and OS/2 from 5.005_54 up can handle can handle 2>&1 
    # This makes our failure diagnostics nicer to read.
    if( MM->os_flavor_is('Unix')                                   or
        (MM->os_flavor_is('Win32') and !MM->os_flavor_is('Win9x')) or
        ($] > 5.00554 and MM->os_flavor_is('OS/2'))
      ) {
        return `$cmd 2>&1`;
    }
    else {
        return `$cmd`;
    }
}


=item B<run_ok>

  my @out = run_ok($cmd);

Like run() but it tests that the result exited normally.

The output from run() will be used as a diagnostic if it fails.

=cut

sub run_ok {
    my $tb = Test::Builder->new;

    my @out = run(@_);

    $tb->cmp_ok( $?, '==', 0, "run(@_)" ) || $tb->diag(@out);

    return wantarray ? @out : join "", @out;
}

=item B<setup_mm_test_root>

Creates a rooted logical to avoid the 8-level limit on older VMS systems.  
No action taken on non-VMS systems.

=cut

sub setup_mm_test_root {
    if( $Is_VMS ) {
        # On older systems we might exceed the 8-level directory depth limit
        # imposed by RMS.  We get around this with a rooted logical, but we
        # can't create logical names with attributes in Perl, so we do it
        # in a DCL subprocess and put it in the job table so the parent sees it.
        open( MMTMP, '>mmtesttmp.com' ) || 
          die "Error creating command file; $!";
        print MMTMP <<'COMMAND';
$ MM_TEST_ROOT = F$PARSE("SYS$DISK:[--]",,,,"NO_CONCEAL")-".][000000"-"]["-"].;"+".]"
$ DEFINE/JOB/NOLOG/TRANSLATION=CONCEALED MM_TEST_ROOT 'MM_TEST_ROOT'
COMMAND
        close MMTMP;

        system '@mmtesttmp.com';
        1 while unlink 'mmtesttmp.com';
    }
}

=item have_compiler

  $have_compiler = have_compiler;

Returns true if there is a compiler available for XS builds.

=cut

sub have_compiler {
    my $have_compiler = 0;

    # ExtUtils::CBuilder prints its compilation lines to the screen.
    # Shut it up.
    use TieOut;
    local *STDOUT = *STDOUT;
    local *STDERR = *STDERR;

    tie *STDOUT, 'TieOut';
    tie *STDERR, 'TieOut';

    eval {
	require ExtUtils::CBuilder;
	my $cb = ExtUtils::CBuilder->new;

	$have_compiler = $cb->have_compiler;
    };

    return $have_compiler;
}

=item slurp

  $contents = slurp($filename);

Returns the $contents of $filename.

Will die if $filename cannot be opened.

=cut

sub slurp {
    my $filename = shift;

    local $/ = undef;
    open my $fh, $filename or die "Can't open $filename for reading: $!";
    my $text = <$fh>;
    close $fh;

    return $text;
}

=back

=head1 AUTHOR

Michael G Schwern <schwern@pobox.com>

=cut

1;

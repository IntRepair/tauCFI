#!/usr/bin/perl -w
use strict;
BEGIN { require PerlBenchmark; import Benchmark; }
require './test.pl';
plan(tests => 6);

=head1 NAME

rt26188 - benchmark speed for keys() on empty hashes

=head1 DESCRIPTION

If you have an empty hash, the speed of keys() depends
on how many keys the hash previously held.

For global hashes, getting the count for previously
big hashes was substantially slower than for lexical hashes.

This test checks that the speed difference for getting
the number or list of keys from an empty hash is about the same
(< 25%) for lexical and global hashes, both previously big and small.

=head1 REFERENCE

This test tests against RT ticket #26188

L<http://rt.perl.org/rt3/Public/Bug/Display.html?id=26188>

=cut

use vars qw(%h_big %h_small);
my %l_big = (1..50000);
my %l_small = (1..10);

%h_big = (1..50000);
%h_small = (1..10);

delete @h_big{keys %h_big};
delete @h_small{keys %h_small};
delete @l_big{keys %l_big};
delete @l_small{keys %l_small};

my $res = timethese shift || -3, {
    big => '1 for keys %h_big',
    small => '1 for keys %h_small',
    scalar_big => '$a = keys %h_big',
    scalar_small => '$a = keys %h_small',

    lex_big => '1 for keys %l_big',
    lex_small => '1 for keys %l_small',
    lex_scalar_big => '$a = keys %l_big',
    lex_scalar_small => '$a = keys %l_small',
}, 'none';

sub iters_per_second {
    $_[0]->iters / $_[0]->cpu_p
}

sub about_as_fast_ok {
    my ($res, $key1, $key2, $name) = @_;
    $name ||= "Speed difference between $key1 and $key2 is less than 25%";
    my %iters_per_second = map { $_ => iters_per_second( $res->{ $_ }) } ($key1, $key2);

    my $ratio = abs(1 - $iters_per_second{ $key1 } / ($iters_per_second{ $key2 } || 1 ));
    if (! cmp_ok( $ratio, '<', 0.25, $name )) {
        diag( sprintf "%20s: %12.2f/s\n", $key1, $iters_per_second{ $key1 } );
        diag( sprintf "%20s: %12.2f/s\n", $key2, $iters_per_second{ $key2 } );
    };
};

about_as_fast_ok( $res, 'scalar_big', 'scalar_small',"Checking the count of hash keys in an empty hash (global)");

about_as_fast_ok( $res, 'big', 'small', "Checking the list of hash keys in an empty hash (global)");

about_as_fast_ok( $res, 'lex_scalar_big', 'lex_scalar_small',"Checking the count of hash keys in an empty hash (lexical)");

about_as_fast_ok( $res, 'lex_big', 'lex_small', "Checking the list of hash keys in an empty hash (lexical)");

about_as_fast_ok( $res, 'lex_scalar_big', 'scalar_big',"Checking the count of hash keys in an empty hash, global vs. lexical");

about_as_fast_ok( $res, 'lex_big', 'big', "Checking the list of hash keys in an empty hash, global vs. lexical");

__END__

# code written
    /* quick bailout if the hash is empty anyway.
       I don't know if placeholders are included in the KEYS count, so a defensive check
    */
    if (! HvKEYS(hv) && !(flags & HV_ITERNEXT_WANTPLACEHOLDERS) )
        return NULL;

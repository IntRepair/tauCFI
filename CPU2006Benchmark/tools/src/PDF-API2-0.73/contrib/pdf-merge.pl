#!/usr/bin/perl
#=======================================================================
#    ____  ____  _____              _    ____ ___   ____
#   |  _ \|  _ \|  ___|  _   _     / \  |  _ \_ _| |___ \
#   | |_) | | | | |_    (_) (_)   / _ \ | |_) | |    __) |
#   |  __/| |_| |  _|    _   _   / ___ \|  __/| |   / __/
#   |_|   |____/|_|     (_) (_) /_/   \_\_|  |___| |_____|
#
#   A Perl Module Chain to faciliate the Creation and Modification
#   of High-Quality "Portable Document Format (PDF)" Files.
#
#   Copyright 1999-2004 Alfred Reibenschuh <areibens@cpan.org>.
#
#=======================================================================
#
#   PERMISSION TO USE, COPY, MODIFY, AND DISTRIBUTE THIS FILE FOR
#   ANY PURPOSE WITH OR WITHOUT FEE IS HEREBY GRANTED, PROVIDED THAT
#   THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE APPEAR IN ALL
#   COPIES.
#
#   THIS FILE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
#   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
#   IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
#   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
#   OF THE USE OF THIS FILE, EVEN IF ADVISED OF THE POSSIBILITY OF
#   SUCH DAMAGE.
#
#   $Id: pdf-merge.pl,v 2.0 2005/11/16 02:16:00 areibens Exp $
#
#=======================================================================
use PDF::API2;

if(2 > scalar @ARGV) {
    print <<"EOT";
Usage: $0 <outfile> <infile1> ... <infileN>

merges serveral pdf files into on ;-)

cheers, 
fredo
EOT
}

my $outfile=shift @ARGV;

my $pdf=PDF::API2->new;

foreach my $in (@ARGV) {
    print STDERR 'loading file $in .';
    my $inpdf=PDF::API2->open($in);
    my $pages=scalar @{$inpdf->{pagestack}};
    foreach my $page (1..$pages) {
        print STDERR "$page.";
        $pdf->importpage($inpdf,$page);
    }
    $inpdf->end();
    print STDERR " done.\n";
}

$pdf->saveas($outfile);

__END__
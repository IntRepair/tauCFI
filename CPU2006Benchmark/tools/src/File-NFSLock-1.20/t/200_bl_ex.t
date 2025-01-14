# Blocking Exclusive Lock Test

use Test;
use File::NFSLock;
use Fcntl qw(O_CREAT O_RDWR O_RDONLY O_TRUNC LOCK_EX);

if ($^O =~ /MSWin/) {
  # Deadlock city (fork/wait)
  print "1..0\n";
  exit;
}

# $m simultaneous processes each trying to count to $n
my $m = 20;
my $n = 50;

$| = 1; # Buffer must be autoflushed because of fork() below.
plan tests => ($m+2);

my $datafile = "testfile.dat";

# Create a blank file
sysopen ( FH, $datafile, O_CREAT | O_RDWR | O_TRUNC );
close (FH);
ok (-e $datafile && !-s _);

for (my $i = 0; $i < $m ; $i++) {
  # For each process
  if (!fork) {
    # Child process need to count to $n
    for (my $j = 0; $j < $n ; $j++) {
      my $lock = new File::NFSLock {
        file => $datafile,
        lock_type => LOCK_EX,
      };
      sysopen(FH, $datafile, O_RDWR);
      # Read the current value
      my $count = <FH>;
      # Increment it
      $count ++;
      # And put it back
      seek (FH,0,0);
      print FH "$count\n";
      close FH;
    }
    exit;
  }
}

for (my $i = 0; $i < $m ; $i++) {
  # Wait until all the children are finished counting
  wait;
  ok 1;
}

# Load up whatever the file says now
sysopen(FH, $datafile, O_RDONLY);
$_ = <FH>;
close FH;
chomp;
# It should be $m processes time $n each
ok $n*$m, $_;

# Wipe the temporary file
unlink $datafile;

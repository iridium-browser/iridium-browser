#!/usr/bin/perl

$exe = "lou_compare";
$tmp_pass = "tmp_pass.txt";
$tmp_fail = "tmp_fail.txt";
@percents = ();

if($#ARGV < 1){ die; }

foreach $file (@ARGV)
{
	chomp($file);
	
	@output = `./tools/$exe < $file`;
	$last = $output[$#output];
	chomp($last);
	
	$last =~ /^([0-9\.]+)%/;
	print("$file:  $1\n");
	push(@percents, $1);
	
	open(TMP, ">>:encoding(UCS-2le)", "$tmp_fail");
	print(TMP "#######################\n");
	print(TMP "$file\n\n");
	open(FAIL, "<:encoding(UTF-16)", "fail.txt");
	foreach (<FAIL>)
	{
		print(TMP "$_");
	}
	close(FAIL);
	close(TMP);
	
	open(TMP, ">>", "$tmp_pass");
	print(TMP "#######################\n");
	print(TMP "$file\n\n");
	open(PASS, "<", "pass.txt");
	foreach (<PASS>)
	{
		print(TMP "$_");
	}
	print(TMP "\n");
	close(PASS);
	close(TMP);
}

$total = 0;
foreach (@percents)
{
	$total += $_;
}
$avg = $total / ($#percents + 1);
print("$avg\n");

`mv $tmp_pass pass.txt`;
`mv $tmp_fail fail.txt`;

__END__




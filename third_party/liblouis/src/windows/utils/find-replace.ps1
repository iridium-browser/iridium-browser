# Simple sed-like utility script

Param(
  [string]$InputFile,
  [string]$OutputFile,
  [string]$Str1,
  [string]$Str2
)

Get-Content $InputFile |
%{ $_ -Replace $Str1,$Str2 } |
Out-File $OutputFile -Encoding UTF8 -Width 256

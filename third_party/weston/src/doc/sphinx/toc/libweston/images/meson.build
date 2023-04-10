# Sphinx does not know look for these files in the source directory, so
# they must be copied to the build directory.
files = [
	'create_output.png',
	'destroy-output.png',
	'head-destroyed.png',
	'initial-heads.png',
	'react-to-heads-changed.png',
]

foreach file : files
	configure_file(input: file, output: file, copy: true)
endforeach

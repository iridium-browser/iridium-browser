function buildjs {

	if [ $1 != "32" -a $1 != "16" ]; then
		echo "argument 1 must either be 32 for UTF32 builts or 16 for UTF16 builds"
		exit 1
	fi
	
	if [ -z $2 ]; then
		echo "argument 2 must be a valid filename"
		exit 1
	fi
	
	set -x

	emcc ./liblouis/.libs/liblouis.a -s RESERVED_FUNCTION_POINTERS=1 -s MODULARIZE=1\
	 -s EXPORT_NAME="'liblouisBuild'" -s EXTRA_EXPORTED_RUNTIME_METHODS="['FS',\
	'Runtime', 'stringToUTF${1}', 'Pointer_Stringify']" --pre-js ./liblouis-js/inc/pre.js\
	 --post-js ./liblouis-js/inc/post.js ${3} -o ./out/${2} &&
	
	cat ./liblouis-js/inc/append.js >> ./out/${2}

	set +x
}

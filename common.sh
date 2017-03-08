# common.sh - defines common operations so we can quickly check the 
# sanity of the environment our scripts run in.

# Somewhat counterintuitively, these return 0 if true and 1 if false.
# That's so we can set -e and do sanity checking quickly.

#
# global constants, relative to the repository root
#

ASSETS_DIR="assets"

ITG2_UTIL_DIR="$ASSETS_DIR/utilities/itg2-util/src"
ITG2_UTIL_FILE="itg2ac-util"
ITG2_UTIL_PATH="$ITG2_UTIL_DIR/$ITG2_UTIL_FILE"

VERIFY_SIG_DIR="src/verify_signature/java"
VERIFY_SIG_FILE="SignFile.java"
VERIFY_SIG_PATH="$VERIFY_SIG_DIR/$VERIFY_SIG_FILE"

#
# generic sanity checking functions and their generic error messages
# 

COMMAND_ERROR="Command \"%s\" not found! Please fix that so we can continue."
FILE_ERROR="File \"%s\" not found! We need that to continue; please find it."

# has_command [cmd] [error] - if cmd cannot be found, display "error" if
# supplied, (which may use %s to get the cmd name) or use a generic error.
function has_command
{
	if [ -z "$1" ]; then
		echo "$0: no command given to check!"
		return 1
	fi

	set +e
	which $1 &> /dev/null
	RET="$?"
	set -e

	if [ $RET -eq 0 ]; then return 0; fi

	# use the custom error message if we have it
	printf "${2-$COMMAND_ERROR}\n" $1
	return 1
}

# has_file [file] [error] - has_command, but for files. Same rules apply.
function has_file
{
	if [ -z "$1" ]; then
		echo "$0: no file given to check!"
		return 1
	fi

	if [ -f $1 ]; then
		return 0;
	else
		# use the custom error message if we have it
		printf "${2-$FILE_ERROR}\n" $1
		return 1
	fi
}

function abspath
{
	echo "$(cd "$(dirname "$1")"; pwd)/$(basename "$1")"
}
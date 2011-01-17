/* Old util I wrote a while back to test joystick input independently
 * of StepMania's InputHandler systems. You might find it useful. ~ Vyhd */

#include <cstdio>
#include <string>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/joystick.h>

char *joydev = "/dev/input/js0";

int jstest_open( char *path, int mode )
{
	struct stat st;
	if( stat( path, &st ) == -1 )
	{
		printf( "Couldn't stat %s: %s\n", path, strerror(errno) );
		return -1;
	}

	if( !S_ISCHR(st.st_mode) )
	{
		printf( "Ignoring %s: not a character device.\n", path );
		return -1;
	}

	return open( path, mode );
}

void jstest_find( int &fd )
{
	fd = jstest_open( joydev, O_RDONLY );

	if( fd != -1 )
		return;

	printf( "Joystick not found. Polling every 5 seconds...\n" );

	int iTimes = 0;

	while( iTimes < 12 )
	{
		iTimes++;

		fd = jstest_open( joydev, O_RDONLY );

		// not found
		if( fd == -1 )
			sleep( 5 );
		else
			break;
	}

	if( fd == -1 )
	{
		printf( "No joystick found in one minute; ceasing find operation.\n" );
		return;
	}
}

int main()
{
	int fd;

	jstest_find( fd );
	printf( "Value of fd: %i\n", fd );

	char sBuffer[1024];
	char *sName;

	if( ioctl( fd, JSIOCGNAME(sizeof(sBuffer)), sBuffer) < 0 )
		sName = "error obtaining name.";
	else
		sName = sBuffer;

	printf( "Found joystick: %s\n", sName );

	struct js_event event;

	unsigned int iBitField = 0;

	while( 1 )
	{
		int iResult = read(fd, &event, sizeof(js_event));

		if( iResult != sizeof(js_event) )
		{
			printf( "Unexpected read result: got %i.\n", iResult );
			break;
		}

		if( event.type != JS_EVENT_BUTTON )
		{
			if( !(event.type | JS_EVENT_INIT) )
				printf( "Non-button event: ignored.\n" );

			continue;
		}

		printf( "Button number %i %s\n", event.number, event.value == 1 ? "pressed" : "released" );
	}

	close( fd );

	return 0;
}








1.	As mentioned by Fabien in his code review

		"Every single input (keyboard, win32 message, mouse, UDP socket) is convered into an event_t and placed in a centralized event queue (sysEvent_t
		eventQue[256]. This allows among other things to record (journalize) each inputs in order to recreate bugs"

	I have included the struct for sysEvent_t below.


					typedef struct {
						int				evTime;
						sysEventType_t	evType;
						int				evValue, evValue2;
						int				evPtrLength;	// bytes of data pointed to by evPtr, for journaling
						void			*evPtr;			// this must be manually freed if not NULL
					} sysEvent_t;

	as you can see, it has a variable sysEventType_t. All the enums are also included below.

					typedef enum {
					  // bk001129 - make sure SE_NONE is zero
						SE_NONE = 0,	// evTime is still valid
						SE_KEY,		// evValue is a key code, evValue2 is the down flag
						SE_CHAR,	// evValue is an ascii char
						SE_MOUSE,	// evValue and evValue2 are reletive signed x / y moves
						SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
						SE_CONSOLE,	// evPtr is a char*
						SE_PACKET	// evPtr is a netadr_t followed by data bytes to evPtrLength
					} sysEventType_t;

	for the purpose of analyzing netcode, we obviously only care about SE_PACKET


2.

					
					win_main.c


					sysEvent_t Sys_GetEvent(void)
					{


					}






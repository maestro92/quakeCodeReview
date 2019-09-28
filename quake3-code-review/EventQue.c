so there are two event ques kept in the game

one is the com_pushedEvents, defined in common.c


					common.c

					#define	MAX_PUSHED_EVENTS	            1024
					// bk001129 - init, also static
					static int com_pushedEventsHead = 0;
					static int com_pushedEventsTail = 0;
					// bk001129 - static
					static sysEvent_t	com_pushedEvents[MAX_PUSHED_EVENTS];


the second is eventQue defined in win_main.c

					win_main.c

					#define	MAX_QUED_EVENTS		256
					#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

					sysEvent_t	eventQue[MAX_QUED_EVENTS];
					int			eventHead, eventTail;
					byte		sys_packetReceived[MAX_MSGLEN];


notice that the max length of a message is 
					
					qcommon.h

					#define	MAX_MSGLEN				16384
















1.					

	
					win_main.c


					WinMain()
					{

						Com_Init( sys_cmdline );

						while(1)
						{

							...
							...

						}

					}


2.	it is then initialized Com_Init( sys_cmdline ){}



					common.c

					void Com_Init( char *commandLine ) 
					{


					  // bk001129 - do this before anything else decides to push events
					  Com_InitPushEvent();

					  ...
					  ...
					}





3.					

					void Com_InitPushEvent(void)
					{

					  // clear the static buffer array
					  // this requires SE_NONE to be accepted as a valid but NOP event
					  memset( com_pushedEvents, 0, sizeof(com_pushedEvents) );
					  // reset counters while we are at it
					  // beware: GetEvent might still return an SE_NONE from the buffer
					  com_pushedEventsHead = 0;
					  com_pushedEventsTail = 0;
					}



#########################################################################################
################################ Pushing Events To Queue ################################
#########################################################################################

1.	so we will start pusshing events when we run the game, let us look at IN_Frame(){};

					win_main.c


					WinMain()
					{

						Com_Init( sys_cmdline );

						while(1)
						{

							...
							...

							// make sure mouse and joystick are only called once a frame
							IN_Frame();

							// run the game
							Com_Frame();

							...
							...

						}

					}



2.	so In IN_Frame(){}, both IN_JoyMove(){} and IN_MouseMove(){} essentially writes to sysEvent_t eventQue[MAX_QUED_EVENTS];
				
					win_input.c

					void IN_Frame()
					{

						// post joystick events
						IN_JoyMove();

						...
						...

						// post events to the system que
						IN_MouseMove();						
					}




3.	











					common.c

					com_pushedEvents()
					{

						
					}

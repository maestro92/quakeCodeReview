
first as usual, I am putting up links that can help me study quake3 source code
	http://gamedev.stackexchange.com/questions/10682/resources-to-help-study-of-quake-3-engine

	https://en.wikipedia.org/wiki/Id_Tech_3

Brian Hook GDC talk 
	https://www.youtube.com/watch?v=TFfkX_ahl94
	http://www.gamers.org/dEngine/quake3/bwh_gdc99.txt


	http://element61.blogspot.com/2005/08/looking-at-quake-3-source-part-1.html


Networking in idTech3
	http://www.dlederle.com/blog/2013/11/07/networking-in-idtech3/


Quake3 Networking Primer
	http://www.ra.is/unlagged/network.html#Q3NP


The Quake3 Networking Model
	http://trac.bookofhook.com/bookofhook/trac.cgi/wiki/Quake3Networking


John Carmack dev log
	http://fd.fabiensanglard.net/doom3/pdfs/johnc-plan_1998.pdf


Quake3 Arena Game Structures
	http://caia.swin.edu.au/reports/110209A/CAIA-TR-110209A.pdf


and of course fabiensanglard
	http://fabiensanglard.net/quake3/







-	besure that sometimes you need to rebuild the solution when you added printf statements

-	I was having so much trouble unlocking the mouse while debugging since the mouse was getting locked in the center of the window.
	this problem was so prominent especially when debugging FiringWeapon. So what i ended up doing is disable mouse completely.

	what I did is I went to win_input.c, and in the IN_Startup(){} function. I commented out the IN_StartupMouse(){} function, and navigate the game
	using the keyboard. When just using the keyboard, firing is mapped to the "ctrl" key.



now so quake3 had an explicit split of client and server



-	The server side is responsible for maintaining the state of the game, 
	determine what is needed by clients and propagate it over the network. 
	It is statically linked against bot.lib which is a separate project because of 
	its chaotic development history mentioned in page 275 of "Masters of Doom":

-	The client side is responsible for predicting where entities are (latency compensation) and render the view. 
	It is statically linked against renderer project: A separate project that would have allowed a Direct3D 
	or even software renderer to be plugged in very easily.

{}







also it seems like
on single player, your machine runs both a server and a client

your server sends a snapshot every frame 
and the client gets a CL_Parsesnapshot every frame

if it is a dedicated server, the client_frame is not run.

even on Lan, when you r setting up a server on your local machinea and playing at the same time,
you yourself gets a snapshot every frame, just like playing a single player game.

the other clients will not get a snapshot everyframe. 


In local mode
when you client does a mouse movement, it sends it over to server, then predicts locally
then one frame afterwards, Server gets the loopback message and runs SV_ClientThink(){}; 

about 

	clSnapshot_t	snapshots[PACKET_BACKUP];

	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds

in local mode it is one to one, meaning 
when i send a userCmd   No.190

then immediately in the client ParseServerMessage side, i will getting the result of my No.190 cmd, (meaning I will get snapshot 190 back). No Latency at all



then if you are just a client connecting to a server, the two numbers will not be insync, (not be one to one)





1.	so here is the entry point of the game

					quake3/win_main.c

					int WinMain()
					{


						...
						...

						Com_Init(sys_cmdline);
						NET_Init();


						while(1)
						{


							startTime = Sys_Milliseconds();

							// make sure mouse and joystick are only called once a frame
							IN_Frame();

							// run the game
							Com_Frame();

							endTime = Sys_Milliseconds();
							totalMsec += endTime - startTime;
							countMsec++;
						}

					}




2.	Let us look into IN_Frame(). As said in the comments this function deals with mouse and joystick inputs

			// make sure mouse and joystick are only called once a frame

	-	according to fabiensanglard, the design here is that every single input (keyboard, win32 message, mouse, UDP socket) is converted into an event_t
	and placed in a centralized event queue (sysEvent_t eventQue[256]). This allows among other things to record each inputs in order to recreate bugs.
					
			
	this design is discussed at length in the John Carmack's dev log (Chapter 10 October, Page 55)

			http://fd.fabiensanglard.net/doom3/pdfs/johnc-plan_1998.pdf




					quake3/win_input.c

					void IN_Frame()
					{

						...
						...

						// post events to the system que
						IN_MouseMove();
					}




3.	So lets look into the IN_MouseMove(){} function.


	-	the IN_Win32Mouse(){} function (if you are on on Windows platform) just calculates the difference in mouse position
		so if there were no changes in mouse position, it just returns, without adding an event into the system Queue



					quake3/win_input.c

					void IN_MouseMove(void)
					{
						if ( g_pMouse ) {
							IN_DIMouse( &mx, &my );
						} else {
							IN_Win32Mouse( &mx, &my );
						}

						if ( !mx && !my ) {
							return;
						}

						Sys_QueEvent( 0, SE_MOUSE, mx, my, 0, NULL );

					}


	-	you can see that we are passing in a SE_MOUSE function type. SE_MOUSE is of type sysEvent_t, and you can find all the types in qcommon.h


					typedef enum 
					{
					  // bk001129 - make sure SE_NONE is zero
						SE_NONE = 0,	// evTime is still valid
						SE_KEY,		// evValue is a key code, evValue2 is the down flag
						SE_CHAR,	// evValue is an ascii char
						SE_MOUSE,	// evValue and evValue2 are reletive signed x / y moves
						SE_JOYSTICK_AXIS,	// evValue is an axis number and evValue2 is the current state (-127 to 127)
						SE_CONSOLE,	// evPtr is a char*
						SE_PACKET	// evPtr is a netadr_t followed by data bytes to evPtrLength
					} sysEventType_t;




4.	If you into the Sys_QueEvent(){} function, it should be pretty straight forward. Nothing surprising.
	you can see that whenever we add an event, we increase eventHead counter.

	whenever we want to remove (or pop) an entry , we increase the eventTail counter. 


					quake3/win_main.c

					void Sys_QueEvent()
					{

						ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];
						if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
							Com_Printf("Sys_QueEvent: overflow\n");
							// we are discarding an event, but don't leak memory
							if ( ev->evPtr ) {
								Z_Free( ev->evPtr );
							}
	--------------->		eventTail++;
						}

	--------------->	eventHead++;

						if ( time == 0 ) {
							time = Sys_Milliseconds();
						}

						ev->evTime = time;
						ev->evType = type;
						ev->evValue = value;
						ev->evValue2 = value2;
						ev->evPtrLength = ptrLength;
						ev->evPtr = ptr;
					}







######################################################################
############################ Com_Frame() #############################
######################################################################

1.	As we go back to step1, we start looking at Com_Frame(){}. Of course like our previous code reviews, i will split this into 
	server code and client code. 

	-	for server code, go into server-frame.c

	-	for client code, go into client-frame.c



					quake3/common.c

					void Com_Frame(void)
					{

						...
						...




						do {
							com_frameTime = Com_EventLoop();
							if ( lastTime > com_frameTime ) {
								lastTime = com_frameTime;		// possible on first frame
							}
							msec = com_frameTime - lastTime;
						} while ( msec < minMsec );
						Cbuf_Execute ();

						...
						...


						//
						// server side
						//
						if ( com_speeds->integer ) {
							timeBeforeServer = Sys_Milliseconds ();
						}

						SV_Frame( msec );
						// if "dedicated" has been modified, start up
						// or shut down the client system.
						// Do this after the server may have started,
						// but before the client tries to auto-connect
						if ( com_dedicated->modified ) {
							// get the latched value
							Cvar_Get( "dedicated", "0", 0 );
							com_dedicated->modified = qfalse;
							if ( !com_dedicated->integer ) {
								CL_Init();
								Sys_ShowConsole( com_viewlog->integer, qfalse );
							} else {
								CL_Shutdown();
								Sys_ShowConsole( 1, qtrue );
							}
						}




						//
						// client system
						//
						if ( !com_dedicated->integer ) {
							//
							// run event loop a second time to get server to client packets
							// without a frame of latency
							//
							if ( com_speeds->integer ) {
								timeBeforeEvents = Sys_Milliseconds ();
							}
							Com_EventLoop();
							Cbuf_Execute ();


							//
							// client side
							//
							if ( com_speeds->integer ) {
								timeBeforeClient = Sys_Milliseconds ();
							}

							CL_Frame( msec );

							if ( com_speeds->integer ) {
								timeAfter = Sys_Milliseconds ();
							}
						}


					}




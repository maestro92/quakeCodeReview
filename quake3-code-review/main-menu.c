

					
1.	all the keyboard inputs when you are in the main window navigating through the menu are processed here




					win_main.c

					void Sys_Init( void )
					{

						...
						...

						Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );

						...
						...
					}




2.					

					Com_Frame()
					{


						do {
							com_frameTime = Com_EventLoopSV();
							if ( lastTime > com_frameTime ) {
								lastTime = com_frameTime;		// possible on first frame
							}
							msec = com_frameTime - lastTime;
						} while ( msec < minMsec );



					}




3.	

					sysEvent_t	Com_GetEventSV(void) {
						if (com_pushedEventsHead > com_pushedEventsTail) {
							com_pushedEventsTail++;

							printf("%s getting events from com_pushedEvents\n", sv_debug_prefix);

							return com_pushedEvents[(com_pushedEventsTail - 1) & (MAX_PUSHED_EVENTS - 1)];
						}

						// printf("%s getting events from Com_GetRealEvent\n", sv_debug_prefix);
						return Com_GetRealEvent();
					}




4.	

					sysEvent_t	Com_GetRealEvent( void ) {


						if()
						{

						}
						else 
						{
							ev = Sys_GetEvent();

							...
							...
						}

					}





5.	remember that when this game runs on windows, it is categorized as a windows application. Meaning if our application wants to accept user input,
	our quake3 process must write the standard "Message loop in Microsoft Windows"

		https://en.wikipedia.org/wiki/Message_loop_in_Microsoft_Windows

	hence when we look at Sys_GetEvent(){}, the function has a small loop there. So if we actually pressed any keyboard key or mouse movement, 
	the Windows OS will give us a message, hence we will going into the while(PeekMessage) path, 






					win_main.c

					sysEvent_t Sys_GetEvent( void ) 
					{

						...
						...


						// pump the message loop
						while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
							if ( !GetMessage (&msg, NULL, 0, 0) ) {
								Com_Quit_f();
							}

							// save the msg time, because wndprocs don't have access to the timestamp
							g_wv.sysMsgTime = msg.time;

							// printf("wndprcs?\n");

							TranslateMessage (&msg);
					      	DispatchMessage (&msg);
						}


						...
						...


					}






	you might wonder, where does the DispatchMessage(){} dispatch the message to? As said in the wikipediate article 

		"
		The message loop does not directly act on the messages that it handles. It dispatches them by calling DispatchMessage, 
		which transfers the message to the 'window procedure' for the window that the message was addressed to. (The 'window procedure' is 
		a callback procedure, which got associated with the window class when it was registered.) (More than one window can use the same window procedure.)

		"

	conveniently, we have intialized the call back in Sys_Init(){}


					win_main.c

					void Sys_Init( void )
					{

						...
						...

						Cvar_Get( "win_wndproc", va("%i", (int)MainWndProc), CVAR_ROM );

						...
						...
					}





6.	so when we look into the MainWndProc(){} callback function, you can see how we process KeyUps and KeyDowns.
	and essentially, we just added these events to our ingame quake3 eventQue.



					win_wndproc.c

					MainWndProc
					{


						switch (uMsg)
						{
							...
							...

							case WM_KEYDOWN:
								printf("WM_KEYDOWN\n");
								Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, MapKey( lParam ), qtrue, 0, NULL );
								break;

							case WM_SYSKEYUP:
							case WM_KEYUP:
								printf("WM_KEYUP\n");
								Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, MapKey( lParam ), qfalse, 0, NULL );
								break;

							...
							...
						}

						...

					}

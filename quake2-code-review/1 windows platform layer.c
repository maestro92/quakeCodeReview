

http://fabiensanglard.net/quake2/

unrolling the code
http://fabiensanglard.net/quake2/notes.html


So I just finished reading quake1 source code (sort of finished reading it.. did not fully understand lots of things);,
now i am going to try to dive into quake2 source code

Quake2 engine is not fundamentally different from quake1

Quake2 is mono-threaded, and the entry point can be found in win32/sys_win.c


Client-Server unification: There is no longer one exe for the client and one for the server, the main executable can act as as server,
a client or both at the same time. Even while playing in singleplayer locally there is still a client and a server running in the same
executable (although data exchange occurs via a local buffer in this case instead of TCP IP/IPX);





Trivia: When playing locally, the communication Client <-> Server is not performed via sockets. 
Instead commands are deposed in a "loopback" buffer via NET_SendLoopPacket on the client portion of the code. 
The server then reconstruct a command from the same buffer using NET_GetLoopPacket.











1.  first thing is that we start from the platform layer. here we look at how it is run on windows

as you can see, this is exactly like what handmade hero does. So Thank you Casey.

we first call initialize our game by calling Qcommon_Init(); and then we go into our game loop,
which we call Qcommon_Frame();

you can also see abunch of windows API calls, using the event message queue. I am not familiar with it,
but I have only heard really bad stuff about that API.


                sys_win.c 


                HINSTANCE   global_hInstance;

                int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
                {

                    ParseCommandLine (lpCmdLine);

                    ...
                    ...

                    Qcommon_Init (argc, argv);
                    oldtime = Sys_Milliseconds ();

                    /* main window message loop */
                    while (1)
                    {
                        // if at a full screen console, don't update unless needed
                        if (Minimized || (dedicated && dedicated->value) )
                        {
                            Sleep (1);
                        }

                        while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
                        {
                            if (!GetMessage (&msg, NULL, 0, 0))
                                Com_Quit ();
                            sys_msg_time = msg.time;
                            TranslateMessage (&msg);
                            DispatchMessage (&msg);
                        }

                        do
                        {
                            newtime = Sys_Milliseconds ();
                            time = newtime - oldtime;
                        } while (time < 1);
                //          Con_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime, oldtime, time);

                        //  _controlfp( ~( _EM_ZERODIVIDE /*| _EM_INVALID*/ ), _MCW_EM );
                        _controlfp( _PC_24, _MCW_PC );
                        Qcommon_Frame (time);

                        oldtime = newtime;
                    }

                    // never gets here
                    return TRUE;
                }



2.  lets look at how Qcommon_Init(); is done

as mentioned Qcommon_Init(); is called by both Client and Server, essentially, "quake common", a shared function
since we are only looking at game logic, i will only highlight game logic related functions();

                common.c

                void Qcommon_Init (int argc, char **argv)
                {
                    ...
                    ...

                    Sys_Init ();

                    NET_Init ();
                    Netchan_Init ();

                    SV_Init ();
                    CL_Init ();

                    ...
                    ...
                }


of course, we are mostly interested in SV_Init(); and CL_Init();








###################################################################################
############################### Server Initalization ##############################
###################################################################################

3. lets check out what is SV_Init(); 







1.  data structures



                g_main.c

                edict_t     *g_edicts;


A global variable, g_edicts, contains an array of all the entities in the world, 
which is used for looping through the entities to animate them, finding them, and disabling them. 
mainly used by the server


                
                g_save.c

                void InitGame(void)
                {

                    ...
                    ...

                    // initialize all entities for this game
                    game.maxentities = maxentities->value;
                    g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
                    globals.edicts = g_edicts;
                    globals.max_edicts = game.maxentities;
                
                    ...
                    ...
                }





2.  
centity_t       cl_entities[MAX_EDICTS];





1.  we start with WinMain in quake2/sys_win.c
    let us first look at the main structure


            quake2/sys_win.c

            WinMain
            {

                ...
                ...


                Qcommon_Init (argc, argv);
                oldtime = Sys_Milliseconds ();


                /* main window message loop */
                while (1)
                {
                    ...
                    ...

                    Qcommon_Frame (time);

                    oldtime = newtime;
                }

                // never gets here
                return TRUE;
            }



2.  Now we go deep into Qcommon_Init()


            common.c

            Qcommon_Init()
            {
                // sets up all the "Function pointers" for converting
                // short, int and float between big endianness and little endianness
                // see q_shared.h for more details  
                Swap_Init();    

                // CBuf is the command buffer. Just a empty buffer through memset
                Cbuf_Init();

                // Check OS version. 
                // needs windows version 4 or greater
                // Reject win32 "Quake2 doesn't run on Win32s"
                Sys_Init()       


            
                NET_Init ();
                Netchan_Init ();    
                
                SV_Init ()
                CL_Init ()
            }


    here we are mostly interested in SV_Init() and CL_Init();



3.  


4.  lets take a look at cl_main.c


            cl_main.c

            CL_Init()
            {

                // grabs videos parameters and set up software or gl dll
                VID_Init ()
                {
                    // get either ref_soft or ref_gl
                    vid_ref = ref_gl

                    // mainly used to update the position of the window
                    VID_CheckChanges();
                }


                S_Init ()
                {


                }


                V_Init ()
                {

                    crosshair = Cvar_Get ("crosshair", "0", CVAR_ARCHIVE);
                
                }



                SRC_Init ()
                {

                }

            }





#############################################################################
############################### Main Game Loop ##############################
#############################################################################


1.  lets go into the main loop

            quake2/sys_win.c
    
            WinMain
            {

                ...
                ...


                /* main window message loop */
                while (1)
                {
                    ...
                    ...

                    Qcommon_Frame (time);

                    oldtime = newtime;
                }

                // never gets here
                return TRUE;
            }


2.  and we go into Qcommon_Frame()

            common.c

            Qcommon_Frame()
            {
                Cbuf_Execute ()

                SV_Frame (dt)

                CL_Frame (dt)
            }


    for more detials in SV_Frame(), refer to server-main.c
    for more detials in CL_Frame(), refer to client-main.c


The interesting thing is that if you start the multiplayer game
if you have the first instance starting the network, it will run both the server and client


if you have the second instance joining the network, it will only run the client





######################################################################
####################### Global Data Structures #######################
######################################################################
        //
        // this structure is left intact through an entire game
        // it should be initialized at dll load time, and read/written to
        // the server.ssv file for savegames
        //
        typedef struct
        {
            char        helpmessage1[512];
            char        helpmessage2[512];
            int         helpchanged;    // flash F1 icon if non 0, play sound
                                        // and increment only if 1, 2, or 3

            gclient_t   *clients;       // [maxclients]

            // can't store spawnpoint in level, because
            // it would get overwritten by the savegame restore
            char        spawnpoint[512];    // needed for coop respawns

            // store latched cvars here that we want to get at often
            int         maxclients;
            int         maxentities;

            // cross level triggers
            int         serverflags;

            // items
            int         num_items;

            qboolean    autosaved;
        } game_locals_t;











4
            cl_main.c

            CL_Frame()
            {

            }



            quake2/sys_win.c

            WinMain
            {
                Qcommon_Init
                {
                    

                    // sets up all the "Function pointers" for converting
                    // short, int and float between big endianness and little endianness
                    // see q_shared.h for more details
                    Swap_Init();    

                    // CBuf is the command buffer. Just a empty buffer through memset
                    Cbuf_Init();

                    // Check OS version. 
                    // needs windows version 4 or greater
                    // Reject win32 "Quake2 doesn't run on Win32s"
                    Sys_Init()                            




                    SV_Init ()
                    {
                        

                    }





                    CL_Init ()
                    {
                        


                        // grabs videos parameters and set up software or gl dll
                        VID_Init ()
                        {
                            // get either ref_soft or ref_gl
                            vid_ref = ref_gl

                            // mainly used to update the position of the window
                            VID_CheckChanges();
                        }


                        S_Init ()
                        {


                        }


                        V_Init ()
                        {

                            crosshair = Cvar_Get ("crosshair", "0", CVAR_ARCHIVE);
                        
                        }



                        SRC_Init ()
                        {

                        }

                    }

                }
            }

2.  continuing the WinMain function, we go into the main while loop 

        while(1)
        {

            dt = newtime - oldtime;

            Qcommon_Frame (dt)
            {

                Cbuf_Execute ()

                SV_Frame (dt)
                {
                    if (!svs.initialized)
                        return;



                    // check timeouts
                    SV_CheckTimeouts ();

                    // get packets from clients
                    SV_ReadPackets ();

                    ...
                    ...

                    // update ping based on the last known frame from all clients
                    SV_CalcPings ();

                    // let everything in the world think and move
                    SV_RunGameFrame ()      
                    {




                    }


                }


                CL_Frame (dt)
                {


                }
            }



            oldtime = newtime;
        }


3.  we go into quake2/sv_main.c to checkout the SV_Frame()
    
    
        quake2/sv_main.c

        SV_Frame(void)
        {

            if (!svs.initialized)
                return;



            // check timeouts
            SV_CheckTimeouts ();

            // get packets from clients
            SV_ReadPackets ();

            ...
            ...

            // update ping based on the last known frame from all clients
            SV_CalcPings ();

            // let everything in the world think and move
            SV_RunGameFrame ()      
        }


4.  we go into quake2/sv_main.c to checkout the SV_RunGameFrame(){}


        quake2/sv_main.c

        SV_RunGameFrame(void)
        {

            // don't run if paused
            if (!sv_paused->value || maxclients->value > 1)
            {
                ge->RunFrame ();

            }


        }

    from the declaration ge->RunFrame() is a just a function pointer

    
        void        (*RunFrame) (void);

    
    running in debug mode and stepping through it, we see that it goes into the 
    G_RunFrame(void) from g_main.c in the game.dll (also in the game visual studio project)


5.  we look into it in game.dll/g_main.c

        game.dll/g_main.c

        G_RunFrame()
        {

            //
            // treat each object in turn
            // even the world gets a chance to think
            //
            ent = &g_edicts[0];
            for (i=0 ; i<globals.num_edicts ; i++, ent++)
            {
                if (!ent->inuse)
                    continue;

                level.current_entity = ent;

                VectorCopy (ent->s.origin, ent->s.old_origin);

                // if the ground entity moved, make sure we are still on it
                if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
                {
                    ent->groundentity = NULL;
                    if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
                    {
                        M_CheckGround (ent);
                    }
                }


                if (i > 0 && i <= maxclients->value)
                {
                    ClientBeginServerFrame (ent);
                    continue;
                }

                G_RunEntity (ent);
            }


            // build the playerstate_t structures for all players
            ClientEndServerFrames ();
        }


    notice in their game, they keep all the objects in an array, and they use the if(!ent->inuse) to 
    skip the ones that are not in use



5.  we see that every object runs the function G_RunEntity in g_phys.c
    and depending on the ent->movetype, we resolve the physics differently
        
        game.dll/g_phys.c

        G_RunEntity (edict_t *ent)
        {
            // don't know what this does
            if (ent->prethink)
                ent->prethink (ent);

            switch ( (int)ent->movetype)
            {
                case MOVETYPE_PUSH:
                case MOVETYPE_STOP:
                    SV_Physics_Pusher (ent);
                    break;
                case MOVETYPE_NONE:
                    SV_Physics_None (ent);
                    break;
                case MOVETYPE_NOCLIP:
                    SV_Physics_Noclip (ent);
                    break;
                case MOVETYPE_STEP:
                    SV_Physics_Step (ent);
                    break;
                case MOVETYPE_TOSS:
                case MOVETYPE_BOUNCE:
                case MOVETYPE_FLY:
                case MOVETYPE_FLYMISSILE:
                    SV_Physics_Toss (ent);
                    break;
                default:
                    gi.error ("SV_Physics: bad movetype %i", (int)ent->movetype);           
            }

        }






7.  going back to the main loop in step 5, we look at ClientEndServerFrames() {}
    
        game.dll/g_main.c


        // build the playerstate_t structures for all players
        ClientEndServerFrames()
        {
            // calc the player views now that all pushing
            // and damage has been added
            for (i=0 ; i<maxclients->value ; i++)
            {
                ent = g_edicts + 1 + i;
                if (!ent->inuse || !ent->client)
                    continue;
                ClientEndServerFrame (ent);
            }
        }


    you can see that we are only calling ClientEndServerFrame on all the players

        ClientEndServerFrame (edict_t *ent)
        {

            ...
            ...

            // detect hitting the floor
            P_FallingDamage (ent);

            // apply all the damage taken this frame
            P_DamageFeedback (ent);

            // determine the view offsets
            SV_CalcViewOffset (ent);

            // determine the gun offsets
            SV_CalcGunOffset (ent);

            ...
        }



8.  we start looking into the client frame

        quake2/cl_main.c


        CL_Frame()
        {

            ...
            ...

            // deals with the mouse
            IN_Frame ();


            // fetch results from server
            CL_ReadPackets ();


            // send a new command message to the server
            CL_SendCommand ();


            // predict all unacknowledged movements
            CL_PredictMovement ();


            // allow rendering DLL change
            VID_CheckChanges ();
            if (!cl.refresh_prepped && cls.state == ca_active)
                CL_PrepRefresh ();


            SCR_UpdateScreen ();

            ...
            ...

        }



9.  we look into the SRC_UpdateScreen() function in quake2/cl_scrn.c


// Structure containing functions exported from refresh DLL
refexport_t re; 

        quake2/cl_scrn.c

        SCR_UpdateScreen()
        {

            ...
            ...


            for ( i = 0; i < numframes; i++ )
            {
                re.BeginFrame( separation[i] );

                if (scr_draw_loading == 2)
                {   //  loading plaque over black screen
                    int     w, h;

                    re.CinematicSetPalette(NULL);
                    scr_draw_loading = false;
                    re.DrawGetPicSize (&w, &h, "loading");
                    re.DrawPic ((viddef.width-w)/2, (viddef.height-h)/2, "loading");
        //          re.EndFrame();
        //          return;
                } 
                // if a cinematic is supposed to be running, handle menus
                // and console specially
                else if (cl.cinematictime > 0)
                {
                    if (cls.key_dest == key_menu)
                    {
                        if (cl.cinematicpalette_active)
                        {
                            re.CinematicSetPalette(NULL);
                            cl.cinematicpalette_active = false;
                        }
                        M_Draw ();
        //              re.EndFrame();
        //              return;
                    }
                    else if (cls.key_dest == key_console)
                    {
                        if (cl.cinematicpalette_active)
                        {
                            re.CinematicSetPalette(NULL);
                            cl.cinematicpalette_active = false;
                        }
                        SCR_DrawConsole ();
        //              re.EndFrame();
        //              return;
                    }
                    else
                    {
                        SCR_DrawCinematic();
        //              re.EndFrame();
        //              return;
                    }
                }
                else 
                {

                    // make sure the game palette is active
                    if (cl.cinematicpalette_active)
                    {
                        re.CinematicSetPalette(NULL);
                        cl.cinematicpalette_active = false;
                    }

                    // do 3D refresh drawing, and then update the screen
                    SCR_CalcVrect ();

                    // clear any dirty part of the background
                    SCR_TileClear ();

                    V_RenderView ( separation[i] );

                    SCR_DrawStats ();
                    if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
                        SCR_DrawLayout ();
                    if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
                        CL_DrawInventory ();

                    SCR_DrawNet ();
                    SCR_CheckDrawCenterString ();

                    if (scr_timegraph->value)
                        SCR_DebugGraph (cls.frametime*300, 0);

                    if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
                        SCR_DrawDebugGraph ();

                    SCR_DrawPause ();

                    SCR_DrawConsole ();

                    M_Draw ();

                    SCR_DrawLoading ();
                }
            }
            re.EndFrame();
        }





10. 
        
        V_RenderView()
        {


        }




11. we look into rendering part,
        
        ref_gl/gl_rmain.c

        R_BeginFrame()
        {

            R_RenderView( fd );
            R_SetLightLevel ();
            R_SetGL2D ();

        }




12. 

        /*
        ================
        R_RenderView

        r_newrefdef must be set before the first call
        ================
        */
        void R_RenderView (refdef_t *fd)
        {

            ...
            ...
            
            R_PushDlights ();

            if (gl_finish->value)
                qglFinish ();

            R_SetupFrame ();

            R_SetFrustum ();

            R_SetupGL ();

            R_MarkLeaves ();    // done here so we know if we're in water

            R_DrawWorld ();

            R_DrawEntitiesOnList ();

            R_RenderDlights ();

            R_DrawParticles ();

            R_DrawAlphaSurfaces ();

            R_Flash();

            if (r_speeds->value)
            {
                ri.Con_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
                    c_brush_polys, 
                    c_alias_polys, 
                    c_visible_textures, 
                    c_visible_lightmaps); 
            }
        }










DWORD:  
double word, a data type definition that is specific to Microsoft Windows.
As defined in the file windows.h, a dword is a unsigned, 32-bit unit of data. It can
contain an integer value in the range 0 through 4,294,967,295.




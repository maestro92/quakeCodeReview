


###############################################################
######################### Server Init #########################
###############################################################

            

1.          

            sv_main.c

            SV_Init()
            {

            }            



2.              

            sv_ccmds.c

            SV_InitOperatorCommands()
            {

                ...
                Cmd_AddCommand ("map", SV_Map_f);
                ...

            }



3.          sv_ccmds.c

            void SV_GameMap_f(void )
            {

                ...
                // start up the next map
                SV_Map (false, Cmd_Argv(1), false );
                
                ...
                ...
            }



4.  
            sv_ccmds.c

            void SV_Map()
            {
                
                ...


                if (sv.state == ss_dead && !sv.loadgame)
                    SV_InitGame (); // the game is just starting
            }


5.  

before I delve further in, I want to point out the server_static_t class

    -   num_client_entities

        from what I have read, this is just the max number of client entities


                typedef struct
                {
                    qboolean    initialized;                // sv_init has completed
                    int         realtime;                   // always increasing, no clamping, etc

                    ...

                    int         spawncount;                 // incremented each server start
                                                            // used to check late spawns

                    client_t    *clients;                   // [maxclients->value];
                    int         num_client_entities;        // maxclients->value*UPDATE_BACKUP*MAX_PACKET_ENTITIES
                    int         next_client_entities;       // next client_entity to use
                    entity_state_t  *client_entities;       // [num_client_entities]

                    ...
                    ...

                } server_static_t;




    -   notice the major for loop happening after the SV_InitGameProgs() function, I advise u to first read
        step 6, which explains what is going on in SV_InitGameProgs(), then come back to read what is going on

    -   so SV_InitGameProgs initalizes the g_edicts list,

        and notice what does the EDICT_NUM does

                #define EDICT_NUM(n) ((edict_t *)((byte *)ge->edicts + ge->edict_size*(n)))

        it actually returns the edict_t by index, (n here is effectively index)

        so the for loop is just initalizing svs.clients with the proper edicts

        and also notice that the it is also setting the client.last cmd to zero. This knowledge will be useful in the networking phase


                sv_init.c

                SV_InitGame()
                {

                    ...
                    ...
                    svs.spawncount = rand();
                    svs.clients = Z_Malloc (sizeof(client_t)*maxclients->value);
                    svs.num_client_entities = maxclients->value*UPDATE_BACKUP*64;
                    svs.client_entities = Z_Malloc (sizeof(entity_state_t)*svs.num_client_entities);

                    ...


                    // init game
                    SV_InitGameProgs ();
                    for (i=0 ; i<maxclients->value ; i++)
                    {
                        ent = EDICT_NUM(i+1);
                        ent->s.number = i+1;
                        svs.clients[i].edict = ent;
                        memset (&svs.clients[i].lastcmd, 0, sizeof(svs.clients[i].lastcmd));
                    }

                }





6.          

                sv_game.c

                void SV_InitGameProgs()
                {
                    ...
                    ...

                    ge->Init();
                }





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

                    // initialize all clients for this game
                    game.maxclients = maxclients->value;
                    game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
                    globals.num_edicts = game.maxclients+1;
                }







################################################################
######################### Server Frame #########################
################################################################


Presumably, you just came from the main_structure.c, below is the main structure of the server frame

    
            quake2/sv_main.c

            SV_Frame (int msec)
            {
                time_before_game = time_after_game = 0;

                // if server is not active, do nothing
                if (!svs.initialized)
                    return;

                svs.realtime += msec;

                // keep the random time dependent
                rand ();

                // check timeouts
                SV_CheckTimeouts ();

                // get packets from clients
                SV_ReadPackets ();

                // move autonomous things around if enough time has passed
                if (!sv_timedemo->value && svs.realtime < sv.time)
                {
                    // never let the time get too far off
                    if (sv.time - svs.realtime > 100)
                    {
                        if (sv_showclamp->value)
                            Com_Printf ("sv lowclamp\n");
                        svs.realtime = sv.time - 100;
                    }
                    NET_Sleep(sv.time - svs.realtime);
                    return;
                }

                // update ping based on the last known frame from all clients
                SV_CalcPings ();

                // give the clients some timeslices
                SV_GiveMsec ();

                // let everything in the world think and move
                SV_RunGameFrame ();

                // send messages back to the clients that had packets read this frame
                SV_SendClientMessages ();

                // save the entire world state if recording a serverdemo
                SV_RecordDemoMessage ();

                // send a heartbeat to the master if needed
                Master_Heartbeat ();

                // clear teleport flags, etc for next frame
                SV_PrepWorldFrame ();

            }



2.  one thing at a time, for details about SV_ReadPackets(), refer to SV_ReadPackets.c. It is really similar to how it is done in quake 1







 
###################################################################################
################################# SV_RunGameFrame #################################
###################################################################################

1.  Next we take a look a SV_RunGameFrame

    we go into quake2/sv_main.c to checkout the SV_RunGameFrame(){}


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
            G_RunFrame(void){} from g_main.c in the game.dll (also in the game visual studio project)










A global variable *g_edicts, contains an array of all the entities in the world, which 
is used for looping through the entities to animate them, finding them

The g_edicts variable is allocated by a TagMalloc function in the import API and 
exported through the export API. This means that the server can also iterate through,
and modify all the entities in the world.


2.  we look into it in game.dll/g_main.c
    
    The engine calls this function every 100ms to advance the world that much. 
    This is where the server frame happens

    -   Notice what we are doing here

        recall that the first few entities are reserved for clients, and there are "max clients" many
        so you can see here that we are calling "ClientBeginServerFrame" for each client.

        and for non client entity, we are calling G_RunEntity

        then afterwards, we are calling "ClientEndServerFrames" for each client

        The order is important because this is the life cycle for each player in the world


            game.dll/g_main.c


            /*
            ================
            G_RunFrame

            Advances the world by 0.1 seconds
            ================
            */
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



2.  we then look into G_RunEntity

    The G_RunEntity calls functions, which handles movement of an entity and 
    calls the entity’s think function (SV_RunThink). 
    For example: a grenade being thrown, an item being dropped, or a door swinging. 
    This is also how monsters get their share of CPU for AI.

                game/g_phys.c

                void G_RunEntity(edict_t *ent)
                {




                }





        most of these will eventually call 

                game/g_phys.c

                qboolean SV_RunThink (edict_t *ent)
                {


                }










###################################################################################
################################# SV_SendClientMessages ###########################
###################################################################################


1.  towards the end of the server frame, the server will start sending messages back to the client


                sv_send.c

                // send messages back to the clients that had packets read this frame
                void SV_SendClientMessages ()
                {


                    ...
                    ...

                    // send a message to each connected client
                    for (i=0, c = svs.clients ; i<maxclients->value; i++, c++)
                    {


                        else if (c->state == cs_spawned)
                        {
                            // don't overrun bandwidth
                            if (SV_RateDrop (c))
                                continue;

                            SV_SendClientDatagram (c);
                        }

                    }
                }


2.  in here we just send the client its snapshot

                sv_send.c

                qboolean SV_SendClientDatagram(client_t* client)
                {

                    SV_BuildClientFrame (client);

                    ...

                    // send over all the relevant entity_state_t
                    // and the player_state_t
                    SV_WriteFrameToClient (client, &msg);

                    ...
                }




3.  it is very important that we look at the SV_BuildClientFrame(); function

    -   notice what we are doing here is that we are iterating through g_edicts array and putting the ones that are visible to the client
        inside the svs.client_entities array. Then the ones in svs.client_entities will be treated further later on.

 
    -   EDICT_NUM(n) is a mcaro defined in server.h 

                #define EDICT_NUM(n) ((edict_t *)((byte *)ge->edicts + ge->edict_size*(n)))
   
        its essentially getting the address of an entry in the ge->edicts array
        and casting it as a (edict_t*).

        its casting (byte *)ge->edicts + ge->edict_size*(n);, to a (edict_t*).


    -   svs.next_client_entities 

        next client_entity to use, it is the next index to fill in in the svs.client_entities array

        and also this variable is ever increasing, which is why when we use the modulus operator on svs.next_client_entities 
        to get the actual index into svs.client_entities

        we do svs.client_entities[svs.next_client_entities % svs.num_client_entities]




                sv_ents.c


                /*
                =============
                SV_BuildClientFrame

                Decides which entities are going to be visible to the client, and
                copies off the playerstat and areabits.
                =============
                */
                SV_BuildClientFrame()
                {


                    ...
                    ...
                    clent = client->edict;
                    if (!clent->client)
                        return;     // not in game yet

                    ...
                    ...

                    // this is the frame we are creating
                    frame = &client->frames[sv.framenum & UPDATE_MASK];

                    frame->senttime = svs.realtime; // save it for ping calc later


                    ...
                    ...


                    for (e=1 ; e<ge->num_edicts ; e++)
                    {
                        ent = EDICT_NUM(e);

                        
                        ////////////////////////////////////////////
                        // CHECKING CONDITIONS IF WE SHOULD IGNORE THIS ENTITY (NOT VISIBLE TO THE CLIENT)
                        ////////////////////////////////////////////
                        ...
                        ...

                        // add it to the circular client_entities array
                        state = &svs.client_entities[svs.next_client_entities%svs.num_client_entities];
                        if (ent->s.number != e)
                        {
                            Com_DPrintf ("FIXING ENT->S.NUMBER!!!\n");
                            ent->s.number = e;
                        }
                        *state = ent->s;

                        // don't mark players missiles as solid
                        if (ent->owner == client->edict)
                            state->solid = 0;

                        svs.next_client_entities++;
                        frame->num_entities++;
                    }

                }






4.  so in the SV_WritePlayerstateToClient and SV_EmitPacketEntities is where 
we construct the snapshot for the Client

                sv_ents.c

                void SV_WriteFrameToClient()
                {

                    // this is the frame we are creating
                    frame = &client->frames[sv.framenum & UPDATE_MASK];
            
                    ...
                    ...



                    {   // we have a valid message to delta from
                        oldframe = &client->frames[client->lastframe & UPDATE_MASK];
                        lastframe = client->lastframe;
                    }


                    // delta encode the playerstate
                    SV_WritePlayerstateToClient (oldframe, frame, msg);

                    // delta encode the entities
                    SV_EmitPacketEntities (oldframe, frame, msg);
                }





        you might wonder where is client->lastframe is updated, you can look into SV_ExecuteClientMessage(){}
        you can see that client->lastframe is updated here whenever we get a userCmd from the user

                SV_ExecuteClientMessage()
                {
                    int     lastframe;


                    case clc_move:

                        ...
                        ...

                        lastframe = MSG_ReadLong (&net_message);
                        if (lastframe != cl->lastframe) 
                        {
                            cl->lastframe = lastframe;
                            if (cl->lastframe > 0) 
                            {
                                cl->frame_latency[cl->lastframe&(LATENCY_COUNTS-1)] = 
                                    svs.realtime - cl->frames[cl->lastframe & UPDATE_MASK].senttime;
                            }
                        }


                    ...
                    ...

                }

        you can see that if the lastFrame we just read in is less different then the one we are currently storing 
        we udpate the lastframe counter and update the latency



5.
                 SV_WritePlayerstateToClient (oldframe, frame, msg);





as to how the "visibility" algorithm works, many games does it differently.




6.  so in this SV_EmitPacketEntities(); function, we are "delta econding the entities", 
    meaning we are only sending the entities that changed.
    
        -   so the function takes in two client_frame_t "client_frame_t* from" and "client_frame_t* to"
            so we taking the "diff" between these two frames and sending the delta of it

        -   when the game runs for its first frame, you will get from_num_entities = 0, becuz *from is null

        -   according to the link below
            http://stackoverflow.com/questions/29310983/what-application-game-state-delta-compression-techniques-algorithm-do-exist

            To keep the objects array in-sync between the client and server, every entity has an entity index from 0 to 1023,
            and it is sent as part of the message from the server to the client


        -   oldindex and newindex are the number of total index, and you can see that the while loop condition is

                    while (newindex < to->num_entities || oldindex < from_num_entities)

            which means we are iterating each object until we reach the end of the from_num_entities, as well as 
            the end of to->num_entities. So we will "examine" each entity in this frame as well as the last frame


        -   in the end, we also wrote a short MSG_WriteShort(){} to mark the end of the packetentities (as written in the comments)
            this will come up when the client reads the packet


        -   newent->number or oldent->number are essentially ids for each entity


                /*
                =============
                SV_EmitPacketEntities

                Writes a delta update of an entity_state_t list to the message.
                =============
                */

                sv_ents.c

                void SV_EmitPacketEntities()
                {


                    newindex = 0;
                    oldindex = 0;
                    while (newindex < to->num_entities || oldindex < from_num_entities)
                    {
                        if (newindex >= to->num_entities)
                            newnum = 9999;
                        else
                        {
                            newent = &svs.client_entities[(to->first_entity+newindex)%svs.num_client_entities];
                            newnum = newent->number;
                        }

                        if (oldindex >= from_num_entities)
                            oldnum = 9999;
                        else
                        {
                            oldent = &svs.client_entities[(from->first_entity+oldindex)%svs.num_client_entities];
                            oldnum = oldent->number;
                        }

                        if (newnum == oldnum)
                        {   // delta update from old position
                            // because the force parm is false, this will not result
                            // in any bytes being emited if the entity has not changed at all
                            // note that players are always 'newentities', this updates their oldorigin always
                            // and prevents warping
                            MSG_WriteDeltaEntity (oldent, newent, msg, false, newent->number <= maxclients->value);
                            oldindex++;
                            newindex++;
                            continue;
                        }


                        if (newnum < oldnum)
                        {   
                            // this is a new entity, send it from the baseline                            
                            printf("    adding new entity\n\n");
                            
                            MSG_WriteDeltaEntity(&sv.baselines[newnum], newent, msg, true, true);
                            newindex++;
                            continue;
                        }

                        if (newnum > oldnum)
                        {   
                            // the old entity isn't present in the new message
                            
                            printf("    removing old entity\n\n");

                            ...
                            ...     // important stuff omitted
                            ...     

                            oldindex++;
                            continue;
                        }
                    }


                    MSG_WriteShort (msg, 0);    // end of packetentities

                }

the graph below explains what is going on

recall that entities from the svs.client_entities were all copied from the g_edicts (this is done in SV_BuildClientFrame);
so when we iterate through client_entities 

(here i have simplied the starting index in client_entities to be at 0, where actually 
    in code, it starts from from->first_entity or to->first_entity);



###########################################
############### ADDING ENTITIES ###########
###########################################

assuming I fired a bullet, which means I have to add an entity in g_edict, and also send it over to the client

so i start with the old frame which does not have the bullet yet

old (FRAME n-1);

    g_edicts index              10      11      12      13      14

    g_edicts             ____________________________________________________
                       |      | ent0  | ent1  | ent2  | NULL  | ent4  | ent5 |
                       |      |       |       |       |       |       |      |
                       |______|_______|_______|_______|_______|_______|______|


                after filtering the ones we actually want to send




    svs.client_entities index   0       1       2       3       4

    svs.client_entities        _______________________________________
                              | ent0  | ent1  | ent2  | ent4  |       |
                              |  10   |  11   |  12   | 14    |       |
                              |_______|_______|_______|_______|_______|



       

Then in the new frame, I add a bullet in the g_edicts at the first available index, which we fill up the NULL


new (FRAME n);
    
    g_edicts index              10      11      12      13      14

    g_edicts            _____________________________________________________
                       |      | ent0  | ent1  | ent2  | ent3  | ent4  | ent5 |
                       |      |       |       |       (bullet);       |      |
                       |______|_______|_______|_______|_______|_______|______|


                after filtering the ones we actually want to send




    svs.client_entities index   0       1       2       3       4

    svs.client_entities        _______________________________________
                              | ent0  | ent1  | ent2  | ent3  | ent4  |
                              |  10   |  11   |  12   |  13   | 14    |
                              |_______|_______|_______|_______|_______|


hence when you get to the third index (which is the bullet);, you have ent4 from old and ent4 from new
so when you compare ent3->number and ent4->number, ent3->number(13) is less then ent4->number(14);,

hence we will go to the if(newnum < oldnum); condition, indicating that we have a entity removed


This also works when we are adding towards the end of the g_edicts list, becuz we set oldnum to 9999, hence this will still work




###########################################
############ REMOVING ENTITIES ############
###########################################

old (FRAME n-1);

    g_edicts index              10      11      12      13      14

    g_edicts             ____________________________________________________
                       |      | ent0  | ent1  | ent2  | ent3  | ent4  | ent5 |
                       |      |       |       |       | (gun);|       |      |
                       |______|_______|_______|_______|_______|_______|______|


                after filtering the ones we actually want to send




    client_entities index       0       1       2       3       4

    client_entities            _______________________________________
                              | ent0  | ent1  | ent2  | ent3  | ent4  |
                              |  10   |  11   |  12   | (gun);| 14    |
                              |_______|_______|_______|_______|_______|



                                                        ^
                                                        |

                                     when player picks up the ent3 (lets say it is a gun); here 



new (FRAME n);
    
    g_edicts index              10      11      12      13      14

    g_edicts             ____________________________________________________
                       |      | ent0  | ent1  | ent2  | NULL  | ent4  | ent5 |
                       |      |       |       |       |       |       |      |
                       |______|_______|_______|_______|_______|_______|______|


                after filtering the ones we actually want to send




    client_entities index       0       1       2       3       4

    client_entities            _______________________________________
                              | ent0  | ent1  | ent2  | ent4  |       |
                              |  10   |  11   |  12   |  14   |       |
                              |_______|_______|_______|_______|_______|


hence when you get to the third index, you have ent3 from old and ent4 from new
so when you compare ent3->number and ent4->number, ent3->number is less then ent4->number,

hence we will go to the if(newnum > oldnum) condition, indicating that we have a entity removed




####################################################
############ MSG_WriteDeltaEntity() ################
####################################################

1.  step4 was pretty heavy weight, which explains how we detect adding entities or removing entities. Now we want to actually go into 
    MSG_WriteDeltaEntity(){} to find out what goes on in there


    -   the so u can see below I only included a few if checks comparing "to" entity_state_s and "from" entity_state_s
        if there are any differences, we then do set some flags in "bits"

    -   also noticed that if is a "newentity", we set some different flags in the "bits" variable

    -   then towards the end, you might noticed the code             

                    if (!bits && !force)
                    {
                        return;     // nothing to send!
                    }

        like what the comments have said, if none of the changes happened on this entity, then we do not send anything. but notice also
        there is a "force" boolean value that we can pass in to force the send action to happen 


                    /*
                    ==================
                    MSG_WriteDeltaEntity

                    Writes part of a packetentities message.
                    Can delta from either a baseline or a previous packet_entity
                    ==================
                    */

                    common.c

                    void MSG_WriteDeltaEntity ( struct entity_state_s *from, 
                                                struct entity_state_s *to, 
                                                sizebuf_t *msg, 
                                                qboolean force, 
                                                qboolean newentity);
                    {

                        ...
                        ...

                        if (to->origin[0] != from->origin[0])
                            bits |= U_ORIGIN1;
                        if (to->origin[1] != from->origin[1])
                            bits |= U_ORIGIN2;
                        if (to->origin[2] != from->origin[2])
                            bits |= U_ORIGIN3;


                        ...
                        ...

                        if (newentity || (to->renderfx & RF_BEAM))
                            bits |= U_OLDORIGIN;



                        //
                        // write the message
                        //
                        if (!bits && !force)
                        {
                            return;     // nothing to send!
                        }

                        ...
                        ...

                    }




2.  lets take a look at step4 again

                void SV_EmitPacketEntities()
                {

                    ...

                    newindex = 0;
                    oldindex = 0;
                    while (newindex < to->num_entities || oldindex < from_num_entities)
                    {
                        ...
                        ...

                        if (newnum == oldnum)
                        {   // delta update from old position
                            // because the force parm is false, this will not result
                            // in any bytes being emited if the entity has not changed at all
                            // note that players are always 'newentities', this updates their oldorigin always
                            // and prevents warping
                            MSG_WriteDeltaEntity (oldent, newent, msg, false, newent->number <= maxclients->value);
                            oldindex++;
                            newindex++;
                            continue;
                        }


                        if (newnum < oldnum)
                        {   
                            // this is a new entity, send it from the baseline                            
                            printf("    adding new entity\n\n");
                            
                            MSG_WriteDeltaEntity(&sv.baselines[newnum], newent, msg, true, true);
                            newindex++;
                            continue;
                        }

                        if (newnum > oldnum)
                        {   
                            // the old entity isn't present in the new message
                            
                            printf("    removing old entity\n\n");

                            ...
                            ...     // important stuff omitted
                            ...     

                            oldindex++;
                            continue;
                        }
                }


    I essentially want to point out the two calls to MSG_WriteDeltaEntity(). the first call in the if (newnum == oldnum), 
    the second in newnum < oldnum. 

    notice in the first call, 
    
                MSG_WriteDeltaEntity(oldent, newent, msg, false, newent->number <= maxclients->value);


    in its last argument, we have "newent->number <= maxclients->value". as pointed out previously, the first few entries in the g_edicts
    are reserved for players. So anything entity index past maxclient->value, we are passing in a "false". 
    Why this is happening is that exactly explained in the comments

                "
                note that players are always 'newentities', this updates their oldorigin always
                and prevents warping
                "















                    typedef struct
                    {
                        entity_state_t  baseline;       // delta from this if not from a previous frame
                        entity_state_t  current;
                        entity_state_t  prev;           // will always be valid, but might just be a copy of current

                        int         serverframe;        // if not current, this ent isn't in the frame

                        int         trailcount;         // for diminishing grenade trails
                        vec3_t      lerp_origin;        // for trails (variable hz)

                        int         fly_stoptime;
                    } centity_t;








(THIS iS FROM QUAKE3, not sure if it is the case for quake 2)
The idea is that 

Game data is compressed using delta compression to reduce network load. That means the server does not 
send a full world snapshot each time, but rather only changes (a delta snapshot) that happened since the last acknowledged update. 
With each packet sent between the client and server, acknowledge numbers are attached to keep track of their data flow. 
Usually full (non-delta) snapshots are only sent when a game starts or a client suffers from heavy packet loss for a couple of seconds. 
Clients can request a full snapshot manually with the cl_fullupdate command.



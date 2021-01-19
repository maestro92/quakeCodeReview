
quake2 bsp tree
https://www.flipcode.com/archives/Quake_2_BSP_File_Format.shtml

For the purpose of rendering, it is useful to have the faces stored in the leaves because of the way the PVS is used. 
If you are not interested in performing collision detection, the faces in the nodes can be completely ignored.

so quake2 bsp stores faces both in nodes and leafs



leaf-storing BSP trees (kind of like kd tree)
-   internal nodes contains diving plane and child sub trees 
-   leaf nodes stores gemoetry 


solid-leaf bsp trees 
-   first get all the supporting plane of every polygon is used as a diving plane


quake2 quake3 brush-storing tree
-   stores the planes the planes that form the halfspace intersection volume that is the solid volume
-   Additionally, the tree contains the beveling planes described in Section 8.4.3 to allow
efficient AABB queries on the tree




##########################################################
###################### Player Physics ####################
##########################################################


1.  So we looked how the entity structure is written, we now look at how Player Physics work.
    again we start from SV_ReadPackets(); and that calls SV_ExecuteClientMessage();


                sv_main.c

                SV_ReadPackets (void)
                {


                    while (NET_GetPacket (NS_SERVER, &net_from, &net_message))
                    {

                        ...
                        ...
                        // check for packets from connected clients
                        for (i=0, cl=svs.clients ; i<maxclients->value ; i++,cl++)
                        {
                            if (Netchan_Process(&cl->netchan, &net_message))
                            {   
                                // this is a valid, sequenced packet, so process it
                                if (cl->state != cs_zombie)
                                {
                                    cl->lastmessage = svs.realtime; // don't timeout
                                    SV_ExecuteClientMessage (cl);
                                }
                            }
                        }
                    }
                }


2.  and again SV_ExecuteClientMessage(); calls SV_ClientThink();

                server\sv_user.c

                void SV_ExecuteClientMessage (client_t *cl)
                {

                    ...
                    SV_ClientThink (cl, &newcmd);

                    ...
                    ...
                }


3.  SV_ClientThink(); calls ge->ClientThink();

                void SV_ClientThink (client_t *cl, usercmd_t *cmd)

                {
                    cl->commandMsec -= cmd->msec;

                    if (cl->commandMsec < 0 && sv_enforcetime->value )
                    {
                        Com_DPrintf ("commandMsec underflow from %s\n", cl->name);
                        return;
                    }

                    ge->ClientThink (cl->edict, cmd);
                }




4.  

so if you want to debug the player, the edict_t that has the following properties
                
                quake2\game\g_local.h

                struct edict_s
                {

                    model = "players/male/tris.md2";
                    ...
                    classname = "player"
                    
                    ...
                }



-   so lets see what the ClientThink(); function do, we basically do two things 


                game\p_client.c

                void ClientThink (edict_t *ent, usercmd_t *ucmd)
                {
                    pmove_t pm;

                    ............................................................
                    ......... Setup and Initialize the pmove_t struct ..........
                    ............................................................


                    ............................................................
                    ..................... Perform the move .....................
                    ............................................................


                    ............................................................
                    ............... Save the results of pmove; ..................
                    ............................................................
                    
                }


-   you can see we create a brand new pmove_t type. which is player move type.
    then we move on to setup and initialize the pmove_t pm

    we first setup the gravity.

                client->ps.pmove.gravity = sv_gravity->value;
                pm.s = client->ps.pmove;


    we first initialize the player origin and velocity on the player_move_t


                for (i=0 ; i<3 ; i++)
                {
                    pm.s.origin[i] = ent->s.origin[i]*8;
                    pm.s.velocity[i] = ent->velocity[i]*8;
                }


    the * 8 is for precision purposes.so dont worry about it. You can just ignore that


-   the pointContents tells you what type of the liquid the player is in. 
http://pages.cs.wisc.edu/~jeremyp/quake/quakec/quakec.pdf

        Returns the contents of the area situated at position pos. Used to know if an area 
        is in water, in slime or in lava. Makes use of the BSP tree, and is supposed to be very fast


-   full code below:



                game\p_client.c

                void ClientThink (edict_t *ent, usercmd_t *ucmd)
                {
                    pmove_t pm;

                    // set up for pmove
                    memset (&pm, 0, sizeof(pm));

                    ...
                    client->ps.pmove.pm_type = PM_NORMAL;

                    client->ps.pmove.gravity = sv_gravity->value;
                    pm.s = client->ps.pmove;

                    for (i=0 ; i<3 ; i++)
                    {
                        pm.s.origin[i] = ent->s.origin[i]*8;
                        pm.s.velocity[i] = ent->velocity[i]*8;
                    }

                    if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
                    {
                        pm.snapinitial = true;
                //      gi.dprintf ("pmove changed!\n");
                    }

                    pm.cmd = *ucmd;

                    pm.trace = PM_trace;    // adds default parms
                    pm.pointcontents = gi.pointcontents;

                    ...
                    ...
                }   




4.  then we perform a player movement

-   we then perform a move by calling pmove()


                game\p_client.c

                void ClientThink (edict_t *ent, usercmd_t *ucmd)
                {
                    ...
                    ...

                    // perform a pmove
                    gi.Pmove (&pm);

                    ...
                    ...
                }




5.  finally we save the result of pmove back to the client.

                game\p_client.c

                void ClientThink (edict_t *ent, usercmd_t *ucmd)
                {
                    ...
                    ...

                    // perform a pmove
                    gi.Pmove (&pm);

                    ...
                    ...
                 
                    // save results of pmove
                    client->ps.pmove = pm.s;
                    client->old_pmove = pm.s;

                }



########################################################
###################### PMove() #########################
########################################################



5.  now lets look inside the Pmove() function. which performs a move

http://p-engine.blogspot.com/2011/01/character-controller-in-id-tech-3-quake.html


you can see that it has the comment "Can be called by either the server or the client"
so when the client does client prediction, this function will also get called.


the main steps are:

-   PM_CatagorizePosition ();
we do collision detection in our BSP tree.
We find out what ground entity we are standing on or if we are in mid air
Essentially, like what it says, we are categorizing the position we are currently standing on.


-   we perform our move in the PM_AirMove(); function

                PM_AirMove ();


-   PM_CatagorizePosition ();
this is our 2nd to categorizing position, after our move. we want to know where we are standing on 
after our move.


-   PM_SnapPosition ();
we snap to a good position that is good for



-   When you run into a wall, pmove attempts to clip your velocity along that plane and scale it back up to your intended speed. 


-   pml is player move local  

                quake2\qcommon\pmove.c

                /*
                ================
                Pmove

                Can be called by either the server or the client
                ================
                */

                void Pmove (pmove_t *pmove)
                {
                    pm = pmove;

                    ...
                    ...

                    // convert origin and velocity to float values
                    pml.origin[0] = pm->s.origin[0]*0.125;
                    pml.origin[1] = pm->s.origin[1]*0.125;
                    pml.origin[2] = pm->s.origin[2]*0.125;

                    pml.velocity[0] = pm->s.velocity[0]*0.125;
                    pml.velocity[1] = pm->s.velocity[1]*0.125;
                    pml.velocity[2] = pm->s.velocity[2]*0.125;




                    // set groundentity, watertype, and waterlevel
    ----------->    PM_CatagorizePosition ();

                    ...
                    ...

                    if (pm->s.pm_flags & PMF_TIME_TELEPORT)
                    {   // teleport pause stays exactly in place
                    }
                    else if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
                    {   // waterjump has no control, but falls
                        ...
                    }
                    else
                    {
                        if (pm->waterlevel >= 2)
                            PM_WaterMove ();
                        else 
                        {
                            vec3_t  angles;

                            VectorCopy(pm->viewangles, angles);
                            if (angles[PITCH] > 180)
                                angles[PITCH] = angles[PITCH] - 360;
                            angles[PITCH] /= 3;

                            AngleVectors (angles, pml.forward, pml.right, pml.up);

    ----------->            PM_AirMove ();
                        }
                    }

                    // set groundentity, watertype, and waterlevel for final spot
    ----------->    PM_CatagorizePosition ();

    ----------->    PM_SnapPosition ();
                }













7.  so we look at how we actually move                 


so there is player movement and player movement local

-   we first get the forwardmove and sidemove 

                fmove = pm->cmd.forwardmove;
                smove = pm->cmd.sidemove;

-   the desired velocity is 

                for (i=0 ; i<2 ; i++)
                    wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;

-   we update velocity in the PM_Accelerate(); function.

    Accelerate the player based on the difference between the current velocity after step 
    1 and the intended velocity of step 
    2, and the current acceleration (air/water/ground);


-   we finally we update the position in the PM_StepSlideMove();

    Apply vertical gravity, compute new position, detect collisions and correct them.

    PM_StepSlideMove and PM_StepSlideMove_ makes sure the player can sidestep, and does it.






Quake 3 does a lot of things, lots of special cases I mean, there is no special case specifically for corners, 
but there is for tight situations when more than one plane is in the way.

They keep a list of plane normals, which they call clip planes, these are used to clip the movement, 
the list is rather short, set to a fixed 5 maximum planes, one spot is always given to the normalized velocity ray, 
to prevent the movement to end up bouncing back too far I guess, and if 
there is a ground normal (the normal of the plane the player is standing on) it gets a spot as well.

The other three or four spots are filled by tracing the start and start+velocity positions against the planes 
(thats where convex brushes come into play), if the number of colliding planes exceeds the fixed maximum, 
the velocity is set to zero (IE: no movement), otherwise... well a lot of stuff happens, 
but in essence, the velocity is clipped and its direction changed so it either parallels or 
doesnt point against any of the clipping planes, if thats not possible, then again, velocity is set to zero.

There is also a limit (4) of how many "bumps" are allowed when clipping the velocity.

Its all in the PM_SlideMove function if you want to look it up, but it gets complex, and then there is a PM_StepSlideMove which is even more computations on top of PM_SlideMove, and so on.





-   full code below:

                pmove_t     *pm;
                pml_t       pml;



                pmove.c

                /*
                ===================
                PM_AirMove

                ===================
                */
                void PM_AirMove (void)
                {
                    int         i;
                    vec3_t      wishvel;
                    float       fmove, smove;
                    vec3_t      wishdir;
                    float       wishspeed;
                    float       maxspeed;

                    fmove = pm->cmd.forwardmove;
                    smove = pm->cmd.sidemove;
                    
                //!!!!! pitch should be 1/3 so this isn't needed??!
                    ...

                    for (i=0 ; i<2 ; i++)
                        wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
                    wishvel[2] = 0;

                    PM_AddCurrents (wishvel);

                    VectorCopy (wishvel, wishdir);
                    wishspeed = VectorNormalize(wishdir);

                //
                // clamp to server defined max speed
                //
                    maxspeed = (pm->s.pm_flags & PMF_DUCKED) ? pm_duckspeed : pm_maxspeed;

                    ...
                    ...
                    
                    if ( pml.ladder )
                    {
                        ...
                        ...
                    }
                    else if ( pm->groundentity )
                    {   // walking on ground
                        pml.velocity[2] = 0; //!!! this is before the accel
                        PM_Accelerate (wishdir, wishspeed, pm_accelerate);

                        ...
                        ...

                        if (!pml.velocity[0] && !pml.velocity[1])
                            return;
                        PM_StepSlideMove ();
                    }
                    else
                    {   // not on ground, so little effect on velocity
                        ...
                        ...
                    }
                }




player acceleration is done in PM_Accelerate();

                /*
                ==============
                PM_Accelerate

                Handles user intended acceleration
                ==============
                */
                void PM_Accelerate (vec3_t wishdir, float wishspeed, float accel)
                {
                    int         i;
                    float       addspeed, accelspeed, currentspeed;

                    currentspeed = DotProduct (pml.velocity, wishdir);
                    addspeed = wishspeed - currentspeed;
                    if (addspeed <= 0)
                        return;
                    accelspeed = accel*pml.frametime*wishspeed;
                    if (accelspeed > addspeed)
                        accelspeed = addspeed;
                    
                    for (i=0 ; i<3 ; i++)
                        pml.velocity[i] += accelspeed*wishdir[i];   
                }




8.  now lets take a look at PM_StepSlideMove();

-   start_o and start_v means starting origin and starting velocity


Check if the calculated velocity vector will cause any collision with the world. 
If so move the player as close to the collision point as possible. 
Repeat a few times to cover the duration of a frame. PM_StepSlideMove_()

Try stepping up - check if we can go 18 units up without hitting a ceiling.

Try to return back down until we touch the ground again. 

                /*
                ==================
                PM_StepSlideMove

                ==================
                */
                void PM_StepSlideMove (void)
                {
                    vec3_t      start_o, start_v;
                    vec3_t      down_o, down_v;
                    trace_t     trace;
                    float       down_dist, up_dist;
                //  vec3_t      delta;
                    vec3_t      up, down;

                    VectorCopy (pml.origin, start_o);
                    VectorCopy (pml.velocity, start_v);

                    PM_StepSlideMove_ ();

                    VectorCopy (pml.origin, down_o);
                    VectorCopy (pml.velocity, down_v);

                    VectorCopy (start_o, up);
                    up[2] += STEPSIZE;

                    trace = pm->trace (up, pm->mins, pm->maxs, up);
                    if (trace.allsolid)
                        return;     // can't step up

                    // try sliding above
                    VectorCopy (up, pml.origin);
                    VectorCopy (start_v, pml.velocity);

                    PM_StepSlideMove_ ();

                    // push down the final amount
                    VectorCopy (pml.origin, down);
                    down[2] -= STEPSIZE;
                    trace = pm->trace (pml.origin, pm->mins, pm->maxs, down);
                    if (!trace.allsolid)
                    {
                        VectorCopy (trace.endpos, pml.origin);
                    }

                #if 0
                    VectorSubtract (pml.origin, up, delta);
                    up_dist = DotProduct (delta, start_v);

                    VectorSubtract (down_o, start_o, delta);
                    down_dist = DotProduct (delta, start_v);
                #else
                    VectorCopy(pml.origin, up);

                    // decide which one went farther
                    down_dist = (down_o[0] - start_o[0])*(down_o[0] - start_o[0])
                        + (down_o[1] - start_o[1])*(down_o[1] - start_o[1]);
                    up_dist = (up[0] - start_o[0])*(up[0] - start_o[0])
                        + (up[1] - start_o[1])*(up[1] - start_o[1]);
                #endif

                    if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
                    {
                        VectorCopy (down_o, pml.origin);
                        VectorCopy (down_v, pml.velocity);
                        return;
                    }
                    //!! Special case
                    // if we were walking along a plane, then we need to copy the Z over
                    pml.velocity[2] = down_v[2];
                }






9.  lets take a look at PM_StepSlideMove_();

    https://www.ryanliptak.com/blog/rampsliding-quake-engine-quirk/


    this is where the cool stuff happens.
    also note that this is reminds me of what Casey did with the collision detection here 

    -   we first keep a copy of the original velocity 

                vec3_t      primal_velocity;

                ...

                VectorCopy (pml.velocity, primal_velocity);
                numplanes = 0;


    -   we start with time_left = pm.frametime. This is essentially dt. 
        we first assume player can move at pml.velocity[i] for the entire duration of dt.

        so we first set the potential end position with end[i]

                for (i=0 ; i<3 ; i++)
                    end[i] = pml.origin[i] + time_left * pml.velocity[i];


    -   we call a trace with a potential end position
                
                trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);


    -   we examine the result of the tracing.
        if the trace.fraction > 0, which means, t > 0, and we moved some distance 

        we want to reset the number of planes. so we want to see how many planes we will 
        bounce off.

        then if the case is that trace.fraction == 1, we moved the full distance.

                if (trace.fraction > 0)
                {   // actually covered some distance
                    VectorCopy (trace.endpos, pml.origin);
                    numplanes = 0;
                }

                if (trace.fraction == 1)
                     break;     // moved the entire distance


    -   in the case where we didnt move the full distance, 

                time_left -= time_left * trace.fraction; 


    -   finally, we add this plane to one of the clipping planes.

                VectorCopy (trace.plane.normal, planes[numplanes]);
                numplanes++;



    -   we then start to go through all the planes, then we call PM_ClipVelocity.
    when a player collides with a surface, the resulting velocity is determined by this PM_ClipVelocity(); function.


                for (i=0 ; i<numplanes ; i++)
                {
                    PM_ClipVelocity (pml.velocity, planes[i], pml.velocity, 1.01);
                    for (j=0 ; j<numplanes ; j++)
                        if (j != i)
                        {
                            if (DotProduct (pml.velocity, planes[j]) < 0)
                                break;  // not ok
                        }
                    if (j == numplanes)
                        break;
                }


    -   there are scenarios where trace.fraction == 0;
    that means the player didnt walk forward at all.





    -   full code below:


                pmove.c


                /*
                ==================
                PM_StepSlideMove

                Each intersection will try to step over the obstruction instead of
                sliding along it.

                Returns a new origin, velocity, and contact entity
                Does not modify any world state?
                ==================
                */

                #define MIN_STEP_NORMAL 0.7     // can't step up onto very steep slopes
                #define MAX_CLIP_PLANES 5
                void PM_StepSlideMove_ (void)
                {
                    int         bumpcount, numbumps;
                    vec3_t      dir;
                    float       d;
                    int         numplanes;
                    vec3_t      planes[MAX_CLIP_PLANES];
                    vec3_t      primal_velocity;
                    int         i, j;
                    trace_t trace;
                    vec3_t      end;
                    float       time_left;
                    
                    numbumps = 4;
                    
                    VectorCopy (pml.velocity, primal_velocity);
                    numplanes = 0;
                    
                    time_left = pml.frametime;

                    for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
                    {
                        for (i=0 ; i<3 ; i++)
                            end[i] = pml.origin[i] + time_left * pml.velocity[i];

                        trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

                        if (trace.allsolid)
                        {   // entity is trapped in another solid
                            pml.velocity[2] = 0;    // don't build up falling damage
                            return;
                        }

                        if (trace.fraction > 0)
                        {   // actually covered some distance
                            VectorCopy (trace.endpos, pml.origin);
                            numplanes = 0;
                        }

                        if (trace.fraction == 1)
                            break;     // moved the entire distance

                        // save entity for contact
                        if (pm->numtouch < MAXTOUCH && trace.ent)
                        {
                            pm->touchents[pm->numtouch] = trace.ent;
                            pm->numtouch++;
                        }
                        
                        time_left -= time_left * trace.fraction;

                        // slide along this plane
                        if (numplanes >= MAX_CLIP_PLANES)
                        {   // this shouldn't really happen
                            VectorCopy (vec3_origin, pml.velocity);
                            break;
                        }

                        VectorCopy (trace.plane.normal, planes[numplanes]);
                        numplanes++;


                //
                // modify original_velocity so it parallels all of the clip planes
                //
                        for (i=0 ; i<numplanes ; i++)
                        {
                            PM_ClipVelocity (pml.velocity, planes[i], pml.velocity, 1.01);
                            for (j=0 ; j<numplanes ; j++)
                                if (j != i)
                                {
                                    if (DotProduct (pml.velocity, planes[j]) < 0)
                                        break;  // not ok
                                }
                            if (j == numplanes)
                                break;
                        }
                        
                        if (i != numplanes)
                        {   // go along this plane
                        }
                        else
                        {   // go along the crease
                            if (numplanes != 2)
                            {
                //              Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
                                VectorCopy (vec3_origin, pml.velocity);
                                break;
                            }
                            CrossProduct (planes[0], planes[1], dir);
                            d = DotProduct (dir, pml.velocity);
                            VectorScale (dir, d, pml.velocity);
                        }

                        //
                        // if velocity is against the original velocity, stop dead
                        // to avoid tiny occilations in sloping corners
                        //
                        if (DotProduct (pml.velocity, primal_velocity) <= 0)
                        {
                            VectorCopy (vec3_origin, pml.velocity);
                            break;
                        }
                    }

                    if (pm->s.pm_time)
                    {
                        VectorCopy (primal_velocity, pml.velocity);
                    }
                }







10. at the last check we check whehter the clipped velocity compares to our primal_velocity


                void PM_StepSlideMove_ (void)
                {

                    vec3_t      primal_velocity;
                    ...
                    ...

                    VectorCopy (pml.velocity, primal_velocity);

                    ...
                    ...

                    for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
                    {

                        ...
                        ...

                        //
                        // if velocity is against the original velocity, stop dead
                        // to avoid tiny occilations in sloping corners
                        //
    --------------->    if (DotProduct (pml.velocity, primal_velocity) <= 0)
                        {
                            VectorCopy (vec3_origin, pml.velocity);
                            break;
                        }
                    }
                }


    lets take an example:

    |
    |
    |<-------- (player)
    |->
    |

when the player walks straight into a wall with the speed (-1, 0, 0), 
the clipped velocity will reflect a slight bit (since quake does a slight overbounce)

if you look at quake2_s PM_ClipVelocity(); code inside PM_StepSlideMove_(); you can see we pass in a 1.01 c
as the overbounce amount. so we get potential jitter.

what quake does is that it does a dot product between the primal speed and the resultant speed,
                
                DotProduct(primal_velocity, clipped_velocity);

and if we get a negative number. hence we break out of the loop, and set the velocity to 0.








11. consider another case, where the player is walking into a wall that he cant climb 
the normal of the wall is not (1, 0, 0)

         ##     
  \   ########
   \ <--####       
    \   ####
     \  #  #
      \_#__#___________________


in this case, the trace.fraction is 0. Setting the fraction to 0 is done in CM_ClipBoxToBrush();

so here we are collding with 2 planes. the one straight one and the non-axial one.
The idea is that numplanes keeps a count on how many planes you are colliding with.
if on the 2nd trace, the trace.fraction is just 0, then you are in contact with 2 planes


                    #
                    #
             ####   #
             ###  p #
             ##     #
                    #
                    #
                    #


it will move out along the crease, which is up in the z axis 


                    #
                    #
             #### X #
             ###  p #
             ##     #
                    #
                    #
                    #


11. quake3 overbounce

https://origamiparade.com/programming-projects/openarena/overbounce/




                #define MIN_STEP_NORMAL 0.7     // can't step up onto very steep slopes
                #define MAX_CLIP_PLANES 5
                void PM_StepSlideMove_ (void)
                {

                    for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
                    {
                        ...
                        ...

                        // save entity for contact
                        if (pm->numtouch < MAXTOUCH && trace.ent)
                        {
                            pm->touchents[pm->numtouch] = trace.ent;
                            pm->numtouch++;
                        }
                    
                    }


                    ...
                    ...
                }









10. this is the same as what Casey taught. you slide off the wall that you are impacting.

                /*
                ==================
                PM_ClipVelocity

                Slide off of the impacting object
                returns the blocked flags (1 = floor, 2 = step / wall)
                ==================
                */
                #define STOP_EPSILON    0.1

                void PM_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
                {
                    float   backoff;
                    float   change;
                    int     i;
                    
                    backoff = DotProduct (in, normal) * overbounce;

                    for (i=0 ; i<3 ; i++)
                    {
                        change = normal[i]*backoff;
                        out[i] = in[i] - change;
                        if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
                            out[i] = 0;
                    }
                }




##########################################################
####################### STEPSIZE #########################
##########################################################

so in the pmove.c file, there is this macro definition "STEPSIZE"

                #include "qcommon.h"



                #define  STEPSIZE    18

                ...
                ...


Regarding this, we found the following explanation 


        https://xoxor4d.github.io/research/mw2-bounce/

        PM_StepSlideMove is needed for stairs and little elevations on the ground so the player 
        doesn’t stop at every brush that is 1 unit heigher than the plane he is walking on. 
        It basically lifts the player up if the brush he walks against is lower or 
        equal the height thats set with the jump_stepSize dvar.


as a matter of fact, when you start the game in the first map. there is a small rock plate 
near where you spawn. with the default 18, you can actually step over it. 
But if you change the STEPSIZE to 1, you cant step over it anymore 

so in the PM_StepSlideMove(); function, you can see that we are testing stepping up, as well as stepping down 

you can see we built a "up" vector, with STEPSIZE going up, and a "down" vector, with STEPSIZE going down.


                void PM_StepSlideMove (void)
                {

                    vec3_t      start_o, start_v;
                    vec3_t      down_o, down_v;
                    trace_t     trace;
                    float       down_dist, up_dist;
                //  vec3_t      delta;
                    vec3_t      up, down;

                    VectorCopy (pml.origin, start_o);
                    VectorCopy (pml.velocity, start_v);

                    PM_StepSlideMove_ ();

                    VectorCopy (pml.origin, down_o);
                    VectorCopy (pml.velocity, down_v);

                    VectorCopy (start_o, up);
    ----------->    up[2] += STEPSIZE;

                    trace = pm->trace (up, pm->mins, pm->maxs, up);
                    if (trace.allsolid)
                        return;     // can't step up

                    // try sliding above
                    VectorCopy (up, pml.origin);
                    VectorCopy (start_v, pml.velocity);

                    PM_StepSlideMove_ ();

                    // push down the final amount
                    VectorCopy (pml.origin, down);
    ----------->    down[2] -= STEPSIZE;
                    trace = pm->trace (pml.origin, pm->mins, pm->maxs, down);
                    if (!trace.allsolid)
                    {
                        VectorCopy (trace.endpos, pml.origin);
                    }


                    VectorCopy(pml.origin, up);

                    // decide which one went farther
                    down_dist = (down_o[0] - start_o[0])*(down_o[0] - start_o[0]) + (down_o[1] - start_o[1])*(down_o[1] - start_o[1]);
                    up_dist = (up[0] - start_o[0])*(up[0] - start_o[0]) + (up[1] - start_o[1])*(up[1] - start_o[1]);

                    if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
                    {
                        VectorCopy (down_o, pml.origin);
                        VectorCopy (down_v, pml.velocity);
                        return;
                    }
                    //!! Special case
                    // if we were walking along a plane, then we need to copy the Z over
                    pml.velocity[2] = down_v[2];
                }











6.  so when you start of the game, these are the numbers

                origin: 128.000000, -320.000000, 24.125000
                pm->mins: -16.000000, -16.000000, -24.000000
                pm->maxs: 16.000000, 16.000000, 32.000000



notice to see what position the player is standing on, 
we actually do a simple line trace into our BSP tree

so at first we are currently standing at pml.origin.

the end point we are going is actually at 

                point[0] = pml.origin[0];
                point[1] = pml.origin[1];
                point[2] = pml.origin[2] - 0.25;




-   full code below:

                quake2\qcommon\pmove.c
                /*
                =============
                PM_CatagorizePosition
                =============
                */
                void PM_CatagorizePosition (void)
                {
                    vec3_t      point;
                    int         cont;
                    trace_t     trace;
                    int         sample1;
                    int         sample2;

                // if the player hull point one unit down is solid, the player
                // is on ground

                // see if standing on something solid   
                    point[0] = pml.origin[0];
                    point[1] = pml.origin[1];
                    point[2] = pml.origin[2] - 0.25;
                    if (pml.velocity[2] > 180) //!!ZOID changed from 100 to 180 (ramp accel)
                    {
                        pm->s.pm_flags &= ~PMF_ON_GROUND;
                        pm->groundentity = NULL;
                    }
                    else
                    {
   --------------->     trace = pm->trace (pml.origin, pm->mins, pm->maxs, point);
                        pml.groundplane = trace.plane;
                        pml.groundsurface = trace.surface;
                        pml.groundcontents = trace.contents;

                        if (!trace.ent || (trace.plane.normal[2] < 0.7 && !trace.startsolid) )
                        {
                            pm->groundentity = NULL;
                            pm->s.pm_flags &= ~PMF_ON_GROUND;
                        }
                        else
                        {
                            pm->groundentity = trace.ent;

                            // hitting solid ground will end a waterjump
                            if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
                            {
                                pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
                                pm->s.pm_time = 0;
                            }

                            if (! (pm->s.pm_flags & PMF_ON_GROUND) )
                            {   // just hit the ground
                                pm->s.pm_flags |= PMF_ON_GROUND;
                                // don't do landing time if we were just going down a slope
                                if (pml.velocity[2] < -200)
                                {
                                    pm->s.pm_flags |= PMF_TIME_LAND;
                                    // don't allow another jump for a little while
                                    if (pml.velocity[2] < -400)
                                        pm->s.pm_time = 25; 
                                    else
                                        pm->s.pm_time = 18;
                                }
                            }
                        }

                #if 0
                        if (trace.fraction < 1.0 && trace.ent && pml.velocity[2] < 0)
                            pml.velocity[2] = 0;
                #endif

                        if (pm->numtouch < MAXTOUCH && trace.ent)
                        {
                            pm->touchents[pm->numtouch] = trace.ent;
                            pm->numtouch++;
                        }
                    }

                //
                // get waterlevel, accounting for ducking
                //
                    pm->waterlevel = 0;
                    pm->watertype = 0;

                    sample2 = pm->viewheight - pm->mins[2];
                    sample1 = sample2 / 2;

                    point[2] = pml.origin[2] + pm->mins[2] + 1; 
                    cont = pm->pointcontents (point);

                    if (cont & MASK_WATER)
                    {
                        pm->watertype = cont;
                        pm->waterlevel = 1;
                        point[2] = pml.origin[2] + pm->mins[2] + sample1;
                        cont = pm->pointcontents (point);
                        if (cont & MASK_WATER)
                        {
                            pm->waterlevel = 2;
                            point[2] = pml.origin[2] + pm->mins[2] + sample2;
                            cont = pm->pointcontents (point);
                            if (cont & MASK_WATER)
                                pm->waterlevel = 3;
                        }
                    }

                }










##########################################################
####################### Gravity ##########################
##########################################################

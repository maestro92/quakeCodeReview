##########################################################
###################### Player Physics ####################
##########################################################

1.  so lets see how player physics work on the server. 
    so when the server reads in client message packet, it will execute the function SV_ExecuteClientMessage();

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


2.  The SV_ExecuteClientMessage(); calls the SV_ClientThink(); function 

                server\sv_user.c

                void SV_ExecuteClientMessage (client_t *cl)
                {

                    ...
                    SV_ClientThink (cl, &newcmd);

                    ...
                    ...
                }


3.  in the game_export, we call ClientThink, in which we pass in the client entity dictionary

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
                                        ^
                                        |
                                        |




4.  here we introduce the entity structure used on the server

                game\g_local.h

                struct edict_s
                {
                    entity_state_t  s;
                    struct gclient_s    *client;    // NULL if not a player
                                                    // the server expects the first part
                                                    // of gclient_s to be a player_state_t
                                                    // but the rest of it is opaque

                    ...
                    ...

                };



####################################################
################ Entity Movement ###################
#################################################### 

5.  Lets see how the entity does movement on the server 

                void ClientThink (edict_t *ent, usercmd_t *ucmd)
                {
                    ...
                    client->ps.pmove.pm_type = PM_NORMAL;
                    ...
                

                }


####################################################
################ Entity Physics ####################
####################################################               

4.  what we are most interested for physics purposes is:


                struct edict_s
                {
                    entity_state_t  s;
                    struct gclient_s    *client;    // NULL if not a player
                                                    // the server expects the first part
                                                    // of gclient_s to be a player_state_t
                                                    // but the rest of it is opaque

                    ...
                    ...

    ----------->    int         svflags;
                    vec3_t      mins, maxs;
                    vec3_t      absmin, absmax, size;
                    solid_t     solid;
                    int         clipmask;
                    edict_t     *owner;


                    ...
                    ...
                }



5.  so when the server initalizes a player: you can see we reset the entity values here

    the player AABB box for physics is just stored in the entity.
    you can see the the AABB box phyiscs is defined here


    we set the solid type to be 

    ----------->    ent->solid = SOLID_BBOX;

    the data is defined below:

                    vec3_t  mins = {-16, -16, -24};
                    vec3_t  maxs = {16, 16, 32};

                    ...
    ----------->    VectorCopy (mins, ent->mins);
    ----------->    VectorCopy (maxs, ent->maxs);


    -   full code below:

                game\p_client.c

                void PutClientInServer (edict_t *ent)
                {
                    vec3_t  mins = {-16, -16, -24};
                    vec3_t  maxs = {16, 16, 32};
                    int     index;
                    vec3_t  spawn_origin, spawn_angles;
                    gclient_t   *client;
                    int     i;
                    client_persistant_t saved;
                    client_respawn_t    resp;

                    ...
                    ...

                    // clear entity values
                    ent->groundentity = NULL;
                    ent->client = &game.clients[index];
                    ent->takedamage = DAMAGE_AIM;
                    ent->movetype = MOVETYPE_WALK;
                    ent->viewheight = 22;
                    ent->inuse = true;
                    ent->classname = "player";
                    ent->mass = 200;
    ----------->    ent->solid = SOLID_BBOX;
                    ent->deadflag = DEAD_NO;
                    ent->air_finished = level.time + 12;
                    ent->clipmask = MASK_PLAYERSOLID;
                    ent->model = "players/male/tris.md2";
                    ent->pain = player_pain;
                    ent->die = player_die;
                    ent->waterlevel = 0;
                    ent->watertype = 0;
                    ent->flags &= ~FL_NO_KNOCKBACK;
                    ent->svflags &= ~SVF_DEADMONSTER;

    ----------->    VectorCopy (mins, ent->mins);
    ----------->    VectorCopy (maxs, ent->maxs);
                    VectorClear (ent->velocity);

                    ...
                    ...
                }










#######################################################################
###################### Entity Model Rendering #########################
#######################################################################


1.  so we have this model field in the entity struct 


                struct edict_s
                {
                    entity_state_t  s;
                    struct gclient_s    *client;    // NULL if not a player
                                                    // the server expects the first part
                                                    // of gclient_s to be a player_state_t
                                                    // but the rest of it is opaque

                    ...
                    ...

                    int         svflags;
                    vec3_t      mins, maxs;
                    vec3_t      absmin, absmax, size;
                    solid_t     solid;
                    int         clipmask;
                    edict_t     *owner;

                    // DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
                    // EXPECTS THE FIELDS IN THAT ORDER!

                    //================================

                    int         movetype;
                    int         flags;

    ----------->    char        *model;
                    float       freetime;           // sv.time when the object was freed
                    

                    ...
                    ...
                }


    so the on the server, the entity is initalized with the model name






5.  so if you want to debug the player, the edict_t that has the following properties
                
                quake2\game\g_local.h

                struct edict_s
                {

                    model = "players/male/tris.md2";
                    ...
                    classname = "player"
                    
                    ...
                }












                server\sv_game.c



                /*
                =================
                PF_setmodel

                Also sets mins and maxs for inline bmodels
                =================
                */
                void PF_setmodel (edict_t *ent, char *name)
                {
                    int     i;
                    cmodel_t    *mod;

                    if (!name)
                        Com_Error (ERR_DROP, "PF_setmodel: NULL");

                    i = SV_ModelIndex (name);
                        
                //  ent->model = name;
                    ent->s.modelindex = i;

                // if it is an inline model, get the size information for it
                    if (name[0] == '*')
                    {
                        mod = CM_InlineModel (name);
                        VectorCopy (mod->mins, ent->mins);
                        VectorCopy (mod->maxs, ent->maxs);
                        SV_LinkEdict (ent);
                    }

                }
























-   so lets see what the ClientThink(); function do 

-   you can see we create a brand new player_move_t

    we first initialize the player origin and velocity on the player_move_t

    the * 8 is for precision purposes.so dont worry about it. You can just ignore that

-   we then perform a move by calling pmove()

-   then we save it back to client->ps.pmove


-   the pointContents tells you what type of the liquid the player is in. 
http://pages.cs.wisc.edu/~jeremyp/quake/quakec/quakec.pdf

        Returns the contents of the area situated at position pos. Used to know if an area 
        is in water, in slime or in lava. Makes use of the BSP tree, and is supposed to be very fast


-   

                game\p_client.c

                void ClientThink (edict_t *ent, usercmd_t *ucmd)
                {
                    pmove_t pm;


                    ...
                    client->ps.pmove.pm_type = PM_NORMAL
                    ...


                    // initialize the player origin and velocity on the player_move_t
                    // with the current position and velocity
                    pm.s = client->ps.pmove;

                    for (i=0 ; i<3 ; i++)
                    {
                        pm.s.origin[i] = ent->s.origin[i]*8;
                        pm.s.velocity[i] = ent->velocity[i]*8;
                    }





                    pm.trace = PM_trace;    // adds default parms
                    pm.pointcontents = gi.pointcontents;

                    // perform a pmove
    ----------->    gi.Pmove (&pm);


                    // save results of pmove
                    client->ps.pmove = pm.s;
                    client->old_pmove = pm.s;
            
                    for (i=0 ; i<3 ; i++)
                    {
                        ent->s.origin[i] = pm.s.origin[i]*0.125;
                        ent->velocity[i] = pm.s.velocity[i]*0.125;
                    }

                    ...
                    ...
                }   





#################################################################
################### Tracing with a Box ##########################
#################################################################

                
so with our player model, we are tracing inside our BSP tree with a box
there is actually a good code analysis for it here.
http://web.archive.org/web/20111112060250/http://www.devmaster.net/articles/quake3collision/


5.  On the Server, we run this 

                quake2\game\p_client.c

                // pmove doesn't need to know about passent and contentmask
                trace_t PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
                {
                    if (pm_passent->health > 0)
                        return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
                    else
                        return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
                }

note that pm_passent is set inside the ClientThink(); function 

so the "pm_passent" is just our player entity

                void ClientThink(edict_t *ent, usercmd_t *ucmd)
                {
                    ...
                    pm_passent = ent;

                    ...
                    gi.Pmove (&pm);
                    ...
                }





6.  the gi.trace(); call come to SV_Trace();
    so you can set a break point here

                quake2\server\sv_world.c

                trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask)
                {
                    moveclip_t  clip;

                    ...

                    // clip to world
    ----------->    clip.trace = CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
                    clip.trace.ent = ge->edicts;
                    if (clip.trace.fraction == 0)
                        return clip.trace;      // blocked by the world

                    clip.contentmask = contentmask;
                    clip.start = start;
                    clip.end = end;
                    clip.mins = mins;
                    clip.maxs = maxs;
                    clip.passedict = passedict;

                    VectorCopy (mins, clip.mins2);
                    VectorCopy (maxs, clip.maxs2);
                    
                    // create the bounding box of the entire move
                    SV_TraceBounds ( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

                    // clip to other solid entities
                    SV_ClipMoveToEntities ( &clip );

                    return clip.trace;
                }

















11. 


                /*
                ================
                CM_TestBoxInBrush
                ================
                */
                void CM_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1,
                                      trace_t *trace, cbrush_t *brush)
                {
                    int         i, j;
                    cplane_t    *plane;
                    float       dist;
                    vec3_t      ofs;
                    float       d1;
                    cbrushside_t    *side;

                    if (!brush->numsides)
                        return;



                    for (i=0 ; i<brush->numsides ; i++)
                    {
                        side = &map_brushsides[brush->firstbrushside+i];
                        plane = side->plane;

                        // FIXME: special case for axial

                        // general box case

                        // push the plane out apropriately for mins/maxs
                        printf("           plane %d", plane.);

                        // FIXME: use signbits into 8 way lookup for each mins/maxs
                        for (j=0 ; j<3 ; j++)
                        {
                            if (plane->normal[j] < 0)
                                ofs[j] = maxs[j];
                            else
                                ofs[j] = mins[j];
                        }
                        dist = DotProduct (ofs, plane->normal);
                        dist = plane->dist - dist;

                        d1 = DotProduct (p1, plane->normal) - dist;

                        // if completely in front of face, no intersection
                        if (d1 > 0)
                            return;

                    }

                    // inside this brush
                    trace->startsolid = trace->allsolid = true;
                    trace->fraction = 0;
                    trace->contents = brush->contents;
                }








8.  

                /*
                ==================
                SV_Trace

                Moves the given mins/maxs volume through the world from start to end.

                Passedict and edicts owned by passedict are explicitly not checked.

                ==================
                */
                trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask)
                {
                    moveclip_t  clip;

                    if (!mins)
                        mins = vec3_origin;
                    if (!maxs)
                        maxs = vec3_origin;

                    memset ( &clip, 0, sizeof ( moveclip_t ) );

                    // clip to world
                    clip.trace = CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
                    clip.trace.ent = ge->edicts;
                    if (clip.trace.fraction == 0)
                        return clip.trace;      // blocked by the world

                    clip.contentmask = contentmask;
                    clip.start = start;
                    clip.end = end;
                    clip.mins = mins;
                    clip.maxs = maxs;
                    clip.passedict = passedict;

                    VectorCopy (mins, clip.mins2);
                    VectorCopy (maxs, clip.maxs2);
                    
                    // create the bounding box of the entire move
    ----------->    SV_TraceBounds ( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

                    // clip to other solid entities
                    SV_ClipMoveToEntities ( &clip );

                    return clip.trace;
                }





9.  

                /*
                ==================
                SV_TraceBounds
                ==================
                */
                void SV_TraceBounds (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
                {
                #if 0
                // debug to test against everything
                boxmins[0] = boxmins[1] = boxmins[2] = -9999;
                boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
                #else
                    int     i;
                    
                    for (i=0 ; i<3 ; i++)
                    {
                        if (end[i] > start[i])
                        {
                            boxmins[i] = start[i] + mins[i] - 1;
                            boxmaxs[i] = end[i] + maxs[i] + 1;
                        }
                        else
                        {
                            boxmins[i] = end[i] + mins[i] - 1;
                            boxmaxs[i] = start[i] + maxs[i] + 1;
                        }
                    }
                #endif
                }







################################################################
################### SV_ClipMoveToEntities ######################
################################################################


10. 

                /*
                ====================
                SV_ClipMoveToEntities

                ====================
                */
                void SV_ClipMoveToEntities ( moveclip_t *clip )
                {
                    int         i, num;
                    edict_t     *touchlist[MAX_EDICTS], *touch;
                    trace_t     trace;
                    int         headnode;
                    float       *angles;

                    num = SV_AreaEdicts (clip->boxmins, clip->boxmaxs, touchlist
                        , MAX_EDICTS, AREA_SOLID);

                    // be careful, it is possible to have an entity in this
                    // list removed before we get to it (killtriggered)
                    for (i=0 ; i<num ; i++)
                    {
                        touch = touchlist[i];
                        if (touch->solid == SOLID_NOT)
                            continue;
                        if (touch == clip->passedict)
                            continue;
                        if (clip->trace.allsolid)
                            return;
                        if (clip->passedict)
                        {
                            if (touch->owner == clip->passedict)
                                continue;   // don't clip against own missiles
                            if (clip->passedict->owner == touch)
                                continue;   // don't clip against owner
                        }

                        if ( !(clip->contentmask & CONTENTS_DEADMONSTER)
                        && (touch->svflags & SVF_DEADMONSTER) )
                                continue;

                        // might intersect, so do an exact clip
                        headnode = SV_HullForEntity (touch);
                        angles = touch->s.angles;
                        if (touch->solid != SOLID_BSP)
                            angles = vec3_origin;   // boxes don't rotate

                        if (touch->svflags & SVF_MONSTER)
                            trace = CM_TransformedBoxTrace (clip->start, clip->end,
                                clip->mins2, clip->maxs2, headnode, clip->contentmask,
                                touch->s.origin, angles);
                        else
                            trace = CM_TransformedBoxTrace (clip->start, clip->end,
                                clip->mins, clip->maxs, headnode,  clip->contentmask,
                                touch->s.origin, angles);

                        if (trace.allsolid || trace.startsolid ||
                        trace.fraction < clip->trace.fraction)
                        {
                            trace.ent = touch;
                            if (clip->trace.startsolid)
                            {
                                clip->trace = trace;
                                clip->trace.startsolid = true;
                            }
                            else
                                clip->trace = trace;
                        }
                        else if (trace.startsolid)
                            clip->trace.startsolid = true;
                    }
                }





11. 

                /*
                ================
                SV_AreaEdicts
                ================
                */
                int SV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list,
                    int maxcount, int areatype)
                {
                    area_mins = mins;
                    area_maxs = maxs;
                    area_list = list;
                    area_count = 0;
                    area_maxcount = maxcount;
                    area_type = areatype;

                    SV_AreaEdicts_r (sv_areanodes);

                    return area_count;
                }




12.    
    

                /*
                ====================
                SV_AreaEdicts_r

                ====================
                */
                void SV_AreaEdicts_r (areanode_t *node)
                {
                    link_t      *l, *next, *start;
                    edict_t     *check;
                    int         count;

                    count = 0;

                    // touch linked edicts
                    if (area_type == AREA_SOLID)
                        start = &node->solid_edicts;
                    else
                        start = &node->trigger_edicts;

                    for (l=start->next  ; l != start ; l = next)
                    {
                        next = l->next;
                        check = EDICT_FROM_AREA(l);

                        if (check->solid == SOLID_NOT)
                            continue;       // deactivated
                        if (check->absmin[0] > area_maxs[0]
                        || check->absmin[1] > area_maxs[1]
                        || check->absmin[2] > area_maxs[2]
                        || check->absmax[0] < area_mins[0]
                        || check->absmax[1] < area_mins[1]
                        || check->absmax[2] < area_mins[2])
                            continue;       // not touching

                        if (area_count == area_maxcount)
                        {
                            Com_Printf ("SV_AreaEdicts: MAXCOUNT\n");
                            return;
                        }

                        area_list[area_count] = check;
                        area_count++;
                    }
                    
                    if (node->axis == -1)
                        return;     // terminal node

                    // recurse down both sides
                    if ( area_maxs[node->axis] > node->dist )
                        SV_AreaEdicts_r ( node->children[0] );
                    if ( area_mins[node->axis] < node->dist )
                        SV_AreaEdicts_r ( node->children[1] );
                }














13. 
                /*
                ================
                PM_SnapPosition

                On exit, the origin will have a value that is pre-quantized to the 0.125
                precision of the network channel and in a valid position.
                ================
                */
                void PM_SnapPosition (void)
                {
                    int     sign[3];
                    int     i, j, bits;
                    short   base[3];
                    // try all single bits first
                    static int jitterbits[8] = {0,4,1,2,3,5,6,7};

                    // snap velocity to eigths
                    for (i=0 ; i<3 ; i++)
                        pm->s.velocity[i] = (int)(pml.velocity[i]*8);

                    for (i=0 ; i<3 ; i++)
                    {
                        if (pml.origin[i] >= 0)
                            sign[i] = 1;
                        else 
                            sign[i] = -1;
                        pm->s.origin[i] = (int)(pml.origin[i]*8);
                        if (pm->s.origin[i]*0.125 == pml.origin[i])
                            sign[i] = 0;
                    }
                    VectorCopy (pm->s.origin, base);

                    // try all combinations
                    for (j=0 ; j<8 ; j++)
                    {
                        bits = jitterbits[j];
                        VectorCopy (base, pm->s.origin);
                        for (i=0 ; i<3 ; i++)
                            if (bits & (1<<i) )
                                pm->s.origin[i] += sign[i];

                        if (PM_GoodPosition ())
                            return;
                    }

                    // go back to the last position
                    VectorCopy (pml.previous_origin, pm->s.origin);
                //  Com_DPrintf ("using previous_origin\n");
                }












5.  so if you look at BSP trees, there is section on "clipping hulls"

                https://developer.valvesoftware.com/wiki/BSP









###############################################################
################## Client Prediction ##########################
###############################################################






5.  the pm->trace call directs to CL_PMTrace(); I am assuming CL means client 
PM is player model. Trace means collision detection

here you can see we pass in two variables, start, end is 
[mins maxs] is the bounding box of the player, 
[start end] is the starting point of the player, and where the player wants to end up

                quake2\client\cl_pred.c

                trace_t     CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
                {
                    trace_t t;

                    // check against world
                    t = CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
                    if (t.fraction < 1.0)
                        t.ent = (struct edict_s *)1;

                    // check all other solid models
                    CL_ClipMoveToEntities (start, mins, maxs, end, &t);

                    return t;
                }











7.  so this it he case where the position didnt move.

                    VectorAdd (start, mins, c1);
                    VectorAdd (start, maxs, c2);

-   so c1 and c2 are the absolute bouding box


                   if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
                    {
                        int     leafs[1024];
                        int     i, numleafs;
                        vec3_t  c1, c2;
                        int     topnode;

                        VectorAdd (start, mins, c1);
                        VectorAdd (start, maxs, c2);
                        for (i=0 ; i<3 ; i++)
                        {
                            c1[i] -= 1;
                            c2[i] += 1;
                        }

                        numleafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
                        for (i=0 ; i<numleafs ; i++)
                        {
                            CM_TestInLeaf (leafs[i]);
                            if (trace_trace.allsolid)
                                break;
                        }
                        VectorCopy (start, trace_trace.endpos);
                        return trace_trace;
                    }



8.  


                int CM_BoxLeafnums_headnode (vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
                {
                    leaf_list = list;
                    leaf_count = 0;
                    leaf_maxcount = listsize;
                    leaf_mins = mins;
                    leaf_maxs = maxs;

                    leaf_topnode = -1;

                    CM_BoxLeafnums_r (headnode);

                    if (topnode)
                        *topnode = leaf_topnode;

                    return leaf_count;
                }





9.  

                void CM_BoxLeafnums_r (int nodenum)
                {
                    cplane_t    *plane;
                    cnode_t     *node;
                    int     s;

                    while (1)
                    {
                        if (nodenum < 0)
                        {
                            if (leaf_count >= leaf_maxcount)
                            {
                //              Com_Printf ("CM_BoxLeafnums_r: overflow\n");
                                return;
                            }
                            leaf_list[leaf_count++] = -1 - nodenum;
                            return;
                        }
                    
    --------------->    node = &map_nodes[nodenum];
                        plane = node->plane;
                //      s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
                        s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
                        if (s == 1)
                            nodenum = node->children[0];
                        else if (s == 2)
                            nodenum = node->children[1];
                        else
                        {   // go down both
                            if (leaf_topnode == -1)
                                leaf_topnode = nodenum;
                            CM_BoxLeafnums_r (node->children[0]);
                            nodenum = node->children[1];
                        }

                    }
                }








                quake2\qcommon\cmodel.c



                int         numplanes;
                cplane_t    map_planes[MAX_MAP_PLANES+6];       // extra for box hull

                int         numnodes;
                cnode_t     map_nodes[MAX_MAP_NODES+6];     // extra for box hull

























6.  

                quake2\client\cl_pred.c



                /*
                ====================
                CL_ClipMoveToEntities

                ====================
                */
                void CL_ClipMoveToEntities ( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr )
                {
                    int         i, x, zd, zu;
                    trace_t     trace;
                    int         headnode;
                    float       *angles;
                    entity_state_t  *ent;
                    int         num;
                    cmodel_t        *cmodel;
                    vec3_t      bmins, bmaxs;

                    for (i=0 ; i<cl.frame.num_entities ; i++)
                    {
                        num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
                        ent = &cl_parse_entities[num];

                        if (!ent->solid)
                            continue;

                        if (ent->number == cl.playernum+1)
                            continue;

                        if (ent->solid == 31)
                        {   // special value for bmodel
                            cmodel = cl.model_clip[ent->modelindex];
                            if (!cmodel)
                                continue;
                            headnode = cmodel->headnode;
                            angles = ent->angles;
                        }
                        else
                        {   // encoded bbox
                            x = 8*(ent->solid & 31);
                            zd = 8*((ent->solid>>5) & 31);
                            zu = 8*((ent->solid>>10) & 63) - 32;

                            bmins[0] = bmins[1] = -x;
                            bmaxs[0] = bmaxs[1] = x;
                            bmins[2] = -zd;
                            bmaxs[2] = zu;

                            headnode = CM_HeadnodeForBox (bmins, bmaxs);
                            angles = vec3_origin;   // boxes don't rotate
                        }

                        if (tr->allsolid)
                            return;

                        trace = CM_TransformedBoxTrace (start, end,
                            mins, maxs, headnode,  MASK_PLAYERSOLID,
                            ent->origin, angles);

                        if (trace.allsolid || trace.startsolid ||
                        trace.fraction < tr->fraction)
                        {
                            trace.ent = (struct edict_s *)ent;
                            if (tr->startsolid)
                            {
                                *tr = trace;
                                tr->startsolid = true;
                            }
                            else
                                *tr = trace;
                        }
                        else if (trace.startsolid)
                            tr->startsolid = true;
                    }
                }



















5.  

                p_client.c

                // pmove doesn't need to know about passent and contentmask
                trace_t PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
                {
                    if (pm_passent->health > 0)
                        return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
                    else
                        return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
                }




##########################################################
###################### Monster Move ######################
##########################################################




8.  

                game/g_moster.c

                void monster_think (edict_t *self)
                {
                    M_MoveFrame (self);
                    if (self->linkcount != self->monsterinfo.linkcount)
                    {
                        self->monsterinfo.linkcount = self->linkcount;
                        M_CheckGround (self);
                    }
                    M_CatagorizePosition (self);
                    M_WorldEffects (self);
                    M_SetEffects (self);
                }




9.



                void M_CheckGround (edict_t *ent)
                {
                    vec3_t      point;
                    trace_t     trace;

                    if (ent->flags & (FL_SWIM|FL_FLY))
                        return;

                    if (ent->velocity[2] > 100)
                    {
                        ent->groundentity = NULL;
                        return;
                    }

                // if the hull point one-quarter unit down is solid the entity is on ground
                    point[0] = ent->s.origin[0];
                    point[1] = ent->s.origin[1];
                    point[2] = ent->s.origin[2] - 0.25;

    ----------->    trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

                    // check steepness
                    if ( trace.plane.normal[2] < 0.7 && !trace.startsolid)
                    {
                        ent->groundentity = NULL;
                        return;
                    }

                //  ent->groundentity = trace.ent;
                //  ent->groundentity_linkcount = trace.ent->linkcount;
                //  if (!trace.startsolid && !trace.allsolid)
                //      VectorCopy (trace.endpos, ent->s.origin);
                    if (!trace.startsolid && !trace.allsolid)
                    {
                        VectorCopy (trace.endpos, ent->s.origin);
                        ent->groundentity = trace.ent;
                        ent->groundentity_linkcount = trace.ent->linkcount;
                        ent->velocity[2] = 0;
                    }
                }







10. 

                game.h 

                typedef struct
                {

                    ...
                    // collision detection
                    trace_t (*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passent, int contentmask);
            
                    ...
                    ...
                };




11.
                
                quake2\server\sv_world.c
                

                trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask)
                {
                    moveclip_t  clip;

                    if (!mins)
                        mins = vec3_origin;
                    if (!maxs)
                        maxs = vec3_origin;

                    memset ( &clip, 0, sizeof ( moveclip_t ) );

                    // clip to world
                    clip.trace = CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
                    clip.trace.ent = ge->edicts;
                    if (clip.trace.fraction == 0)
                        return clip.trace;      // blocked by the world

                    clip.contentmask = contentmask;
                    clip.start = start;
                    clip.end = end;
                    clip.mins = mins;
                    clip.maxs = maxs;
                    clip.passedict = passedict;

                    VectorCopy (mins, clip.mins2);
                    VectorCopy (maxs, clip.maxs2);
                    
                    // create the bounding box of the entire move
                    SV_TraceBounds ( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

                    // clip to other solid entities
                    SV_ClipMoveToEntities ( &clip );

                    return clip.trace;
                }





12.             
                quake2\qcommon\cmodel.c


                trace_t CM_BoxTrace (vec3_t start, vec3_t end,
                                          vec3_t mins, vec3_t maxs,
                                          int headnode, int brushmask)
                {
                    int     i;

                    checkcount++;       // for multi-check avoidance

                    c_traces++;         // for statistics, may be zeroed

                    // fill in a default trace
                    memset (&trace_trace, 0, sizeof(trace_trace));
                    trace_trace.fraction = 1;
                    trace_trace.surface = &(nullsurface.c);

                    if (!numnodes)  // map not loaded
                        return trace_trace;

                    trace_contents = brushmask;
                    VectorCopy (start, trace_start);
                    VectorCopy (end, trace_end);
                    VectorCopy (mins, trace_mins);
                    VectorCopy (maxs, trace_maxs);

                    //
                    // check for position test special case
                    //
                    if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
                    {
                        int     leafs[1024];
                        int     i, numleafs;
                        vec3_t  c1, c2;
                        int     topnode;

                        VectorAdd (start, mins, c1);
                        VectorAdd (start, maxs, c2);
                        for (i=0 ; i<3 ; i++)
                        {
                            c1[i] -= 1;
                            c2[i] += 1;
                        }

                        numleafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
                        for (i=0 ; i<numleafs ; i++)
                        {
                            CM_TestInLeaf (leafs[i]);
                            if (trace_trace.allsolid)
                                break;
                        }
                        VectorCopy (start, trace_trace.endpos);
                        return trace_trace;
                    }

                    //
                    // check for point special case
                    //
                    if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
                        && maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
                    {
                        trace_ispoint = true;
                        VectorClear (trace_extents);
                    }
                    else
                    {
                        trace_ispoint = false;
                        trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
                        trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
                        trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
                    }

                    //
                    // general sweeping through world
                    //
                    CM_RecursiveHullCheck (headnode, 0, 1, start, end);

                    if (trace_trace.fraction == 1)
                    {
                        VectorCopy (end, trace_trace.endpos);
                    }
                    else
                    {
                        for (i=0 ; i<3 ; i++)
                            trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
                    }
                    return trace_trace;
                }
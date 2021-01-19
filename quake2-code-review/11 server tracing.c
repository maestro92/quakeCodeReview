


#################################################################
################### Tracing with a Box ##########################
#################################################################

So assuming you just came from pmove movement code.c. The pmove(); function makes a lot of calls 
to the pm->trace(); function. here we take a look at what does the trace(); function do. 

1.  so lets start from PM_StepSlideMove(); 


                void PM_StepSlideMove_ (void)
                {

                    for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
                    {
                        ...
                        trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

                        ...
                        ...
                    }

                }


2.  that calls PM_trace();

so with our player movement, we are tracing inside our BSP tree with a box
there is actually a good code analysis for it here.
http://web.archive.org/web/20111112060250/http://www.devmaster.net/articles/quake3collision/


                quake2-vs2015-master\game\p_client.c
                
                edict_t *pm_passent;

                // pmove doesn't need to know about passent and contentmask
                trace_t PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
                {
                    if (pm_passent->health > 0)
    --------------->    return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
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




3.  the gi.trace(); call come to SV_Trace();
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




7.  The min and max passed in here are in object space 

    for the main player, mins and maxs will always be 

                mins:   {-16 -16 -24}
                maxs:   { 16  16  32}

    start and end will be in absolute position

    the player is just a simple bounding box, so we call CM_BoxTrace();

    so the player is just a box, so we go into CM_BoxTrace();

-   so you can see that we have a "check for position test special case"
    this is the case when the position is exactly the same where 
    start equals end, so the player didnt move

-   trace_extents stores the maximum of the absolute value of each axis in the box. 
                For example, 
                    if traceMins was (-100,-3,-15) and 
                       traceMaxs was (55,  22, 7), 

            then traceExtents would be (100,22,15).

-   we set values of trace_extents and trace_ispoint. We then use these two inside collision detection


-   full code below:

                cmodel.c

                trace_t     CM_BoxTrace (vec3_t start, vec3_t end,
                                          vec3_t mins, vec3_t maxs,
                                          int headnode, int brushmask)
                {
                    ...

                    // fill in a default trace
                    memset (&trace_trace, 0, sizeof(trace_trace));
                    trace_trace.fraction = 1;
                    trace_trace.surface = &(nullsurface.c);

                    ...
                    ...

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
    ----------->    CM_RecursiveHullCheck (headnode, 0, 1, start, end);

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
                





7.  So the most important function here is the CM_RecursiveHullCheck. This is where 
we do our collision detection in our BSP tree.

so lets see the arguments being passed in:

    num:    the bsp node id.
    p1f:    point 1 fraction. starts at 0.
    p2f:    point 2 fraction. starts at 1.
    p1:     start pos of the box sweeping trajectory
    p2:     end pos of the box sweeping trajectory


essentially for box to sweep from p1 to p2, we are mapping it to 0 to 1.

                qcommon\cmodel.c

                void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
                {
                    ...
                    ...

                    //
                    // find the point distances to the seperating plane
                    // and the offset for the size of the box
                    //
                    node = map_nodes + num;
                    plane = node->plane;

   ------------>    if (plane->type < 3)
                    {
                        t1 = p1[plane->type] - plane->dist;
                        t2 = p2[plane->type] - plane->dist;
                        offset = trace_extents[plane->type];
                    }
                    else
                    {
                        t1 = DotProduct (plane->normal, p1) - plane->dist;
                        t2 = DotProduct (plane->normal, p2) - plane->dist;
                        if (trace_ispoint)
                            offset = 0;
                        else
                            offset = fabs(trace_extents[0]*plane->normal[0]) +
                                fabs(trace_extents[1]*plane->normal[1]) +
                                fabs(trace_extents[2]*plane->normal[2]);
                    }

                    ...

                    // see which sides we need to consider
                    if (t1 >= offset && t2 >= offset)
                    {
                        CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
                        return;
                    }
                    if (t1 < -offset && t2 < -offset)
                    {
                        CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
                        return;
                    }

                    // put the crosspoint DIST_EPSILON pixels on the near side
                    if (t1 < t2)
                    {
                        idist = 1.0/(t1-t2);
                        side = 1;
                        frac2 = (t1 + offset + DIST_EPSILON)*idist;
                        frac = (t1 - offset + DIST_EPSILON)*idist;
                    }
                    else if (t1 > t2)
                    {
                        idist = 1.0/(t1-t2);
                        side = 0;
                        frac2 = (t1 - offset - DIST_EPSILON)*idist;
                        frac = (t1 + offset + DIST_EPSILON)*idist;
                    }
                    else
                    {
                        side = 0;
                        frac = 1;
                        frac2 = 0;
                    }

                    // move up to the node
                    if (frac < 0)
                        frac = 0;
                    if (frac > 1)
                        frac = 1;
                        
                    midf = p1f + (p2f - p1f)*frac;
                    for (i=0 ; i<3 ; i++)
                        mid[i] = p1[i] + frac*(p2[i] - p1[i]);

                    CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);


                    // go past the node
                    if (frac2 < 0)
                        frac2 = 0;
                    if (frac2 > 1)
                        frac2 = 1;
                        
                    midf = p1f + (p2f - p1f)*frac2;
                    for (i=0 ; i<3 ; i++)
                        mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

                    CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
                }



now lets look at the details 

                if (plane->type < 3)
                {
                    t1 = p1[plane->type] - plane->dist;
                    t2 = p2[plane->type] - plane->dist;
                    offset = trace_extents[plane->type];
                }
                else
                {
                    t1 = DotProduct (plane->normal, p1) - plane->dist;
                    t2 = DotProduct (plane->normal, p2) - plane->dist;
                    if (trace_ispoint)
                        offset = 0;
                    else
                        offset = fabs(trace_extents[0]*plane->normal[0]) +
                            fabs(trace_extents[1]*plane->normal[1]) +
                            fabs(trace_extents[2]*plane->normal[2]);
                }



so the different types of plane can be different type value 

                // 0-2 are axial planes
                #define PLANE_X         0
                #define PLANE_Y         1
                #define PLANE_Z         2

                // 3-5 are non-axial planes snapped to the nearest
                #define PLANE_ANYX      3
                #define PLANE_ANYY      4
                #define PLANE_ANYZ      5

                // planes (x&~1) and (x&~1)+1 are always opposites



so if plane->type < 3, then we only need PLANE_X, PLANE_Y and PLANE_Z
                    
the most general formula is of course

                t1 = DotProduct (plane->normal, p1) - plane->dist;

but in the case of (1, 0, 0)
so therefore in the case, PLANE_X, PLANE_Y and PLANE_Z can be simplied into

                t1 = p1[plane->type] - plane->dist;


so here offset is just the distance from the center point of our box to the plane.



in the case where we dont have an axis aligned plane, we go into the else case 
we do the distance calculation for the most general cases

                t1 = DotProduct (plane->normal, p1) - plane->dist;
                t2 = DotProduct (plane->normal, p2) - plane->dist;
                if (trace_ispoint)
                    offset = 0;
                else
                    offset = fabs(trace_extents[0]*plane->normal[0]) +
                        fabs(trace_extents[1]*plane->normal[1]) +
                        fabs(trace_extents[2]*plane->normal[2]);


again, the offset is just the calculation for the distance from the center of our box to the plane.







8.  so the math here

                t1 = p1[plane->type] - plane->dist;
                t2 = p2[plane->type] - plane->dist;
                offset = trace_extents[plane->type];


                if (t1 >= offset && t2 >= offset)


is essentially 
                start[plane->type] > plane->dist + offset

if we put into the proper general formula, its

                DotProduct (plane->normal, p1) > plane->dist + offset.

which make sense. 
                
                essentially we want to see which side of the splitting plane this lies.


                qcommon\cmodel.c

                void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
                {
                    ...
                    ...

                    //
                    // find the point distances to the seperating plane
                    // and the offset for the size of the box
                    //
                    node = map_nodes + num;
                    plane = node->plane;

                    if (plane->type < 3)
                    {
                        t1 = p1[plane->type] - plane->dist;
                        t2 = p2[plane->type] - plane->dist;
                        offset = trace_extents[plane->type];
                    }
                    else
                    {
                        ...
                        ...
                    }

                    ...

                    // see which sides we need to consider
                    if (t1 >= offset && t2 >= offset)
                    {
                        CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
                        return;
                    }
                    if (t1 < -offset && t2 < -offset)
                    {
                        CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
                        return;
                    }

                    ...
                    ...
                }





9.  now we see what happens if we go into a leaf node 

                cmodel.c

                void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
                {
                    ...
                    ...

                    if (trace_trace.fraction <= p1f)
                        return;     // already hit something nearer

                    // if < 0, we are in a leaf node
                    if (num < 0)
                    {
    --------------->    CM_TraceToLeaf (-1-num);
                        return;
                    }

                    ...
                    ...
                }





10. Here you can see we are doing a collision detection against a brush             

                quake2\qcommon\cmodel.c

                /*
                ================
                CM_TraceToLeaf
                ================
                */
                void CM_TraceToLeaf (int leafnum)
                {
                    int         k;
                    int         brushnum;
                    cleaf_t     *leaf;
                    cbrush_t    *b;

                    leaf = &map_leafs[leafnum];
                    if ( !(leaf->contents & trace_contents))
                        return;

                    printf("leaf->numleafbrushes %d", leaf->numleafbrushes);

                    // trace line against all brushes in the leaf
                    for (k=0 ; k<leaf->numleafbrushes ; k++)
                    {
                        brushnum = map_leafbrushes[leaf->firstleafbrush+k];
                        b = &map_brushes[brushnum];
                        if (b->checkcount == checkcount)
                            continue;   // already checked this brush in another leaf
                        b->checkcount = checkcount;

                        if ( !(b->contents & trace_contents))
                            continue;
    ---------------->   CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
                        if (!trace_trace.fraction)
                            return;
                    }

                }




11. Here we are calling CM_ClipBoxToBrush(); We want to populate how much trace->fraction.


                /*
                ================
                CM_ClipBoxToBrush
                ================
                */
                void CM_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
                                      trace_t *trace, cbrush_t *brush)
                {
                    int         i, j;
                    cplane_t    *plane, *clipplane;
                    float       dist;
                    float       enterfrac, leavefrac;
                    vec3_t      ofs;
                    float       d1, d2;
                    qboolean    getout, startout;
                    float       f;
                    cbrushside_t    *side, *leadside;

                    enterfrac = -1;
                    leavefrac = 1;
                    clipplane = NULL;

                    if (!brush->numsides)
                        return;

                    c_brush_traces++;

                    getout = false;
                    startout = false;
                    leadside = NULL;

                    printf("           brush->numsides %d\n", brush->numsides);

                    for (i=0 ; i<brush->numsides ; i++)
                    {
                        side = &map_brushsides[brush->firstbrushside+i];
                        plane = side->plane;

                        PrintVec("                  plane\n", plane->normal);

                        // FIXME: special case for axial

                        if (!trace_ispoint)
                        {   // general box case

                            // push the plane out apropriately for mins/maxs

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
                        }
                        else
                        {   // special point case
                            dist = plane->dist;
                        }

                        d1 = DotProduct (p1, plane->normal) - dist;
                        d2 = DotProduct (p2, plane->normal) - dist;

                        if (d2 > 0)
                            getout = true;  // endpoint is not in solid
                        if (d1 > 0)
                            startout = true;

                        // if completely in front of face, no intersection
                        if (d1 > 0 && d2 >= d1)
                            return;

                        if (d1 <= 0 && d2 <= 0)
                            continue;

                        // crosses face
                        if (d1 > d2)
                        {   // enter
                            f = (d1-DIST_EPSILON) / (d1-d2);
                            if (f > enterfrac)
                            {
                                enterfrac = f;
                                clipplane = plane;
                                leadside = side;
                            }
                        }
                        else
                        {   // leave
                            f = (d1+DIST_EPSILON) / (d1-d2);
                            if (f < leavefrac)
                                leavefrac = f;
                        }
                    }

                    if (!startout)
                    {   // original point was inside brush
                        trace->startsolid = true;
                        if (!getout)
                            trace->allsolid = true;
                        return;
                    }
                    if (enterfrac < leavefrac)
                    {
                        if (enterfrac > -1 && enterfrac < trace->fraction)
                        {
                            if (enterfrac < 0)
                                enterfrac = 0;
                            trace->fraction = enterfrac;
                            trace->plane = *clipplane;
                            trace->surface = &(leadside->surface->c);
                            trace->contents = brush->contents;
                        }
                    }
                }









































1. so player tracing function is CL_PMTrace: Client_PlayerModel_Trace(); function.

                /*
                ================
                CL_PMTrace
                ================
                */
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



12. 

                /*
                ==================
                CM_BoxTrace
                ==================
                */
                trace_t     CM_BoxTrace (vec3_t start, vec3_t end,
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

                    /*
                    PrintVec("start", start);
                    PrintVec("end", end);
                    PrintVec("mins", mins);
                    PrintVec("maxs", maxs);
                    */

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




#####################################################################
################################# physics ###########################
#####################################################################

here we want to look at how the map file works and how the physics work



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
    ----------->        M_CheckGround (ent);
                    }
                }


                if (i > 0 && i <= maxclients->value)
                {
    -------->       ClientBeginServerFrame (ent);
                    continue;
                }

                G_RunEntity (ent);
            }


            // build the playerstate_t structures for all players
            ClientEndServerFrames ();
        }


    notice in their game, they keep all the objects in an array, and they use the if(!ent->inuse) to 
    skip the ones that are not in use



5.  
                g_monster.c

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

                    trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

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



6.  here we define tne the different move types    
                
                g_local.h 

                // edict->movetype values
                typedef enum
                {
                    MOVETYPE_NONE,          // never moves
                    MOVETYPE_NOCLIP,        // origin and angles change with no interaction
                    MOVETYPE_PUSH,          // no clip to world, push on box contact
                    MOVETYPE_STOP,          // no clip to world, stops on box contact

                    MOVETYPE_WALK,          // gravity
                    MOVETYPE_STEP,          // gravity, special edge handling
                    MOVETYPE_FLY,
                    MOVETYPE_TOSS,          // gravity
                    MOVETYPE_FLYMISSILE,    // extra size to monsters
                    MOVETYPE_BOUNCE
                } movetype_t;



7.  as the main player, you are the MOVETYPE_PUSH





#####################################################################
################################# edict_s ###########################
#####################################################################

1.  The definition of an entity is defined in game.h


                game/game.h


                typedef struct edict_s edict_t;

                ...
                ...

                struct edict_s
                {
                    entity_state_t  s;
                    struct gclient_s    *client;
    ----------->    qboolean    inuse;
                    int         linkcount;

                    // FIXME: move these fields to a server private sv_entity_t
                    link_t      area;               // linked to a division node or leaf
                    
                    int         num_clusters;       // if -1, use headnode instead
                    int         clusternums[MAX_ENT_CLUSTERS];
                    int         headnode;           // unused if num_clusters != -1
                    int         areanum, areanum2;

                    //================================

                    int         svflags;            // SVF_NOCLIENT, SVF_DEADMONSTER, SVF_MONSTER, etc
                    vec3_t      mins, maxs;
                    vec3_t      absmin, absmax, size;
                    solid_t     solid;
                    int         clipmask;
                    edict_t     *owner;

                    // the game dll can add anything it wants after
                    // this point in the structure
                };

                ...
                ...

                struct edict_s  *edicts;


some of the important things for game logic are "entity_state_t" and "inuse"

2.  the entity_state_t variable

                game/game.h

                struct edict_s
                {
    ----------->    entity_state_t  s;

                    ...
                    ...

                };


that is defined in q_shared.h

                // entity_state_t is the information conveyed from the server
                // in an update message about entities that the client will
                // need to render in some way
                typedef struct entity_state_s
                {
    ----------->    int     number;         // edict index

                    vec3_t  origin;
                    vec3_t  angles;
                    vec3_t  old_origin;     // for lerping
                    int     modelindex;
                    int     modelindex2, modelindex3, modelindex4;  // weapons, CTF flags, etc
                    int     frame;
                    int     skinnum;
                    unsigned int        effects;        // PGM - we're filling it, so it needs to be unsigned
                    int     renderfx;
                    int     solid;          // for client side prediction, 8*(bits 0-4) is x/y radius
                                            // 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
                                            // gi.linkentity sets this properly
                    int     sound;          // for looping sounds, to guarantee shutoff
                    int     event;          // impulse events -- muzzle flashes, footsteps, etc
                                            // events only go out for a single frame, they
                                            // are automatically cleared each frame
                } entity_state_t;


the "number" field is like an id for entities, which also happens to be the index in the edict array.




3.  the "inuse" flag is to indicate whether an entity in the edicts array is active or killed



##################################################################################
######################### Adding and Removing Entity #############################
##################################################################################

so in quake_s entity system, when entity gets killed, it uses a flag to indicate whether 
that entity is active or not. 


                g_utils.c

                void G_InitEdict (edict_t *e)
                {
    ----------->    e->inuse = true;
                    e->classname = "noclass";
                    e->gravity = 1.0;
                    e->s.number = e - g_edicts;
                }


when entity dies, we just set the ed->inuse to false

                /*
                =================
                G_FreeEdict

                Marks the edict as free
                =================
                */
                void G_FreeEdict (edict_t *ed)
                {
                    gi.unlinkentity (ed);       // unlink from world

                    if ((ed - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))
                    {
                //      gi.dprintf("tried to free special edict\n");
                        return;
                    }

                    memset (ed, 0, sizeof(*ed));
                    ed->classname = "freed";
                    ed->freetime = level.time;
    ----------->    ed->inuse = false;
                }



-   when adding a new entity, we call the G_Spawn(); function 
    as you can see, we are just looking for an empty spot in the g_edicts array. 

                g_utils.c

                /*
                =================
                G_Spawn

                Either finds a free edict, or allocates a new one.
                Try to avoid reusing an entity that was recently freed, because it
                can cause the client to think the entity morphed into something else
                instead of being removed and recreated, which can cause interpolated
                angles and bad trails.
                =================
                */
                edict_t *G_Spawn (void)
                {
                    int         i;
                    edict_t     *e;

                    e = &g_edicts[(int)maxclients->value+1];
                    for ( i=maxclients->value+1 ; i<globals.num_edicts ; i++, e++)
                    {
                        // the first couple seconds of server time can involve a lot of
                        // freeing and allocating, so relax the replacement policy
                        if (!e->inuse && ( e->freetime < 2 || level.time - e->freetime > 0.5 ) )
                        {
                            G_InitEdict (e);
                            return e;
                        }
                    }
                    
                    if (i == game.maxentities)
                        gi.error ("ED_Alloc: no free edicts");
                        
                    globals.num_edicts++;
                    G_InitEdict (e);
                    return e;
                }

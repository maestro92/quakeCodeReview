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



###############################################################################
########################## Physics and Rendering ##############################
###############################################################################


                game\q_shared.h

                // entity_state_t is the information conveyed from the server
                // in an update message about entities that the client will
                // need to render in some way
                typedef struct entity_state_s
                {
                    int     number;         // edict index

                    vec3_t  origin;
                    vec3_t  angles;
                    vec3_t  old_origin;     // for lerping
    -----------》    int     modelindex;
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



















                game\g_local.h

                struct edict_s
                {
                    entity_state_t  s;
                    struct gclient_s    *client;    // NULL if not a player
                                                    // the server expects the first part
                                                    // of gclient_s to be a player_state_t
                                                    // but the rest of it is opaque

                    qboolean    inuse;
                    int         linkcount;

                    // FIXME: move these fields to a server private sv_entity_t
                    link_t      area;               // linked to a division node or leaf
                    
                    int         num_clusters;       // if -1, use headnode instead
                    int         clusternums[MAX_ENT_CLUSTERS];
                    int         headnode;           // unused if num_clusters != -1
                    int         areanum, areanum2;

                    //================================

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

                    char        *model;
                    float       freetime;           // sv.time when the object was freed
                    
                    //
                    // only used locally in game, not by server
                    //
                    char        *message;
                    char        *classname;
                    int         spawnflags;

                    float       timestamp;

                    float       angle;          // set in qe3, -1 = up, -2 = down
                    char        *target;
                    char        *targetname;
                    char        *killtarget;
                    char        *team;
                    char        *pathtarget;
                    char        *deathtarget;
                    char        *combattarget;
                    edict_t     *target_ent;

                    float       speed, accel, decel;
                    vec3_t      movedir;
                    vec3_t      pos1, pos2;

                    vec3_t      velocity;
                    vec3_t      avelocity;
                    int         mass;
                    float       air_finished;
                    float       gravity;        // per entity gravity multiplier (1.0 is normal)
                                                // use for lowgrav artifact, flares

                    edict_t     *goalentity;
                    edict_t     *movetarget;
                    float       yaw_speed;
                    float       ideal_yaw;

                    float       nextthink;
                    void        (*prethink) (edict_t *ent);
                    void        (*think)(edict_t *self);
                    void        (*blocked)(edict_t *self, edict_t *other);  //move to moveinfo?
                    void        (*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
                    void        (*use)(edict_t *self, edict_t *other, edict_t *activator);
                    void        (*pain)(edict_t *self, edict_t *other, float kick, int damage);
                    void        (*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

                    float       touch_debounce_time;        // are all these legit?  do we need more/less of them?
                    float       pain_debounce_time;
                    float       damage_debounce_time;
                    float       fly_sound_debounce_time;    //move to clientinfo
                    float       last_move_time;

                    int         health;
                    int         max_health;
                    int         gib_health;
                    int         deadflag;
                    qboolean    show_hostile;

                    float       powerarmor_time;

                    char        *map;           // target_changelevel

                    int         viewheight;     // height above origin where eyesight is determined
                    int         takedamage;
                    int         dmg;
                    int         radius_dmg;
                    float       dmg_radius;
                    int         sounds;         //make this a spawntemp var?
                    int         count;

                    edict_t     *chain;
                    edict_t     *enemy;
                    edict_t     *oldenemy;
                    edict_t     *activator;
                    edict_t     *groundentity;
                    int         groundentity_linkcount;
                    edict_t     *teamchain;
                    edict_t     *teammaster;

                    edict_t     *mynoise;       // can go in client only
                    edict_t     *mynoise2;

                    int         noise_index;
                    int         noise_index2;
                    float       volume;
                    float       attenuation;

                    // timing variables
                    float       wait;
                    float       delay;          // before firing targets
                    float       random;

                    float       teleport_time;

                    int         watertype;
                    int         waterlevel;

                    vec3_t      move_origin;
                    vec3_t      move_angles;

                    // move this to clientinfo?
                    int         light_level;

                    int         style;          // also used as areaportal number

                    gitem_t     *item;          // for bonus items

                    // common data blocks
                    moveinfo_t      moveinfo;
                    monsterinfo_t   monsterinfo;
                };


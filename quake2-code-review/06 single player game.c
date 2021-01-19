


1.  so even for a single player game, we spawn a local server 

                quake2/server/sv_init.c

                void SV_SpawnServer (char *server, char *spawnpoint, server_state_t serverstate, qboolean attractloop, qboolean loadgame)
                {

                    ...
                    ...
                    // load and spawn all other entities
                    ge->SpawnEntities ( sv.name, CM_EntityString(), spawnpoint );


                }

                game/g_spawn.c 

                void SpawnEntities (char *mapname, char *entities, char *spawnpoint)
                {       
                    ...
                    ...

                    memset (g_edicts, 0, game.maxentities * sizeof (g_edicts[0]));


                    // set client fields on player ents
                    for (i=0 ; i<game.maxclients ; i++)
                        g_edicts[i+1].client = game.clients + i;


                    ED_CallSpawn (ent);
                }

2.  so I printed out the three 

mapname: "base1"

entities: {
    "nextmap" "base2"
    "sky" "unit1_"
    "message" "Outer Base"
    "classname" "worldspawn"
    "sounds" "9"
}
{
    "origin" "32 -224 24"
    "classname" "info_player_coop"
    "angle" "90"
}
    ...
    ...
{
    "origin" "-1824 1288 120"
    "target" "t20"
    "spawnflags" "769"
    "classname" "monster_infantry"
}
    ...
    ...

{
    "origin" "-158 1424 -128"
    "spawnflags" "2"
    "classname" "misc_deadsoldier"
}
{
    "origin" "-226 1424 -112"
    "classname" "item_health_small"
}
{
    "origin" "-440 0 48"
    "target" "t159"
    "targetname" "t2"
    "classname" "trigger_relay"
}
{
    "origin" "800 -240 32"
    "targetname" "t150"
    "delay" ".2"
    "dmg" "1"
    "classname" "target_explosion"
}

spawnpoint:

                void SpawnEntities (char *mapname, char *entities, char *spawnpoint)
                {

                    printf("mapname: %s\n", mapname);
                    printf("entities: %s\n", entities);
                    printf("spawnpoint: %s\n", spawnpoint);

  
                }



3.  so we spawn entities based on the incoming text files 

-   so we parse the entity based on the initalizating string 

-   then we call ED_CallSpawn();

                void SpawnEntities (char *mapname, char *entities, char *spawnpoint)
                {

                    printf("mapname: %s\n", mapname);
                    printf("entities: %s\n", entities);
                    printf("spawnpoint: %s\n", spawnpoint);


                    ...
                    ...

                    if (!ent)
                        ent = g_edicts;
                    else
                        ent = G_Spawn ();
                    
                    entities = ED_ParseEdict (entities, ent);
                    ...
                    ...


                    ED_CallSpawn (ent);
                }




4.  so different entities will have different spawn functions 
                
                game/g_spawn.c 

                // check normal spawn functions
                for (s=spawns ; s->name ; s++)
                {
                    if (!strcmp(s->name, ent->classname))
                    {   // found it
   ------------>        s->spawn (ent);
                        return;
                    }
                }




the first spawn function is the world 


recall the first entity in the entity string is 
                {
                    "nextmap" "base2"
                    "sky" "unit1_"
                    "message" "Outer Base"
                    "classname" "worldspawn"
                    "sounds" "9"
                }


so reasonably, its corresponding spawn function is 

                /*QUAKED worldspawn (0 0 0) ?

                Only used for the world.
                "sky"   environment map name
                "skyaxis"   vector axis for rotating sky
                "skyrotate" speed of rotation in degrees/second
                "sounds"    music cd track number
                "gravity"   800 is default gravity
                "message"   text to print at user logon
                */
                void SP_worldspawn (edict_t *ent)
                {

                    printf("SP_worldspawn");
                    ent->movetype = MOVETYPE_PUSH;
                    ent->solid = SOLID_BSP;
                    ent->inuse = true;          // since the world doesn't use G_Spawn()
                    ent->s.modelindex = 1;      // world model is always index 1


                }





5.  the 2nd tye are info player 

                game\p_client.c

                void SP_info_player_coop(edict_t *self)
                {
                    if (!coop->value)
                    {
                        G_FreeEdict (self);
                        return;
                    }

                    if((Q_stricmp(level.mapname, "jail2") == 0)   ||
                       (Q_stricmp(level.mapname, "jail4") == 0)   ||
                       (Q_stricmp(level.mapname, "mine1") == 0)   ||
                       (Q_stricmp(level.mapname, "mine2") == 0)   ||
                       (Q_stricmp(level.mapname, "mine3") == 0)   ||
                       (Q_stricmp(level.mapname, "mine4") == 0)   ||
                       (Q_stricmp(level.mapname, "lab") == 0)     ||
                       (Q_stricmp(level.mapname, "boss1") == 0)   ||
                       (Q_stricmp(level.mapname, "fact3") == 0)   ||
                       (Q_stricmp(level.mapname, "biggun") == 0)  ||
                       (Q_stricmp(level.mapname, "space") == 0)   ||
                       (Q_stricmp(level.mapname, "command") == 0) ||
                       (Q_stricmp(level.mapname, "power2") == 0) ||
                       (Q_stricmp(level.mapname, "strike") == 0))
                    {
                        // invoke one of our gross, ugly, disgusting hacks
                        self->think = SP_FixCoopSpots;
                        self->nextthink = level.time + FRAMETIME;
                    }
                }






6.  spawning "item_health_small"

                game\g_items.c

                void SP_item_health_small (edict_t *self)
                {
                    if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
                    {
                        G_FreeEdict (self);
                        return;
                    }

                    self->model = "models/items/healing/stimpack/tris.md2";
                    self->count = 2;
                    SpawnItem (self, FindItem ("Health"));
                    self->style = HEALTH_IGNORE_MAX;
                    gi.soundindex ("items/s_health.wav");
                }




7.  spawning "target_explosion"

                game\g_target.c

                void SP_target_explosion (edict_t *ent)
                {
                    ent->use = use_target_explosion;
                    ent->svflags = SVF_NOCLIENT;
                }




8.  spawning "misc_deadsoldier"

                game\g_misc.c

                void SP_misc_deadsoldier (edict_t *ent)
                {
                    if (deathmatch->value)
                    {   // auto-remove for deathmatch
                        G_FreeEdict (ent);
                        return;
                    }

                    ent->movetype = MOVETYPE_NONE;
                    ent->solid = SOLID_BBOX;
                    ent->s.modelindex=gi.modelindex ("models/deadbods/dude/tris.md2");

                    // Defaults to frame 0
                    if (ent->spawnflags & 2)
                        ent->s.frame = 1;
                    else if (ent->spawnflags & 4)
                        ent->s.frame = 2;
                    else if (ent->spawnflags & 8)
                        ent->s.frame = 3;
                    else if (ent->spawnflags & 16)
                        ent->s.frame = 4;
                    else if (ent->spawnflags & 32)
                        ent->s.frame = 5;
                    else
                        ent->s.frame = 0;

                    VectorSet (ent->mins, -16, -16, 0);
                    VectorSet (ent->maxs, 16, 16, 16);
                    ent->deadflag = DEAD_DEAD;
                    ent->takedamage = DAMAGE_YES;
                    ent->svflags |= SVF_MONSTER|SVF_DEADMONSTER;
                    ent->die = misc_deadsoldier_die;
                    ent->monsterinfo.aiflags |= AI_GOOD_GUY;

                    gi.linkentity (ent);
                }
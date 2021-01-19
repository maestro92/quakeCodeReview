
quake2 bsp tree
https://www.flipcode.com/archives/Quake_2_BSP_File_Format.shtml

Traditionally when discussing BSP trees a distinction is made between those BSP trees that store the faces in the 
leaves (leaf-based BSP trees) and those that store them in the internal nodes (node-based BSP trees). 
The variation of BSP tree Quake uses is actually both; the reason for this is the dual use of the BSP tree 
in both rendering and in collision detection. For the purpose of rendering, it is useful to have the faces stored 
in the leaves because of the way the PVS is used. If you are not interested in performing collision detection, 
the faces in the nodes can be completely ignored.


so quake2 bsp stores faces both in nodes and leafs


leaf-storing BSP trees (kind of like kd tree)
-   internal nodes contains diving plane and child sub trees 
-   leaf nodes stores gemoetry, which is all the planes


solid-leaf bsp trees 
-   first get all the supporting plane of every polygon is used as a diving plane


quake2 quake3 brush-storing tree
-   both nodes and leaf stores the faces of the geometry






so the idea is that when quake loads into map data it loads all the data into arrays.
So Quake has 19 arrays related to its bsp map 

                Index   Name                    Description
                    0   Entities                MAP entity text buffer  
                    1   Planes                  Plane array 
                    2   Vertices                Vertex array    
                    3   Visibility              Compressed PVS data and directory for all clusters  
                    4   Nodes                   Internal node array for the BSP tree    
                    5   Texture Information     Face texture application array  
                    6   Faces                   Face array  
                    7   Lightmaps               Lightmaps   
                    8   Leaves                  Internal leaf array of the BSP tree 
                    9   Leaf Face Table         Index lookup table for referencing the face array from a leaf   
                    10  Leaf Brush Table        ?   
                    11  Edges                   Edge array  
                    12  Face Edge Table         Index lookup table for referencing the edge array from a face   
                    13  Models                  ?   
                    14  Brushes                 ?   
                    15  Brush Sides             ?   
                    16  Pop                     ?   
                    17  Areas                   ?           
                    18  Area Portals            ?



in the quake2\qcommon\qfiles.h, you can see the defined macros 

                #define LUMP_ENTITIES       0
                #define LUMP_PLANES         1
                #define LUMP_VERTEXES       2
                #define LUMP_VISIBILITY     3
                #define LUMP_NODES          4
                #define LUMP_TEXINFO        5
                #define LUMP_FACES          6
                #define LUMP_LIGHTING       7
                #define LUMP_LEAFS          8
                #define LUMP_LEAFFACES      9
                #define LUMP_LEAFBRUSHES    10
                #define LUMP_EDGES          11
                #define LUMP_SURFEDGES      12
                #define LUMP_MODELS         13
                #define LUMP_BRUSHES        14
                #define LUMP_BRUSHSIDES     15
                #define LUMP_POP            16
                #define LUMP_AREAS          17
                #define LUMP_AREAPORTALS    18
                #define HEADER_LUMPS        19

as you can see, we have a master array of Edges, Faces, Vertices, Planes etc etc 





2.  lets look at the definitions 

As you know, a BSP tree is made up of many nodes, each with a dividing plane, splitting and splitting until it results in a leaf with no children.

lets start with all the basics 

                quake2-vs2015-master\qcommon\qfiles.h

                typedef struct
                {
                    float   point[3];
                } dvertex_t;


                // note that edge 0 is never used, because negative edge nums are used for
                // counterclockwise use of the edge in a face
                typedef struct
                {
                    unsigned short  v[2];       // vertex numbers
                } dedge_t;


                typedef struct
                {
                    float   normal[3];
                    float   dist;
                    int     type;       // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
                } dplane_t;



these are all pretty straight forward


3.  now lets look at the bsp node structure
As mentioned both the nodes and leaf nodes stores the geometry, hence we have the 
these two fields are used for collision detection 

                uint16   first_face;        // index of the first face (in the face array)
                uint16   num_faces;         // number of consecutive edges (in the face array)

-   Node Lump

                typedef struct
                {
                    int         planenum;
                    int         children[2];    // negative numbers are -(leafs+1), not nodes
                    short       mins[3];        // for frustom culling
                    short       maxs[3];
                    unsigned short  firstface;
                    unsigned short  numfaces;   // counting both sides
                } dnode_t;



so the idea is that each node will store a few faces


https://developer.valvesoftware.com/wiki/Source_BSP_File_Format
The mins and maxs is the bounding box of firstface and numfaces

 The firstface and numfaces are indices into the face array that show which map 
 faces are contained in this node, or zero if none are. 




4.  now we look at the leaf nodes. Which is very similar


-   Leaf Lump

                typedef struct
                {
                    int             contents;           // OR of all brushes (not needed?)

                    short           cluster;
                    short           area;

                    short           mins[3];            // for frustum culling
                    short           maxs[3];

                    unsigned short  firstleafface;
                    unsigned short  numleaffaces;

                    unsigned short  firstleafbrush;
                    unsigned short  numleafbrushes;
                } dleaf_t;


contents, area, firstleafbrush and numleafbrushes are not really used, so we can ignore them.
so to summarize, we have  


                typedef struct
                {
                    ...

                    short           cluster;
                    ...

                    short           mins[3];            // for frustum culling
                    short           maxs[3];

                    unsigned short  firstleafface;
                    unsigned short  numleaffaces;

                    ...
                    ...
                } dleaf_t;






https://www.xuebuyuan.com/2208841.html
http://web.archive.org/web/20111112060250/http://www.devmaster.net/articles/quake3collision/

Quake 3 BSP Collision Detection


In a Quake 3 BSP, each leaf in the world has a set of brushes that are associated with it. 
Each brush, in turn, has a set of brush planes which are associated with it. 
When clipped together, these brush planes form a convex volume which represents the brush that you are trying to perform collision detection against.

So, the way that id Software approached an interface to this collision detection, 
and the way I, too, decided to design it, was to provide a single trace function 
for the game to call which will determine if there is a collision with the world (among other uses). 
    This trace function takes several arguments. First, it takes the starting point for a line segment. 
Second, it takes the point that you are trying to end up at. It also takes a bounding box around this point. 
Just to clarify, this box represents the bounding area for the object that you are trying to move through the world. It is not the bounding box for the line segment.




The trace function moves along the line between this starting and ending point, and checks each leaf it encounters. 
If it collides with something, it stores a value representing how far along this line it got before it ran into something, and then progresses on. Each time it runs into something, 
if the collision point is closer than the last collision point, it stores the new one instead. This will give you the closest point of collision along the line as an end result.







4B. Checking the Brushes
Each brush in the BSP has a number of brush sides that are associated with it. 
Each of these brush sides contains an index into the plane array for the plane that represents it. 
In order for us to check for collisions against a brush, we have to loop through this list of brushes, and check each individually.








##################################################################
########################### Quake Brush ##########################
##################################################################

https://quakewiki.org/wiki/Quake_Map_Format

So i downloaded TrenchBroom, that can make quake2 maps. It ouputs .map files
https://kristianduske.com/trenchbroom/

.map file 

                // Game: Quake 2
                // Format: Quake2
                // entity 0
                {
                "classname" "worldspawn"
                // brush 0
                {
                    ( 0 32 32 ) ( 0 0 32 ) ( 0 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 0 32 ) ( 32 0 32 ) ( 32 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 0 0 ) ( 32 32 0 ) ( 0 32 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 32 32 ) ( 32 32 32 ) ( 32 0 32 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 32 0 ) ( 32 32 32 ) ( 0 32 32 ) __TB_empty 0 -0.99041 0 1 1 0 0 0
                    ( 32 0 32 ) ( 32 32 32 ) ( 32 32 0 ) __TB_empty 0.0262604 -0.712383 0 1 1 0 0 0
                }
                }


this is a 6 sided cuboid. The plane of its first face is 3 points: ( 0 32 32 ) ( 0 0 32 ) ( 0 0 0 )
if you think about it, 3 points define a plane. so there you have it. 
we have 6 planes here, which is our 6 sided cublid


the plane of the first face is defined by 3 points

__TB_empty is the material used
0 0 0 1 1 0 0 0 is stuff like X offset, Y offset, Rotation, X scale y Scale and stuff


now lets see a similar file, but here we have a slope.


                // Game: Quake 2
                // Format: Quake2
                // entity 0
                {
                "classname" "worldspawn"
                // brush 0
                {
                    ( 0 0 0 ) ( 0 1 0 ) ( 0 0 1 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 0 0 ) ( 0 0 1 ) ( 1 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 128 128 32 ) ( 128 129 32 ) ( 129 128 32 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 96 32 ) ( 0 128 0 ) ( 128 128 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 128 128 32 ) ( 128 128 33 ) ( 128 129 32 ) __TB_empty 0 0 0 1 1 0 0 0
                }
                }




as you can see, brushes is just a collection of planes that encompasses the volume 

Brushes are one of the two primary components of a MAP file. Each brush defines a solid region. 
Brushes define this region as the intersection of four or more planes. 
Each plane is defined by three noncolinear vertices. These vertices must go in a clockwise oriented sequence:




here is another example that only has 5 faces 

                // Game: Quake 2
                // Format: Quake2
                // entity 0
                {
                "classname" "worldspawn"
                // brush 0
                {
                    ( 0 0 32 ) ( 0 0 0 ) ( 0 32 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 0 32 ) ( 32 0 32 ) ( 32 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 0 0 ) ( 32 32 0 ) ( 0 32 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 32 0 ) ( 32 32 0 ) ( 32 0 32 ) __TB_empty -0.6 0 0 1 1 0 0 0
                    ( 32 0 32 ) ( 32 32 0 ) ( 32 0 0 ) __TB_empty 0.935577 -0.187675 0 1 1 0 0 0
                }
                }




if you have a giant map, and you have a entirely flat ground
When I looked at other people_s quake2 maps, that ground is actually divded into multiple smaller cuboids

for example, here we have two cuboids next to each other, forming a larger ground 

                // Game: Quake 2
                // Format: Quake2
                // entity 0
                {
                "classname" "worldspawn"
                // brush 0
                {
                    ( 0 32 32 ) ( 0 0 32 ) ( 0 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 0 32 ) ( 32 0 32 ) ( 32 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 0 0 ) ( 32 32 0 ) ( 0 32 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 0 32 32 ) ( 32 32 32 ) ( 32 0 32 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 32 0 ) ( 32 32 32 ) ( 0 32 32 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 0 32 ) ( 32 32 32 ) ( 32 32 0 ) __TB_empty 0 0 0 1 1 0 0 0
                }
                // brush 1
                {
                    ( 32 32 32 ) ( 32 0 32 ) ( 32 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 0 32 ) ( 64 0 32 ) ( 64 0 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 64 0 0 ) ( 64 32 0 ) ( 32 32 0 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 32 32 32 ) ( 64 32 32 ) ( 64 0 32 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 64 32 0 ) ( 64 32 32 ) ( 32 32 32 ) __TB_empty 0 0 0 1 1 0 0 0
                    ( 64 0 32 ) ( 64 32 32 ) ( 64 32 0 ) __TB_empty 0 0 0 1 1 0 0 0
                }
                }








##################################################################
########################### Actual Code ##########################
##################################################################

5.  now lets look at the actual code:

                
1.  So when we load a game, we go through a bunch of calls             

                void SV_GameMap_f (void)
                {
                    ...
                    
                    // start up the next map
                    SV_Map (false, Cmd_Argv(1), false );

                    ...
                    ...
                }

2.


                void SV_Map (qboolean attractloop, char *levelstring, qboolean loadgame)
                {
                    ...
                    SV_SpawnServer (level, spawnpoint, ss_game, attractloop, loadgame);
                    ...
                }



3.  
                void SV_SpawnServer (char *server, char *spawnpoint, server_state_t serverstate, qboolean attractloop, qboolean loadgame)
                {

                    ...
                    ...

                    sv.models[1] = CM_LoadMap (sv.configstrings[CS_MODELS+1], false, &checksum);
                }

3.  then inside the LoadMap(); function, we first read our map file
    so if you play the game, the first map that gets loaded is: "maps/base1.bsp"

    so the argument "char *name" that gets passed in is "maps/base1.bsp"

    if you look inside the quake2 asset folder "baseq2", there is this "maps.list" file

    if you open it up you can see 

                q2dm1 "The Edge"
                q2dm2 "Tokay's Towers"
                q2dm3 "The Frag Pipe"
                q2dm4 "Lost Hallways"
                q2dm5 "The Pits"
                q2dm6 "Lava Tomb"
                q2dm7 "The Slimy Place"
                q2dm8 "WareHouse"
    ------->    base1 "Outer Base"
                base2 "Installation"
                base3 "Comm Center"
                ...
                ...

-   so below we call the CM_LoadMap(); function


                quake2-vs2015-master\qcommon\cmodel.c

                cmodel_t *CM_LoadMap (char *name, qboolean clientload, unsigned *checksum)
                {
                    unsigned        *buf;
                    int             i;
                    dheader_t       header;
                    int             length;
                    static unsigned last_checksum;

                    map_noareas = Cvar_Get ("map_noareas", "0", 0);

                    ...
                    ...

                    // free old stuff
                    numplanes = 0;
                    numnodes = 0;
                    numleafs = 0;
                    numcmodels = 0;
                    numvisibility = 0;
                    numentitychars = 0;
                    map_entitystring[0] = 0;
                    map_name[0] = 0;

                    ...
                    ...

                    //
                    // load the file
                    //
                    length = FS_LoadFile (name, (void **)&buf);
                    if (!buf)
                        Com_Error (ERR_DROP, "Couldn't load %s", name);

                    last_checksum = LittleLong (Com_BlockChecksum (buf, length));
                    *checksum = last_checksum;

                    header = *(dheader_t *)buf;
                    for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
                        ((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);

                    if (header.version != BSPVERSION)
                        Com_Error (ERR_DROP, "CMod_LoadBrushModel: %s has wrong version number (%i should be %i)"
                        , name, header.version, BSPVERSION);

                    cmod_base = (byte *)buf;

                    // load into heap
                    CMod_LoadSurfaces (&header.lumps[LUMP_TEXINFO]);
                    CMod_LoadLeafs (&header.lumps[LUMP_LEAFS]);
                    CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES]);
                    CMod_LoadPlanes (&header.lumps[LUMP_PLANES]);
                    CMod_LoadBrushes (&header.lumps[LUMP_BRUSHES]);
                    CMod_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES]);
                    CMod_LoadSubmodels (&header.lumps[LUMP_MODELS]);
                    CMod_LoadNodes (&header.lumps[LUMP_NODES]);
                    CMod_LoadAreas (&header.lumps[LUMP_AREAS]);
                    CMod_LoadAreaPortals (&header.lumps[LUMP_AREAPORTALS]);
                    CMod_LoadVisibility (&header.lumps[LUMP_VISIBILITY]);
                    CMod_LoadEntityString (&header.lumps[LUMP_ENTITIES]);

                    FS_FreeFile (buf);

                    CM_InitBoxHull ();

                    memset (portalopen, 0, sizeof(portalopen));
                    FloodAreaConnections ();

                    strcpy (map_name, name);

                    return &map_cmodels[0];
                }



4.  the main chunk is that we are loading a bunch of arrays 
                // load into heap
                CMod_LoadSurfaces (&header.lumps[LUMP_TEXINFO]);
                CMod_LoadLeafs (&header.lumps[LUMP_LEAFS]);
                CMod_LoadLeafBrushes (&header.lumps[LUMP_LEAFBRUSHES]);
                CMod_LoadPlanes (&header.lumps[LUMP_PLANES]);
                CMod_LoadBrushes (&header.lumps[LUMP_BRUSHES]);
                CMod_LoadBrushSides (&header.lumps[LUMP_BRUSHSIDES]);
                CMod_LoadSubmodels (&header.lumps[LUMP_MODELS]);
                CMod_LoadNodes (&header.lumps[LUMP_NODES]);
                CMod_LoadAreas (&header.lumps[LUMP_AREAS]);
                CMod_LoadAreaPortals (&header.lumps[LUMP_AREAPORTALS]);
                CMod_LoadVisibility (&header.lumps[LUMP_VISIBILITY]);
                CMod_LoadEntityString (&header.lumps[LUMP_ENTITIES]);

we are loading all of these into our own storage, which is 

                quake2\qcommon\cmodel.c
    
                char        map_name[MAX_QPATH];

                int         numbrushsides;
                cbrushside_t map_brushsides[MAX_MAP_BRUSHSIDES];

                int         numtexinfo;
                mapsurface_t    map_surfaces[MAX_MAP_TEXINFO];

                int         numplanes;
                cplane_t    map_planes[MAX_MAP_PLANES+6];       // extra for box hull

                int         numnodes;
                cnode_t     map_nodes[MAX_MAP_NODES+6];     // extra for box hull

                int         numleafs = 1;   // allow leaf funcs to be called without a map
                cleaf_t     map_leafs[MAX_MAP_LEAFS];
                int         emptyleaf, solidleaf;

                int         numleafbrushes;
                unsigned short  map_leafbrushes[MAX_MAP_LEAFBRUSHES];

                int         numcmodels;
                cmodel_t    map_cmodels[MAX_MAP_MODELS];

                int         numbrushes;
                cbrush_t    map_brushes[MAX_MAP_BRUSHES];

                int         numvisibility;
                byte        map_visibility[MAX_MAP_VISIBILITY];
                dvis_t      *map_vis = (dvis_t *)map_visibility;

                int         numentitychars;
                char        map_entitystring[MAX_MAP_ENTSTRING];

                int         numareas = 1;
                carea_t     map_areas[MAX_MAP_AREAS];

                int         numareaportals;
                dareaportal_t map_areaportals[MAX_MAP_AREAPORTALS];

                int         numclusters = 1;

                mapsurface_t    nullsurface;

                int         floodvalid;

                qboolean    portalopen[MAX_MAP_AREAPORTALS];


                cvar_t      *map_noareas;



one thing to notice that we actually allocate 6 more planes and nodes 

                int         numplanes;
                cplane_t    map_planes[MAX_MAP_PLANES+6];       // extra for box hull

                int         numnodes;
                cnode_t     map_nodes[MAX_MAP_NODES+6];     // extra for box hull

this is because we want to create space for the hull for the world







Note that for the cmodel.c, we have new definitions for the nodes and leafs



                typedef struct
                {
                    cplane_t    *plane;
                    int         children[2];        // negative numbers are leafs
                } cnode_t;


                typedef struct
                {
                    int         contents;
                    int         cluster;
                    int         area;
                    unsigned short  firstleafbrush;
                    unsigned short  numleafbrushes;
                } cleaf_t;


                typedef struct
                {
                    int         contents;
                    int         numsides;
                    int         firstbrushside;
                    int         checkcount;     // to avoid repeated testings
                } cbrush_t;


                // plane_t structure
                // !!! if this is changed, it must be changed in asm code too !!!
                typedef struct cplane_s
                {
                    vec3_t  normal;
                    float   dist;
                    byte    type;           // for fast side tests
                    byte    signbits;       // signx + (signy<<1) + (signz<<1)
                    byte    pad[2];
                } cplane_t;









4.  for example lets look at the CMod_LoadPlanes(); function 
as you can see, we are outputing into "map_planes" and "numplanes"

                void CMod_LoadPlanes (lump_t *l)
                {
                    ...
                    ...

                    out = map_planes;   
                    numplanes = count;

                    for ( i=0 ; i<count ; i++, in++, out++)
                    {
                        bits = 0;
                        for (j=0 ; j<3 ; j++)
                        {
                            out->normal[j] = LittleFloat (in->normal[j]);
                            if (out->normal[j] < 0)
                                bits |= 1<<j;
                        }

                        out->dist = LittleFloat (in->dist);
                        out->type = LittleLong (in->type);
                        out->signbits = bits;
                    }
                }



    
##############################################################
##################### Clipping Hulls #########################
##############################################################


5.  after loading all these values, we call "CM_InitBoxHull"

so the idea about a clipping hull is explained in a valve link 
https://developer.valvesoftware.com/wiki/BSP

there is a section called "What are clipping hulls"
hull means: "the main body of a ship or other vessel, including the bottom, sides, and deck but not the masts, 
superstructure, rigging, engines, and other fittings. "


there is also link describing how Hulls worked in quake 1
http://www.gamers.org/dEngine/quake/spec/quake-spec31.html


The first hull is a bounding box that defines the extents of the whole word. 




-   for the first map, we have 

seems like we are artificually adding an exra head node 
in which we do 


                box_headnode = numnodes;
                box_planes = &map_planes[numplanes];
                ...
                ...

                box_brush = &map_brushes[numbrushes];
                ...
                ...

                box_leaf = &map_leafs[numleafs];
                ...
                ...

                map_leafbrushes[numleafbrushes] = numbrushes;


-   full code below:

                quake2\qcommon\cmodel.c

                cplane_t    *box_planes;
                int         box_headnode;
                cbrush_t    *box_brush;
                cleaf_t     *box_leaf;



                /*
                ===================
                CM_InitBoxHull

                Set up the planes and nodes so that the six floats of a bounding box
                can just be stored out and get a proper clipping hull structure.
                ===================
                */
                void CM_InitBoxHull (void)
                {
                    int         i;
                    int         side;
                    cnode_t     *c;
                    cplane_t    *p;
                    cbrushside_t    *s;

                    box_headnode = numnodes;
                    box_planes = &map_planes[numplanes];
                    if (numnodes+6 > MAX_MAP_NODES
                        || numbrushes+1 > MAX_MAP_BRUSHES
                        || numleafbrushes+1 > MAX_MAP_LEAFBRUSHES
                        || numbrushsides+6 > MAX_MAP_BRUSHSIDES
                        || numplanes+12 > MAX_MAP_PLANES)
                        Com_Error (ERR_DROP, "Not enough room for box tree");

                    box_brush = &map_brushes[numbrushes];
                    box_brush->numsides = 6;
                    box_brush->firstbrushside = numbrushsides;
                    box_brush->contents = CONTENTS_MONSTER;

                    box_leaf = &map_leafs[numleafs];
                    box_leaf->contents = CONTENTS_MONSTER;
                    box_leaf->firstleafbrush = numleafbrushes;
                    box_leaf->numleafbrushes = 1;

                    map_leafbrushes[numleafbrushes] = numbrushes;

                    for (i=0 ; i<6 ; i++)
                    {
                        side = i&1;

                        // brush sides
                        s = &map_brushsides[numbrushsides+i];
                        s->plane =  map_planes + (numplanes+i*2+side);
                        s->surface = &nullsurface;

                        // nodes
                        c = &map_nodes[box_headnode+i];
                        c->plane = map_planes + (numplanes+i*2);
                        c->children[side] = -1 - emptyleaf;
                        if (i != 5)
                            c->children[side^1] = box_headnode+i + 1;
                        else
                            c->children[side^1] = -1 - numleafs;

                        // planes
                        p = &box_planes[i*2];
                        p->type = i>>1;
                        p->signbits = 0;
                        VectorClear (p->normal);
                        p->normal[i>>1] = 1;

                        p = &box_planes[i*2+1];
                        p->type = 3 + (i>>1);
                        p->signbits = 0;
                        VectorClear (p->normal);
                        p->normal[i>>1] = -1;
                    }   
                }


    i = 0
        p = boxplane[0]
        p->type = PLANE_X;
        p.normal = {1, 0, 0}


        p = boxplane[1]
        p->type = PLANE_ANYX;
        p.normal = {-1, 0, 0}


    i = 1
        p = boxplane[2]
        p->type = PLANE_X;
        p.normal = {1, 0, 0}


        p = boxplane[3]
        p->type = PLANE_ANYX;
        p.normal = {-1, 0, 0}


    i = 2
        p = boxplane[4]
        p->type = PLANE_X;
        p.normal = {1, 0, 0}


        p = boxplane[5]
        p->type = PLANE_ANYX;
        p.normal = {-1, 0, 0}


    i = 3
        p = boxplane[6]
        p->type = PLANE_X;
        p.normal = {1, 0, 0}


        p = boxplane[7]
        p->type = PLANE_ANYX;
        p.normal = {-1, 0, 0}


    i = 4
        p = boxplane[8]
        p->type = PLANE_X;
        p.normal = {1, 0, 0}


        p = boxplane[9]
        p->type = PLANE_ANYX;
        p.normal = {-1, 0, 0}        

    i = 5
        p = boxplane[10]
        p->type = PLANE_X;
        p.normal = {1, 0, 0}


        p = boxplane[11]
        p->type = PLANE_ANYX;
        p.normal = {-1, 0, 0}   









10. q_shared.h 

                // a trace is returned when a box is swept through the world
                typedef struct
                {
                    qboolean    allsolid;   // if true, plane is not valid
                    qboolean    startsolid; // if true, the initial point was in a solid area
                    float       fraction;   // time completed, 1.0 = didn't hit anything
                    vec3_t      endpos;     // final position
                    cplane_t    plane;      // surface normal at impact
                    csurface_t  *surface;   // surface hit
                    int         contents;   // contents on other side of surface hit
                    struct edict_s  *ent;       // not set by CM_*() functions
                } trace_t;
















####################################################################
######################### quake3 ###################################
####################################################################

In quake 3, node only contain a splitter plane number 

                typedef struct {
                    float       normal[3];
                    float       dist;
                } q3_dplane_t;

                typedef struct {
                    int         planeNum;
                    int         children[2];    // negative numbers are -(leafs+1), not nodes
                    int         mins[3];        // for frustom culling
                    int         maxs[3];
                } q3_dnode_t;

                typedef struct {
                    int         cluster;            // -1 = opaque cluster (do I still store these?)
                    int         area;

                    int         mins[3];            // for frustum culling
                    int         maxs[3];

                    int         firstLeafSurface;
                    int         numLeafSurfaces;

                    int         firstLeafBrush;
                    int         numLeafBrushes;
                } q3_dleaf_t;



The leafs, like the faces, are a very important part of the BSP information. 
They store the visibility cluster, the area portal, the leaf bounding box, 
the index into the faces, the number of leaf faces, the index into the brushes for collision,



https://www.gamedev.net/forums/topic/175502-brushes-in-a-quake3-bsp-file/



When a map is compiled to BSP format, it isnt composed of brushes any more. 
The BSP compilation process collects all the polygons of every brush, 
converts them all to triangles, and creates a BSP tree from them. 
The Quake III BSP format, in the simplest terms, 
is nothing but a collection of planes (triangles) and BSP node data 
(along with other stuff like entity positions, etc.)

However, certain brushes are preserved through the BSP process: 
brushes tied to entities like movers. I dont know how you would access these 
(I have little experience with Quake IIIs BSP files), but there are myriad tutorials 
out on the net which deal with exactly these topics. Look for them with Google.
Another good place to start is www.GameTutorials.com_s Quake III BSP tutorials.

As regards collision detection: You have two points, right? The start and end positions? 
Well, I think (I dont completely know, but this is how I would do it) that you take the two points, 
and see which BSP node they are in. Then, all you have to do is test the triangles contained in that node, 
and you have your collision detection. If the start and end positions are in different nodes, 
you have to test the nodes they are in, plus the nodes which connect the nodes they are in. 
I think that one of the GameTutorials.com exercises does have some rudimentary BSP collision detection, 
but if it doesnt, there_s always the net...



http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_2.htm#CMFMF

http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm







That bounding box slightly exagerates the actual size of the node and all its childs. 
Each bounding box boundary seems to be rounded to the next multiple of 16, 
so that the bounding box is at least 32 units larger than it should be.

That means that the level coordinates must all remain roughly between -32700 and +32700.

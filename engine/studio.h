/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef STUDIO_H
#define STUDIO_H

#include "shader.h"
#include "color24.h"
#include "lightlimits.h"
#include "mathlib.h"
#include <stdint.h>

/*
==============================================================================

STUDIO MODELS

Studio models are position independent, so the cache manager can move them.
==============================================================================
*/

// header
#define STUDIO_VERSION	10
#define IDSTUDIOHEADER	(('T'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDST"
#define IDSEQGRPHEADER	(('Q'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDSQ"

// studio limits
#define MAXSTUDIOVERTS		16384	// max vertices per submodel
#define MAXSTUDIOSKINS		256	// total textures
#define MAXSTUDIOBONES		128	// total bones actually used
#define MAXSTUDIOMODELS		32	// sub-models per model
#define MAXSTUDIOBODYPARTS		32	// body parts per submodel
#define MAXSTUDIOGROUPS		16	// sequence groups (e.g. barney01.mdl, barney02.mdl, e.t.c)
#define MAXSTUDIOCONTROLLERS		32	// max controllers per model
#define MAXSTUDIOATTACHMENTS		64	// max attachments per model
#define MAXSTUDIOBONEWEIGHTS		4	// absolute hardware limit!
#define MAXSTUDIONAME		32	// a part of specs
#define MAXSTUDIOPOSEPARAM		24
#define MAXEVENTSTRING		64
#define MAX_STUDIO_LIGHTMAP_SIZE	256	// must match with engine const!!!

// client-side model flags
#define STUDIO_ROCKET		(1<<0)	// leave a trail
#define STUDIO_GRENADE		(1<<1)	// leave a trail
#define STUDIO_GIB			(1<<2)	// leave a trail
#define STUDIO_ROTATE		(1<<3)	// rotate (bonus items)
#define STUDIO_TRACER		(1<<4)	// green split trail
#define STUDIO_ZOMGIB		(1<<5)	// small blood trail
#define STUDIO_TRACER2		(1<<6)	// orange split trail + rotate
#define STUDIO_TRACER3		(1<<7)	// purple trail
#define STUDIO_AMBIENT_LIGHT		(1<<8)	// force to use ambient shading 
#define STUDIO_TRACE_HITBOX		(1<<9)	// always use hitbox trace instead of bbox
#define STUDIO_FORCE_SKYLIGHT		(1<<10)	// always grab lightvalues from the sky settings (even if sky is invisible)

#define STUDIO_HAS_BUMP		(1<<16)	// loadtime set
#define STUDIO_STATIC_PROP		(1<<29)	// hint for engine
#define STUDIO_HAS_BONEINFO		(1<<30)	// extra info about bones (pose matrix, procedural index etc)
#define STUDIO_HAS_BONEWEIGHTS	(1<<31)	// yes we got support of bone weighting

// lighting & rendermode options
#define STUDIO_NF_FLATSHADE		0x0001
#define STUDIO_NF_CHROME		0x0002
#define STUDIO_NF_FULLBRIGHT		0x0004
#define STUDIO_NF_NOMIPS		0x0008	// ignore mip-maps
#define STUDIO_NF_SMOOTH		0x0010	// smooth tangent space
#define STUDIO_NF_ADDITIVE		0x0020	// rendering with additive mode
#define STUDIO_NF_MASKED		0x0040	// use texture with alpha channel
#define STUDIO_NF_NORMALMAP		0x0080	// indexed normalmap
#define STUDIO_NF_GLOSSMAP		0x0100	// glossmap
#define STUDIO_NF_GLOSSPOWER		0x0200
#define STUDIO_NF_LUMA		0x0400	// self-illuminate parts
#define STUDIO_NF_ALPHASOLID		0x0800	// use with STUDIO_NF_MASKED to have solid alphatest surfaces for env_static
#define STUDIO_NF_TWOSIDE		0x1000	// render mesh as twosided
#define STUDIO_NF_HEIGHTMAP		0x2000

#define STUDIO_NF_NODRAW		(1<<16)	// failed to create shader for this mesh
#define STUDIO_NF_NODLIGHT		(1<<17)	// failed to create dlight shader for this mesh
#define STUDIO_NF_NOSUNLIGHT		(1<<18)	// failed to create sun light shader for this mesh

#define STUDIO_NF_HAS_ALPHA		(1<<20)	// external texture has alpha-channel
#define STUDIO_NF_HAS_DETAIL		(1<<21)	// studiomodels has detail textures

#define STUDIO_NF_COLORMAP		(1<<30)	// can changed by colormap command
#define STUDIO_NF_UV_COORDS		(1<<31)	// using half-float coords instead of ST

// motion flags
#define STUDIO_X			0x0001
#define STUDIO_Y			0x0002	
#define STUDIO_Z			0x0004
#define STUDIO_XR			0x0008
#define STUDIO_YR			0x0010
#define STUDIO_ZR			0x0020

#define STUDIO_LX			0x0040
#define STUDIO_LY			0x0080
#define STUDIO_LZ			0x0100
#define STUDIO_LXR			0x0200
#define STUDIO_LYR			0x0400
#define STUDIO_LZR			0x0800
#define STUDIO_LINEAR		0x1000
#define STUDIO_QUADRATIC_MOTION	0x2000
#define STUDIO_RESERVED		0x4000	// g-cont. reserved one bit for me
#define STUDIO_TYPES		0x7FFF
#define STUDIO_RLOOP		0x8000	// controller that wraps shortest distance

// bonecontroller types
#define STUDIO_MOUTH		4

// sequence flags
#define STUDIO_LOOPING		0x0001	// ending frame should be the same as the starting frame
#define STUDIO_SNAP			0x0002	// do not interpolate between previous animation and this one
#define STUDIO_DELTA		0x0004	// this sequence "adds" to the base sequences, not slerp blends
#define STUDIO_AUTOPLAY		0x0008	// temporary flag that forces the sequence to always play
#define STUDIO_POST			0x0010	// 
#define STUDIO_ALLZEROS		0x0020	// this animation/sequence has no real animation data
#define STUDIO_BLENDPOSE		0x0040   	// to differentiate GoldSrc style blending from Source style blending (with pose parameters)
#define STUDIO_CYCLEPOSE		0x0080	// cycle index is taken from a pose parameter index
#define STUDIO_REALTIME		0x0100	// cycle index is taken from a real-time clock, not the animations cycle index
#define STUDIO_LOCAL		0x0200	// sequence has a local context sequence
#define STUDIO_HIDDEN		0x0400	// don't show in default selection views
#define STUDIO_IKRULES		0x0800	// sequence has IK-rules
#define STUDIO_ACTIVITY		0x1000	// Has been updated at runtime to activity index
#define STUDIO_EVENT		0x2000	// Has been updated at runtime to event index
#define STUDIO_WORLD		0x4000	// sequence blends in worldspace
#define STUDIO_LIGHT_FROM_ROOT	0x8000	// get lighting point from root bonepos not from entity origin

// autolayer flags
#define STUDIO_AL_POST		0x0001	// 
#define STUDIO_AL_SPLINE		0x0002	// convert layer ramp in/out curve is a spline instead of linear
#define STUDIO_AL_XFADE		0x0004	// pre-bias the ramp curve to compense for a non-1 weight,
					// assuming a second layer is also going to accumulate
#define STUDIO_AL_NOBLEND		0x0008	// animation always blends at 1.0 (ignores weight)
#define STUDIO_AL_LOCAL		0x0010	// layer is a local context sequence
#define STUDIO_AL_POSE		0x0020	// layer blends using a pose parameter instead of parent cycle

typedef struct
{
	int		ident;
	int		version;

	char		name[64];
	int		length;

	vec3_t		eyeposition;	// ideal eye position
	vec3_t		min;		// ideal movement hull size
	vec3_t		max;			

	vec3_t		bbmin;		// clipping bounding box
	vec3_t		bbmax;		

	int		flags;

	int		numbones;		// bones
	int		boneindex;

	int		numbonecontrollers;	// bone controllers
	int		bonecontrollerindex;

	int		numhitboxes;	// complex bounding boxes
	int		hitboxindex;			
	
	int		numseq;		// animation sequences
	int		seqindex;

	int		numseqgroups;	// demand loaded sequences
	int		seqgroupindex;

	int		numtextures;	// raw textures
	int		textureindex;
	int		texturedataindex;

	int		numskinref;	// replaceable textures
	int		numskinfamilies;
	int		skinindex;

	int		numbodyparts;		
	int		bodypartindex;

	int		numattachments;	// queryable attachable points
	int		attachmentindex;

	int		studiohdr2index;
	int		soundindex;	// UNUSED

	int		soundgroups;	// UNUSED
	int		soundgroupindex;	// UNUSED

	int		numtransitions;	// animation node to animation node transition graph
	int		transitionindex;
} studiohdr_t;

// extra header to hold more offsets
typedef struct
{
	int		numposeparameters;
	int		poseparamindex;

	int		numikautoplaylocks;
	int		ikautoplaylockindex;

	int		numikchains;
	int		ikchainindex;

	int		keyvalueindex;
	int		keyvaluesize;

	int		numhitboxsets;
	int		hitboxsetindex;

	int		unused[6];	// for future expansions
} studiohdr2_t;

// header for demand loaded sequence group data
typedef struct 
{
	int		id;
	int		version;

	char		name[64];
	int		length;
} studioseqhdr_t;

// bone flags
#define BONE_ALWAYS_PROCEDURAL	0x0001		// bone is always procedurally animated
#define BONE_SCREEN_ALIGN_SPHERE	0x0002	// bone aligns to the screen, not constrained in motion.
#define BONE_SCREEN_ALIGN_CYLINDER	0x0004	// bone aligns to the screen, constrained by it's own axis.
#define BONE_JIGGLE_PROCEDURAL	0x0008
#define BONE_FIXED_ALIGNMENT		0x0010	// bone can't spin 360 degrees, all interpolation is normalized around a fixed orientation

#define BONE_USED_BY_HITBOX		0x00000100	// bone (or child) is used by a hit box
#define BONE_USED_BY_ATTACHMENT	0x00000200	// bone (or child) is used by an attachment point
#define BONE_USED_BY_VERTEX		0x00000400	// bone (or child) is used by the toplevel model via skinned vertex
#define BONE_USED_BY_BONE_MERGE	0x00000800
#define BONE_USED_RESERVED		0x00001000	// bone reserved to keep it (because of $keepfreebones flag)
#define BONE_USED_MASK		(BONE_USED_BY_HITBOX|BONE_USED_BY_ATTACHMENT|BONE_USED_BY_VERTEX|BONE_USED_BY_BONE_MERGE|BONE_USED_RESERVED)
#define BONE_USED_BY_ANYTHING		BONE_USED_MASK

// bones
typedef struct 
{
	char		name[MAXSTUDIONAME];// bone name for symbolic links
	int		parent;		// parent bone
	int		flags;		// bone flags
	int		bonecontroller[6];	// bone controller index, -1 == none
	float		value[6];		// default DoF values
	float		scale[6];		// scale for delta DoF values
} mstudiobone_t;

#define STUDIO_PROC_AXISINTERP	1
#define STUDIO_PROC_QUATINTERP	2
#define STUDIO_PROC_AIMATBONE		3
#define STUDIO_PROC_AIMATATTACH	4
#define STUDIO_PROC_JIGGLE		5

typedef struct
{
	int		control;		// local transformation of this bone used to calc 3 point blend
	int		axis;		// axis to check
	vec3_t		pos[6];		// X+, X-, Y+, Y-, Z+, Z-
	vec4_t		quat[6];		// X+, X-, Y+, Y-, Z+, Z-
} mstudioaxisinterpbone_t;

typedef struct
{
	float		inv_tolerance;	// 1.0f / radian angle of trigger influence
	vec4_t		trigger;		// angle to match
	vec3_t		pos;		// new position
	vec4_t		quat;		// new angle
} mstudioquatinterpinfo_t;

typedef struct
{
	int		control;		// local transformation to check
	int		numtriggers;
	int		triggerindex;
} mstudioquatinterpbone_t;

// extra info for bones
typedef struct
{
	float		poseToBone[3][4];	// boneweighting reqiures
	vec4_t		qAlignment;
	int		proctype;
	int		procindex;	// procedural rule
	vec4_t		quat;		// aligned bone rotation
	int		reserved[10];	// for future expansions
} mstudioboneinfo_t;

// JIGGLEBONES
#define JIGGLE_IS_FLEXIBLE		0x01
#define JIGGLE_IS_RIGID		0x02
#define JIGGLE_HAS_YAW_CONSTRAINT	0x04
#define JIGGLE_HAS_PITCH_CONSTRAINT	0x08
#define JIGGLE_HAS_ANGLE_CONSTRAINT	0x10
#define JIGGLE_HAS_LENGTH_CONSTRAINT	0x20
#define JIGGLE_HAS_BASE_SPRING	0x40
#define JIGGLE_IS_BOING		0x80	// simple squash and stretch sinusoid "boing"

typedef struct 
{
	int		flags;

	// general params
	float		length;		// how from from bone base, along bone, is tip
	float		tipMass;

	// flexible params
	float		yawStiffness;
	float		yawDamping;	
	float		pitchStiffness;
	float		pitchDamping;	
	float		alongStiffness;
	float		alongDamping;	

	// angle constraint
	float		angleLimit;	// maximum deflection of tip in radians
	
	// yaw constraint
	float		minYaw;		// in radians
	float		maxYaw;		// in radians
	float		yawFriction;
	float		yawBounce;
	
	// pitch constraint
	float		minPitch;		// in radians
	float		maxPitch;		// in radians
	float		pitchFriction;
	float		pitchBounce;

	// base spring
	float		baseMass;
	float		baseStiffness;
	float		baseDamping;	
	float		baseMinLeft;
	float		baseMaxLeft;
	float		baseLeftFriction;
	float		baseMinUp;
	float		baseMaxUp;
	float		baseUpFriction;
	float		baseMinForward;
	float		baseMaxForward;
	float		baseForwardFriction;

	// boing
	float		boingImpactSpeed;
	float		boingImpactAngle;
	float		boingDampingRate;
	float		boingFrequency;
	float		boingAmplitude;
} mstudiojigglebone_t;

typedef struct
{
	int		parent;
	int		aim;		// might be bone or attach
	vec3_t		aimvector;
	vec3_t		upvector;
	vec3_t		basepos;
} mstudioaimatbone_t;

// bone controllers
typedef struct 
{
	int		bone;		// -1 == 0
	int		type;		// X, Y, Z, XR, YR, ZR, M
	float		start;
	float		end;
	int		rest;		// byte index value at rest
	int		index;		// 0-3 user set controller, 4 mouth
} mstudiobonecontroller_t;

// intersection boxes
typedef struct
{
	int		bone;
	int		group;		// intersection group
	vec3_t		bbmin;		// bounding box
	vec3_t		bbmax;		
} mstudiobbox_t;

typedef struct 
{
	char		name[MAXSTUDIONAME];
	int		numhitboxes;
	int		hitboxindex;
} mstudiohitboxset_t;

typedef struct studio_cache_user_s
{
	int32_t		data;		// extradata
} studio_cache_user_t;

// demand loaded sequence groups
typedef struct
{
	char		label[MAXSTUDIONAME];	// textual name
	char		name[64];		// file name
	studio_cache_user_t	cache;		// cache index pointer
	int		data;		// hack for group 0
} mstudioseqgroup_t;

// events
typedef struct mstudioevent_s
{
	int		frame;
	int		event;
	int		type;
	char 		options[MAXEVENTSTRING];
} mstudioevent_t;

#define STUDIO_ATTACHMENT_LOCAL	(1<<0)	// vectors are filled

// attachment
typedef struct 
{
	char		name[MAXSTUDIONAME];
	int		flags;
	int		bone;
	vec3_t		org;		// attachment position
	vec3_t		vectors[3];	// attachment vectors
} mstudioattachment_t;

#define IK_SELF		1
#define IK_WORLD		2
#define IK_GROUND		3
#define IK_RELEASE		4
#define IK_ATTACHMENT	5
#define IK_UNLATCH		6

typedef struct
{
	float		scale[6];
	unsigned short	offset[6];
} mstudioikerror_t;

typedef struct
{
	int		index;

	int		type;
	int		chain;

	int		bone;
	int		attachment;	// attachment index

	int		slot;		// iktarget slot.  Usually same as chain.
	float		height;
	float		radius;
	float		floor;
	vec3_t		pos;
	vec4_t		quat;

	int		ikerrorindex;	// compressed IK error

	int		iStart;
	float		start;		// beginning of influence
	float		peak;		// start of full influence
	float		tail;		// end of full influence
	float		end;		// end of all influence
	float		contact;		// frame footstep makes ground concact
	float		drop;		// how far down the foot should drop when reaching for IK
	float		top;		// top of the foot box

	int		unused[4];	// for future expansions
} mstudioikrule_t;

typedef struct
{
	int		chain;
	float		flPosWeight;
	float		flLocalQWeight;
	int		flags;

	int		unused[4];	// for future expansions
} mstudioiklock_t;

typedef struct 
{
	int		endframe;				
	int		motionflags;
	float		v0;		// velocity at start of block
	float		v1;		// velocity at end of block
	float		angle;		// YAW rotation at end of this blocks movement
	vec3_t		vector;		// movement vector relative to this blocks initial angle
	vec3_t		position;		// relative to start of animation???
} mstudiomovement_t;

// additional info for each animation in sequence blend group or single sequence
typedef struct
{
	char		label[MAXSTUDIONAME];	// animation label (may be matched with sequence label)
	float		fps;		// frames per second (match with sequence fps or be different)	
	int		flags;		// looping/non-looping flags
	int		numframes;	// frames per animation

	// piecewise movement
	int		nummovements;	// piecewise movement
	int		movementindex;

	int		numikrules;
	int		ikruleindex;	// non-zero when IK data is stored in the mdl

	int		unused[8];	// for future expansions
} mstudioanimdesc_t;

// autoplaying sequences
typedef struct
{
	short		iSequence;
	short		iPose;
	int		flags;
	float		start;		// beginning of influence
	float		peak;		// start of full influence
	float		tail;		// end of full influence
	float		end;		// end of all influence
} mstudioautolayer_t;

// sequence descriptions
typedef struct
{
	char		label[MAXSTUDIONAME];	// sequence label

	float		fps;		// frames per second	
	int		flags;		// looping/non-looping flags

	int		activity;
	int		actweight;

	int		numevents;
	int		eventindex;

	int		numframes;	// number of frames per sequence

	int		weightlistindex;	// weightlists
	int		iklockindex;	// IK locks

	int		motiontype;	
	int		posekeyindex;	// index of pose parameter
	vec3_t		linearmovement;
	int		autolayerindex;	// autolayer descriptions
	int		keyvalueindex;	// local key-values

	vec3_t		bbmin;		// per sequence bounding box
	vec3_t		bbmax;		

	int		numblends;
	int		animindex;	// mstudioanim_t pointer relative to start of sequence group data
					// [blend][bone][X, Y, Z, XR, YR, ZR]

	int		blendtype[2];	// X, Y, Z, XR, YR, ZR (same as paramindex)
	float		blendstart[2];	// starting value  (same as paramstart)
	float		blendend[2];	// ending value  (same as paramend)
	byte		groupsize[2];	// 255 x 255 blends should be enough
	byte		numautolayers;	// count of autoplaying layers
	byte		numiklocks;	// IK-locks per sequence

	int		seqgroup;		// sequence group for demand loading

	int		entrynode;	// transition node at entry
	int		exitnode;		// transition node at exit
	byte		nodeflags;	// transition rules (really this is bool)
	byte		cycleposeindex;	// index of pose parameter to use as cycle index
	byte		fadeintime;	// ideal cross fade in time (0.2 secs default) time = (fadeintime / 100)
	byte		fadeouttime;	// ideal cross fade out time (0.2 msecs default)  time = (fadeouttime / 100)

	int		animdescindex;	// mstudioanimdesc_t [blend]
} mstudioseqdesc_t;

typedef struct 
{
	char		name[MAXSTUDIONAME];
	int		flags;		// ????
	float		start;		// starting value
	float		end;		// ending value
	float		loop;		// looping range, 0 for no looping, 360 for rotations, etc.
} mstudioposeparamdesc_t;

typedef struct
{
	unsigned short	offset[6];
} mstudioanim_t;

// animation frames
typedef union 
{
	struct
	{
		byte	valid;
		byte	total;
	} num;
	short		value;
} mstudioanimvalue_t;

// body part index
typedef struct
{
	char		name[64];
	int		nummodels;
	int		base;
	int		modelindex;	// index into models array
} mstudiobodyparts_t;

// skin info
typedef struct mstudiotex_s
{
	char		name[64];
	int		flags;
	int		width;
	int		height;
	int		index;
} mstudiotexture_t;

// ikinfo
typedef struct 
{
	int		bone;
	vec3_t		kneeDir;		// ideal bending direction (per link, if applicable)
	vec3_t		unused0;		// unused
} mstudioiklink_t;

typedef struct
{
	char		name[MAXSTUDIONAME];
	int		linktype;
	int		numlinks;
	int		linkindex;
} mstudioikchain_t;

typedef struct
{
	byte		weight[4];
	char		bone[4]; 
} mstudioboneweight_t;

// skin families
// short	index[skinfamilies][skinref]

// studio models
typedef struct
{
	char		name[64];

	int		type;		// UNUSED
	float		boundingradius;	// UNUSED

	int		nummesh;
	int		meshindex;

	int		numverts;		// number of unique vertices
	int		vertinfoindex;	// vertex bone info
	int		vertindex;	// vertex vec3_t
	int		numnorms;		// number of unique surface normals
	int		norminfoindex;	// normal bone info
	int		normindex;	// normal vec3_t

	int		blendvertinfoindex;	// boneweighted vertex info
	int		blendnorminfoindex;	// boneweighted normal info
} mstudiomodel_t;

// vec3_t	boundingbox[model][bone][2];		// complex intersection info

// meshes
typedef struct 
{
	int		numtris;
	int		triindex;
	int		skinref;
	int		numnorms;		// per mesh normals
	int		normindex;	// UNUSED!
} mstudiomesh_t;

typedef struct 
{
	short		vertindex;	// index into vertex array
	short		normindex;	// index into normal array
	short		s,t;		// s,t position on skin
} mstudiotrivert_t;

/*
===========================

USER-DEFINED DATA

===========================
*/
// this struct may be expaned by user request
typedef struct vbomesh_s
{
	unsigned int	skinref;			// skin reference
	unsigned int	numVerts;			// trifan vertices count
	unsigned int	numElems;			// trifan elements count
	int		lightmapnum;		// each mesh should use only once atlas page!

	unsigned int	vbo, vao, ibo;		// buffer objects
	vec3_t		mins, maxs;		// right transform to get screencopy
	int		parentbone;		// parent bone to transform AABB
	unsigned short	uniqueID;			// to reject decal drawing
	unsigned int	cacheSize;		// debug info: uploaded cache size for this buffer
} vbomesh_t;

// each mstudiotexture_t has a material
typedef struct mstudiomat_s
{
	mstudiotexture_t	*pSource;			// pointer to original texture

	unsigned short	gl_diffuse_id;		// diffuse texture
	unsigned short	gl_detailmap_id;		// detail texture
	unsigned short	gl_normalmap_id;		// normalmap
	unsigned short	gl_specular_id;		// specular
	unsigned short	gl_glowmap_id;		// self-illuminate parts
	unsigned short	gl_heightmap_id;		// parallax stuff

	// this part is shared with matdesc_t
	float		smoothness;		// smoothness factor
	float		detailScale[2];		// detail texture scales x, y
	float		reflectScale;		// reflection scale for translucent water
	float		refractScale;		// refraction scale for mirrors, windows, water
	float		aberrationScale;		// chromatic abberation
	float		reliefScale;		// relief-mapping
	struct matdef_t	*effects;			// hit, impact, particle effects etc
	int		flags;			// mstudiotexture_t->flags

	// cached shadernums
	shader_t		forwardScene;
	shader_t		forwardLightSpot;
	shader_t		forwardLightOmni[2];
	shader_t		forwardLightProj;
	shader_t		deferredScene;
	shader_t		deferredLight;
	shader_t		forwardDepth;

	unsigned short	lastRenderMode;		// for catch change render modes
} mstudiomaterial_t;

typedef struct mstudiosurface_s
{
	int		flags;			// match with msurface_t->flags
	int		texture_step;

	short		lightextents[2];
	unsigned short	light_s[MAXLIGHTMAPS];
	unsigned short	light_t[MAXLIGHTMAPS];
	byte		lights[MAXDYNLIGHTS];// static lights that affected this face (255 = no lights)

	int		lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];

	color24		*samples;		// note: this is the actual lightmap data for this surface
	color24		*normals;		// note: this is the actual deluxemap data for this surface
	byte		*shadows;		// note: occlusion map for this surface
} mstudiosurface_t;
	
typedef struct
{
	vbomesh_t		*meshes;			// meshes per submodel
	int		nummesh;			// mstudiomodel_t->nummesh
} msubmodel_t;

// triangles
typedef struct mbodypart_s
{
	int		base;			// mstudiobodyparts_t->base
	msubmodel_t	*models[MAXSTUDIOBODYPARTS];	// submodels per body part
	int		nummodels;		// mstudiobodyparts_t->nummodels
} mbodypart_t;

typedef struct mvbocache_s
{
	mstudiosurface_t	*surfaces;
	int		numsurfaces;

	mbodypart_t	*bodyparts;
	int		numbodyparts;

	bool		update_light;		// gamma or brightness was changed so we need to reload lightmaps
} mstudiocache_t;

typedef struct mposebone_s
{
	matrix3x4		posetobone[MAXSTUDIOBONES];
} mposetobone_t;

#endif//STUDIO_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
                                                                                                                                               
#include "lv_list.h"
#include "lv_input.h"
#include "lv_actor.h"
#include "lv_bin.h"

/* WARNING: Utterly shit ahead, i've screwed up on this and i need to
 * rewrite it. And i can't say i feel like it at the moment so be
 * patient :) */


static void fix_depth_with_bin (VisBin *bin, VisVideo *video, int depth);
static int bin_get_depth_using_preferred (VisBin *bin, int depthflag);

static void fix_depth_with_bin (VisBin *bin, VisVideo *video, int depth)
{
	/* Is supported within bin natively */
	if ((bin->depthflag & depth) > 0) {
		visual_video_set_depth (video, depth);
	} else {
		/* Not supported by the bin, taking the highest depth from the bin */
		visual_video_set_depth (video,
				visual_video_depth_get_highest_nogl (bin->depthflag));
	}
}

static int bin_get_depth_using_preferred (VisBin *bin, int depthflag)
{
	if (bin->depthpreferred == VISUAL_BIN_DEPTH_LOWEST)
		return visual_video_depth_get_lowest (depthflag);
	else
		return visual_video_depth_get_highest (depthflag);
}

/**
 * @defgroup VisBin VisBin
 * @{
 */

VisBin *visual_bin_new ()
{
	VisBin *bin;

	bin = malloc (sizeof (VisBin));
	memset (bin, 0, sizeof (VisBin));

	bin->morphautomatic = TRUE;

	bin->depthpreferred = VISUAL_BIN_DEPTH_HIGHEST;

	return bin;
}

int visual_bin_realize (VisBin *bin)
{
	if (bin == NULL)
		return -1;

	if (bin->actor != NULL)
		visual_actor_realize (bin->actor);

	if (bin->input != NULL)
		visual_input_realize (bin->input);

	if (bin->morph != NULL)
		visual_morph_realize (bin->morph);

	return 0;
}

int visual_bin_destroy (VisBin *bin)
{
	if (bin == NULL)
		return -1;

	if (bin->actor != NULL)
		visual_actor_destroy (bin->actor);

	if (bin->input != NULL)
		visual_input_destroy (bin->input);

	if (bin->morph != NULL)
		visual_morph_destroy (bin->morph);

	if (bin->actmorphmanaged == TRUE) {
		visual_actor_destroy (bin->actmorph);
		visual_video_free_with_buffer (bin->actmorphvideo);
	}

	if (bin->privvid != NULL)
		visual_video_free_with_buffer (bin->privvid);
	
	visual_bin_free (bin);

	return 0;
}

int visual_bin_free (VisBin *bin)
{
	if (bin == NULL)
		return -1;

	free (bin);

	return 0;
}

int visual_bin_set_actor (VisBin *bin, VisActor *actor)
{
	if (bin == NULL)
		return -1;

	bin->actor = actor;

	bin->managed = FALSE;

	return 0;
}

VisActor *visual_bin_get_actor (VisBin *bin)
{
	if (bin == NULL)
		return NULL;

	return bin->actor;
}

int visual_bin_set_input (VisBin *bin, VisInput *input)
{
	if (bin == NULL)
		return -1;

	bin->input = input;

	bin->inputmanaged = FALSE;

	return 0;
}

VisInput *visual_bin_get_input (VisBin *bin)
{
	if (bin == NULL)
		return NULL;

	return bin->input;
}

int visual_bin_set_morph (VisBin *bin, VisMorph *morph)
{
	if (bin == NULL)
		return -1;

	bin->morph = morph;

	bin->morphmanaged = FALSE;

	return 0;
}

int visual_bin_set_morph_by_name (VisBin *bin, char *morphname)
{
	VisMorph *morph;
	int depthflag;

	if (bin == NULL)
		return -1;

	morph = visual_morph_new (morphname);

	bin->morph = morph;
	bin->morphmanaged = TRUE;
	
	if (morph->plugin == NULL)
		return -1;

	depthflag = visual_morph_get_supported_depth (morph);

	if (visual_video_depth_is_supported (depthflag, bin->actvideo->depth) <= 0) {
		visual_morph_destroy (morph);
		bin->morph = NULL;

		return -2;
	}
	
	return 0;
}

VisMorph *visual_bin_get_morph (VisBin *bin)
{
	if (bin == NULL)
		return NULL;

	return bin->morph;
}

int visual_bin_connect (VisBin *bin, VisActor *actor, VisInput *input)
{
	if (bin == NULL)
		return -1;

	visual_bin_set_actor (bin, actor);
	visual_bin_set_input (bin, input);

	return 0;
}

int visual_bin_connect_by_names (VisBin *bin, char *actname, char *inname)
{
	VisActor *actor;
	VisInput *input;
	int depthflag;
	int depth;

	if (bin == NULL)
		return -1;

	/* Create the actor */
	actor = visual_actor_new (actname);
	if (actor == NULL)
		return -1;

	/* Check and set required depth */
	depthflag = visual_actor_get_supported_depth (actor);

	/* GL plugin, and ONLY a GL plugin */
	if (depthflag == VISUAL_VIDEO_DEPTH_GL)
		visual_bin_set_depth (bin, VISUAL_VIDEO_DEPTH_GL);
	else {
		depth = bin_get_depth_using_preferred (bin, depthflag);

		/* Is supported within bin natively */
		if ((bin->depthflag & depth) > 0) {
			visual_bin_set_depth (bin, depth);
		} else {
			/* Not supported by the bin, taking the highest depth from the bin */
			visual_bin_set_depth (bin,
				visual_video_depth_get_highest_nogl (bin->depthflag));
		}
	}

	/* Initialize the managed depth */
	bin->depthforcedmain = bin->depth;

	/* Create the input */
	input = visual_input_new (inname);
	if (input == NULL)
		return -1;

	/* Connect */
	visual_bin_connect (bin, actor, input);

	bin->managed = TRUE;
	bin->inputmanaged = TRUE;

	return 0;
}

int visual_bin_sync (VisBin *bin, int noevent)
{
	VisVideo *video;
	VisVideo *actvideo;

	if (bin == NULL)
		return -1;

	printf ("[sync] starting sync\n");

	/* Sync the actor regarding morph */
	if (bin->morphing == TRUE && bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->depthfromGL != TRUE) {
		visual_morph_set_video (bin->morph, bin->actvideo);
	
		video = bin->privvid;

		visual_video_free_buffer (video);
		visual_video_clone (video, bin->actvideo);

		printf ("[sync] pitches actvideo %d, new video %d\n", bin->actvideo->pitch, video->pitch);

		printf ("[sync] phase1 %p\n", bin->privvid);
		if (bin->actmorph->video->depth == VISUAL_VIDEO_DEPTH_GL) {
			visual_video_set_buffer (video, NULL);
			video = bin->actvideo;
		} else
			visual_video_allocate_buffer (video);
		
		printf ("[sync] phase2\n");
	} else {
		video = bin->actvideo;
		printf ("[sync] setting new video from actvideo %d %d\n", video->depth, video->bpp);
	}

	/* Main actor */
//	visual_actor_realize (bin->actor);
	visual_actor_set_video (bin->actor, video);

	printf ("[sync] one last video pitch check %d depth old %d forcedmain %d noevent %d\n", video->pitch, bin->depthold,
			bin->depthforcedmain, noevent);

	if (bin->managed == TRUE) {
		if (bin->depthold == VISUAL_VIDEO_DEPTH_GL)
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, noevent, TRUE);
	} else {
		if (bin->depthold == VISUAL_VIDEO_DEPTH_GL)
			visual_actor_video_negotiate (bin->actor, 0, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actor, 0, noevent, FALSE);
	}
	
	printf ("[sync] pitch after main actor negotiate %d\n", video->pitch);

	/* Morphing actor */
	if (bin->actmorphmanaged == TRUE && bin->morphing == TRUE &&
			bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH) {

		actvideo = bin->actmorphvideo;

		visual_video_free_buffer (actvideo);

		visual_video_clone (actvideo, video);

		if (bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL)
			visual_video_allocate_buffer (actvideo);

		visual_actor_realize (bin->actmorph);

		printf ("[sync] phase3 pitch of real framebuffer %d\n", bin->actvideo->pitch);
		if (bin->actmorphmanaged == TRUE)
			visual_actor_video_negotiate (bin->actmorph, bin->depthforced, FALSE, TRUE);
		else
			visual_actor_video_negotiate (bin->actmorph, 0, FALSE, FALSE);
	}

	printf ("[sync] end sync function\n\n");
	return 0;
}

int visual_bin_set_video (VisBin *bin, VisVideo *video)
{
	if (bin == NULL)
		return -1;
	
	bin->actvideo = video;

	return 0;
}

int visual_bin_set_supported_depth (VisBin *bin, int depthflag)
{
	if (bin == NULL)
		return -1;

	bin->depthflag = depthflag;

	return 0;
}

int visual_bin_set_preferred_depth (VisBin *bin, VisBinDepth depthpreferred)
{
	if (bin == NULL)
		return -1;

	bin->depthpreferred = depthpreferred;

	return 0;
}

int visual_bin_set_depth (VisBin *bin, int depth)
{
	if (bin == NULL)
		return -1;

	bin->depthold = bin->depth;

	if (visual_video_depth_is_supported (bin->depthflag, depth) != TRUE)
		return -2;

	printf ("[set-depth] old: %d new: %d\n", bin->depth, depth);
	if (bin->depth != depth)
		bin->depthchanged = TRUE;

	if (bin->depth == VISUAL_VIDEO_DEPTH_GL && bin->depthchanged == TRUE)
		bin->depthfromGL = TRUE;
	else
		bin->depthfromGL = FALSE;

	bin->depth = depth;

	visual_video_set_depth (bin->actvideo, depth);

	return 0;
}

int visual_bin_get_depth (VisBin *bin)
{
	if (bin == NULL)
		return -1;

	return bin->depth;
}

int visual_bin_depth_changed (VisBin *bin)
{
	if (bin == NULL)
		return -1;
	
	if (bin->depthchanged == FALSE)
		return FALSE;

	bin->depthchanged = FALSE;

	return TRUE;
}

VisPalette *visual_bin_get_palette (VisBin *bin)
{
	if (bin == NULL)
		return NULL;

	if (bin->morphing == TRUE)
		return visual_morph_get_palette (bin->morph);
	else
		return visual_actor_get_palette (bin->actor);
}

int visual_bin_switch_actor_by_name (VisBin *bin, char *actname)
{
	VisActor *actor;
	VisVideo *video;
	int depthflag;
	int depth;

	if (bin == NULL || actname == NULL)
		return -1;

	printf ("[switch-by-name] switching to a new actor: %s, old actor: %s\n", actname, bin->actor->plugin->ref->name);

	/* Destroy if there already is a managed one */
	if (bin->actmorphmanaged == TRUE) {
		if (bin->actmorph != NULL) {
			visual_actor_destroy (bin->actmorph);

			if (bin->actmorphvideo != NULL)
				visual_video_free_with_buffer (bin->actmorphvideo);
		}
	}

	/* Create a new managed actor */
	actor = visual_actor_new (actname);
	if (actor == NULL)
		return -1;

	video = visual_video_new ();

	visual_video_clone (video, bin->actvideo);

	depthflag = visual_actor_get_supported_depth (actor);
	if (visual_video_depth_is_supported (depthflag, VISUAL_VIDEO_DEPTH_GL) == TRUE) {
		printf ("[switch-by-name] Switching TO gl\n");

		bin->depthforced = VISUAL_VIDEO_DEPTH_GL;
		bin->depthforcedmain = VISUAL_VIDEO_DEPTH_GL;
	
		visual_video_set_depth (video, VISUAL_VIDEO_DEPTH_GL);

		visual_bin_set_depth (bin, VISUAL_VIDEO_DEPTH_GL);
		bin->depthchanged = TRUE;

	} else {
		printf ("[switch-by-name] Switch away from GL -- or non gl switch\n");

		
		/* Switching from GL */
		depth = bin_get_depth_using_preferred (bin, depthflag);

		fix_depth_with_bin (bin, video, depth);

		printf ("[switch-by-name] after depth fixating\n");
		
		/* After a depth change, the pitch value needs an update from the client
		 * if it's different from width * bpp, after a visual_bin_sync
		 * the issues are fixed again */
		printf ("[switch-by-name] video depth (from fixate): %d\n", video->depth);

		/* FIXME check if there are any unneeded depth transform environments and drop these */
		printf ("[switch-by-name] checking if we need to drop something: depthforcedmain: %d actvideo->depth %d\n",
				bin->depthforcedmain, bin->actvideo->depth);

		/* Drop a transformation environment when not needed */
		if (bin->depthforcedmain != bin->actvideo->depth) {
			visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, TRUE, TRUE);
			printf ("[switch-by-name] [[[[optionally a bogus transform environment, dropping]]]]\n");
		}

		if (bin->actvideo->depth > video->depth
				&& bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL
				&& bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH) {

			printf ("[switch-by-name] old depth is higher, video depth %d, depth %d bin depth %d\n", video->depth, depth,
					bin->depth);
			
			bin->depthforced = depth;
			bin->depthforcedmain = bin->depth;
			
			visual_bin_set_depth (bin, bin->actvideo->depth);

			visual_video_set_depth (video, bin->actvideo->depth);

		} else if (bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL) {

			printf ("[switch-by-name] new depth is higher, or equal: video depth %d, depth %d bin depth %d\n", video->depth, depth,
					bin->depth);

			printf ("[switch-by-name] depths i can locate: actvideo: %d   bin: %d   bin-old: %d\n", bin->actvideo->depth,
					bin->depth, bin->depthold);
			
			bin->depthforced = video->depth;
			bin->depthforcedmain = bin->depth;

			printf ("[switch-by-name] depthforcedmain in switch by name: %d\n", bin->depthforcedmain);
			printf ("[switch-by-name] visual_bin_set_depth %d\n", video->depth);
			visual_bin_set_depth (bin, video->depth);

		} else {
			/* Don't force ourself into a GL depth, seen we do a direct
			 * switch in the run */
			bin->depthforced = video->depth;
			bin->depthforcedmain = video->depth;
			
			printf ("[switch-by-name] from gl TO framebuffer for real, framebuffer depth: %d\n", video->depth);
		}

		printf ("[switch-by-name] Target depth selected: %d\n", depth);
		visual_video_set_dimension (video, video->width, video->height);

		printf ("[switch-by-name] switch by name pitch: %d\n", bin->actvideo->pitch);
		if (bin->actvideo->depth != VISUAL_VIDEO_DEPTH_GL)
			visual_video_set_pitch (video, bin->actvideo->pitch);
		
		printf ("[switch-by-name] before allocating buffer\n");
		visual_video_allocate_buffer (video);
		printf ("[switch-by-name] after allocating buffer\n");
	}

	printf ("[switch-by-name] video pitch of that what connects to the new actor %d\n", video->pitch);
	visual_actor_set_video (actor, video);

	bin->actmorphvideo = video;
	bin->actmorphmanaged = TRUE;

	printf ("[switch-by-name] switching ******************************************\n");
	visual_bin_switch_actor (bin, actor);


	printf ("[switch-by-name] end switch actor by name function ******************\n\n");
	return 0;
}

int visual_bin_switch_actor (VisBin *bin, VisActor *actor)
{
	VisVideo *privvid;

	if (bin == NULL || actor == NULL)
		return -1;

	/* Set the new actor */
	bin->actmorph = actor;

	printf ("[switch-actor] entering function\n");
	
	/* Free the private video */
	if (bin->privvid != NULL) {
		visual_video_free_with_buffer (bin->privvid);
		bin->privvid = NULL;
	}

	printf ("[switch-actor] depth of the main actor is %d\n", bin->actor->video->depth);

	/* Starting the morph, but first check if we don't have anything todo with openGL */
	if (bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->actmorph->video->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->depthfromGL != TRUE) {

		if (bin->morph != NULL && bin->morph->plugin != NULL) {
			visual_morph_set_rate (bin->morph, 0);
		
			visual_morph_set_video (bin->morph, bin->actvideo);
		}

		bin->morphrate = 0;
		bin->morphstepsdone = 0;

		printf ("[switch-actor] phase 1\n");
		/* Allocate a private video for the main actor, so the morph
		 * can draw to the framebuffer */
		privvid = visual_video_new ();

		printf ("[switch-actor] actvideo->depth %d actmorph->video->depth %d\n",
				bin->actvideo->depth, bin->actmorph->video->depth);

		printf ("[switch-actor] phase 2\n");
		visual_video_clone (privvid, bin->actvideo);
		printf ("[switch-actor] phase 3 pitch privvid %d actvideo %d\n", privvid->pitch, bin->actvideo->pitch);

		visual_video_allocate_buffer (privvid);

		printf ("[switch-actor] phase 4\n");
		/* Initial privvid initialize */
	
		printf ("[switch-actor]: actmorph->video->depth %d %p\n", bin->actmorph->video->depth,
				bin->actvideo->screenbuffer);
		
		if (bin->actvideo->screenbuffer != NULL && privvid->screenbuffer != NULL)
			memcpy (privvid->screenbuffer, bin->actvideo->screenbuffer, privvid->size);
		else if (privvid->screenbuffer != NULL)
			memset (privvid->screenbuffer, 0, privvid->size);

		visual_actor_set_video (bin->actor, privvid);
		bin->privvid = privvid;
	} else {
		printf ("[switch-actor]: pointer actvideo->screenbuffer %p\n", bin->actvideo->screenbuffer);
		if (bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL &&
				bin->actvideo->screenbuffer != NULL) {
			memset (bin->actvideo->screenbuffer, 0, bin->actvideo->size);
		}
	}

	printf ("[switch-actor] Leaving, actor->video->depth: %d actmorph->video->depth: %d\n",
			bin->actor->video->depth, bin->actmorph->video->depth);

	bin->morphing = TRUE;

	return 0;
}

int visual_bin_switch_finalize (VisBin *bin)
{
	int depthflag;

	if (bin == NULL)
		return -1;

	printf ("FINALIZING\n");
	if (bin->managed == TRUE)
		visual_actor_destroy (bin->actor);

	/* Copy over the depth to be sure, and for GL plugins */
//	bin->actvideo->depth = bin->actmorphvideo->depth;
//	visual_video_set_depth (bin->actvideo, bin->actmorphvideo->depth);

	if (bin->actmorphmanaged == TRUE) {
		visual_video_free_with_buffer (bin->actmorphvideo);
		bin->actmorphvideo = NULL;
	}
	
	visual_video_free_with_buffer (bin->privvid);
	bin->privvid = NULL;

	bin->actor = bin->actmorph;
	bin->actmorph = NULL;
	
	visual_actor_set_video (bin->actor, bin->actvideo);
	
	bin->morphing = FALSE;

	if (bin->morphmanaged == TRUE) {
		visual_morph_destroy (bin->morph);
		bin->morph = NULL;
	}

	printf (" - in finalize - fscking depth from actvideo: %d %d\n", bin->actvideo->depth, bin->actvideo->bpp);

	
//	visual_bin_set_depth (bin, bin->actvideo->depth);

	depthflag = visual_actor_get_supported_depth (bin->actor);
	fix_depth_with_bin (bin, bin->actvideo, bin_get_depth_using_preferred (bin, depthflag));
	visual_bin_set_depth (bin, bin->actvideo->depth);

	bin->depthforcedmain = bin->actvideo->depth;
	printf ("oi bin->depthforcedmain in finalize %d\n", bin->depthforcedmain);

	// FIXME replace with a depth fixer
	if (bin->depthchanged == TRUE) {
		printf ("negotiate without event\n");
		visual_actor_video_negotiate (bin->actor, bin->depthforcedmain, TRUE, TRUE);
		printf ("end negotiate without event\n");
	//	visual_bin_sync (bin);
	}

	printf ("END OF FINALIZE\n");

	return 0;
}

int visual_bin_switch_set_style (VisBin *bin, VisBinSwitchStyle style)
{
	if (bin == NULL)
		return -1;

	bin->morphstyle = style;

	return 0;
}

int visual_bin_switch_set_steps (VisBin *bin, int steps)
{
	if (bin == NULL)
		return -1;

	bin->morphsteps = steps;

	return 0;
}

int visual_bin_switch_set_automatic (VisBin *bin, int automatic)
{
	if (bin == NULL)
		return -1;

	bin->morphautomatic = automatic;

	return 0;
}

int visual_bin_switch_set_rate (VisBin *bin, float rate)
{
	if (bin == NULL)
		return -1;

	bin->morphrate = rate;

	return 0;
}

int visual_bin_run (VisBin *bin)
{
	if (bin == NULL)
		return -1;

	visual_input_run (bin->input);

//	printf ("[bin_run] wooohoo running morphing %d %d\n", bin->morphing,
//			bin->actor->plugin->realized);

	/* If we have a direct switch, do this BEFORE we run the actor,
	 * else we can get into trouble especially with GL, also when
	 * switching away from a GL plugin this is needed */
	if (bin->morphing == TRUE) {
		/* We realize here, because it doesn't realize
		 * on switch, the reason for this is so that after a
		 * switch call, especially in a managed bin the
		 * depth can be requested and set, this is important
		 * for openGL plugins, the realize method checks
		 * for double realize itself so we don't have
		 * to check this, it's a bit hacky */
 		if (bin->actmorph->plugin->realized == FALSE) {
/*			printf ("OI, we're realizing and negotiating the actor: %s %d %d\n",
					bin->actmorph->plugin->ref->name,
					bin->actmorph->video->depth, bin->depthforced);
*/			
			visual_actor_realize (bin->actmorph);
			
			if (bin->actmorphmanaged == TRUE)
				visual_actor_video_negotiate (bin->actmorph, bin->depthforced, FALSE, TRUE);
			else
				visual_actor_video_negotiate (bin->actmorph, 0, FALSE, FALSE);
		}

		/* When we've got multiple switch events without a sync we need
		 * to realize the main actor as well */
		if (bin->actor->plugin->realized == FALSE) {
/*			printf ("IO, we're realizing and negotiatig the fscking main actorn\n");
*/			
			visual_actor_realize (bin->actor);
			
			if (bin->managed == TRUE)
				visual_actor_video_negotiate (bin->actor, bin->depthforced, FALSE, TRUE);
			else
				visual_actor_video_negotiate (bin->actor, 0, FALSE, FALSE);
		}

		/* When the style is DIRECT or the context is GL we shouldn't try
		 * to morph and instead finalize at once */
		if (bin->morphstyle == VISUAL_SWITCH_STYLE_DIRECT ||
			bin->actor->video->depth == VISUAL_VIDEO_DEPTH_GL) {
		
			visual_bin_switch_finalize (bin);

			/* We can't start drawing yet, the client needs to catch up with
			 * the depth change */
			return 0;
		}
	}

	/* We realize here because in a managed bin the depth for openGL is
	 * requested after the connect, thus we can realize there yet */
	visual_actor_realize (bin->actor);

//	printf (" X before run main\n");
//	printf (" depth: %d %p realized %d\n", bin->actor->video->depth, bin->actor->video->screenbuffer,
//			bin->actor->plugin->realized);
//	printf ("size: %d %d\n", bin->actvideo->width, bin->actvideo->height);
	visual_actor_run (bin->actor, bin->input->audio);
//	printf (" Y after run main\n");

	if (bin->morphing == TRUE) {
		if (bin->morphstyle == VISUAL_SWITCH_STYLE_MORPH &&
			bin->actmorph->video->depth != VISUAL_VIDEO_DEPTH_GL &&
			bin->actor->video->depth != VISUAL_VIDEO_DEPTH_GL) {

			visual_actor_run (bin->actmorph, bin->input->audio);

			if (bin->morph == NULL || bin->morph->plugin == NULL) {
				visual_bin_switch_finalize (bin);
				return 0;
			}

			visual_morph_set_rate (bin->morph, bin->morphrate);

			/* Same goes for the morph, we realize it here for depth changes
			 * (especially the openGL case */
			visual_morph_realize (bin->morph);
			visual_morph_run (bin->morph, bin->input->audio, bin->actor->video, bin->actmorph->video);

			if (bin->morphautomatic == TRUE) {
				bin->morphrate += (1.000 / bin->morphsteps);
				bin->morphstepsdone++;

				/* Morph is done (only do this when automaticly morphing) */
				if (bin->morphstepsdone >= bin->morphsteps)
					visual_bin_switch_finalize (bin);
			}
		} else {
//			visual_bin_switch_finalize (bin);
		}
	}

	return 0;
}

/**
 * @}
 */

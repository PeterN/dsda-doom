/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:  Heads-up displays
 *
 *-----------------------------------------------------------------------------
 */

// killough 5/3/98: remove unnecessary headers

#include "doomstat.h"
#include "hu_stuff.h"
#include "hu_lib.h"
#include "st_stuff.h" /* jff 2/16/98 need loc of status bar */
#include "s_sound.h"
#include "dstrings.h"
#include "sounds.h"
#include "d_deh.h"   /* Ty 03/27/98 - externalization of mapnamesx arrays */
#include "g_game.h"
#include "r_main.h"
#include "p_inter.h"
#include "p_tick.h"
#include "p_map.h"
#include "sc_man.h"
#include "m_misc.h"
#include "r_main.h"
#include "lprintf.h"
#include "p_setup.h"
#include "w_wad.h"
#include "e6y.h" //e6y
#include "g_overflow.h"

#include "dsda.h"
#include "dsda/exhud.h"
#include "dsda/map_format.h"
#include "dsda/mapinfo.h"
#include "dsda/pause.h"
#include "dsda/settings.h"
#include "dsda/stretch.h"

static player_t*  plr;

static dboolean    always_off = false;
static dboolean    message_on;
static dboolean    message_list; //2/26/98 enable showing list of messages
dboolean           message_dontfuckwithme;
static dboolean    message_nottobefuckedwith;
static int         message_counter;
static int         yellow_message;

typedef struct custom_message_s
{
  int ticks;
  int cm;
  int sfx;
  const char *msg;
} custom_message_t;

typedef struct message_thinker_s
{
  thinker_t thinker;
  int plr;
  int delay;
  custom_message_t msg;
} message_thinker_t;

static custom_message_t custom_message[MAX_MAXPLAYERS];
static custom_message_t *custom_message_p;

//jff 2/16/98 status color change levels
int hud_ammo_red;      // ammo percent less than which status is red
int hud_ammo_yellow;   // ammo percent less is yellow more green
int hud_health_red;    // health amount less than which status is red
int hud_health_yellow; // health amount less than which status is yellow
int hud_health_green;  // health amount above is blue, below is green

void HU_InitThresholds(void)
{
  hud_health_red = dsda_IntConfig(dsda_config_hud_health_red);
  hud_health_yellow = dsda_IntConfig(dsda_config_hud_health_yellow);
  hud_health_green = dsda_IntConfig(dsda_config_hud_health_green);
  hud_ammo_red = dsda_IntConfig(dsda_config_hud_ammo_red);
  hud_ammo_yellow = dsda_IntConfig(dsda_config_hud_ammo_yellow);
}

dsda_string_t hud_title;

static void HU_FetchTitle(void)
{
  if (hud_title.string)
    dsda_FreeString(&hud_title);

  dsda_HUTitle(&hud_title);
}

static void HU_InitMessages(void)
{
  custom_message_p = &custom_message[displayplayer];
  message_on = false;
  message_dontfuckwithme = false;
  message_nottobefuckedwith = false;
  yellow_message = false;
}

static void HU_InitPlayer(void)
{
  // killough 3/7/98
  plr = &players[displayplayer];
}

int HU_GetHealthColor(int health, int def)
{
  int result;

  if (health < hud_health_red)
    result = CR_RED;
  else if (health < hud_health_yellow)
    result = CR_GOLD;
  else if (health <= hud_health_green)
    result = CR_GREEN;
  else
    result = def;

  return result;
}

typedef struct crosshair_s
{
  int lump;
  int w, h, flags;
  int target_x, target_y, target_z, target_sprite;
  float target_screen_x, target_screen_y;
} crosshair_t;

static crosshair_t crosshair;

static const char *crosshair_nam[HU_CROSSHAIRS] =
  { NULL, "CROSS1", "CROSS2", "CROSS3", "CROSS4", "CROSS5", "CROSS6", "CROSS7" };

static int hudadd_crosshair;
static int hudadd_crosshair_scale;
static int hudadd_crosshair_health;
static int hudadd_crosshair_target;
static int hudadd_crosshair_lock_target;

void HU_InitCrosshair(void)
{
  hudadd_crosshair_scale = dsda_IntConfig(dsda_config_hudadd_crosshair_scale);
  hudadd_crosshair_health = dsda_IntConfig(dsda_config_hudadd_crosshair_health);
  hudadd_crosshair_target = dsda_IntConfig(dsda_config_hudadd_crosshair_target);
  hudadd_crosshair_lock_target = dsda_IntConfig(dsda_config_hudadd_crosshair_lock_target);
  hudadd_crosshair = dsda_IntConfig(dsda_config_hudadd_crosshair);

  if (!hudadd_crosshair || !crosshair_nam[hudadd_crosshair])
    return;

  crosshair.lump = W_CheckNumForNameInternal(crosshair_nam[hudadd_crosshair]);
  if (crosshair.lump == LUMP_NOT_FOUND)
    return;

  crosshair.w = R_NumPatchWidth(crosshair.lump);
  crosshair.h = R_NumPatchHeight(crosshair.lump);

  crosshair.flags = VPT_TRANS;
  if (hudadd_crosshair_scale)
    crosshair.flags |= VPT_STRETCH;
}

dboolean HU_CrosshairEnabled(void)
{
  return hudadd_crosshair > 0;
}

void SetCrosshairTarget(void)
{
  crosshair.target_screen_x = 0.0f;
  crosshair.target_screen_y = 0.0f;

  if (hudadd_crosshair_lock_target && crosshair.target_sprite >= 0)
  {
    float x, y, z;
    float winx, winy, winz;

    x = -(float)crosshair.target_x / MAP_SCALE;
    z =  (float)crosshair.target_y / MAP_SCALE;
    y =  (float)crosshair.target_z / MAP_SCALE;

    if (R_Project(x, y, z, &winx, &winy, &winz))
    {
      int top, bottom, h;
      stretch_param_t *params = dsda_StretchParams(crosshair.flags);

      if (V_IsSoftwareMode())
      {
        winy += (float)(viewheight/2 - centery);
      }

      top = SCREENHEIGHT;
      h = crosshair.h;
      if (hudadd_crosshair_scale)
      {
        h = h * params->video->height / 200;
      }
      bottom = top - viewheight + h;
      winy = BETWEEN(bottom, top, winy);

      if (!hudadd_crosshair_scale)
      {
        crosshair.target_screen_x = winx - (crosshair.w / 2);
        crosshair.target_screen_y = SCREENHEIGHT - winy - (crosshair.h / 2);
      }
      else
      {
        crosshair.target_screen_x = (winx - params->deltax1) * 320.0f / params->video->width - (crosshair.w / 2);
        crosshair.target_screen_y = 200 - (winy - params->deltay1) * 200.0f / params->video->height - (crosshair.h / 2);
      }
    }
  }
}

mobj_t *HU_Target(void)
{
  fixed_t slope;
  angle_t an = plr->mo->angle;

  // intercepts overflow guard
  overflows_enabled = false;
  slope = P_AimLineAttack(plr->mo, an, 16*64*FRACUNIT, 0);
  if (plr->readyweapon == wp_missile || plr->readyweapon == wp_plasma || plr->readyweapon == wp_bfg)
  {
    if (!linetarget)
      slope = P_AimLineAttack(plr->mo, an += 1<<26, 16*64*FRACUNIT, 0);
    if (!linetarget)
      slope = P_AimLineAttack(plr->mo, an -= 2<<26, 16*64*FRACUNIT, 0);
  }
  overflows_enabled = true;

  return linetarget;
}

void HU_DrawCrosshair(void)
{
  int cm;

  if (!hudadd_crosshair)
    return;

  crosshair.target_sprite = -1;

  if (
    !crosshair_nam[hudadd_crosshair] ||
    crosshair.lump == -1 ||
    automap_active ||
    menuactive ||
    dsda_Paused()
  )
  {
    return;
  }

  if (hudadd_crosshair_health)
    cm = HU_GetHealthColor(plr->health, CR_LIGHTBLUE);
  else
    cm = dsda_IntConfig(dsda_config_hudadd_crosshair_color);

  if (hudadd_crosshair_target || hudadd_crosshair_lock_target)
  {
    mobj_t *target;

    target = HU_Target();

    if (target && !(target->flags & MF_SHADOW))
    {
      crosshair.target_x = target->x;
      crosshair.target_y = target->y;
      crosshair.target_z = target->z;
      crosshair.target_z += target->height / 2 + target->height / 8;
      crosshair.target_sprite = target->sprite;

      if (hudadd_crosshair_target)
        cm = dsda_IntConfig(dsda_config_hudadd_crosshair_target_color);
    }
  }

  SetCrosshairTarget();

  if (crosshair.target_screen_x != 0)
  {
    float x = crosshair.target_screen_x;
    float y = crosshair.target_screen_y;
    V_DrawNumPatchPrecise(x, y, 0, crosshair.lump, cm, crosshair.flags);
  }
  else
  {
    int x, y, st_height;

    if (!hudadd_crosshair_scale)
    {
      st_height = (R_PartialView() ? ST_SCALED_HEIGHT : 0);
      x = (SCREENWIDTH - crosshair.w) / 2;
      y = (SCREENHEIGHT - st_height - crosshair.h) / 2;
    }
    else
    {
      st_height = (R_PartialView() ? ST_HEIGHT : 0);
      x = (320 - crosshair.w) / 2;
      y = (200 - st_height - crosshair.h) / 2;
    }

    V_DrawNumPatch(x, y, 0, crosshair.lump, cm, crosshair.flags);
  }
}

//
// HU_Start(void)
//
// Create and initialize the heads-up display
//
// This routine must be called after any change to the heads up configuration
// in order for the changes to take effect in the actual displays
//
// Passed nothing, returns nothing
//
void HU_Start(void)
{
  HU_InitThresholds();
  HU_InitPlayer();
  HU_InitMessages();
  HU_FetchTitle();
  HU_InitCrosshair();

  dsda_InitExHud();
}

//
// HU_Drawer()
//
// Draw all the pieces of the heads-up display
//
// Passed nothing, returns nothing
//
void HU_Drawer(void)
{
  // don't draw anything if there's a fullscreen menu up
  if (menuactive == mnact_full)
    return;

  V_BeginUIDraw();

  HU_DrawCrosshair();

  dsda_DrawExHud();

  V_EndUIDraw();
}

char* player_message;
char* secret_message;

static void HU_UpdateMessage(const char* message)
{
  if (player_message)
    Z_Free(player_message);

  player_message = Z_Strdup(message);
}

char* HU_PlayerMessage(void) {
  return message_on ? player_message : NULL;
}

static void HU_UpdateSecretMessage(const char* message)
{
  if (secret_message)
    Z_Free(secret_message);

  secret_message = Z_Strdup(message);
}

char* HU_SecretMessage(void) {
  return custom_message_p->ticks > 0 ? secret_message : NULL;
}

//
// HU_Ticker()
//
// Update the hud displays once per frame
//
// Passed nothing, returns nothing
//
void HU_Ticker(void)
{
  int i;

  // tick down message counter if message is up
  if (message_counter && !--message_counter)
  {
    message_on = false;
    message_nottobefuckedwith = false;
    yellow_message = false;
  }

  // if messages on, or "Messages Off" is being displayed
  // this allows the notification of turning messages off to be seen
  if (dsda_ShowMessages() || message_dontfuckwithme)
  {
    // display message if necessary
    if ((plr->message && !message_nottobefuckedwith)
        || (plr->message && message_dontfuckwithme))
    {
      // update the active message
      HU_UpdateMessage(plr->message);

      // clear the message to avoid posting multiple times
      plr->message = 0;
      // note a message is displayed
      message_on = true;
      // start the message persistence counter
      message_counter = HU_MSGTIMEOUT;
      // transfer "Messages Off" exception to the "being displayed" variable
      message_nottobefuckedwith = message_dontfuckwithme;
      // clear the flag that "Messages Off" is being posted
      message_dontfuckwithme = 0;

      yellow_message = plr->yellowMessage;
      // hexen_note: use FONTAY_S for yellow messages (new font, y_message, etc)
    }
  }

  // centered messages
  for (i = 0; i < g_maxplayers; i++)
  {
    if (custom_message[i].ticks > 0)
      custom_message[i].ticks--;
  }

  if (custom_message_p->msg)
  {
    HU_UpdateSecretMessage(custom_message_p->msg);

    custom_message_p->msg = NULL;

    if (custom_message_p->sfx > 0 && custom_message_p->sfx < num_sfx)
    {
      S_StartVoidSound(custom_message_p->sfx);
    }
  }

  dsda_UpdateExHud();
}

//
// HU_Responder()
//
// Responds to input events that affect the heads up displays
//
// Passed the event to respond to, returns true if the event was handled
//
dboolean HU_Responder(event_t *ev)
{
  if (dsda_InputActivated(dsda_input_repeat_message)) // phares
  {
    message_on = true;
    message_counter = HU_MSGTIMEOUT;

    return true;
  }

  return false;
}

void T_ShowMessage (message_thinker_t* message)
{
  if (--message->delay > 0)
    return;

  SetCustomMessage(message->plr, message->msg.msg, 0,
    message->msg.ticks, message->msg.cm, message->msg.sfx);

  P_RemoveThinker(&message->thinker); // unlink and free
}

int SetCustomMessage(int plr, const char *msg, int delay, int ticks, int cm, int sfx)
{
  custom_message_t item;

  if (plr < 0 || plr >= g_maxplayers || !msg || ticks < 0 ||
      sfx < 0 || sfx >= num_sfx || cm < 0 || cm >= CR_LIMIT)
  {
    return false;
  }

  item.msg = msg;
  item.ticks = ticks;
  item.cm = cm;
  item.sfx = sfx;

  if (delay <= 0)
  {
    custom_message[plr] = item;
  }
  else
  {
    message_thinker_t *message = Z_CallocLevel(1, sizeof(*message));
    message->thinker.function = T_ShowMessage;
    message->delay = delay;
    message->plr = plr;

    message->msg = item;

    P_AddThinker(&message->thinker);
  }

  return true;
}

void ClearMessage(void)
{
  message_counter = 0;
  yellow_message = false;
}

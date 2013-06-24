//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "ac/display.h"
#include "gfx/ali3d.h"
#include "ac/common.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "ac/character.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/global_audio.h"
#include "ac/global_game.h"
#include "ac/gui.h"
#include "ac/mouse.h"
#include "ac/overlay.h"
#include "ac/record.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "ac/topbarsettings.h"
#include "debug/debug_log.h"
#include "game/game_objects.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "media/audio/audio.h"
#include "platform/base/agsplatformdriver.h"
#include "ac/spritecache.h"

using AGS::Common::Bitmap;
namespace BitmapHelper = AGS::Common::BitmapHelper;

extern int longestline;
extern int scrnwid,scrnhit;
extern Bitmap *virtual_screen;
extern ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
extern volatile int timerloop;
extern AGSPlatformDriver *platform;
extern volatile unsigned long globalTimerCounter;
extern int time_between_timers;
extern int offsetx, offsety;
extern int frames_per_second;
extern int loops_per_character;
extern IAGSFontRenderer* fontRenderers[MAX_FONTS];
extern int spritewidth[MAX_SPRITES],spriteheight[MAX_SPRITES];
extern SpriteCache spriteset;

int display_message_aschar=0;
char *heightTestString = "ZHwypgfjqhkilIK";


TopBarSettings topBar;
// draw_text_window: draws the normal or custom text window
// create a new bitmap the size of the window before calling, and
//   point ds to it
// returns text start x & y pos in parameters
Bitmap *screenop = NULL;
int wantFreeScreenop = 0;
int texthit;

// Pass yy = -1 to find Y co-ord automatically
// allowShrink = 0 for none, 1 for leftwards, 2 for rightwards
// pass blocking=2 to create permanent overlay
int _display_main(int xx,int yy,int wii,char*todis,int blocking,int usingfont,int asspch, int isThought, int allowShrink, bool overlayPositionFixed) 
{
    bool alphaChannel = false;
    ensure_text_valid_for_font(todis, usingfont);
    break_up_text_into_lines(wii-6,usingfont,todis);
    texthit = wgetfontheight(usingfont);

    // AGS 2.x: If the screen is faded out, fade in again when displaying a message box.
    if (!asspch && (loaded_game_file_version <= kGameVersion_272))
        play.ScreenIsFadedOut = 0;

    // if it's a normal message box and the game was being skipped,
    // ensure that the screen is up to date before the message box
    // is drawn on top of it
    if ((play.SkipUntilCharacterStops >= 0) && (blocking == 1))
        render_graphics();

    EndSkippingUntilCharStops();

    if (topBar.wantIt) {
        // ensure that the window is wide enough to display
        // any top bar text
        int topBarWid = wgettextwidth_compensate(topBar.text, topBar.font);
        topBarWid += multiply_up_coordinate(play.TopBarBorderWidth + 2) * 2;
        if (longestline < topBarWid)
            longestline = topBarWid;
        // the top bar should behave like DisplaySpeech wrt blocking
        blocking = 0;
    }

    if (asspch > 0) {
        // update the all_buttons_disabled variable in advance
        // of the adjust_x/y_for_guis calls
        play.DisabledUserInterface++;
        update_gui_disabled_status();
        play.DisabledUserInterface--;
    }

    if (xx == OVR_AUTOPLACE) ;
    // centre text in middle of screen
    else if (yy<0) yy=(scrnhit/2-(numlines*texthit)/2)-3;
    // speech, so it wants to be above the character's head
    else if (asspch > 0) {
        yy-=numlines*texthit;
        if (yy < 5) yy=5;
        yy = adjust_y_for_guis (yy);
    }

    if (longestline < wii - get_fixed_pixel_size(6)) {
        // shrink the width of the dialog box to fit the text
        int oldWid = wii;
        //if ((asspch >= 0) || (allowShrink > 0))
        // If it's not speech, or a shrink is allowed, then shrink it
        if ((asspch == 0) || (allowShrink > 0))
            wii = longestline + get_fixed_pixel_size(6);

        // shift the dialog box right to align it, if necessary
        if ((allowShrink == 2) && (xx >= 0))
            xx += (oldWid - wii);
    }

    if (xx<-1) { 
        xx=(-xx)-wii/2;
        if (xx < 0)
            xx = 0;

        xx = adjust_x_for_guis (xx, yy);

        if (xx + wii >= scrnwid)
            xx = (scrnwid - wii) - 5;
    }
    else if (xx<0) xx=scrnwid/2-wii/2;

    int ee, extraHeight = get_fixed_pixel_size(6);
    Bitmap *ds = GetVirtualScreen();
    color_t text_color = ds->GetCompatibleColor(15);
    if (blocking < 2)
        remove_screen_overlay(OVER_TEXTMSG);

    screenop = BitmapHelper::CreateTransparentBitmap((wii > 0) ? wii : 2, numlines*texthit + extraHeight, final_col_dep);
    ds = SetVirtualScreen(screenop);

    // inform draw_text_window to free the old bitmap
    wantFreeScreenop = 1;

    if ((strlen (todis) < 1) || (strcmp (todis, "  ") == 0) || (wii == 0)) ;
    // if it's an empty speech line, don't draw anything
    else if (asspch) { //text_color = ds->GetCompatibleColor(12);
        int ttxleft = 0, ttxtop = get_fixed_pixel_size(3), oriwid = wii - 6;
        int usingGui = -1, drawBackground = 0;

        if ((asspch < 0) && (game.Options[OPT_SPEECHTYPE] >= 2)) {
            usingGui = play.SpeechTextWindowGuiIndex;
            drawBackground = 1;
        }
        else if ((isThought) && (game.Options[OPT_THOUGHTGUI] > 0)) {
            usingGui = game.Options[OPT_THOUGHTGUI];
            // make it treat it as drawing inside a window now
            if (asspch > 0)
                asspch = -asspch;
            drawBackground = 1;
        }

        if (drawBackground)
            draw_text_window_and_bar(ds, &ttxleft, &ttxtop, &xx, &yy, &wii, 0, usingGui);
        else if ((ShouldAntiAliasText()) && (final_col_dep >= 24))
            alphaChannel = true;

        for (ee=0;ee<numlines;ee++) {
            //int ttxp=wii/2 - wgettextwidth_compensate(lines[ee], usingfont)/2;
            int ttyp=ttxtop+ee*texthit;
            // asspch < 0 means that it's inside a text box so don't
            // centre the text
            if (asspch < 0) {
                if ((usingGui >= 0) && 
                    ((game.Options[OPT_SPEECHTYPE] >= 2) || (isThought)))
                    text_color = ds->GetCompatibleColor(guis[usingGui].ForegroundColor);
                else
                    text_color = ds->GetCompatibleColor(-asspch);

                wouttext_aligned(ds, ttxleft, ttyp, oriwid, usingfont, text_color, lines[ee], play.DisplayTextAlignment);
            }
            else {
                text_color = ds->GetCompatibleColor(asspch);
                //wouttext_outline(ttxp,ttyp,usingfont,lines[ee]);
                wouttext_aligned(ds, ttxleft, ttyp, wii, usingfont, text_color, lines[ee], play.SpeechTextAlignment);
            }
        }
    }
    else {
        int xoffs,yoffs, oriwid = wii - 6;
        draw_text_window_and_bar(ds, &xoffs,&yoffs,&xx,&yy,&wii);

        if (game.Options[OPT_TWCUSTOM] > 0)
        {
            alphaChannel = guis[game.Options[OPT_TWCUSTOM]].HasAlphaChannel();
        }

        adjust_y_coordinate_for_text(&yoffs, usingfont);

        for (ee=0;ee<numlines;ee++)
            wouttext_aligned (ds, xoffs, yoffs + ee * texthit, oriwid, usingfont, text_color, lines[ee], play.DisplayTextAlignment);
    }

    wantFreeScreenop = 0;

    int ovrtype = OVER_TEXTMSG;
    if (blocking == 2) ovrtype=OVER_CUSTOM;
    else if (blocking >= OVER_CUSTOM) ovrtype=blocking;

    int nse = add_screen_overlay(xx, yy, ovrtype, screenop, alphaChannel);

    ds = SetVirtualScreen(virtual_screen);
    if (blocking>=2) {
        return screenover[nse].type;
    }

    if (blocking) {
        if (play.FastForwardCutscene) {
            remove_screen_overlay(OVER_TEXTMSG);
            play.MessageTime=-1;
            return 0;
        }

        /*    wputblock(xx,yy,screenop,1);
        remove_screen_overlay(OVER_TEXTMSG);*/

        if (!play.MouseCursorHidden)
            domouse(1);
        // play.SkipDisplayMethod has same values as SetSkipSpeech:
        // 0 = click mouse or key to skip
        // 1 = key only
        // 2 = can't skip at all
        // 3 = only on keypress, no auto timer
        // 4 = mouse only
        int countdown = GetTextDisplayTime (todis);
        int skip_setting = user_to_internal_skip_speech(play.SkipDisplayMethod);
        while (1) {
            timerloop = 0;
            NEXT_ITERATION();
            /*      if (!play.MouseCursorHidden)
            domouse(0);
            write_screen();*/

            render_graphics();

            update_polled_stuff_and_crossfade();
            if (mgetbutton()>NONE) {
                // If we're allowed, skip with mouse
                if (skip_setting & SKIP_MOUSECLICK)
                    break;
            }
            if (kbhit()) {
                // discard keypress, and don't leave extended keys over
                int kp = getch();
                if (kp == 0) getch();

                // let them press ESC to skip the cutscene
                check_skip_cutscene_keypress (kp);
                if (play.FastForwardCutscene)
                    break;

                if (skip_setting & SKIP_KEYPRESS)
                    break;
            }
            while ((timerloop == 0) && (play.FastForwardCutscene == 0)) {
                update_polled_stuff_if_runtime();
                platform->YieldCPU();
            }
            countdown--;

            if (channels[SCHAN_SPEECH] != NULL) {
                // extend life of text if the voice hasn't finished yet
                if ((!rec_isSpeechFinished()) && (play.FastForwardCutscene == 0)) {
                    if (countdown <= 1)
                        countdown = 1;
                }
                else  // if the voice has finished, remove the speech
                    countdown = 0;
            }

            if ((countdown < 1) && (skip_setting & SKIP_AUTOTIMER))
            {
                play.IgnoreUserInputUntilTime = globalTimerCounter + (play.DisplayTextIgnoreUserInputDelayMs / time_between_timers);
                break;
            }
            // if skipping cutscene, don't get stuck on No Auto Remove
            // text boxes
            if ((countdown < 1) && (play.FastForwardCutscene))
                break;
        }
        if (!play.MouseCursorHidden)
            domouse(2);
        remove_screen_overlay(OVER_TEXTMSG);

        construct_virtual_screen(true);
    }
    else {
        // if the speech does not time out, but we are skipping a cutscene,
        // allow it to time out
        if ((play.MessageTime < 0) && (play.FastForwardCutscene))
            play.MessageTime = 2;

        if (!overlayPositionFixed)
        {
            screenover[nse].positionRelativeToScreen = false;
            screenover[nse].x += offsetx;
            screenover[nse].y += offsety;
        }

        GameLoopUntilEvent(UNTIL_NOOVERLAY,0);
    }

    play.MessageTime=-1;
    return 0;
}

void _display_at(int xx,int yy,int wii,char*todis,int blocking,int asspch, int isThought, int allowShrink, bool overlayPositionFixed) {
    int usingfont=FONT_NORMAL;
    if (asspch) usingfont=FONT_SPEECH;
    int needStopSpeech = 0;

    EndSkippingUntilCharStops();

    if (todis[0]=='&') {
        // auto-speech
        int igr=atoi(&todis[1]);
        while ((todis[0]!=' ') & (todis[0]!=0)) todis++;
        if (todis[0]==' ') todis++;
        if (igr <= 0)
            quit("Display: auto-voice symbol '&' not followed by valid integer");
        if (play_speech(play.NarratorCharacterIndex,igr)) {
            // if Voice Only, then blank out the text
            if (play.SpeechVoiceMode == 2)
                todis = "  ";
        }
        needStopSpeech = 1;
    }
    _display_main(xx,yy,wii,todis,blocking,usingfont,asspch, isThought, allowShrink, overlayPositionFixed);

    if (needStopSpeech)
        stop_speech();
}

int   source_text_length = -1;

int GetTextDisplayTime (const char *text, int canberel) {
    int uselen = strlen(text);

    int fpstimer = frames_per_second;

    // if it's background speech, make it stay relative to game speed
    if ((canberel == 1) && (play.BkgSpeechRelativeToGameSpeed == 1))
        fpstimer = 40;

    if (source_text_length >= 0) {
        // sync to length of original text, to make sure any animations
        // and music sync up correctly
        uselen = source_text_length;
        source_text_length = -1;
    }
    else {
        if ((text[0] == '&') && (play.UnfactorVoiceTagFromDisplayTime != 0)) {
            // if there's an "&12 text" type line, remove "&12 " from the source
            // length
            int j = 0;
            while ((text[j] != ' ') && (text[j] != 0))
                j++;
            j++;
            uselen -= j;
        }

    }

    if (uselen <= 0)
        return 0;

    if (play.TextDisplaySpeed + play.TextSpeedModifier <= 0)
        quit("!Text speed is zero; unable to display text. Check your game.text_speed settings.");

    // Store how many game loops per character of text
    // This is calculated using a hard-coded 15 for the text speed,
    // so that it's always the same no matter how fast the user
    // can read.
    loops_per_character = (((uselen/play.LipsyncSpeed)+1) * fpstimer) / uselen;

    int textDisplayTimeInMS = ((uselen / (play.TextDisplaySpeed + play.TextSpeedModifier)) + 1) * 1000;
    if (textDisplayTimeInMS < play.DisplayTextMinTimeMs)
        textDisplayTimeInMS = play.DisplayTextMinTimeMs;

    return (textDisplayTimeInMS * fpstimer) / 1000;
}

bool ShouldAntiAliasText() {
    return (game.Options[OPT_ANTIALIASFONTS] != 0);
}

void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char*texx) {
    
    color_t outline_color = ds->GetCompatibleColor(play.SpeechTextOutlineColour);
    if (game.FontOutline[usingfont] >= 0) {
        // MACPORT FIX 9/6/5: cast
        wouttextxy(ds, xxp, yyp, (int)game.FontOutline[usingfont], outline_color, texx);
    }
    else if (game.FontOutline[usingfont] == FONT_OUTLINE_AUTO) {
        int outlineDist = 1;

        if ((game.Options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[usingfont]->SupportsExtendedCharacters(usingfont))) {
            // if it's a scaled up SCI font, move the outline out more
            outlineDist = get_fixed_pixel_size(1);
        }

        // move the text over so that it's still within the bounding rect
        xxp += outlineDist;
        yyp += outlineDist;

        wouttextxy(ds, xxp - outlineDist, yyp, usingfont, outline_color, texx);
        wouttextxy(ds, xxp + outlineDist, yyp, usingfont, outline_color, texx);
        wouttextxy(ds, xxp, yyp + outlineDist, usingfont, outline_color, texx);
        wouttextxy(ds, xxp, yyp - outlineDist, usingfont, outline_color, texx);
        wouttextxy(ds, xxp - outlineDist, yyp - outlineDist, usingfont, outline_color, texx);
        wouttextxy(ds, xxp - outlineDist, yyp + outlineDist, usingfont, outline_color, texx);
        wouttextxy(ds, xxp + outlineDist, yyp + outlineDist, usingfont, outline_color, texx);
        wouttextxy(ds, xxp + outlineDist, yyp - outlineDist, usingfont, outline_color, texx);
    }

    wouttextxy(ds, xxp, yyp, usingfont, text_color, texx);
}

void wouttext_aligned (Bitmap *ds, int usexp, int yy, int oriwid, int usingfont, color_t text_color, const char *text, int align) {

    if (align == SCALIGN_CENTRE)
        usexp = usexp + (oriwid / 2) - (wgettextwidth_compensate(text, usingfont) / 2);
    else if (align == SCALIGN_RIGHT)
        usexp = usexp + (oriwid - wgettextwidth_compensate(text, usingfont));

    wouttext_outline(ds, usexp, yy, usingfont, text_color, text);
}

int wgetfontheight(int font) {
    int htof = wgettextheight(heightTestString, font);

    // automatic outline fonts are 2 pixels taller
    if (game.FontOutline[font] == FONT_OUTLINE_AUTO) {
        // scaled up SCI font, push outline further out
        if ((game.Options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
            htof += get_fixed_pixel_size(2);
        // otherwise, just push outline by 1 pixel
        else
            htof += 2;
    }

    return htof;
}

int wgettextwidth_compensate(const char *tex, int font) {
    int wdof = wgettextwidth(tex, font);

    if (game.FontOutline[font] == FONT_OUTLINE_AUTO) {
        // scaled up SCI font, push outline further out
        if ((game.Options[OPT_NOSCALEFNT] == 0) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
            wdof += get_fixed_pixel_size(2);
        // otherwise, just push outline by 1 pixel
        else
            wdof += get_fixed_pixel_size(1);
    }

    return wdof;
}

void do_corner(Bitmap *ds, int sprn,int xx1,int yy1,int typx,int typy) {
    if (sprn<0) return;
    Bitmap *thisone = spriteset[sprn];
    if (thisone == NULL)
        thisone = spriteset[0];

    put_sprite_256(ds, xx1+typx*spritewidth[sprn],yy1+typy*spriteheight[sprn],thisone);
    //  wputblock(xx1+typx*spritewidth[sprn],yy1+typy*spriteheight[sprn],thisone,1);
}

int get_but_pic(GuiMain*guo,int indx) {
    return guibuts[guo->ControlRefs[indx] & 0xFFFF].NormalImage;
}

void draw_button_background(Bitmap *ds, int xx1,int yy1,int xx2,int yy2,GuiMain*iep) {
    color_t draw_color;
    if (iep==NULL) {  // standard window
        draw_color = ds->GetCompatibleColor(15);
        ds->FillRect(Rect(xx1,yy1,xx2,yy2), draw_color);
        draw_color = ds->GetCompatibleColor(16);
        ds->DrawRect(Rect(xx1,yy1,xx2,yy2), draw_color);
        /*    draw_color = ds->GetCompatibleColor(opts.tws.backcol); ds->FillRect(Rect(xx1,yy1,xx2,yy2);
        draw_color = ds->GetCompatibleColor(opts.tws.ds->GetTextColor()); ds->DrawRect(Rect(xx1+1,yy1+1,xx2-1,yy2-1);*/
    }
    else {
        if (loaded_game_file_version < kGameVersion_262) // < 2.62
        {
            // Color 0 wrongly shows as transparent instead of black
            // From the changelog of 2.62:
            //  - Fixed text windows getting a black background if colour 0 was
            //    specified, rather than being transparent.
            if (iep->BackgroundColor == 0)
                iep->BackgroundColor = 16;
        }

        if (iep->BackgroundColor >= 0) draw_color = ds->GetCompatibleColor(iep->BackgroundColor);
        else draw_color = ds->GetCompatibleColor(0); // black backrgnd behind picture

        if (iep->BackgroundColor > 0)
            ds->FillRect(Rect(xx1,yy1,xx2,yy2), draw_color);

        int leftRightWidth = spritewidth[get_but_pic(iep,4)];
        int topBottomHeight = spriteheight[get_but_pic(iep,6)];
        if (iep->BackgroundImage>0) {
            if ((loaded_game_file_version <= kGameVersion_272) // 2.xx
                && (spriteset[iep->BackgroundImage]->GetWidth() == 1)
                && (spriteset[iep->BackgroundImage]->GetHeight() == 1) 
                && (*((unsigned int*)spriteset[iep->BackgroundImage]->GetData()) == 0x00FF00FF))
            {
                // Don't draw fully transparent dummy GUI backgrounds
            }
            else
            {
                // offset the background image and clip it so that it is drawn
                // such that the border graphics can have a transparent outside
                // edge
                int bgoffsx = xx1 - leftRightWidth / 2;
                int bgoffsy = yy1 - topBottomHeight / 2;
                ds->SetClip(Rect(bgoffsx, bgoffsy, xx2 + leftRightWidth / 2, yy2 + topBottomHeight / 2));
                int bgfinishx = xx2;
                int bgfinishy = yy2;
                int bgoffsyStart = bgoffsy;
                while (bgoffsx <= bgfinishx)
                {
                    bgoffsy = bgoffsyStart;
                    while (bgoffsy <= bgfinishy)
                    {
                        wputblock(ds, bgoffsx, bgoffsy, spriteset[iep->BackgroundImage], 0);
                        bgoffsy += spriteheight[iep->BackgroundImage];
                    }
                    bgoffsx += spritewidth[iep->BackgroundImage];
                }
                // return to normal clipping rectangle
                ds->SetClip(Rect(0, 0, ds->GetWidth() - 1, ds->GetHeight() - 1));
            }
        }
        int uu;
        for (uu=yy1;uu <= yy2;uu+=spriteheight[get_but_pic(iep,4)]) {
            do_corner(ds, get_but_pic(iep,4),xx1,uu,-1,0);   // left side
            do_corner(ds, get_but_pic(iep,5),xx2+1,uu,0,0);  // right side
        }
        for (uu=xx1;uu <= xx2;uu+=spritewidth[get_but_pic(iep,6)]) {
            do_corner(ds, get_but_pic(iep,6),uu,yy1,0,-1);  // top side
            do_corner(ds, get_but_pic(iep,7),uu,yy2+1,0,0); // bottom side
        }
        do_corner(ds, get_but_pic(iep,0),xx1,yy1,-1,-1);  // top left
        do_corner(ds, get_but_pic(iep,1),xx1,yy2+1,-1,0);  // bottom left
        do_corner(ds, get_but_pic(iep,2),xx2+1,yy1,0,-1);  //  top right
        do_corner(ds, get_but_pic(iep,3),xx2+1,yy2+1,0,0);  // bottom right
    }
}

// Calculate the width that the left and right border of the textwindow
// GUI take up
int get_textwindow_border_width (int twgui) {
    if (twgui < 0)
        return 0;

    if (!guis[twgui].IsTextWindow())
        quit("!GUI set as text window but is not actually a text window GUI");

    int borwid = spritewidth[get_but_pic(&guis[twgui], 4)] + 
        spritewidth[get_but_pic(&guis[twgui], 5)];

    return borwid;
}

// get the hegiht of the text window's top border
int get_textwindow_top_border_height (int twgui) {
    if (twgui < 0)
        return 0;

    if (!guis[twgui].IsTextWindow())
        quit("!GUI set as text window but is not actually a text window GUI");

    return spriteheight[get_but_pic(&guis[twgui], 6)];
}

void draw_text_window(Bitmap *ds, int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight, int ifnum) {
    if (ifnum < 0)
        ifnum = game.Options[OPT_TWCUSTOM];

    if (ifnum <= 0) {
        if (ovrheight)
            quit("!Cannot use QFG4 style options without custom text window");
        draw_button_background(ds, 0,0,ds->GetWidth() - 1,ds->GetHeight() - 1,NULL);
        xins[0]=3;
        yins[0]=3;
    }
    else {
        if (ifnum >= game.GuiCount)
            quitprintf("!Invalid GUI %d specified as text window (total GUIs: %d)", ifnum, game.GuiCount);
        if (!guis[ifnum].IsTextWindow())
            quit("!GUI set as text window but is not actually a text window GUI");

        int tbnum = get_but_pic(&guis[ifnum], 0);

        wii[0] += get_textwindow_border_width (ifnum);
        xx[0]-=spritewidth[tbnum];
        yy[0]-=spriteheight[tbnum];
        if (ovrheight == 0)
            ovrheight = numlines*texthit;

        if ((wantFreeScreenop > 0) && (screenop != NULL))
            delete screenop;
        screenop = BitmapHelper::CreateTransparentBitmap(wii[0],ovrheight+6+spriteheight[tbnum]*2,final_col_dep);
        ds = SetVirtualScreen(screenop);
        int xoffs=spritewidth[tbnum],yoffs=spriteheight[tbnum];
        draw_button_background(ds, xoffs,yoffs,(ds->GetWidth() - xoffs) - 1,(ds->GetHeight() - yoffs) - 1,&guis[ifnum]);
        xins[0]=xoffs+3;
        yins[0]=yoffs+3;
    }

}

void draw_text_window_and_bar(Bitmap *ds, int*xins,int*yins,int*xx,int*yy,int*wii,int ovrheight, int ifnum) {

    draw_text_window(ds, xins, yins, xx, yy, wii, ovrheight, ifnum);

    if ((topBar.wantIt) && (screenop != NULL)) {
        // top bar on the dialog window with character's name
        // create an enlarged window, then free the old one
        Bitmap *newScreenop = BitmapHelper::CreateBitmap(screenop->GetWidth(), screenop->GetHeight() + topBar.height, final_col_dep);
        newScreenop->Blit(screenop, 0, 0, 0, topBar.height, screenop->GetWidth(), screenop->GetHeight());
        delete screenop;
        screenop = newScreenop;
        Bitmap *ds = SetVirtualScreen(screenop);

        // draw the top bar
        color_t draw_color = ds->GetCompatibleColor(play.TopBarBkgColour);
        ds->FillRect(Rect(0, 0, screenop->GetWidth() - 1, topBar.height - 1), draw_color);
        if (play.TopBarBkgColour != play.TopBarBorderColour) {
            // draw the border
            draw_color = ds->GetCompatibleColor(play.TopBarBorderColour);
            for (int j = 0; j < multiply_up_coordinate(play.TopBarBorderWidth); j++)
                ds->DrawRect(Rect(j, j, screenop->GetWidth() - (j + 1), topBar.height - (j + 1)), draw_color);
        }

        // draw the text
        int textx = (screenop->GetWidth() / 2) - wgettextwidth_compensate(topBar.text, topBar.font) / 2;
        color_t text_color = ds->GetCompatibleColor(play.TopBarTextColour);
        wouttext_outline(ds, textx, play.TopBarBorderWidth + get_fixed_pixel_size(1), topBar.font, text_color, topBar.text);

        // don't draw it next time
        topBar.wantIt = 0;
        // adjust the text Y position
        yins[0] += topBar.height;
    }
    else if (topBar.wantIt)
        topBar.wantIt = 0;
}

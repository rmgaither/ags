
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_CN_AC__OLDGAMESETUPSTRUCT_H
#define __AGS_CN_AC__OLDGAMESETUPSTRUCT_H

#include "ac/characterinfo.h"       // OldCharacterInfo, CharacterInfo
#ifdef UNUSED_CODE
#include "ac/eventblock.h"       // EventBlock
#endif
#include "ac/interfaceelement.h"    // InterfaceElement
#include "ac/inventoryiteminfo.h"   // InventoryItemInfo
#include "ac/mousecursor.h"      // MouseCursor
#include "ac/wordsdictionary.h"  // WordsDictionary
#include "script/cc_script.h"           // ccScript

struct OriGameSetupStruct {
    char              gamename[30];
    char              options[20];
    unsigned char     paluses[256];
    color             defpal[256];
    InterfaceElement  iface[10];
    int               numiface;
    int               numviews;
    MouseCursor       mcurs[10];
    char              *globalscript;
    int               numcharacters;
    OldCharacterInfo     *chars;
#ifdef UNUSED_CODE
    EventBlock        __charcond[50];   // [IKM] 2012-06-22: does not seem to be used anywhere
    EventBlock        __invcond[100];   // same
#endif
    ccScript          *compiled_script;
    int               playercharacter;
    unsigned char     __old_spriteflags[2100];
    int               totalscore;
    short             numinvitems;
    InventoryItemInfo invinfo[100];
    int               numdialog, numdlgmessage;
    int               numfonts;
    int               color_depth;              // in bytes per pixel (ie. 1 or 2)
    int               target_win;
    int               dialog_bullet;            // 0 for none, otherwise slot num of bullet point
    short             hotdot, hotdotouter;   // inv cursor hotspot dot
    int               uniqueid;    // random key identifying the game
    int               reserved[2];
    short             numlang;
    char              langcodes[MAXLANGUAGE][3];
    char              *messages[MAXGLOBALMES];
};

struct OriGameSetupStruct2 : public OriGameSetupStruct {
    unsigned char   fontflags[10];
    char            fontoutline[10];
    int             numgui;
    WordsDictionary *dict;
    int             reserved2[8];
};

struct OldGameSetupStruct : public OriGameSetupStruct2 {
    unsigned char spriteflags[6000];
};

#endif // __AGS_CN_AC__OLDGAMESETUPSTRUCT_H

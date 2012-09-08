
#include <stdio.h>
#include "util/wgt2allg.h"
#include "gui/guitextbox.h"
#include "gui/guimain.h"
#include "font/agsfontrenderer.h"	// fontRenderers;
#include "util/datastream.h"

using AGS::Common::CDataStream;

DynamicArray<GUITextBox> guitext;
int numguitext = 0;

void GUITextBox::WriteToFile(CDataStream *out)
{
  GUIObject::WriteToFile(out);
  // MACPORT FIXES: swap
  out->WriteArray(&text[0], sizeof(char), 200);
  out->WriteArray(&font, sizeof(int), 3);
}

void GUITextBox::ReadFromFile(CDataStream *in, int version)
{
  GUIObject::ReadFromFile(in, version);
  // MACPORT FIXES: swap
  in->ReadArray(&text[0], sizeof(char), 200);
  in->ReadArray(&font, sizeof(int), 3);
  if (textcol == 0)
    textcol = 16;
}

void GUITextBox::Draw()
{

  check_font(&font);
  wtextcolor(textcol);
  wsetcolor(textcol);
  if ((exflags & GTF_NOBORDER) == 0) {
    wrectangle(x, y, x + wid - 1, y + hit - 1);
    if (get_fixed_pixel_size(1) > 1)
      wrectangle(x + 1, y + 1, x + wid - get_fixed_pixel_size(1), y + hit - get_fixed_pixel_size(1));
  }

  Draw_text_box_contents();
}

void GUITextBox::KeyPress(int kp)
{
  guis_need_update = 1;
  // backspace, remove character
  if ((kp == 8) && (strlen(text) > 0)) {
    text[strlen(text) - 1] = 0;
    return;
  } else if (kp == 8)
    return;

  // other key, continue
  if ((kp >= 128) && (!fontRenderers[font]->SupportsExtendedCharacters(font)))
    return;

  if (kp == 13) {
    activated++;
    return;
  }

  if (strlen(text) >= 199)
    return;

  text[strlen(text) + 1] = 0;
  text[strlen(text)] = kp;

  // if the new string is too long, remove the new character
  if (wgettextwidth(text, font) > (wid - (6 + get_fixed_pixel_size(5))))
    text[strlen(text) - 1] = 0;

}

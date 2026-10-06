#include <conio.h>
#include <dos.h>
#include <i86.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MENU_TITLE
#define MENU_TITLE "Program Menu"
#endif

#ifndef MENU_EXE_NAME
#define MENU_EXE_NAME "DOSMENU.EXE"
#endif

#define MAX_PROGRAMS 256
#define NAME_LENGTH  13
#define LIST_ROW     4
#define PROGRAM_START_DELAY 1000

typedef struct {
  char name[NAME_LENGTH];
} program_t;

static program_t programs[MAX_PROGRAMS];
static unsigned int program_count;
static int screen_page;
static unsigned char cursor_start;
static unsigned char cursor_end;

static void get_screen_size(int *width, int *height)
{
  union REGS inregs;
  union REGS outregs;
  unsigned char rows;

  inregs.h.ah = 0x0F;
  int86(0x10, &inregs, &outregs);
  *width = outregs.h.ah;
  screen_page = outregs.h.bh;

  rows = *((unsigned char __far *) MK_FP(0x40, 0x84)) + 1;
  *height = rows >= 15 && rows <= 60 ? rows : 25;
}

static void set_cursor(int column, int row)
{
  union REGS inregs;
  union REGS outregs;

  inregs.h.ah = 0x02;
  inregs.h.bh = screen_page;
  inregs.h.dh = row - 1;
  inregs.h.dl = column - 1;
  int86(0x10, &inregs, &outregs);
}

static void save_cursor_shape(void)
{
  union REGS inregs;
  union REGS outregs;

  inregs.h.ah = 0x03;
  inregs.h.bh = screen_page;
  int86(0x10, &inregs, &outregs);
  cursor_start = outregs.h.ch;
  cursor_end = outregs.h.cl;
}

static void set_cursor_visible(int visible)
{
  union REGS inregs;
  union REGS outregs;

  inregs.h.ah = 0x01;
  if (visible) {
    inregs.h.ch = cursor_start;
    inregs.h.cl = cursor_end;
  } else {
    inregs.h.ch = 0x20;
    inregs.h.cl = 0;
  }
  int86(0x10, &inregs, &outregs);
}

static void write_char(int column, int row, int character, int attribute)
{
  union REGS inregs;
  union REGS outregs;

  set_cursor(column, row);
  inregs.h.ah = 0x09;
  inregs.h.al = character;
  inregs.h.bh = screen_page;
  inregs.h.bl = attribute;
  inregs.x.cx = 1;
  int86(0x10, &inregs, &outregs);
}

static void clear_screen(int width, int height)
{
  union REGS inregs;
  union REGS outregs;

  inregs.h.ah = 0x06;
  inregs.h.al = 0;
  inregs.h.bh = 0x07;
  inregs.h.ch = 0;
  inregs.h.cl = 0;
  inregs.h.dh = height - 1;
  inregs.h.dl = width - 1;
  int86(0x10, &inregs, &outregs);
  set_cursor(1, 1);
}

static void find_programs(void)
{
  struct find_t file;

  program_count = 0;
  if (_dos_findfirst("*.EXE", _A_NORMAL, &file))
    return;

  do {
    if (stricmp(file.name, MENU_EXE_NAME) && program_count < MAX_PROGRAMS) {
      strncpy(programs[program_count].name, file.name, NAME_LENGTH - 1);
      programs[program_count].name[NAME_LENGTH - 1] = '\0';
      program_count++;
    }
  } while (!_dos_findnext(&file));
}

static void draw_text(int column, int row, int width, const char *text)
{
  int count;

  set_cursor(column, row);
  count = width - column;
  while (count-- && *text)
    putch(*text++);
}

static void draw_rule(int row, int width, int left, int right)
{
  int column;

  write_char(1, row, left, 0x07);
  for (column = 0; column < width - 2; column++)
    write_char(column + 2, row, 0xC4, 0x07);
  write_char(width, row, right, 0x07);
}

static void draw_framed_text(int row, int width, const char *text)
{
  int column;

  write_char(1, row, 0xB3, 0x07);
  for (column = 0; column < width - 2; column++)
    write_char(column + 2, row, *text ? *text++ : ' ', 0x07);
  write_char(width, row, 0xB3, 0x07);
}

static void draw_centered_framed_text(int row, int width, const char *text)
{
  int length;
  int column;

  length = strlen(text);
  if (length > width - 2)
    length = width - 2;

  draw_framed_text(row, width, "");
  column = (width - length) / 2 + 1;
  set_cursor(column, row);
  while (length-- && *text)
    putch(*text++);
}

static void draw_menu_item(int row, int width, const char *text,
                           int highlighted)
{
  int column;
  int character;

  write_char(1, row, 0xB3, 0x07);
  for (column = 0; column < width - 2; column++) {
    character = *text ? *text++ : ' ';
    write_char(column + 2, row, character, highlighted ? 0x70 : 0x07);
  }
  write_char(width, row, 0xB3, 0x07);
}

static int visible_rows(int height)
{
  int rows;

  rows = height - 8;
  return rows < 1 ? 1 : rows;
}

static void draw_program_list(unsigned int selected, unsigned int top,
                              int width, int height)
{
  int visible;
  int row;

  visible = visible_rows(height);
  for (row = 0; row < visible; row++) {
    unsigned int index;
    const char *name;

    index = top + row;
    name = index < program_count ? programs[index].name : "";
    draw_menu_item(LIST_ROW + row, width, name,
                   index == selected && index < program_count);
  }

  if (!program_count)
    draw_framed_text(LIST_ROW, width, "No test EXE files found.");
}

static void draw_page_indicators(unsigned int top, int width, int height)
{
  int visible;
  unsigned int pages;
  unsigned int page;

  visible = visible_rows(height);
  pages = (program_count + visible - 1) / visible;
  if (!pages)
    pages = 1;
  page = top / visible;

  write_char(width - 1, 3, page ? 0x18 : 0xC4, 0x07);
  write_char(width - 1, height - 4,
             page + 1 < pages ? 0x19 : 0xC4, 0x07);
}

static void draw_page_status(unsigned int top, int width, int height)
{
  int visible;
  int length;
  int column;
  unsigned int pages;
  const char *text;
  char page_status[32];

  visible = visible_rows(height);
  pages = (program_count + visible - 1) / visible;
  if (!pages)
    pages = 1;
  sprintf(page_status, "PGUP/PGDN: PAGE %u OF %u",
          (unsigned int) (top / visible) + 1, (unsigned int) pages);
  draw_framed_text(height - 3, width, "");
  length = strlen(page_status);
  if (length > width - 2)
    length = width - 2;
  column = (width - length) / 2 + 1;
  text = page_status;
  while (length-- && *text)
    write_char(column++, height - 3, *text++, 0x07);
}

static void draw_menu(unsigned int selected, unsigned int top,
                      int width, int height)
{
  clear_screen(width, height);
  draw_rule(1, width, 0xDA, 0xBF);
  draw_centered_framed_text(2, width, MENU_TITLE);
  draw_rule(3, width, 0xC3, 0xB4);
  draw_program_list(selected, top, width, height);
  draw_rule(height - 4, width, 0xC3, 0xB4);
  draw_page_status(top, width, height);
  draw_centered_framed_text(height - 2, width,
                             "UP/DOWN: SELECT  ENTER: RUN  ESC: EXIT");
  draw_rule(height - 1, width, 0xC0, 0xD9);
  draw_page_indicators(top, width, height);
}

static int read_key(void)
{
  int key;

  key = getch();
  if (key == 0 || key == 0xE0)
    key = getch() << 8;
  return key;
}

static void launch_program(unsigned int selected)
{
  char path[NAME_LENGTH + 3];
  char message[NAME_LENGTH + 16];
  int width;
  int height;

  sprintf(path, ".\\%s", programs[selected].name);
  sprintf(message, "Running %s...", programs[selected].name);
  get_screen_size(&width, &height);
  set_cursor_visible(1);
  clear_screen(width, height);
  draw_text((width - strlen(message)) / 2 + 1, (height + 1) / 2,
            width, message);
  delay(PROGRAM_START_DELAY);
  clear_screen(width, height);
  spawnl(P_WAIT, path, programs[selected].name, (char *) 0);
  set_cursor_visible(0);
  find_programs();
}

int main(void)
{
  unsigned int selected;
  unsigned int top;
  unsigned int previous_selected;
  unsigned int previous_top;
  unsigned int previous_count;
  int key;
  int visible;
  int width;
  int height;

  selected = 0;
  top = 0;
  find_programs();

  get_screen_size(&width, &height);
  save_cursor_shape();
  set_cursor_visible(0);
  visible = visible_rows(height);
  if (selected >= program_count)
    selected = program_count ? program_count - 1 : 0;
  top = (selected / visible) * visible;
  draw_menu(selected, top, width, height);

  for (;;) {
    key = read_key();

    if (key == 27) {
      get_screen_size(&width, &height);
      clear_screen(width, height);
      set_cursor_visible(1);
      return 0;
    }

    previous_selected = selected;
    previous_top = top;
    previous_count = program_count;
    if (key == 0x4800 && program_count) {
      if (selected > top)
        selected--;
    } else if (key == 0x5000 && program_count) {
      if (selected + 1 < program_count && selected + 1 < top + visible)
        selected++;
    } else if ((key == 0x4900 || key == 0x5100) && program_count) {
      if (key == 0x4900) {
        if (top)
          selected = top > (unsigned int) visible ? top - visible : 0;
      } else if (top + visible < program_count) {
        selected = top + visible;
      }
    } else if ((key == '\r' || key == '\n') && program_count) {
      launch_program(selected);
      get_screen_size(&width, &height);
      visible = visible_rows(height);
      if (selected >= program_count)
        selected = program_count ? program_count - 1 : 0;
      top = (selected / visible) * visible;
      draw_menu(selected, top, width, height);
      continue;
    } else if (!program_count && key != 27) {
      find_programs();
    }

    if (program_count != previous_count) {
      selected = 0;
      top = 0;
      draw_menu(selected, top, width, height);
    } else if (selected != previous_selected) {
      top = (selected / visible) * visible;
      if (key == 0x4900 || key == 0x5100 || top != previous_top) {
        draw_program_list(selected, top, width, height);
        draw_page_indicators(top, width, height);
        draw_page_status(top, width, height);
      } else {
        draw_menu_item(LIST_ROW + selected - top, width,
                       programs[selected].name, 1);
        draw_menu_item(LIST_ROW + previous_selected - top, width,
                       programs[previous_selected].name, 0);
      }
    }
  }
}

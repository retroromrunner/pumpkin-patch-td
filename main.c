#include <stdint.h>

#define REG_DISPCNT (*(volatile uint16_t *)0x04000000)
#define REG_VCOUNT (*(volatile uint16_t *)0x04000006)
#define REG_KEYINPUT (*(volatile uint16_t *)0x04000130)
#define REG_SOUNDCNT_L (*(volatile uint16_t *)0x04000080)
#define REG_SOUNDCNT_X (*(volatile uint16_t *)0x04000084)
#define REG_SND1CNT_H (*(volatile uint16_t *)0x04000062)
#define REG_SND1CNT_X (*(volatile uint16_t *)0x04000064)
#define REG_SND2CNT_L (*(volatile uint16_t *)0x04000068)
#define REG_SND2CNT_H (*(volatile uint16_t *)0x0400006C)
#define REG_SND4CNT_L (*(volatile uint16_t *)0x04000078)
#define REG_SND4CNT_H (*(volatile uint16_t *)0x0400007C)

#define SCREEN_W 240
#define SCREEN_H 160
#define TILE 16
#define MAP_W 30
#define MAP_H 18
#define MAX_TOWERS 44
#define MAX_ENEMIES 28
#define MAX_PROJECTILES 24
#define PROJECTILE_FRAMES 10
#define COLOR_MEM ((volatile uint16_t *)0x05000000)
#define RGB15(r, g, b) ((uint16_t)((r) | ((g) << 5) | ((b) << 10)))
#define KEY_A 0x0001
#define KEY_B 0x0002
#define KEY_SELECT 0x0004
#define KEY_START 0x0008
#define KEY_RIGHT 0x0010
#define KEY_LEFT 0x0020
#define KEY_UP 0x0040
#define KEY_DOWN 0x0080

enum { TITLE, DAY, NIGHT, WIN, LOSE };
enum { SCARECROW, LANTERN, CAULDRON, MOONFLOWER, BAT_ROOST };
enum { ZOMBIE, SKELETON, GHOST, TRICKSTER, WEREWOLF, HORSEMAN };

typedef struct { int8_t x, y; } Point;
typedef struct {
  int8_t x, y, kind, level, branch;
  uint16_t cooldown;
} Tower;
typedef struct {
  int32_t x, y;
  int16_t hp, max_hp, speed;
  int8_t kind, target, slow, poison;
  uint8_t reward;
} Enemy;
typedef struct {
  int16_t sx, sy, tx, ty;
  uint8_t age, color;
} Projectile;
static const Point route[] = {
  {-1, 2}, {4, 2}, {4, 5}, {9, 5}, {9, 2}, {14, 2}, {14, 7},
  {10, 7}, {10, 10}, {16, 10}, {16, 5}, {21, 5}, {21, 9},
  {25, 9}, {25, 13}, {20, 13}, {20, 16}, {25, 16}, {30, 16}
};
#define ROUTE_COUNT (sizeof(route) / sizeof(route[0]))

static const uint16_t colors[] = {
  RGB15(2, 2, 5), RGB15(5, 4, 12), RGB15(8, 7, 15), RGB15(12, 9, 15),
  RGB15(18, 10, 12), RGB15(25, 15, 9), RGB15(12, 21, 6), RGB15(15, 23, 7),
  RGB15(24, 24, 9), RGB15(27, 13, 5), RGB15(13, 10, 17), RGB15(20, 13, 17),
  RGB15(23, 28, 26), RGB15(31, 29, 22), RGB15(31, 6, 4), RGB15(18, 29, 9),
  RGB15(20, 27, 18), RGB15(31, 14, 4), RGB15(31, 24, 9), RGB15(24, 12, 24),
  RGB15(5, 4, 8), RGB15(20, 14, 8), RGB15(12, 8, 18), RGB15(16, 10, 19),
  RGB15(15, 8, 13), RGB15(15, 10, 8), RGB15(5, 4, 10), RGB15(23, 18, 10),
  RGB15(18, 25, 12), RGB15(23, 16, 8), RGB15(30, 25, 17), RGB15(31, 20, 5)
};

static const uint8_t font[][5] = {
  {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},
  {7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},{1,1,1,5,2},
  {5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},
  {6,5,6,4,4},{2,5,5,7,3},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},
  {5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},
  {7,1,2,4,7},
  {2,5,5,5,2},{2,6,2,2,7},{6,1,2,4,7},{6,1,3,1,6},{5,5,7,1,1},
  {7,4,6,1,6},{3,4,6,5,2},{7,1,2,2,2},{2,5,2,5,2},{2,5,3,1,6},
  {0,0,2,0,0},{0,0,0,0,2},{0,0,7,0,0},{1,1,2,4,4},{2,2,2,2,2}
};

static volatile uint8_t *frame;
static uint8_t back_page, route_map[MAP_W][MAP_H];
static Tower towers[MAX_TOWERS];
static Enemy enemies[MAX_ENEMIES];
static Projectile projectiles[MAX_PROJECTILES];
static uint8_t tower_count, enemy_count;
static uint8_t mode, day, selected;
static uint8_t pumpkins, seeds, spawn_left;
static uint16_t candy, frame_count, spawn_clock, music_clock, wave_clear_clock;
static uint16_t old_keys, held_frames, cursor_x, cursor_y;
static int16_t camera_x, camera_y;
static uint8_t paused, message_timer, message_code;
static uint8_t message_value;
static uint8_t message_tower_x, message_tower_y;
static uint16_t message_cost;
static uint16_t tower_hover_timer;
static uint8_t tower_hover_kind, tower_hover_level, tower_hover_branch;
static uint8_t title_note, night_note;
static uint16_t random_seed;
static uint16_t simulation_frame;

static const uint16_t tower_cost[] = {50, 75, 65, 80, 90};
static const uint8_t tower_range[] = {56, 44, 49, 52, 68};
static const uint8_t tower_damage[] = {9, 20, 5, 0, 7};
static const uint8_t tower_rate[] = {38, 63, 76, 0, 27};
static const uint16_t melody[] = {
  440, 523, 659, 698, 659, 523, 415, 440,
  349, 440, 523, 587, 523, 440, 415, 330,
  440, 523, 659, 831, 880, 831, 659, 523,
  587, 523, 440, 349, 330, 415, 440, 330
};
static const uint16_t night_melody[] = {
  294, 349, 392, 466, 440, 392, 554, 587,
  466, 440, 349, 330, 392, 466, 554, 440,
  294, 349, 392, 466, 554, 587, 554, 466,
  440, 392, 349, 330, 294, 330, 392, 294
};
static const uint16_t night_bass[] = {147, 174, 196, 220, 147, 165, 196, 233};

static uint32_t rand_next(void) {
  random_seed = (uint16_t)(random_seed * 25173u + 13849u);
  return random_seed;
}

static void set_pixel(int x, int y, uint8_t color) {
  if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H) {
    volatile uint16_t *pair = (volatile uint16_t *)(frame + y * SCREEN_W + (x & ~1));
    uint16_t pixels = *pair;
    if (x & 1) *pair = (uint16_t)((pixels & 0x00FFu) | ((uint16_t)color << 8));
    else *pair = (uint16_t)((pixels & 0xFF00u) | color);
  }
}

static void rect(int x, int y, int w, int h, uint8_t color) {
  if (x + w <= 0 || y + h <= 0 || x >= SCREEN_W || y >= SCREEN_H) return;
  uint16_t pair = (uint16_t)(color | ((uint16_t)color << 8));
  int yy;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > SCREEN_W) w = SCREEN_W - x;
  if (y + h > SCREEN_H) h = SCREEN_H - y;
  if (w <= 0 || h <= 0) return;
  for (yy = y; yy < y + h; yy++) {
    volatile uint8_t *row = frame + yy * SCREEN_W + x;
    int width = w;
    if (((uintptr_t)row & 1u) && width) { *row++ = color; width--; }
    {
      volatile uint16_t *pixels = (volatile uint16_t *)row;
      while (width >= 2) { *pixels++ = pair; width -= 2; }
      row = (volatile uint8_t *)pixels;
    }
    if (width) *row = color;
  }
}

static void outline(int x, int y, int w, int h, uint8_t color) {
  rect(x, y, w, 1, color); rect(x, y + h - 1, w, 1, color);
  rect(x, y, 1, h, color); rect(x + w - 1, y, 1, h, color);
}

static void text(const char *s, int x, int y, uint8_t color, int scale) {
  int row, col, glyph;
  while (*s) {
    char ch = *s++;
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch >= 'A' && ch <= 'Z') glyph = ch - 'A';
    else if (ch >= '0' && ch <= '9') glyph = 26 + ch - '0';
    else if (ch == ':') glyph = 36;
    else if (ch == '.') glyph = 37;
    else if (ch == '-') glyph = 38;
    else if (ch == '!') glyph = 39;
    else if (ch == '+') glyph = 40;
    else if (ch == '/') glyph = 41;
    else { x += 4 * scale; continue; }
    for (row = 0; row < 5; row++)
      for (col = 0; col < 3; col++)
        if (font[glyph][row] & (1u << (2 - col)))
          set_pixel(x + col * scale, y + row * scale, color);
    x += 5 * scale;
  }
}

static int length(const char *s) {
  int n = 0;
  while (s[n]) n++;
  return n;
}

static void centered(const char *s, int y, uint8_t color, int scale) {
  text(s, (SCREEN_W - length(s) * 5 * scale + scale) / 2, y, color, scale);
}

static void play_tone(uint16_t hz, uint8_t duration) {
  uint16_t period;
  if (hz < 40) hz = 40;
  period = (uint16_t)(2048u - 131072u / hz);
  REG_SND1CNT_H = 0xF0F0;
  REG_SND1CNT_X = (uint16_t)(0xC000 | period);
  (void)duration;
}

static void sfx(uint8_t kind) {
  uint16_t hz = 440;
  if (kind == 0) {
    uint16_t high = (uint16_t)(2048u - 131072u / 740u);
    uint16_t low = (uint16_t)(2048u - 131072u / 554u);
    REG_SND1CNT_H = 0xF2F0;
    REG_SND1CNT_X = (uint16_t)(0xC000 | high);
    REG_SND2CNT_L = 0xF2E0;
    REG_SND2CNT_H = (uint16_t)(0xC000 | low);
    return;
  }
  else if (kind == 1) hz = 590;
  else if (kind == 2) {
    uint16_t high = (uint16_t)(2048u - 131072u / 330u);
    uint16_t low = (uint16_t)(2048u - 131072u / 247u);
    REG_SND1CNT_H = 0xF2F0;
    REG_SND1CNT_X = (uint16_t)(0xC000 | high);
    REG_SND2CNT_L = 0xF2E0;
    REG_SND2CNT_H = (uint16_t)(0xC000 | low);
    return;
  }
  else if (kind == 3) hz = 170;
  else if (kind == 4) hz = 220;
  else if (kind == 5) hz = 660;
  else if (kind == 6) {
    REG_SND4CNT_L = 0xF53C;
    REG_SND4CNT_H = 0xC017;
    return;
  }
  else if (kind == 7) hz = 520;
  else if (kind == 8) {
    uint16_t high = (uint16_t)(2048u - 131072u / 415u);
    uint16_t low = (uint16_t)(2048u - 131072u / 311u);
    REG_SND1CNT_H = 0xF2F0;
    REG_SND1CNT_X = (uint16_t)(0xC000 | high);
    REG_SND2CNT_L = 0xF2E0;
    REG_SND2CNT_H = (uint16_t)(0xC000 | low);
    return;
  }
  REG_SND2CNT_L = 0xF0E0;
  REG_SND2CNT_H = (uint16_t)(0xC000 | (2048u - 131072u / hz));
}

static void music_update(void) {
  if (mode != TITLE && mode != NIGHT) return;
  if (++music_clock < (mode == TITLE ? 4 : 8)) return;
  music_clock = 0;
  if (mode == TITLE) {
    uint16_t hz = melody[title_note++ & 31];
    play_tone(hz, 18);
    if ((title_note & 3) == 0) {
      uint16_t bass = melody[(title_note >> 2) & 31] / 2;
      REG_SND2CNT_L = 0xF0E0;
      REG_SND2CNT_H = (uint16_t)(0xC000 | (2048u - 131072u / bass));
    }
  } else {
    uint8_t index = night_note++ & 31;
    play_tone(night_melody[index], 18);
    if ((index & 3) == 0) {
      uint16_t bass = night_bass[(index >> 2) & 7];
      REG_SND2CNT_L = 0xF0E0;
      REG_SND2CNT_H = (uint16_t)(0xC000 | (2048u - 131072u / bass));
    }
  }
}

static void stop_music(void) {
  REG_SND1CNT_X = 0;
  REG_SND1CNT_H = 0x0080;
  REG_SND2CNT_L = 0x0080;
  REG_SND2CNT_H = 0;
  music_clock = 0;
}

static void update_projectiles(void) {
  int i;
  for (i = 0; i < MAX_PROJECTILES; i++)
    if (projectiles[i].age && projectiles[i].age < PROJECTILE_FRAMES) projectiles[i].age++;
    else projectiles[i].age = 0;
}

static void spawn_projectile(int sx, int sy, int tx, int ty, uint8_t color) {
  int i;
  for (i = 0; i < MAX_PROJECTILES; i++) {
    Projectile *p = &projectiles[i];
    if (p->age) continue;
    p->sx = (int16_t)sx; p->sy = (int16_t)sy;
    p->tx = (int16_t)tx; p->ty = (int16_t)ty;
    p->age = 1; p->color = color;
    return;
  }
}

static void draw_pumpkin(int x, int y, int gone) {
  if (gone) {
    rect(x - 5, y - 2, 10, 5, 21); rect(x - 3, y + 3, 6, 2, 20);
    return;
  }
  rect(x - 6, y - 4, 12, 8, 17); rect(x - 7, y - 2, 14, 4, 17);
  rect(x - 3, y - 4, 2, 8, 18); rect(x + 2, y - 3, 2, 6, 18);
  rect(x - 1, y - 7, 2, 3, 15); rect(x + 1, y - 7, 3, 1, 15);
  rect(x - 3, y - 1, 2, 2, 20); rect(x + 2, y - 1, 2, 2, 20);
  rect(x - 1, y + 2, 3, 1, 20);
}

static void draw_candy(int x, int y) {
  rect(x, y, 4, 6, 18); rect(x + 1, y + 2, 2, 2, 17); rect(x + 1, y, 2, 1, 13);
}

static void draw_seed(int x, int y) {
  rect(x + 1, y, 3, 5, 15); rect(x + 2, y + 1, 1, 2, 13);
}

static void build_route_map(void) {
  unsigned i;
  for (i = 0; i + 1 < ROUTE_COUNT; i++) {
    int x = route[i].x, y = route[i].y;
    int dx = (route[i + 1].x > x) - (route[i + 1].x < x);
    int dy = (route[i + 1].y > y) - (route[i + 1].y < y);
    while (x != route[i + 1].x || y != route[i + 1].y) {
      if ((unsigned)x < MAP_W && (unsigned)y < MAP_H) route_map[x][y] = 1;
      x += dx; y += dy;
    }
    if ((unsigned)x < MAP_W && (unsigned)y < MAP_H) route_map[x][y] = 1;
  }
}

static Tower *tower_at(int x, int y) {
  int i;
  for (i = 0; i < tower_count; i++)
    if (towers[i].x == x && towers[i].y == y) return &towers[i];
  return 0;
}

static void set_message(uint8_t code, uint8_t value) {
  message_code = code;
  message_value = value;
  message_timer = 120;
  if (code == 3) message_cost = (uint16_t)(35 + (value - 1) * 25);
  else if (code == 13) {
    Tower *t = tower_at(cursor_x, cursor_y);
    if (t) message_cost = (uint16_t)(35 + t->level * 25);
  } else if (code == 14) message_cost = 85;
  if (code == 3 || code == 13 || code == 14) {
    message_tower_x = (uint8_t)cursor_x;
    message_tower_y = (uint8_t)cursor_y;
  }
}

static void place_or_upgrade(void) {
  Tower *t = tower_at(cursor_x, cursor_y);
  if (mode != DAY) return;
  if (t) {
    uint16_t price = (uint16_t)(35 + t->level * 25);
    uint8_t seed_price = t->level == 2 ? 2 : 0;
    if (t->level >= 3) { set_message(1, 0); return; }
    if (candy < price) { set_message(2, (uint8_t)price); return; }
    if (seeds < seed_price) { set_message(15, seed_price); return; }
    candy -= price; seeds -= seed_price; t->level++;
    if (t->level == 3) {
      sfx(5);
      set_message(14, (uint8_t)t->branch);
    }
    else sfx(0);
    if (t->level < 3) set_message(3, t->level);
    return;
  }
  if (tower_count >= MAX_TOWERS || route_map[cursor_x][cursor_y]) { set_message(4, 0); return; }
  if (candy < tower_cost[selected]) { set_message(2, (uint8_t)tower_cost[selected]); return; }
  candy -= tower_cost[selected];
  towers[tower_count].x = (int8_t)cursor_x;
  towers[tower_count].y = (int8_t)cursor_y;
  towers[tower_count].kind = (int8_t)selected;
  towers[tower_count].level = 1;
  towers[tower_count].branch = 0;
  towers[tower_count].cooldown = 1;
  tower_count++;
  sfx(0);
  set_message(5, 0);
}

static void sell_tower(void) {
  int i;
  for (i = 0; i < tower_count; i++) {
    if (towers[i].x == cursor_x && towers[i].y == cursor_y) {
      uint16_t refund = (uint16_t)(tower_cost[(uint8_t)towers[i].kind] * (55 + (towers[i].level - 1) * 25) / 100);
      int j;
      candy += refund;
      for (j = i; j + 1 < tower_count; j++) towers[j] = towers[j + 1];
      tower_count--; sfx(2); set_message(6, (uint8_t)refund); return;
    }
  }
  set_message(7, 0);
}

static void start_night(void) {
  int i;
  if (mode != DAY) return;
  mode = NIGHT; paused = 0; enemy_count = 0;
  wave_clear_clock = 0;
  music_clock = 0;
  night_note = 0;
  tower_hover_timer = 0;
  spawn_left = (uint8_t)(7 + day * 2 + (day == 5));
  spawn_clock = 30;
  for (i = 0; i < tower_count; i++) towers[i].cooldown = 2;
  sfx(4); set_message(8, day);
}

static void spawn_enemy(void) {
  Enemy *e;
  uint8_t roll;
  if (enemy_count >= MAX_ENEMIES || !spawn_left) return;
  roll = (uint8_t)(rand_next() >> 8);
  e = &enemies[enemy_count++];
  e->x = -8 * 256; e->y = 2 * TILE * 256 + 8 * 256;
  e->target = 1; e->slow = 0; e->poison = 0;
  if (day == 5 && spawn_left == 1) {
    e->kind = HORSEMAN; e->hp = 450; e->speed = 125; e->reward = 45;
  } else if (day >= 3 && roll < 48) {
    e->kind = GHOST; e->hp = (int16_t)(32 + day * 9); e->speed = 220; e->reward = 9;
  } else if (day >= 2 && roll < 98) {
    e->kind = SKELETON; e->hp = (int16_t)(28 + day * 8); e->speed = 285; e->reward = 8;
  } else if (day >= 3 && roll < 147) {
    e->kind = WEREWOLF; e->hp = (int16_t)(95 + day * 18); e->speed = 175; e->reward = 17;
  } else if (roll < 190) {
    e->kind = TRICKSTER; e->hp = (int16_t)(12 + day * 4); e->speed = 300; e->reward = 5;
  } else {
    e->kind = ZOMBIE; e->hp = (int16_t)(30 + day * 8); e->speed = 130; e->reward = 11;
  }
  e->max_hp = e->hp;
  spawn_left--;
}

static void enemy_remove(int index, int escaped) {
  int j;
  Enemy e = enemies[index];
  if (escaped) {
    if (pumpkins) pumpkins--;
    set_message(9, pumpkins);
    sfx(3);
  } else {
    candy += e.reward;
    set_message(10, e.reward);
    sfx(1);
  }
  for (j = index; j + 1 < enemy_count; j++) enemies[j] = enemies[j + 1];
  enemy_count--;
}

static void enemy_damage(int index, int damage) {
  enemies[index].hp -= (int16_t)damage;
  if (enemies[index].hp <= 0) enemy_remove(index, 0);
}

static void update_enemies(void) {
  int i;
  for (i = 0; i < enemy_count; i++) {
    Enemy *e = &enemies[i];
    Point p;
    int32_t tx, ty, step;
    if ((unsigned)e->target >= ROUTE_COUNT) { enemy_remove(i--, 1); continue; }
    p = route[(uint8_t)e->target];
    tx = (int32_t)(p.x * TILE + 8) * 256;
    ty = (int32_t)(p.y * TILE + 8) * 256;
    step = e->slow ? e->speed * 3 / 2 : e->speed * 3;
    if (e->x != tx) {
      if (e->x < tx) { e->x += step; if (e->x >= tx) e->x = tx; }
      else { e->x -= step; if (e->x <= tx) e->x = tx; }
    } else if (e->y != ty) {
      if (e->y < ty) { e->y += step; if (e->y >= ty) e->y = ty; }
      else { e->y -= step; if (e->y <= ty) e->y = ty; }
    } else e->target++;
    if (e->slow) e->slow--;
    if (e->poison) {
      e->poison--;
      if ((simulation_frame & 7) == 0) enemy_damage(i, 2);
      if (i >= enemy_count) break;
    }
  }
}

static int abs_i(int n) { return n < 0 ? -n : n; }

static void update_towers(void) {
  int i, j;
  for (i = 0; i < tower_count; i++) {
    Tower *t = &towers[i];
    int target = -1, best = -99999;
    int tx = t->x * TILE + 8, ty = t->y * TILE + 8;
    int range = tower_range[(uint8_t)t->kind] + (t->level - 1) * 7;
    if (t->kind == SCARECROW && t->level >= 3 && t->branch == 1) range = 22;
    if (t->cooldown) { t->cooldown--; continue; }
    if (t->kind == MOONFLOWER) continue;
    for (j = 0; j < enemy_count; j++) {
      int ex = enemies[j].x / 256, ey = enemies[j].y / 256;
      int dx = abs_i(ex - tx), dy = abs_i(ey - ty);
      int progress = enemies[j].target * 100 - dx - dy;
      if (enemies[j].kind == GHOST && t->kind != LANTERN && t->kind != CAULDRON) continue;
      if (dx + dy <= range && progress > best) { target = j; best = progress; }
    }
    if (target >= 0) {
      int damage = tower_damage[(uint8_t)t->kind] * (100 + (t->level - 1) * 42) / 100;
      int target_x = enemies[target].x / 256, target_y = enemies[target].y / 256;
      uint8_t shot_color = t->kind == CAULDRON ? 15 :
        t->kind == LANTERN ? 13 : t->kind == BAT_ROOST ? 12 : 18;
      spawn_projectile(tx, ty, target_x, target_y, shot_color);
      if (t->kind == CAULDRON) {
        enemies[target].slow = (uint8_t)(t->level >= 3 && t->branch == 0 ? 100 : 48);
        if (t->level >= 2) enemies[target].poison = 72;
      }
      if (t->kind == LANTERN && t->level >= 2) enemies[target].poison = 60;
      if (t->kind == BAT_ROOST && t->level >= 3 && t->branch == 1) damage *= 2;
      if (t->kind == LANTERN && t->level >= 3 && t->branch == 1) damage = damage * 3 / 2;
      if (t->kind == SCARECROW && t->level >= 3 && t->branch == 1) damage *= 2;
      if (t->kind == SCARECROW && t->level >= 3 && t->branch == 0) {
        int k;
        for (k = enemy_count - 1; k >= 0; k--) {
          int dx = abs_i(enemies[k].x / 256 - target_x);
          int dy = abs_i(enemies[k].y / 256 - target_y);
          if (dx + dy < 22) enemy_damage(k, damage / 2);
        }
      } else enemy_damage(target, damage);
      if (t->kind == LANTERN && t->level >= 3 && t->branch == 0) {
        int k;
        for (k = enemy_count - 1; k >= 0; k--) {
          if (abs_i(enemies[k].x / 256 - target_x) + abs_i(enemies[k].y / 256 - target_y) < 20)
            enemy_damage(k, damage / 2);
        }
      }
      if (t->kind == CAULDRON && t->level >= 3 && t->branch == 1) {
        int k;
        for (k = 0; k < enemy_count; k++) {
          if (abs_i(enemies[k].x / 256 - target_x) + abs_i(enemies[k].y / 256 - target_y) < 22)
            enemies[k].poison = 60;
        }
      }
      if (t->kind == BAT_ROOST && t->level >= 3 && t->branch == 0) {
        int k;
        for (k = enemy_count - 1; k >= 0; k--) {
          if (abs_i(enemies[k].x / 256 - target_x) + abs_i(enemies[k].y / 256 - target_y) < 18)
            enemy_damage(k, damage / 2);
        }
      }
      t->cooldown = (uint16_t)(tower_rate[(uint8_t)t->kind] * (t->level >= 3 ? 55 : 75) / 100);
    }
  }
  for (i = 0; i < tower_count; i++) {
    Tower *a = &towers[i];
    if (a->kind != MOONFLOWER) continue;
    for (j = 0; j < tower_count; j++) {
      Tower *t = &towers[j];
      if (t == a || t->kind == MOONFLOWER) continue;
      if (abs_i(t->x - a->x) + abs_i(t->y - a->y) <= 3 + a->level) {
        uint8_t boost = a->level >= 3 && a->branch == 1 ? 3 : 1;
        if (t->cooldown > boost) t->cooldown -= boost;
      }
    }
    if (a->level >= 3 && a->branch == 0) {
      for (j = 0; j < enemy_count; j++) {
        int dx = abs_i(enemies[j].x / 256 - a->x * TILE - 8);
        int dy = abs_i(enemies[j].y / 256 - a->y * TILE - 8);
        if (enemies[j].kind != HORSEMAN && dx + dy < 28) enemies[j].slow = 30;
      }
    }
  }
}

static void update_game(void) {
  if (mode != NIGHT || paused) return;
  simulation_frame++;
  update_projectiles();
  if (spawn_left) {
    if (spawn_clock) spawn_clock--;
    else { spawn_enemy(); spawn_clock = (uint16_t)(36 - day * 2); }
  }
  update_enemies();
  update_towers();
  if (pumpkins == 0) { mode = LOSE; stop_music(); return; }
  if (!spawn_left && !enemy_count) {
    if (++wave_clear_clock > 45) {
      wave_clear_clock = 0;
      if (day == 5) { mode = WIN; stop_music(); sfx(5); }
      else {
        stop_music();
        mode = DAY; day++; seeds += (uint8_t)((pumpkins + 2) / 3);
        candy += (uint16_t)(25 + pumpkins * 5);
        set_message(11, pumpkins);
      }
    }
  } else wave_clear_clock = 0;
}

static void update_camera(void) {
  if (cursor_x < camera_x + 2) camera_x = cursor_x - 2;
  if (cursor_x > camera_x + 12) camera_x = cursor_x - 12;
  if (cursor_y < camera_y + 2) camera_y = cursor_y - 2;
  if (cursor_y > camera_y + 5) camera_y = cursor_y - 5;
  if (camera_x < 0) camera_x = 0;
  if (camera_x > MAP_W - 15) camera_x = MAP_W - 15;
  if (camera_y < 0) camera_y = 0;
  if (camera_y > MAP_H - 8) camera_y = MAP_H - 8;
}

static void new_game(void) {
  int i;
  stop_music();
  mode = DAY; day = 1; candy = 150; seeds = 0; pumpkins = 8;
  selected = 0;
  tower_count = enemy_count = 0; camera_x = camera_y = 0;
  for (i = 0; i < MAX_PROJECTILES; i++) projectiles[i].age = 0;
  cursor_x = 2; cursor_y = 3; paused = 0; wave_clear_clock = 0;
  simulation_frame = 0;
  set_message(12, 0); sfx(7);
}

static void key_update(void) {
  uint16_t keys = (uint16_t)(~REG_KEYINPUT & 0x03FF);
  uint16_t pressed = keys & (uint16_t)~old_keys;
  uint16_t move = keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);
  uint16_t previous_x = cursor_x, previous_y = cursor_y;
  int repeat = 0;
  if (move) {
    held_frames++;
    if (held_frames == 1 || (held_frames > 4 && !(held_frames & 1))) repeat = 1;
  } else held_frames = 0;
  if (repeat) {
    if (keys & KEY_UP && cursor_y) cursor_y--;
    if (keys & KEY_DOWN && cursor_y < MAP_H - 1) cursor_y++;
    if (keys & KEY_LEFT && cursor_x) cursor_x--;
    if (keys & KEY_RIGHT && cursor_x < MAP_W - 1) cursor_x++;
    if (cursor_x != previous_x || cursor_y != previous_y) sfx(6);
  }
  if ((cursor_x != previous_x || cursor_y != previous_y) &&
      (message_code == 3 || message_code == 13 || message_code == 14) &&
      (cursor_x != message_tower_x || cursor_y != message_tower_y))
    message_timer = 0;
  if (mode == NIGHT && (cursor_x != previous_x || cursor_y != previous_y)) {
    Tower *hovered = tower_at(cursor_x, cursor_y);
    if (hovered) {
      tower_hover_kind = (uint8_t)hovered->kind;
      tower_hover_level = (uint8_t)hovered->level;
      tower_hover_branch = (uint8_t)hovered->branch;
      tower_hover_timer = 180;
    }
  }
  if (pressed & KEY_A) {
    if (mode == TITLE || mode == WIN || mode == LOSE) new_game();
    else place_or_upgrade();
  }
  if (pressed & KEY_B) sell_tower();
  if (pressed & KEY_SELECT) {
    Tower *t = tower_at(cursor_x, cursor_y);
    if (t) { t->branch ^= 1; set_message(13, t->branch); }
    else selected = (uint8_t)((selected + 1) % 5);
    sfx(8);
  }
  if (pressed & KEY_START) {
    if (mode == TITLE || mode == WIN || mode == LOSE) new_game();
    else if (mode == DAY) start_night();
    else if (mode == NIGHT) paused ^= 1;
  }
  old_keys = keys;
}

static void draw_tile(int x, int y, int mx, int my) {
  int path = route_map[mx][my];
  uint8_t grass = mode == DAY ? ((mx + my) & 1 ? 6 : 7) : ((mx + my) & 1 ? 1 : 2);
  unsigned detail = (unsigned)(mx * 17 + my * 31);
  rect(x, y, TILE, TILE, path ? (mode == DAY ? 21 : 22) : grass);
  if (path) {
    rect(x, y, TILE, 2, mode == DAY ? 18 : 24);
    rect(x, y + TILE - 2, TILE, 2, mode == DAY ? 20 : 23);
    rect(x + ((mx * 3) & 8), y + 7, 3, 1, mode == DAY ? 20 : 23);
  } else if (mode == DAY && detail % 19 == 3) {
    int leaf_x = x + 3 + (int)(detail % 9);
    int leaf_y = y + 4 + (int)((detail >> 3) % 8);
    rect(leaf_x, leaf_y, 3, 2, 17);
    rect(leaf_x + 2, leaf_y + 1, 2, 1, 18);
  } else if (mode == DAY && detail % 29 == 7) {
    rect(x + 6, y + 5, 2, 3, 28);
    rect(x + 4, y + 7, 6, 2, 18);
    rect(x + 5, y + 9, 4, 2, 17);
  } else if (mode == NIGHT && detail % 17 == 5) {
    int glow_x = x + 4 + (int)(detail % 9);
    int glow_y = y + 4 + (int)((detail >> 2) % 8);
    rect(glow_x, glow_y, 2, 2, 15);
    set_pixel(glow_x, glow_y, 13);
  } else if (detail % 7 == 2) {
    rect(x + ((mx * 7) & 12), y + ((my * 5) & 12), 2, 1, mode == DAY ? 8 : 3);
  }
}

static void draw_tower(Tower *t) {
  int x = (t->x - camera_x) * TILE, y = 24 + (t->y - camera_y) * TILE;
  int k = t->kind;
  uint8_t trim = 0;
  rect(x + 3, y + 14, 10, 2, mode == DAY ? 20 : 0);
  if (k == SCARECROW) {
    rect(x + 7, y + 6, 3, 10, 20); rect(x + 4, y + 3, 9, 8, 18);
    rect(x + 5, y + 2, 7, 6, 13); rect(x + 6, y + 4, 1, 1, 20);
    rect(x + 10, y + 4, 1, 1, 20); rect(x + 4, y + 11, 9, 2, 21);
    rect(x + 11, y + 5, 3, 2, 13);
    rect(x + 4, y + 2, 9, 1, 17);
  } else if (k == LANTERN) {
    draw_pumpkin(x + 8, y + 9, 0); rect(x + 6, y + 6, 2, 2, 13);
    rect(x + 9, y + 6, 2, 2, 13); rect(x + 7, y + 9, 3, 1, 13);
    rect(x + 7, y + 13, 3, 3, 20);
  } else if (k == CAULDRON) {
    rect(x + 4, y + 6, 9, 9, 25); rect(x + 3, y + 5, 11, 3, 19);
    rect(x + 6, y + 8, 5, 4, 15); rect(x + 7, y + 8, 2, 1, 13);
    rect(x + 7, y + 3, 2, 2, 15);
  } else if (k == MOONFLOWER) {
    rect(x + 7, y + 8, 2, 8, 15); rect(x + 4, y + 10, 3, 5, 16);
    rect(x + 9, y + 10, 4, 5, 16); rect(x + 6, y + 3, 5, 6, 12);
    rect(x + 7, y + 4, 3, 3, 13); rect(x + 5, y + 2, 2, 2, 18);
    rect(x + 10, y + 1, 2, 2, 18);
  } else {
    rect(x + 4, y + 9, 9, 6, 26); rect(x + 6, y + 5, 5, 5, 26);
    rect(x + 6, y + 2, 5, 5, 19); rect(x + 5, y + 2, 7, 2, 20);
    rect(x + 7, y + 4, 1, 1, 13); rect(x + 10, y + 4, 1, 1, 13);
    rect(x + 3, y + 11, 2, 3, 20); rect(x + 12, y + 11, 2, 3, 20);
  }
  if (t->level > 1) {
    trim = t->level > 2 ? (t->branch ? 31 : 22) : 25;
    outline(x + 2, y + 1, 12, 15, trim);
    rect(x + 5, y + 1, 2, 2, t->level > 2 ? trim : 13);
  }
  if (t->level > 2) {
    if (k == SCARECROW && t->branch) {
      rect(x + 4, y, 8, 2, 31);
      rect(x + 3, y + 1, 10, 1, 29);
      rect(x + 2, y + 7, 2, 2, 31);
      rect(x + 12, y + 7, 2, 2, 31);
    } else if (k == SCARECROW) {
      rect(x + 2, y + 6, 3, 2, 22);
      rect(x + 11, y + 6, 3, 2, 22);
      rect(x + 5, y + 1, 6, 2, 22);
    } else {
      rect(x + 9, y + 1, 2, 2, trim);
      rect(x + 2, y + 3, 2, 2, trim);
    }
  }
}

static void draw_enemy(Enemy *e) {
  int x = e->x / 256 - camera_x * TILE;
  int y = 24 + e->y / 256 - camera_y * TILE;
  int k = e->kind;
  if (x < -12 || x > 250 || y < 10 || y > 160) return;
  rect(x - 5, y + 6, 10, 2, 0);
  if (k == GHOST) {
    rect(x - 5, y - 6, 10, 9, 12); rect(x - 4, y - 8, 8, 2, 12);
    rect(x - 6, y - 4, 12, 4, 12); rect(x - 3, y - 3, 2, 2, 26);
    rect(x + 2, y - 3, 2, 2, 26); rect(x - 2, y + 2, 3, 1, 13);
  } else if (k == SKELETON) {
    rect(x - 4, y - 7, 8, 7, 13); rect(x - 3, y - 5, 2, 2, 20);
    rect(x + 2, y - 5, 2, 2, 20); rect(x - 1, y, 2, 6, 21);
    rect(x - 5, y + 1, 10, 2, 21); rect(x - 3, y + 6, 2, 3, 21);
    rect(x + 2, y + 6, 2, 3, 21);
  } else if (k == TRICKSTER) {
    rect(x - 4, y - 3, 8, 8, 19); rect(x - 4, y - 8, 8, 6, 18);
    rect(x - 5, y - 9, 10, 2, 20); rect(x - 2, y - 1, 1, 1, 20);
    rect(x + 2, y - 1, 1, 1, 20); rect(x - 3, y + 5, 2, 3, 18);
    rect(x + 2, y + 5, 2, 3, 18);
  } else if (k == WEREWOLF) {
    rect(x - 5, y - 1, 10, 7, 25); rect(x - 5, y - 8, 10, 8, 18);
    rect(x - 5, y - 11, 3, 4, 18); rect(x + 3, y - 11, 3, 4, 18);
    rect(x - 3, y - 5, 2, 2, 18); rect(x + 2, y - 5, 2, 2, 18);
    rect(x - 2, y - 1, 4, 2, 20); rect(x - 4, y + 5, 3, 4, 25);
    rect(x + 2, y + 5, 3, 4, 25);
  } else if (k == HORSEMAN) {
    rect(x - 7, y, 14, 9, 20); rect(x - 6, y - 4, 12, 5, 20);
    rect(x - 4, y - 10, 8, 7, 26); rect(x - 3, y - 8, 2, 2, 17);
    rect(x + 2, y - 8, 2, 2, 17); rect(x - 5, y - 12, 10, 3, 20);
    rect(x - 3, y - 14, 6, 3, 20); rect(x + 5, y - 8, 8, 2, 13);
    rect(x + 11, y - 10, 2, 5, 13);
  } else {
    rect(x - 5, y - 1, 10, 7, 25); rect(x - 4, y - 7, 8, 7, 15);
    rect(x - 3, y - 9, 6, 3, 15); rect(x - 3, y - 5, 2, 2, 18);
    rect(x + 2, y - 5, 2, 2, 18); rect(x - 2, y - 1, 4, 1, 20);
    rect(x - 4, y + 5, 3, 3, 25); rect(x + 2, y + 5, 3, 3, 25);
  }
  rect(x - 7, y - 14, 14, 2, 20);
  {
    int health = 14 * e->hp / e->max_hp;
    if (health < 1) health = 1;
    rect(x - 7, y - 14, health, 1, e->hp * 2 < e->max_hp ? 14 : 15);
  }
}

static void draw_projectiles(void) {
  int i;
  for (i = 0; i < MAX_PROJECTILES; i++) {
    Projectile *p = &projectiles[i];
    int x, y, tail_x, tail_y;
    if (!p->age) continue;
    x = p->sx + (p->tx - p->sx) * p->age / PROJECTILE_FRAMES - camera_x * TILE;
    y = 24 + p->sy + (p->ty - p->sy) * p->age / PROJECTILE_FRAMES - camera_y * TILE;
    tail_x = p->sx + (p->tx - p->sx) * (p->age - 1) / PROJECTILE_FRAMES - camera_x * TILE;
    tail_y = 24 + p->sy + (p->ty - p->sy) * (p->age - 1) / PROJECTILE_FRAMES - camera_y * TILE;
    set_pixel(tail_x, tail_y, p->color);
    set_pixel(x, y, p->color);
    rect(x - 1, y - 1, 3, 3, p->color);
    set_pixel(x, y, 13);
  }
}

static const char *tower_name(uint8_t kind, int level, int branch);
static void draw_number(int value, int x, int y, uint8_t color);
static void draw_tower_stats_at(uint8_t kind, int level, int branch,
  int x, int y, uint8_t color);
static void draw_tower_info(uint8_t kind, int level, int branch, int x, int y, uint8_t color);

static void draw_hud(void) {
  uint8_t bg = mode == DAY ? 24 : 1;
  uint8_t hi = mode == DAY ? 18 : 12;
  rect(0, 0, SCREEN_W, 24, bg); rect(0, 23, SCREEN_W, 1, hi);
  if (mode == DAY) {
    char day_text[2] = {(char)('0' + day), 0};
    text("DAY:", 3, 3, 18, 1);
    text(day_text, 24, 3, 13, 1);
    text("BUILD", 34, 3, 13, 1);
  } else {
    char day_text[2] = {(char)('0' + day), 0};
    text("NIGHT - WAVE", 3, 3, 12, 1);
    text(day_text, 69, 3, 13, 1);
  }
  draw_candy(86, 2); {
    char value[6]; int n = 0, v = candy;
    do { value[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 5);
    { int a; for (a = 0; a < n / 2; a++) { char c = value[a]; value[a] = value[n - 1 - a]; value[n - 1 - a] = c; } }
    value[n] = 0; text(value, 92, 3, 18, 1);
  }
  draw_seed(120, 2); {
    char seed_text[4] = {(char)('0' + seeds / 10), (char)('0' + seeds % 10), 0, 0};
    text(seed_text[0] == '0' ? seed_text + 1 : seed_text, 126, 3, 15, 1);
  }
  draw_pumpkin(156, 11, 0);
  {
    char pumpkin_count[2] = {(char)('0' + pumpkins), 0};
    text("X", 165, 3, 13, 1);
    text(pumpkin_count, 171, 3, 18, 1);
  }
  if (mode == DAY) {
    Tower *hovered = tower_at(cursor_x, cursor_y);
    if (hovered) draw_tower_info((uint8_t)hovered->kind, hovered->level,
      hovered->branch, 3, 14, 18);
    else {
      const char *name = tower_name(selected, 1, 0);
      int cost_x;
      text("A -", 3, 14, 13, 1);
      text(name, 18, 14, 18, 1);
      cost_x = 18 + length(name) * 5 + 5;
      draw_number(tower_cost[selected], cost_x, 14, 18);
      cost_x += (tower_cost[selected] >= 100 ? 3 : 2) * 5 + 5;
      draw_tower_stats_at(selected, 1, 0, cost_x, 14, 18);
    }
    text("START DUSK", 184, 14, 13, 1);
  } else {
    if (tower_hover_timer) {
      draw_tower_info(tower_hover_kind, tower_hover_level, tower_hover_branch, 3, 14, 18);
    } else {
      text(paused ? "PAUSED  ARROWS - PAN" : "START - PAUSE  ARROWS - PAN", 3, 14, 13, 1);
    }
    text("MOBS", 177, 14, 13, 1);
    {
      uint8_t remaining = (uint8_t)(enemy_count + spawn_left);
      char nums[3] = {(char)('0' + remaining / 10), (char)('0' + remaining % 10), 0};
      text(remaining < 10 ? nums + 1 : nums, 204, 14, 18, 1);
    }
  }
}

static int tower_speed(int kind, int level) {
  int rate = tower_rate[kind];
  if (!rate) return 0;
  rate = rate * (level >= 3 ? 55 : 75) / 100;
  return 120 / (rate ? rate : 1);
}

static const char *tower_name(uint8_t kind, int level, int branch) {
  if (level >= 3) {
    if (kind == SCARECROW) return branch ? "STRAW SENTINEL" : "CROW CALLER";
    if (kind == LANTERN) return branch ? "WILL O WISP" : "INFERNO JACK";
    if (kind == CAULDRON) return branch ? "HEX POT" : "CANDY GLUE";
    if (kind == MOONFLOWER) return branch ? "STAR BLOOM" : "BLOOD MOON";
    return branch ? "VAMPIRE BATS" : "NIGHT SWARM";
  }
  if (kind == SCARECROW) return "SCARECROW";
  if (kind == LANTERN) return "LANTERN";
  if (kind == CAULDRON) return "CAULDRON";
  if (kind == MOONFLOWER) return "MOONFLOWER";
  return "BAT ROOST";
}

static void draw_number(int value, int x, int y, uint8_t color) {
  char digits[4];
  int n = 0, i;
  do { digits[n++] = (char)('0' + value % 10); value /= 10; } while (value && n < 3);
  for (i = n - 1; i >= 0; i--) {
    char digit[2] = {digits[i], 0};
    text(digit, x, y, color, 1);
    x += 5;
  }
}

static const char *tower_effect(uint8_t kind, int level, int branch) {
  if (kind == LANTERN) return level >= 2 ? "poison" : "burn";
  if (kind == CAULDRON) return level >= 2 ? "poison" : "slow";
  if (kind == MOONFLOWER) return level >= 3 && branch ? "fear" : "buff";
  if (kind == BAT_ROOST) return level >= 3 ? (branch ? "drain" : "swarm") : "swarm";
  if (kind == SCARECROW && level >= 3) return branch ? "swat" : "flock";
  return "-";
}

static void draw_tower_stats_at(uint8_t kind, int level, int branch, int x, int y, uint8_t color) {
  char stats[24] = "D";
  int damage = tower_damage[kind] * (100 + (level - 1) * 42) / 100;
  int speed = tower_speed(kind, level), pos = 1;
  const char *effect;
  if (level >= 3 && kind == SCARECROW && branch) damage *= 2;
  if (level >= 3 && kind == LANTERN && branch) damage = damage * 3 / 2;
  if (level >= 3 && kind == BAT_ROOST && branch) damage *= 2;
  effect = tower_effect(kind, level, branch);
  if (damage >= 10) stats[pos++] = (char)('0' + damage / 10);
  stats[pos++] = (char)('0' + damage % 10);
  stats[pos++] = '/'; stats[pos++] = 'S';
  stats[pos++] = (char)('0' + speed);
  stats[pos++] = '/';
  while (*effect) stats[pos++] = *effect++;
  stats[pos] = 0;
  text(stats, x, y, color, 1);
}

static void draw_tower_info(uint8_t kind, int level, int branch, int x, int y, uint8_t color) {
  const char *name = tower_name(kind, level, branch);
  text(name, x, y, color, 1);
  draw_tower_stats_at(kind, level, branch, x + length(name) * 5 + 5, y, color);
}

static void draw_message(void) {
  const char *s = "";
  Tower *hovered = tower_at(cursor_x, cursor_y);
  int evolution_message = message_timer && (message_code == 13 || message_code == 14);
  int evolution_active = evolution_message && hovered &&
    cursor_x == message_tower_x && cursor_y == message_tower_y;
  int detail_message = message_timer &&
    ((message_code == 3 && hovered && cursor_x == message_tower_x &&
      cursor_y == message_tower_y) || (evolution_message && evolution_active));
  switch (message_code) {
    case 1: s = "FULLY EVOLVED"; break;
    case 2: s = "NEED MORE CANDY"; break;
    case 3: s = "TOWER UPGRADED"; break;
    case 4: s = "BUILD ON CLEAR GRASS"; break;
    case 5: s = "TOWER PLANTED"; break;
    case 6: s = "TOWER SOLD"; break;
    case 7: s = "NO TOWER HERE"; break;
    case 8: s = "THE PARADE IS COMING"; break;
    case 9: s = "PUMPKIN EATEN"; break;
    case 10: s = "CANDY CORN EARNED"; break;
    case 11: s = "DAWN! PATCH SURVIVED"; break;
    case 12: s = "DEFEND THE PUMPKINS"; break;
    case 13: s = "EVOLUTION SELECTED"; break;
    case 14: s = "TOWER EVOLVED"; break;
    case 15: s = "NEED MOON SEEDS"; break;
    default: s = ""; break;
  }
  if (detail_message && hovered) {
    const int projected = message_code == 13;
    int level = projected ? 3 : hovered->level;
    const char *label = message_code == 13 ? "BRANCH -" :
      message_code == 14 ? "EVOLVED -" : "UPGRADE -";
    const char *name = tower_name((uint8_t)hovered->kind, level, hovered->branch);
    int name_x = 4 + length(label) * 5 + 2;
    int cost_x = name_x + length(name) * 5 + 7;
    int cost = message_cost;
    rect(0, 151, SCREEN_W, 9, mode == DAY ? 24 : 1);
    text(label, 4, 153, 13, 1);
    text(name, name_x, 153, 18, 1);
    text("-", cost_x, 153, 13, 1);
    cost_x += 7;
    draw_number(cost, cost_x, 153, 18);
    cost_x += (cost >= 100 ? 3 : 2) * 5 + 5;
    draw_tower_stats_at((uint8_t)hovered->kind, level, hovered->branch,
      cost_x, 153, 18);
  } else if (message_timer && s[0] && (!evolution_message || evolution_active)) {
    rect(0, 151, SCREEN_W, 9, mode == DAY ? 24 : 1);
    text(s, 4, 153, 13, 1);
    if (message_code == 2 || message_code == 6 || message_code == 9 || message_code == 10) {
      char value[4] = {(char)('0' + message_value / 10), (char)('0' + message_value % 10), 0, 0};
      text(message_value < 10 ? value + 1 : value, 100, 153, 18, 1);
    }
  } else {
    int x;
    int cost;
    int stat_kind = selected, stat_level = 1, stat_branch = 0;
    rect(0, 151, SCREEN_W, 9, mode == DAY ? 24 : 1);
    if (mode == DAY) {
      text("A PLANT/UPGRADE B SELL", 4, 153, 13, 1);
      x = 4 + length("A PLANT/UPGRADE B SELL") * 5 + 3;
      if (hovered) {
        stat_kind = (uint8_t)hovered->kind;
        stat_level = hovered->level;
        stat_branch = hovered->branch;
        cost = hovered->level >= 3 ? 0 : 35 + hovered->level * 25;
      } else cost = tower_cost[selected];
      text("-", x, 153, 13, 1); x += 7;
      if (cost) draw_number(cost, x, 153, 18);
      else text("MAX", x, 153, 18, 1);
      x += (cost >= 100 ? 3 : 2) * 5 + 5;
      draw_tower_stats_at((uint8_t)stat_kind, stat_level, stat_branch, x, 153, 18);
    } else {
      text(!spawn_left && !enemy_count ? "DAWN IN 1 SEC" : "DAWN WHEN MOBS 0", 4, 153, 13, 1);
    }
  }
}

static void render_world(void) {
  int x, y, i;
  for (y = 0; y < 8; y++)
    for (x = 0; x < 15; x++) {
      int mx = camera_x + x, my = camera_y + y;
      draw_tile(x * TILE, 24 + y * TILE, mx, my);
    }
  for (i = 0; i < tower_count; i++) draw_tower(&towers[i]);
  for (i = 0; i < enemy_count; i++) draw_enemy(&enemies[i]);
  draw_projectiles();
  {
    int cx = (cursor_x - camera_x) * TILE, cy = 24 + (cursor_y - camera_y) * TILE;
    uint8_t c = mode == DAY ? 13 : 12;
    outline(cx + 1, cy + 1, 14, 14, c);
    rect(cx + 1, cy + 1, 3, 1, c); rect(cx + 1, cy + 1, 1, 3, c);
  }
  if (mode == NIGHT) {
    rect(214, 27, 11, 11, 13); rect(211, 31, 12, 4, 13);
    rect(218, 29, 2, 2, 2); rect(221, 34, 2, 2, 2);
    for (i = 0; i < 8; i++) {
      int sx = (i * 47 + 13) % SCREEN_W;
      int sy = 27 + ((i * 29 + frame_count / 8) % 118);
      set_pixel(sx, sy, (i & 1) ? 12 : 13);
    }
  }
}

static void draw_title(void) {
  int i, x, y;
  rect(0, 0, SCREEN_W, SCREEN_H, 1);
  for (i = 0; i < 36; i++) {
    x = (i * 73 + 19) % SCREEN_W; y = (i * 37 + 8) % 100;
    set_pixel(x, y, i % 4 ? 12 : 13);
  }
  rect(203, 17, 13, 2, 13); rect(199, 19, 21, 4, 13);
  rect(196, 23, 25, 12, 13); rect(199, 35, 21, 4, 13);
  rect(203, 39, 13, 2, 13); rect(201, 27, 3, 3, 1);
  rect(210, 31, 3, 3, 1); rect(216, 24, 2, 2, 12);
  centered("PUMPKIN", 35, 18, 3);
  centered("PARADE", 53, 17, 3);
  centered("GUARDIAN OF THE PATCH", 72, 15, 1);
  draw_pumpkin(120, 101, 0);
  rect(59, 91, 4, 40, 20); rect(52, 85, 17, 13, 18); rect(54, 83, 13, 10, 13);
  rect(56, 87, 2, 2, 20); rect(63, 87, 2, 2, 20); rect(50, 96, 21, 2, 13);
  rect(169, 96, 12, 12, 12); rect(171, 93, 8, 3, 12);
  rect(171, 99, 2, 2, 26); rect(177, 99, 2, 2, 26);
  text("5 NIGHTS  5 TOWERS", 64, 124, 13, 1);
  centered("A / START TO BEGIN", 143 + ((frame_count >> 3) & 1), 18, 1);
  text("CHIPTUNE ON", 81, 154, 12, 1);
}

static void draw_end(void) {
  rect(20, 48, 200, 61, 1); outline(20, 48, 200, 61, mode == WIN ? 18 : 17);
  centered(mode == WIN ? "PATCH PROTECTED" : "PATCH OVERRUN", 62, mode == WIN ? 18 : 17, 2);
  centered(mode == WIN ? "THE HARVEST IS YOURS" : "THE PARADE GOT THROUGH", 80, 13, 1);
  centered("A / START TO PLAY AGAIN", 97, 12, 1);
}

static void render(void) {
  frame = (volatile uint8_t *)(0x06000000 + (back_page ? 0xA000 : 0));
  if (mode == TITLE) draw_title();
  else {
    draw_hud();
    render_world();
    draw_message();
    if (mode == WIN || mode == LOSE) draw_end();
    if (paused) {
      rect(46, 62, 148, 34, 1); outline(46, 62, 148, 34, 18);
      centered("PARADE PAUSED", 72, 18, 2);
      centered("START TO RESUME", 86, 13, 1);
    }
  }
  while (REG_VCOUNT >= 160) { }
  while (REG_VCOUNT < 160) { }
  REG_DISPCNT = (uint16_t)(0x0404 | (back_page ? 0x0010 : 0));
  back_page ^= 1;
}

int main(void) {
  unsigned i, tick;
  mode = TITLE; day = 1; pumpkins = 8; candy = 150; random_seed = 0x3d71;
  cursor_x = 2; cursor_y = 3;
  back_page = 0;
  wave_clear_clock = 0;
  REG_DISPCNT = 0x0404;
  for (i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) COLOR_MEM[i] = colors[i];
  REG_SOUNDCNT_X = 0x0080;
  REG_SOUNDCNT_L = 0xFF77;
  build_route_map();
  for (;;) {
    key_update();
    for (tick = 0; tick < 2; tick++) update_game();
    update_camera();
    if (message_timer) message_timer--;
    if (tower_hover_timer) tower_hover_timer--;
    frame_count++;
    music_update();
    render();
  }
  return 0;
}

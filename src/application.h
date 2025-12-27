// application.c
typedef struct {
  float x, y;
} Position;

#define GAME_UPDATE(name) void name(Position *pos)
typedef GAME_UPDATE(GameUpdate);
GAME_UPDATE(game_update_stub) {
  (void)pos;
}

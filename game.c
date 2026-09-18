#include "sim.h"
#include "fb.h"
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

static struct termios oldt;
static int raw, player, level, coins, got, dead;
static int goal = -1;
static float t0;

static const char *paths[] = {
    "/home/working/cMarble/levels/1.scene",
    "/home/working/cMarble/levels/2.scene",
    "/home/working/cMarble/levels/3.scene",
    "levels/1.scene", "levels/2.scene", "levels/3.scene",
};

static void io_open(void)
{
    struct termios t;
    tcgetattr(0, &oldt);
    t = oldt;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &t);
    raw = 1;
}

static void io_close(void)
{
    if (raw)
        tcsetattr(0, TCSANOW, &oldt);
}

static int load_level(Sim *s, int lv)
{
    char a[128], b[128];
    int i;
    snprintf(a, sizeof a, "/home/working/cMarble/levels/%d.scene", lv);
    snprintf(b, sizeof b, "levels/%d.scene", lv);
    if (!sim_load(s, a) && !sim_load(s, b))
        return 0;
    player = 0;
    goal = -1;
    coins = got = dead = 0;
    for (i = 0; i < s->n; i++) {
        if (s->b[i].layer == 2) {
            coins++;
            s->b[i].col = 3;
            s->b[i].mask = 0;
        }
        if (s->b[i].layer == 4) {
            goal = i;
            s->b[i].col = 2;
            s->b[i].mask = 0;
        }
    }
    snprintf(s->banner, sizeof s->banner, "cMarble L%d", lv);
    return 1;
}

static void tick_game(Sim *s)
{
    int hit[16], n, i;
    if (dead)
        return;
    if (s->b[player].y < -1.5f) {
        dead = 1;
        snprintf(s->hud, sizeof s->hud, "fell — R retry");
        return;
    }
    n = sim_query(s, s->b[player].x, s->b[player].y, s->b[player].z, 0.7f, hit, 16);
    for (i = 0; i < n; i++) {
        int id = hit[i];
        if (id == player)
            continue;
        if (s->b[id].layer == 2 && s->b[id].y > -10.f) {
            s->b[id].y = -20.f;
            s->b[id].awake = 0;
            got++;
        }
        if (s->b[id].layer == 4 && got >= coins)
            dead = 2; /* win */
    }
}

int main(void)
{
    Sim s;
    int run = 1;
    float acc = 0;
    struct timeval tv0, tv1;
    if (fb_open() < 0) {
        fprintf(stderr, "cMarble needs /dev/fb0\n");
        return 1;
    }
    level = 1;
    if (!load_level(&s, level)) {
        fprintf(stderr, "missing levels/1.scene\n");
        return 1;
    }
    io_open();
    gettimeofday(&tv0, NULL);
    t0 = 0;
    while (run) {
        unsigned char buf[16];
        int n = (int)read(0, buf, sizeof buf), i;
        float fx = 0, fz = 0, fy = 0, cy = cosf(s.yaw), sy = sinf(s.yaw);
        for (i = 0; i < n; i++) {
            unsigned char c = buf[i];
            if (c == 'q' || c == 'Q')
                s.yaw -= 0.12f;
            else if (c == 'e' || c == 'E')
                s.yaw += 0.12f;
            else if (c == 'a' || c == 'A' || (c == 0x1b && i + 2 < n && buf[i + 2] == 'D'))
                fx -= 1;
            else if (c == 'd' || c == 'D' || (c == 0x1b && i + 2 < n && buf[i + 2] == 'C'))
                fx += 1;
            else if (c == 'w' || c == 'W' || (c == 0x1b && i + 2 < n && buf[i + 2] == 'A'))
                fz += 1;
            else if (c == 's' || c == 'S' || (c == 0x1b && i + 2 < n && buf[i + 2] == 'B'))
                fz -= 1;
            else if (c == ' ')
                fy = 1;
            else if (c == '+' || c == '=') {
                s.foc *= 1.12f;
                if (s.foc > 420)
                    s.foc = 420;
            } else if (c == '-') {
                s.foc /= 1.12f;
                if (s.foc < 90)
                    s.foc = 90;
            } else if (c == 'r' || c == 'R')
                load_level(&s, level);
            else if (c == 'n' || c == 'N') {
                if (level < 3)
                    load_level(&s, ++level);
            } else if (c == '1')
                s.debug ^= 1;
            else if (c == 'x' || c == 'X')
                run = 0;
            if (c == 0x1b && i + 2 < n)
                i += 2;
        }
        if (!dead && (fx || fz || fy))
            sim_kick(&s, player, (fx * cy + fz * sy) * 14.f, fy * 22.f,
                     (-fx * sy + fz * cy) * 14.f);

        gettimeofday(&tv1, NULL);
        {
            float frame = (tv1.tv_sec - tv0.tv_sec) + (tv1.tv_usec - tv0.tv_usec) * 1e-6f;
            if (frame < 0.001f)
                frame = 0.001f;
            if (frame > 0.05f)
                frame = 0.05f;
            s.fps = 1.f / frame;
            acc += frame;
            t0 += frame;
            tv0 = tv1;
        }
        while (acc >= 1.f / 60.f) {
            sim_step(&s, 1.f / 60.f);
            tick_game(&s);
            acc -= 1.f / 60.f;
        }
        if (dead == 2) {
            snprintf(s.hud, sizeof s.hud, "CLEAR  %.0fs  N next  R retry", t0);
            if (level < 3) {
                /* stay until N */
            }
        } else if (!dead)
            snprintf(s.hud, sizeof s.hud, "gems %d/%d   %.0fs   R retry", got, coins, t0);
        sim_cam_follow(&s, player);
        sim_draw(&s);
        usleep(8000);
    }
    io_close();
    fb_close();
    return 0;
}

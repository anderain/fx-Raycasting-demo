#include "fxlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

#define MY_PI 3.1415926
#define ABS(a) ((a) >= 0 ? (a) : -(a))
#define DEG_TO_RAD(a) ((a) * MY_PI / 180)
#define RAD_TO_DEG(a) ((a) * 180 / MY_PI)
#define EPS 1.0e-8f

#define MYKEY_UP            2,9
#define MYKEY_DOWN          3,8
#define MYKEY_LEFT          3,9
#define MYKEY_RIGHT         2,8
#define MYKEY_ENTER         3,2
#define MYKEY_ESC           4,8

#define MYKEY_NUM_0         7,2
#define MYKEY_NUM_1         7,3
#define MYKEY_NUM_2         6,3
#define MYKEY_NUM_3         5,3
#define MYKEY_NUM_4         7,4
#define MYKEY_NUM_5         6,4
#define MYKEY_NUM_6         5,4
#define MYKEY_NUM_7         7,5
#define MYKEY_NUM_8         6,5
#define MYKEY_NUM_9         5,5
#define MYKEY_CHAR_PLUS     4,3
#define MYKEY_CHAR_MINUS    3,3

extern const unsigned char mapobj_tEXTURE_0[];
extern const unsigned char mapobj_tEXTURE_1[];

struct {
    float x;
    float y;
    float dist;
    int view;    // DEG
    int forward; // DEG
} Camera = {
    0.0f, -1.0f, 10.0f, 90, 90
};

#define e 4
typedef struct tag_mapobj_t{
    float sx;
    float sy;
    float ex;
    float ey;
    int type;
    unsigned char *texture;
} mapobj_t;

mapobj_t wall[] = {
    {-e,e,e,e, 0, mapobj_tEXTURE_0},
    {e,e,e,-e, 0, mapobj_tEXTURE_0},
    {e,-e,-e,-e, 0, mapobj_tEXTURE_0},
    {-e,-e,-e,e, 0, mapobj_tEXTURE_0},
    {1,2,1,4, 0, mapobj_tEXTURE_1},
    {1,2,3,2, 0, mapobj_tEXTURE_1},
    {-2,0,-2,2, 0, mapobj_tEXTURE_1},
    {-2,2,-1,2, 0, mapobj_tEXTURE_1},
    {-2,-1,-2,-3, 0, mapobj_tEXTURE_1},
    {-2,-3,0,-3, 0, mapobj_tEXTURE_1},
    {1,-1,1,-2, 0, mapobj_tEXTURE_1},
    {1,-2,3,-2, 0, mapobj_tEXTURE_1}
};

unsigned char *screen_buffer;

#define SCREEN_BUFFER(x, y) (screen_buffer[((y) << 7) + (x)])

float PRE_SIN[360];
float PRE_COS[360];

int tri_arg;
float tri_ret;

void MY_SIN() {
    if (tri_arg < 0)
        tri_ret = -PRE_SIN[(-tri_arg) % 360];
    else
        tri_ret = PRE_SIN[tri_arg % 360];
}

void MY_COS() {
    if (tri_arg < 0)
        tri_ret = PRE_COS[(-tri_arg) % 360];
    else
        tri_ret = PRE_COS[tri_arg % 360];
}

void initTri() {
    int angle;
    for (angle = 0; angle < 360; ++angle) {
        float rad_angle = DEG_TO_RAD(angle);
        PRE_SIN[angle] = sin(rad_angle);
        PRE_COS[angle] = cos(rad_angle);
    }
}
// distance args
float d_arg_ax, d_arg_ay, d_arg_bx, d_arg_by;
float d_ret;
// distance def
void distance() {
    d_ret = sqrt((d_arg_ax - d_arg_bx) * (d_arg_ax - d_arg_bx)
               + (d_arg_ay - d_arg_by) * (d_arg_ay - d_arg_by));
}

// intersection args
float i_arg_ax, i_arg_ay, i_arg_bx, i_arg_by;
float i_arg_cx, i_arg_cy, i_arg_dx, i_arg_dy;
float *i_arg_p_isx, *i_arg_p_isy;
int i_ret;
// intersection def
void intersection() {

    float denominator = (i_arg_by - i_arg_ay) * (i_arg_dx - i_arg_cx)
                      - (i_arg_ax - i_arg_bx) * (i_arg_cy - i_arg_dy);
    float x, y;

    i_ret = 0;

    if (ABS(denominator) < EPS) {
        return;  
    }

    x = ((i_arg_bx - i_arg_ax) * (i_arg_dx - i_arg_cx) *(i_arg_cy - i_arg_ay)
      +  (i_arg_by - i_arg_ay) * (i_arg_dx - i_arg_cx) * i_arg_ax
      -  (i_arg_dy - i_arg_cy) * (i_arg_bx - i_arg_ax) * i_arg_cx ) / denominator;
    y =-((i_arg_by - i_arg_ay) * (i_arg_dy - i_arg_cy) *(i_arg_cx - i_arg_ax)
      +  (i_arg_bx - i_arg_ax) * (i_arg_dy - i_arg_cy) * i_arg_ay
      -  (i_arg_dx - i_arg_cx) * (i_arg_by - i_arg_ay) * i_arg_cy ) / denominator;

    if ((x - i_arg_ax) * (x - i_arg_bx) <= 0 && (y - i_arg_ay) * (y - i_arg_by) <= 0
     && (x - i_arg_cx) * (x - i_arg_dx) <= 0 && (y - i_arg_cy) * (y - i_arg_dy) <= 0) {
        *i_arg_p_isx = x;
        *i_arg_p_isy = y;
        i_ret = 1;
        return;
    }
}

int iskeydown(int code1, int code2) {
	int kcode1, kcode2; short unused = 0;
	if (Bkey_GetKeyWait(&kcode1, &kcode2, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &unused)) {
		if (kcode1 == code1 && kcode2 == code2)
			return 1;
	}
	return 0;
}

int draw_step = 1;
int moving_draw_step = 4;

void redraw() {
    int col_max = 128;
    int row_max = 64;
    int col;
    int i, j, k;
    // float ray_sx;
    // float ray_sy;
    float ray_ex;
    float ray_ey;
    float isx = 0.0f, isy = 0.0f;
    // --- for draw ---
    int r_top, s_top;
    int r_bottom, s_bottom;
    int row;
    int height;
    int tx, ty;
    mapobj_t *nearest_wall, *w;

    Bdisp_AllClr_VRAM();
    memset(screen_buffer, 0, 128 * 64);

    // for (i = 0; i < 8; ++i) {
    //     for (j = 0; j < col_max; ++j) {
    //         Bdisp_SetPoint_VRAM(j, 31 + (i << 2), 1);
    //     }
    // }

    for (col = draw_step - 1; col < col_max; col += draw_step) {
        int theta = (Camera.view * (col - 64)) >> 7;
        int alpha = Camera.forward + theta;
        
        float nearest = Camera.dist + 1;
        float nearestIsx;
        float nearestIsy;

        nearest_wall = NULL;

        tri_arg = alpha; MY_COS(); ray_ex = Camera.x + Camera.dist * tri_ret;
        tri_arg = alpha; MY_SIN(); ray_ey = Camera.y + Camera.dist * tri_ret;
        
        for (i = 0; i < sizeof(wall) / sizeof(wall[0]); ++i) {
            w = wall + i;
            
            i_arg_ax = Camera.x;    i_arg_ay = Camera.y;
            i_arg_bx = ray_ex;      i_arg_by = ray_ey;
            i_arg_cx = w->sx;       i_arg_cy = w->sy;
            i_arg_dx = w->ex;       i_arg_dy = w->ey;
            i_arg_p_isx = &isx;     i_arg_p_isy = &isy;

            intersection();

            if (i_ret) {
                float d;
                d_arg_ax = Camera.x;    d_arg_ay = Camera.y;
                d_arg_bx = isx;         d_arg_by = isy;
                distance();
                tri_arg = theta;
                MY_COS();
                d = d_ret * tri_ret;
                if (d < nearest) {
                    nearest = d;
                    nearest_wall = w;
                    nearestIsx = isx;
                    nearestIsy = isy;
                }
            }
        }

        if (nearest_wall != NULL) {
            int tex_left, tex_top, tex_pxl;
            float delta, left_p;
            int depth = nearest * 10;

            s_top = (row_max >> 1) - 36 / nearest;
            s_bottom = (row_max >> 1) + 36 / nearest;
            height = s_bottom - s_top;
            w = nearest_wall;

            delta = w->ex - w->sx;
            delta = ABS(delta);

            if (delta > EPS) {
                left_p = (nearestIsx - w->sx) / (w->sx - w->ex);
            }
            else {
                left_p = (nearestIsy - w->sy) / (w->sy - w->ey);
            }
            left_p = ABS(left_p);

            d_arg_ax = w->sx;    d_arg_ay = w->sy;
            d_arg_bx = w->ex;    d_arg_by = w->ey;
            distance();
            tex_left = left_p * 32 * d_ret;
            tex_left = tex_left & 31;

            r_top = s_top < 0 ? 0 : s_top;
            r_bottom = s_bottom >= row_max ? row_max - 1 : s_bottom;

            for (row = r_top; row <= r_bottom; row += draw_step) {
                tex_top = (((row - s_top) << 5) - 1) / height;
                tex_pxl = *(w->texture + (tex_top << 2) + (tex_left >> 3));
                tex_pxl = (tex_pxl >> (tex_left & 7)) & 1;
                for (j = 0; j < draw_step; ++j) {
                    for (k = 0; k < draw_step; ++k) {
                        tx = col_max - col - 1 + j;
                        ty = row + k;
                        SCREEN_BUFFER(tx, ty) = depth + (tex_pxl << 6);
                        // Bdisp_SetPoint_VRAM(tx, ty, tex_pxl);
                    }
                }
            }
        }
    }
    for (tx = 0; tx < 128; ++tx) {
        for (ty = 0; ty < 64; ++ty) {
            Bdisp_SetPoint_VRAM(tx, ty, SCREEN_BUFFER(tx, ty) >> 6);
        }
    }
    // display info
    {
        char strBuffer[100];
        sprintf(strBuffer, "Res:%dx%d(%dx%d)", 128 / draw_step, 64 / draw_step, 128 / moving_draw_step, 64 / moving_draw_step);
        PrintMini(0, 0, (const unsigned char*)strBuffer, 0);
    }
    Bdisp_PutDisp_DD();

    draw_step--;
    if (draw_step < 1) draw_step = 1;
}

int AddIn_main(int isAppli, unsigned short OptionNum)
{
    unsigned int key;
    float move_step = 0.1f;
    unsigned char _screen_buffer[128 * 64];

    screen_buffer = _screen_buffer;

    initTri();

    redraw();

    while (1){
        redraw();
        if (iskeydown(MYKEY_NUM_5)) { // UP
            tri_arg = Camera.forward;
            MY_COS(); Camera.x += tri_ret * move_step;
            MY_SIN(); Camera.y += tri_ret * move_step;
            draw_step = moving_draw_step;
        }
        else if (iskeydown(MYKEY_NUM_2)) { // DOWN
            tri_arg = Camera.forward; 
            MY_COS(); Camera.x -= tri_ret * move_step;
            MY_SIN(); Camera.y -= tri_ret * move_step;
            draw_step = moving_draw_step;
        }
        else if (iskeydown(MYKEY_NUM_4)) { // TURN LEFT
            Camera.forward += 2;
            draw_step = moving_draw_step;
        }
        else if (iskeydown(MYKEY_NUM_6)) { // TURN RIGHT
            Camera.forward -= 2;
            draw_step = moving_draw_step;
        }
        else if (iskeydown(MYKEY_NUM_1)) { // LEFT
            tri_arg = Camera.forward + 90; 
            MY_COS(); Camera.x += tri_ret * move_step;
            MY_SIN(); Camera.y += tri_ret * move_step;
            draw_step = moving_draw_step;
        }
        else if (iskeydown(MYKEY_NUM_3)) { // RIGHT
            tri_arg = Camera.forward + 90; 
            MY_COS(); Camera.x -= tri_ret * move_step;
            MY_SIN(); Camera.y -= tri_ret * move_step;
            draw_step = moving_draw_step;
        }
        else if (iskeydown(MYKEY_CHAR_MINUS)) { // -
            moving_draw_step++;
            if (moving_draw_step > 8) moving_draw_step = 8;
            Sleep(200);
        }
        else if (iskeydown(MYKEY_CHAR_PLUS)) { // +
            moving_draw_step--;
            if (moving_draw_step < 1) moving_draw_step = 1;
            Sleep(200);
        }
        else if (iskeydown(MYKEY_ESC)) {
            break;
        }
    }

    locate(1, 1);
    Print("Done, press [MENU]");

    while (1) {
        GetKey(&key);
    }

    return 1;
}

#pragma section _BR_Size
unsigned long BR_Size;
#pragma section
#pragma section _TOP
int InitializeSystem(int isAppli, unsigned short OptionNum)
{
    return INIT_ADDIN_APPLICATION(isAppli, OptionNum);
}
#pragma section

const unsigned char mapobj_tEXTURE_0[] = {
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xe0,0x0,0x0,0x7,
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
};

const unsigned char mapobj_tEXTURE_1[] = {
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
    0x0,0x3,0x4,0x3,
    0x4,0x83,0x1,0x13,
    0x0,0xb,0x4,0x23,
    0xc,0x87,0xa1,0x3,
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
    0x4b,0x0,0x13,0x10,
    0x3,0x0,0x83,0x4e,
    0x43,0x48,0x3,0x24,
    0x3,0x0,0xa3,0x2a,
    0x43,0x40,0x3,0x4,
    0x3,0x2,0x23,0x16,
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
    0x80,0x3,0x0,0x3,
    0x4,0x87,0x1,0x57,
    0x0,0xb,0x4,0x3,
    0x24,0x83,0x1,0x13,
    0x40,0xf,0x4,0x23,
    0xc,0x83,0xa1,0x7,
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
    0x3,0x2,0x3,0x0,
    0x3,0x0,0x8b,0x4a,
    0x43,0x44,0x3,0x20,
    0xb,0x2,0xa3,0x2,
    0x47,0x41,0x3,0x10,
    0x3,0x2,0xa3,0x0,
    0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,
};

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <X11/Xlib.h>


#define CELL_COLS 500
#define CELL_ROWS 500

#define CELL_SIZE 2

#define HEIGHT CELL_ROWS*CELL_SIZE
#define WIDTH CELL_COLS*CELL_SIZE


int32_t cells[CELL_ROWS][CELL_COLS] = {0};
int32_t next_cells[CELL_ROWS][CELL_COLS] = {0};

int8_t in_bounds(int32_t, int32_t, int64_t, int64_t);
void gc_put_pixel(void *, int32_t, int32_t, uint32_t);
void gc_fill_rectangle(void *, int32_t, int32_t, int32_t, int32_t, uint32_t);
void update(Display *, GC *, Window *, XImage *);
void draw_cells(void *);
void topple();

uint32_t decodeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) + (g << 8) + b;
}

int8_t exitloop = 0;
int8_t auto_update = 0;

int main(void)
{
    Display *display = XOpenDisplay(NULL);

    int screen = DefaultScreen(display);

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0,
        WIDTH, HEIGHT,
        0, 0,
        0
    );

    char *memory = (char *) malloc(sizeof(uint32_t)*HEIGHT*WIDTH);

    XWindowAttributes winattr = {0};
    XGetWindowAttributes(display, window, &winattr);

    XImage *image = XCreateImage(
        display, winattr.visual, winattr.depth,
        ZPixmap, 0, memory,
        WIDTH, HEIGHT,
        32, WIDTH*4
    );

    GC graphics = XCreateGC(display, window, 0, NULL);

    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delete_window, 1);

    Mask key = KeyPressMask | KeyReleaseMask;
    XSelectInput(display, window, ExposureMask | key);

    XMapWindow(display, window);
    XSync(display, False);

    XEvent event;

    cells[CELL_ROWS/2][CELL_COLS/2] = 2000000;
    next_cells[CELL_ROWS/2][CELL_COLS/2] = 2000000;

    draw_cells(memory);
    update(display, &graphics, &window, image);

    while(!exitloop) {
        while(XPending(display) > 0) {
            XNextEvent(display, &event);
            switch(event.type) {
                case Expose: {
                    update(display, &graphics, &window, image);
                    break;
                }
                case ClientMessage: {
                    if((Atom) event.xclient.data.l[0] == delete_window) {
                        exitloop = 1;   
                    }
                    break;
                }
                case KeyPress: {
                    if(event.xkey.keycode == 0x24) {
                        topple();
                        draw_cells(memory);
                        update(display, &graphics, &window, image);
                    }
                    if(event.xkey.keycode == 0x41) {
                        auto_update = !auto_update;
                    }
                    break;
                }
            }
        }

        if(auto_update) {
            for(int i = 0; i < 1000; ++i)
                topple();
            draw_cells(memory);
            update(display, &graphics, &window, image);
        }
    }


    XCloseDisplay(display);

    free(memory);

    return 0;
}

void topple()
{
    for(int j = 0; j < CELL_ROWS; ++j) {
        for(int i = 0; i < CELL_COLS; ++i) {
            if(cells[j][i] > 3) {
                cells[j][i] -= 4;
                next_cells[j][i] -= 4;
                if(in_bounds(i, j-1, CELL_COLS, CELL_ROWS)) next_cells[j-1][i]++;
                if(in_bounds(i, j+1, CELL_COLS, CELL_ROWS)) next_cells[j+1][i]++;
                if(in_bounds(i-1, j, CELL_COLS, CELL_ROWS)) next_cells[j][i-1]++;
                if(in_bounds(i+1, j, CELL_COLS, CELL_ROWS)) next_cells[j][i+1]++;
            }
        }
    }

    memcpy(&cells, &next_cells, CELL_COLS*CELL_ROWS*sizeof(int32_t));
}

void draw_cells(void *memory)
{
    for(int j = 0; j < CELL_ROWS; ++j) {
        for(int i = 0; i < CELL_COLS; ++i) {
            uint32_t color = decodeRGB(255, 255, 0);
            switch(cells[j][i]) {
                case 0: color = decodeRGB(255, 255, 0); break;
                case 1: color = decodeRGB(0, 185, 64); break;
                case 2: color = decodeRGB(0, 104, 255); break;
                case 3: color = decodeRGB(122, 0, 229); break;
                default: color = decodeRGB(255, 0, 0);
            }
            gc_fill_rectangle(memory, i*CELL_SIZE, j*CELL_SIZE, (i+1)*CELL_SIZE, (j+1)*CELL_SIZE, color);
        }
    }
}

void update(Display *display, GC *graphics, Window *window, XImage *image)
{
    XPutImage(
        display,
        *window,
        *graphics,
        image,
        0, 0,
        0, 0,
        WIDTH, HEIGHT
    );

    XSync(display, False);
}

int8_t in_bounds(int32_t x, int32_t y, int64_t w, int64_t h)
{
    return (x >= 0 && x < w && y >= 0 && y < h);
}

void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color)
{
    if(in_bounds(x, y, WIDTH, HEIGHT))
        *((uint32_t *) memory + y * WIDTH + x) = color;
}


void gc_fill_rectangle(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    for(int y = y0; y < y1; ++y) {
        for(int x = x0; x < x1; ++x) {
            gc_put_pixel(memory, x, y, color);
        }
    }
}




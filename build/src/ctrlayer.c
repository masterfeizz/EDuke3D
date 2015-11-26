#include "compat.h"
#include "renderlayer.h"
#include "cache1d.h"
#include "a.h"
#include "build.h"
#include "osd.h"
#include "scancodes.h"
#include "engine_priv.h"

int32_t xres=400, yres=240, bpp=8, fullscreen=1, bytesperline = 400;

char quitevent=0, appactive=1, novideo=0;

intptr_t frameplace;

uint8_t *framebuffer;
uint16_t *fb;
uint16_t ctrlayer_pal[256];

char modechange=1;
char videomodereset = 0;

char offscreenrendering=0;

int32_t inputchecked = 0;

int32_t lockcount=0;


// Joystick dead and saturation zones
uint16_t *joydead, *joysatur;

void Touch_TouchpadTap();
void Touch_KeyboardTap();
void Touch_ProcessTap();
void Touch_DrawOverlay();
void Touch_Init();
void Touch_Update();

u16* touchpadOverlay;
u16* keyboardOverlay;
char lastKey = 0;
int tmode;
u16* tfb;
touchPosition oldtouch, touch;
u64 tick;

u64 lastTap = 0;

int main(int argc, char **argv){

	osSetSpeedupEnable(true);
	gfxInit(GSP_RGB565_OES,GSP_RGB565_OES,false);
	gfxSetDoubleBuffering(GFX_TOP, false);
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	gfxSet3D(false);
	hidInit();
	consoleInit(GFX_BOTTOM, NULL);

	framebuffer = malloc(  400 * 240 );

	fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

	baselayer_init();

    Touch_Init();

	int r = app_main(argc, (const char **)argv);

	return r;
}

//
// initsystem() -- init 3DS systems
//
int32_t initsystem(void)
{
    frameplace = 0;
    atexit(uninitsystem);
    return 0;
}


//
// uninitsystem() -- uninit 3DS systems
//
void uninitsystem(void)
{
    uninitinput();
    uninittimer();
    hidExit();
    gfxExit();
}

//
// initprintf() -- prints a formatted string to the intitialization window
//

void initprintf(const char *f, ...)
{
    va_list va;
    char buf[2048];

    va_start(va, f);
    Bvsnprintf(buf, sizeof(buf), f, va);
    va_end(va);

    initputs(buf);
}

//
// initputs() -- prints a string to the intitialization window
//
void initputs(const char *buf)
{
    static char dabuf[2048];

    OSD_Puts(buf);
    handleevents();

}

//
// debugprintf() -- prints a formatted debug string to stderr
//
void debugprintf(const char *f, ...)
{
	va_list va;
    char buf[2048];

    va_start(va, f);
    Bvsnprintf(buf, sizeof(buf), f, va);
    va_end(va);

    initputs(buf);
}

int32_t wm_msgbox(const char *name, const char *fmt, ...)
{
    char buf[2048];
    va_list va;

    UNREFERENCED_PARAMETER(name);

    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
    va_end(va);

    printf(buf);
    return 0;
}

int32_t wm_ynbox(const char *name, const char *fmt, ...)
{
    char buf[2048];
    va_list va;

    UNREFERENCED_PARAMETER(name);

    va_start(va,fmt);
    vsprintf(buf,fmt,va);
    va_end(va);

    printf(buf);
    return 0;
}

void wm_setapptitle(const char *name)
{
    UNREFERENCED_PARAMETER(name);
}

//
// initinput() -- init input system
//
int32_t initinput(void)
{
    int32_t i, j;


    if (!keyremapinit)
        for (i = 0; i < 256; i++) keyremap[i] = i;
    keyremapinit = 1;

    inputdevices = 1 | 2 | 4;  // keyboard (1) mouse (2) joystick (4)
    mousegrab = 0;

    memset(key_names, 0, sizeof(key_names));

    joynumbuttons = 8;
    joynumhats = 1;
    joynumaxes = 4;
    joyhat = (int32_t *)Bcalloc(1, sizeof(int32_t));

    joyaxis = (int32_t *)Bcalloc(joynumaxes, sizeof(int32_t));
    joydead = (uint16_t *)Bcalloc(joynumaxes, sizeof(uint16_t));
    joysatur = (uint16_t *)Bcalloc(joynumaxes, sizeof(uint16_t));

    joyhat[0] = -1;

    return 0;
}

//
// uninitinput() -- uninit input system
//
void uninitinput(void)
{
    uninitmouse();
}

static const char *joynames[8] =
{
    "A", "B", "X", "Y", "L", "R", "ZL", "ZR"
};

const char *getjoyname(int32_t what, int32_t num)
{
    static char tmp[64];

    switch (what)
    {
        case 0:  // axis
            if ((unsigned)num > (unsigned)joynumaxes)
                return NULL;
            Bsprintf(tmp, "Axis %d", num);
            return (char *)tmp;

        case 1:  // button
            if ((unsigned)num > (unsigned)joynumbuttons)
                return NULL;
            return joynames[num];

        case 2:  // hat
            if ((unsigned)num > (unsigned)joynumhats)
                return NULL;
            Bsprintf(tmp, "Hat %d", num);
            return (char *)tmp;

        default: return NULL;
    }
}

//
// setjoydeadzone() -- sets the dead and saturation zones for the joystick
//
void setjoydeadzone(int32_t axis, uint16_t dead, uint16_t satur)
{
    joydead[axis] = dead;
    joysatur[axis] = satur;
}


//
// getjoydeadzone() -- gets the dead and saturation zones for the joystick
//
void getjoydeadzone(int32_t axis, uint16_t *dead, uint16_t *satur)
{
    *dead = joydead[axis];
    *satur = joysatur[axis];
}

//
// initmouse() -- init mouse input
//
int32_t initmouse(void)
{
    moustat=AppMouseGrab;
    grabmouse(AppMouseGrab); // FIXME - SA
    return 0;
}

//
// uninitmouse() -- uninit mouse input
//
void uninitmouse(void)
{
    grabmouse(0);
    moustat = 0;
}

//
// grabmouse() -- show/hide mouse cursor
//
void grabmouse(char a)
{
    if (appactive && moustat)
    {
            mousegrab = a;
    }
    else
        mousegrab = a;

    mousex = mousey = 0;
}

void AppGrabMouse(char a)
{
    if (!(a & 2))
    {
        grabmouse(a);
        AppMouseGrab = mousegrab;
    }

}

void setkey(uint32_t keycode, int state){
    if(state)
        joyb |=  1 << keycode;
    else
        joyb &=  ~(1 << keycode);
}
static int32_t hatpos =0;

static int32_t hatvals[16] = {
    -1,     // centre
    0,      // up 1
    9000,   // right 2
    4500,   // up+right 3
    18000,  // down 4
    -1,     // down+up!! 5
    13500,  // down+right 6
    -1,     // down+right+up!! 7
    27000,  // left 8
    27500,  // left+up 9
    -1,     // left+right!! 10
    -1,     // left+right+up!! 11
    22500,  // left+down 12
    -1,     // left+down+up!! 13
    -1,     // left+down+right!! 14
    -1,     // left+down+right+up!! 15
};

void handleevents_buttons(u32 keys, int state){
    uint32_t mod = 1;
    int lastpos = hatpos;
    if(!state)
        mod = -1;

    if( keys & KEY_SELECT){
        if (OSD_HandleScanCode(sc_Escape, state))
        {
            SetKey(sc_Escape, state);

            if (keypresscallback)
                keypresscallback(sc_Escape, state);
        }
    }
    if( keys & KEY_START){
        if (OSD_HandleScanCode(sc_Return, state))
        {
            SetKey(sc_Return, state);

            if (keypresscallback)
                keypresscallback(sc_Return, state);
        }
    }

    //Buttons

    if( keys & KEY_A)
        setkey(0, state);
    if( keys & KEY_B)
        setkey(1, state);
    if( keys & KEY_X)
        setkey(2, state);
    if( keys & KEY_Y)
        setkey(3, state);
    if( keys & KEY_L)
        setkey(4, state);
    if( keys & KEY_R)
        setkey(5, state);
    if( keys & KEY_ZL)
        setkey(6, state);
    if( keys & KEY_ZR)
        setkey(7, state);

    //Hat (Dpad)

    if( keys & KEY_DLEFT)
        hatpos += 8 * mod;
    if( keys & KEY_DRIGHT)
        hatpos += 2 * mod;
    if( keys & KEY_DUP)
        hatpos += 1 * mod;
    if( keys & KEY_DDOWN)
        hatpos += 4 * mod;


    if(hatpos > 15)
        hatpos = 0;

    if(joyhat && lastpos != hatpos)
        joyhat[0] = hatvals[hatpos];
}

void handleevents_axes(void){
    if(!joyaxis)
        return;

    circlePosition circlepad, cstick;

    hidCircleRead(&circlepad);
    hidCstickRead(&cstick);


    joyaxis[0] =  circlepad.dx * 10;
    joyaxis[1] =  -circlepad.dy * 20;
    joyaxis[2] =  cstick.dx * 10;
    joyaxis[3] =  -cstick.dy * 20;

    for(int i = 0; i < 4; i++){
        if((-joydead[i]/100 < joyaxis[i]) && (joydead[i]/100 > joyaxis[i]))
            joyaxis[i] = 0;
    }

}


extern void processAudio(void);

int32_t handleevents(void)
{
    hidScanInput();
    u32 kDown = hidKeysDown();
    u32 kUp = hidKeysUp();

    if(kDown)
        handleevents_buttons(kDown, 1);
    if(kUp)
        handleevents_buttons(kUp, 0);

    handleevents_axes();

    Touch_Update();

    sampletimer();
    return 0;
}

int32_t handleevents_peekkeys(void)
{
    return 0;
}

//
// releaseallbuttons()
//
void releaseallbuttons(void)
{
    joyb = 0;
    hatpos = 0;
}

static uint32_t timerfreq;
static uint32_t timerlastsample;
int32_t timerticspersec=0;
static double msperu64tick = 0;
static void(*usertimercallback)(void) = NULL;


//
// inittimer() -- initialize timer
//
int32_t inittimer(int32_t tickspersecond)
{
    if (timerfreq) return 0;	// already installed

    initprintf("Initializing timer\n");

    totalclock = 0;
    timerfreq = 268123480.0;
    timerticspersec = tickspersecond;
    timerlastsample = svcGetSystemTick() * timerticspersec / timerfreq;

    usertimercallback = NULL;

    msperu64tick = 1000.0 / (double)268123480.0;

    return 0;
}

//
// uninittimer() -- shut down timer
//
void uninittimer(void)
{
    if (!timerfreq) return;

    timerfreq=0;

    msperu64tick = 0;
}

//
// sampletimer() -- update totalclock
//
void sampletimer(void)
{

    processAudio(); //This need to be fixed -- see driver_ctr.c for more info

    uint64_t i;
    int32_t n;

    if (!timerfreq) return;
    i = svcGetSystemTick();
    n = (int32_t)((i*timerticspersec / timerfreq) - timerlastsample);
    //printf("tick\n");
    if (n <= 0) return;

    totalclock += n;
    timerlastsample += n;

    //if (usertimercallback)
    //    for (; n > 0; n--) usertimercallback();
}

//
// getticks() -- returns the ticks count
//
uint32_t getticks(void)
{
    return (uint32_t)svcGetSystemTick();
}

//
// gettimerfreq() -- returns the number of ticks per second the timer is configured to generate
//
int32_t gettimerfreq(void)
{
    return 268123480.0;
}

static inline double u64_to_double(u64 value) {
    return (((double)(u32)(value >> 32))*0x100000000ULL+(u32)value);
}

// Returns the time since an unspecified starting time in milliseconds.
ATTRIBUTE((flatten))
double gethiticks(void)
{
    return (double)u64_to_double(svcGetSystemTick() * msperu64tick);
}

//
// installusertimercallback() -- set up a callback function to be called when the timer is fired
//
void(*installusertimercallback(void(*callback)(void)))(void)
{
    void(*oldtimercallback)(void);

    oldtimercallback = usertimercallback;
    usertimercallback = callback;

    return oldtimercallback;
}


//
// system_getcvars() -- propagate any cvars that are read post-initialization
//
void system_getcvars(void)
{

}

//
// resetvideomode() -- resets the video system
//
void resetvideomode(void)
{

}

// begindrawing()

void begindrawing(void)
{

    //if (offscreenrendering) return;

    frameplace = (intptr_t)framebuffer;
    //printf("Begin Drawing");
    bytesperline = 400;
    calc_ylookup(bytesperline, 240);

    modechange=0;
}

// enddrawing()

void enddrawing(void){
	//printf("End Drawing\n");

}
//
// showframe() -- update the display
//
void showframe(int32_t w)
{
	UNREFERENCED_PARAMETER(w);
  if (offscreenrendering) return;
  int x,y;

	for(x=0; x<400; x++){
		for(y=0; y<240;y++){
			fb[((x*240) + (239 -y))] = ctrlayer_pal[framebuffer[y*400 + x]];
		}
	}
}

#define ADDMODE(x,y,c,f,n) if (validmodecnt<MAXVALIDMODES) { \
    validmode[validmodecnt].xdim=x; \
    validmode[validmodecnt].ydim=y; \
    validmode[validmodecnt].bpp=c; \
    validmode[validmodecnt].fs=f; \
    validmode[validmodecnt].extra=n; \
    validmodecnt++; \
}

static char modeschecked=0;

void getvalidmodes(void)
{

    if (modeschecked) return;
    validmodecnt=0;
    ADDMODE(400,240,8,1,-1)
    modeschecked=1;
}

//
// checkvideomode() -- makes sure the video mode passed is legal
//
int32_t checkvideomode(int32_t *x, int32_t *y, int32_t c, int32_t fs, int32_t forced)
{
    int32_t i, nearest=-1, dx, dy, odx=9999, ody=9999;
    getvalidmodes();
    if ( c > 8 ) return -1;
    // fix up the passed resolution values to be multiples of 8
    // and at least 320x200 or at most MAXXDIMxMAXYDIM
    *x = clamp(*x, 320, MAXXDIM);
    *y = clamp(*y, 200, MAXYDIM);

    for (i = 0; i < validmodecnt; i++)
    {
        if (validmode[i].bpp != c || validmode[i].fs != fs)
            continue;

        dx = klabs(validmode[i].xdim - *x);
        dy = klabs(validmode[i].ydim - *y);

        if (!(dx | dy))
        {
            // perfect match
            nearest = i;
            break;
        }

        if ((dx <= odx) && (dy <= ody))
        {
            nearest = i;
            odx = dx;
            ody = dy;
        }
    }

    if (nearest < 0)
        return -1;

    *x = validmode[nearest].xdim;
    *y = validmode[nearest].ydim;
    return nearest;
}

int32_t setvideomode(int32_t x, int32_t y, int32_t c, int32_t fs)
{
	xres = x;
    yres = y;
    bpp =  c;
    fullscreen = fs;
    bytesperline = 400;
    numpages =  1;
    frameplace = 0;
    modechange = 1;
    videomodereset = 0;

    OSD_ResizeDisplay(x, y);
		fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

    return 0;
}

//
// setgamma
//
int32_t setgamma(void)
{
    return 0;
}

//
// setpalette() -- set palette values
//
int32_t setpalette(int32_t start, int32_t num)
{
    int32_t i, n;

    if (bpp > 8)
        return 0;


	uint8_t *pal = curpalettefaded;
	uint8_t r, g, b;
	uint16_t *table = ctrlayer_pal;
	for(i=0; i<256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		table[0] = RGB8_to_565(r,g,b);
		table++;
		pal += 4;
	}

    return 0;
}

//Im reusing most of the code from ctrQuake. This needs to be cleaned up

//Touchscreen mode identifiers
#define TMODE_TOUCHPAD 1
#define TMODE_KEYBOARD 2

//Keyboard is currently laid out on a 14*4 grid of 20px*20px boxes for lazy implementation
char keymap[14 * 6] = {
  sc_Escape, sc_F1, sc_F2, sc_F3, sc_F4, sc_F5, sc_F6, sc_F7, sc_F8, sc_F9, sc_F10, sc_F11, sc_F12, 0,
  sc_Tilde, sc_1, sc_2, sc_3, sc_4, sc_5, sc_6, sc_7, sc_8, sc_9, sc_0, sc_Minus, sc_Equals, sc_BackSpace,
  sc_Tab, sc_Q, sc_W, sc_E, sc_R, sc_T, sc_Y, sc_U, sc_I, sc_O, sc_P, sc_OpenBracket, sc_CloseBracket, sc_BackSlash,
  0, sc_A , sc_S, sc_D, sc_F, sc_G, sc_H, sc_J, sc_K, sc_L, sc_SemiColon, sc_Quote, sc_Enter, sc_Enter,
  sc_LeftShift, sc_Z, sc_X, sc_C, sc_V, sc_B, sc_N, sc_M, sc_Comma, sc_Period, sc_Slash, 0, sc_UpArrow, 0,
  0, 0 , 0, 0, sc_Space, sc_Space, sc_Space, sc_Space, sc_Space, sc_Space, 0, sc_LeftArrow, sc_DownArrow, sc_RightArrow
};

char charmap[2][14 * 6] = {
    {
        27, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        '`' , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q' , 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\',
        0, 'a' , 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\r', '\r',
        0, 'z' , 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0,
        0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },

    {
        27, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        '`' , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '_', '+', '\b',
        '\t', 'q' , 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '|',
        0, 'a' , 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', '\r', '\r',
        0, 'z' , 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0, 0, 0,
        0, 0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
};

void Touch_Init(){
    tmode = TMODE_TOUCHPAD; //Start in touchpad Mode
    tfb = (u16*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

    //Load overlay files from sdmc for easier testing
    FILE *texture = fopen("touchpadOverlay.bin", "rb");
    if(!texture){
        printf("Could not open touchpadOverlay.bin\n");
        goto ERROR;
    }
    fseek(texture, 0, SEEK_END);
    int size = ftell(texture);
    fseek(texture, 0, SEEK_SET);
    touchpadOverlay = malloc(size);
    fread(touchpadOverlay, 1, size, texture);
    fclose(texture);

    texture = fopen("keyboardOverlay.bin", "rb");
    if(!texture){
        printf("Could not open keyboardOverlay.bin\n");
        goto ERROR;
    }
    fseek(texture, 0, SEEK_END);
    size = ftell(texture);
    fseek(texture, 0, SEEK_SET);
    keyboardOverlay = malloc(size);
    fread(keyboardOverlay, 1, size, texture);
    fclose(texture);

    return;

    ERROR:
    printf("Press START to exit\n");
    while(1){
        hidScanInput();
        uint32_t kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;
    }
    gfxExit();
    exit(1);
}

void Touch_TouchpadUpdate(){
    if(hidKeysDown() & KEY_TOUCH){
        hidTouchRead(&oldtouch);
    }

    if(hidKeysHeld() & KEY_TOUCH){
        hidTouchRead(&touch);
        mousex += (touch.px - oldtouch.px)*10;
        mousey += (touch.py - oldtouch.py)*10;
        oldtouch = touch;
    }

    if(hidKeysUp() & KEY_TOUCH){
        if(touch.py > 195 && touch.py < 240 && touch.px > 270 && touch.px < 320){
            tmode = 2;
            Touch_DrawOverlay();
        }
    }
}

int shiftToggle = 0;

void keyEvent(char code, bool press){
    int32_t j;
    if ((j = OSD_HandleScanCode(code, press)) <= 0)
    {
        if (j == -1)  // osdkey
            for (j = 0; j < KEYSTATUSSIZ; ++j)
                if (GetKey(j))
                {
                    SetKey(j, 0);
                    if (keypresscallback)
                        keypresscallback(j, 0);
                }
    }

    if (press)
    {
        if (!GetKey(code))
        {
            SetKey(code, 1);
            if (keypresscallback)
                keypresscallback(code, 1);
        }
    }

    else {
        SetKey(code, 0);
        if (keypresscallback)
            keypresscallback(code, 0);
    }
}
void Touch_KeyboardUpdate(){

    if(hidKeysUp() & KEY_TOUCH){
        if(touch.py > 195 && touch.py < 240 && touch.px > 270 && touch.px < 320){
            tmode = 1;
            shiftToggle = 0;
            keyEvent(sc_LeftShift, shiftToggle);
            Touch_DrawOverlay();
        }
    }

    char key = 0;
    char charvalue = 0;
    if(hidKeysHeld() & KEY_TOUCH){

        hidTouchRead(&touch);
        if(touch.py > 5 && touch.py < 138 && touch.px > 5 && touch.px < 315){
            key = keymap[((touch.py - 6) / 22) * 14 + (touch.px - 6)/22];
            charvalue = charmap[shiftToggle][((touch.py - 6) / 22) * 14 + (touch.px - 6)/22];
        }
    }

    if(lastKey != key){
        keyEvent(lastKey, 0);
        lastKey = 0;
    }

    if(key == sc_LeftShift && lastKey != key){
      shiftToggle = !shiftToggle;
      keyEvent(key, shiftToggle);
      Touch_DrawOverlay();
    }

    else if(key !=0 && lastKey != key){
        if (OSD_HandleChar(charvalue))
            keyascfifo_insert(charvalue);

        keyEvent(key, true);
        lastKey = key;
    }
}

void Touch_Update(){
    if(tmode == TMODE_TOUCHPAD)
        Touch_TouchpadUpdate();
    else
        Touch_KeyboardUpdate();
}

void Touch_DrawOverlay(){
    u16* overlay = 0;
    if(tmode == TMODE_TOUCHPAD)
        overlay = touchpadOverlay;
    else
        overlay = keyboardOverlay;

    if(!overlay) return;
  
    int x,y;

    for(x=0; x<320; x++){
        for(y=0; y<240;y++){
            tfb[(x*240 + (239 - y))] = overlay[(y*320 + x)];
        }
    }

    if(tmode == TMODE_KEYBOARD && shiftToggle == 1){
        for(x=20; x<24; x++){
            for(y=98; y<102;y++){
                tfb[(x*240 + (239 - y))] = RGB8_to_565(0,255,0);
            }
        }
    }
}

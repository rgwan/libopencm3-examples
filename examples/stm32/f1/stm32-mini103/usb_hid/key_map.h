#define MOD_CTRL	0x01
#define MOD_SHIFT	0x02
#define MOD_ALT		0x04
#define MOD_GUI		0x08

#define LEFT(mod)   (mod)
#define RIGHT(mod)  ((mod << 4))

#define GRP(MOD, Key) (MOD << 8 |(Key & 0xff))

#define KEY_A		4
#define KEY_B		5
#define KEY_C		6
#define KEY_D		7
#define KEY_E		8
#define KEY_F		9
#define KEY_G		10
#define KEY_H		11
#define KEY_I		12
#define KEY_J		13
#define KEY_K		14
#define KEY_L		15
#define KEY_M		16
#define KEY_N		17
#define KEY_O		18
#define KEY_P		19
#define KEY_Q		20
#define KEY_R		21
#define KEY_S		22
#define KEY_T		23
#define KEY_U		24
#define KEY_V		25
#define KEY_W		26
#define KEY_X		27
#define KEY_Y		28
#define KEY_Z		29
#define KEY_1		30
#define KEY_2		31
#define KEY_3		32
#define KEY_4		33
#define KEY_5		34
#define KEY_6		35
#define KEY_7		36
#define KEY_8		37
#define KEY_9		38
#define KEY_0		39
#define KEY_ENTER	40
#define KEY_ESC		41
#define KEY_BACKSPACE	42
#define KEY_TAB		43
#define KEY_SPACE	44
#define KEY_MINUS	45
#define KEY_EQUAL	46
#define KEY_LEFT_BRACE	47
#define KEY_RIGHT_BRACE	48
#define KEY_BACKSLASH	49
#define KEY_NUMBER	50
#define KEY_SEMICOLON	51
#define KEY_QUOTE	52
#define KEY_TILDE	53
#define KEY_COMMA	54
#define KEY_PERIOD	55
#define KEY_SLASH	56
#define KEY_CAPS_LOCK	57
#define KEY_F1		58
#define KEY_F2		59
#define KEY_F3		60
#define KEY_F4		61
#define KEY_F5		62
#define KEY_F6		63
#define KEY_F7		64
#define KEY_F8		65
#define KEY_F9		66
#define KEY_F10		67
#define KEY_F11		68
#define KEY_F12		69
#define KEY_PRINTSCREEN	70
#define KEY_SCROLL_LOCK	71
#define KEY_PAUSE	72
#define KEY_INSERT	73
#define KEY_HOME	74
#define KEY_PAGE_UP	75
#define KEY_DELETE	76
#define KEY_END		77
#define KEY_PAGE_DOWN	78
#define KEY_RIGHT	79
#define KEY_LEFT	80
#define KEY_DOWN	81
#define KEY_UP		82
#define KEY_NUM_LOCK	83
#define KEYPAD_SLASH	84
#define KEYPAD_ASTERIX	85
#define KEYPAD_MINUS	86
#define KEYPAD_PLUS	87
#define KEYPAD_ENTER	88
#define KEYPAD_1	89
#define KEYPAD_2	90
#define KEYPAD_3	91
#define KEYPAD_4	92
#define KEYPAD_5	93
#define KEYPAD_6	94
#define KEYPAD_7	95
#define KEYPAD_8	96
#define KEYPAD_9	97
#define KEYPAD_0	98
#define KEYPAD_PERIOD	99

#define PS2_A			0x1C
#define PS2_B			0x32
#define PS2_C			0x21
#define PS2_D			0x23
#define PS2_E			0x24
#define PS2_F			0x2B
#define PS2_G			0x34
#define PS2_H			0x33
#define PS2_I			0x43
#define PS2_J			0x3B
#define PS2_K			0x42
#define PS2_L			0x4B
#define PS2_M			0x3A
#define PS2_N			0x31
#define PS2_O			0x44
#define PS2_P			0x4D
#define PS2_Q			0x15
#define PS2_R			0x2D
#define PS2_S			0x1B
#define PS2_T			0x2C
#define PS2_U			0x3C
#define PS2_V			0x2A
#define PS2_W			0x1D
#define PS2_X			0x22
#define PS2_Y			0x35
#define PS2_Z			0x1A
#define PS2_0			0x45
#define PS2_1			0x16
#define PS2_2			0x1E
#define PS2_3			0x26
#define PS2_4			0x25
#define PS2_5			0x2E
#define PS2_6			0x36
#define PS2_7			0x3D
#define PS2_8			0x3E
#define PS2_9			0x46
#define PS2_BACKQUOTE	0x0E
#define PS2_DASH		0x4E
#define PS2_EQUAL		0x55
#define PS2_BACKSLASH	0x5D
#define PS2_BKSP		0x66
#define PS2_SPACE		0x29
#define PS2_TAB			0x0D
#define PS2_CAPSLOCK	0x58
#define PS2_L_SHFT		0x12
#define PS2_L_CTRL		0x14
#define PS2_L_ALT		0x11
#define PS2_R_SHFT		0x59
#define PS2_EX_R_CTRL	0x114
#define PS2_EX_R_ALT	0x111
#define PS2_EX_APPS		0x2F
#define PS2_ENTER		0x5A
#define PS2_ESC			0x76
#define PS2_F1			0x05
#define PS2_F2			0x06
#define PS2_F3			0x04
#define PS2_F4			0x0C
#define PS2_F5			0x03
#define PS2_F6			0x0B
#define PS2_F7			0x83
#define PS2_F8			0x0A
#define PS2_F9			0x01
#define PS2_F10			0x09
#define PS2_F11			0x78
#define PS2_F12			0x07
#define PS2_EX_PRNT		0x112
#define PS2_EX_SCRN		0x17C
#define PS2_SCROLL		0x17E
#define PS2_L_BRACKET		0x154
#define PS2_EX_INSERT		0x170
#define PS2_EX_HOME		0x16C
#define PS2_EX_PG_UP		0x17D
#define PS2_EX_DELETE		0x171
#define PS2_EX_END		0x169
#define PS2_EX_PG_DN		0x17A
#define PS2_EX_U_ARROW		0x175
#define PS2_EX_L_ARROW		0x16B
#define PS2_EX_D_ARROW		0x172
#define PS2_EX_R_ARROW		0x174
#define PS2_NUM			0x77
#define PS2_R_BRACKET	0x5B
#define PS2_SEMICOLON	0x4C
#define PS2_APOSTROPHE	0x52
#define PS2_COMMA		0x41
#define PS2_DOT			0x49
#define PS2_SLASH		0x4A
#define PS2_RELEASED	0xF0

static const uint16_t key_map[10][7] = 
{
	{GRP(MOD_ALT, KEY_F3), GRP(RIGHT(MOD_SHIFT), 0), KEY_F6, KEY_F12, KEY_F10, GRP(MOD_ALT, KEY_F2), KEY_F3},
	{KEY_A, KEY_F, KEY_K, KEY_P, GRP(MOD_ALT, KEY_F6), GRP(MOD_ALT, KEY_F7), KEY_F9},
	{KEY_B, KEY_G, KEY_L, KEY_Q, KEY_F1, KEY_F2, KEY_F11},
	{KEY_C, KEY_H, KEY_M, KEY_R, KEY_F8, KEY_F7, GRP(MOD_ALT, KEY_F8)},
	{KEY_D, KEY_I, KEY_N, KEY_S, KEY_U, KEY_W, KEY_Y},
	{KEY_E, KEY_J, KEY_O, KEY_T, KEY_V, KEY_X, KEY_Z},
	{KEY_PAGE_UP, KEY_LEFT, KEY_1, KEY_4, KEY_7, KEY_PERIOD, GRP(MOD_ALT, KEY_F5)},
	{KEY_UP, KEY_DOWN, KEY_2, KEY_5, KEY_8, KEY_0, KEY_BACKSPACE},
	{KEY_PAGE_DOWN, KEY_RIGHT, KEY_3, KEY_6, KEY_9, 0, KEY_TAB},
	{GRP(MOD_CTRL, KEY_PRINTSCREEN), GRP(MOD_ALT, KEY_F4), KEY_ESC, KEY_F5, KEY_F4, GRP(MOD_ALT, KEY_F1), KEY_ENTER}
};

static const uint32_t PS2_map[10][7] = 
{
	{GRP(PS2_L_ALT, PS2_F3), GRP(RIGHT(PS2_R_SHFT), 0), PS2_F6, PS2_F12, PS2_F10, GRP(PS2_L_ALT, PS2_F2), PS2_F3},
	{PS2_A, PS2_F, PS2_K, PS2_P, GRP(PS2_L_ALT, PS2_F6), GRP(PS2_L_ALT, PS2_F7), PS2_F9},
	{PS2_B, PS2_G, PS2_L, PS2_Q, PS2_F1, PS2_F2, PS2_F11},
	{PS2_C, PS2_H, PS2_M, PS2_R, PS2_F8, PS2_F7, GRP(PS2_L_ALT, PS2_F8)},
	{PS2_D, PS2_I, PS2_N, PS2_S, PS2_U, PS2_W, PS2_Y},
	{PS2_E, PS2_J, PS2_O, PS2_T, PS2_V, PS2_X, PS2_Z},
	{GRP(PS2_EX_PG_UP, 0), GRP(PS2_EX_L_ARROW, 0), PS2_1, PS2_4, PS2_7, PS2_DOT, GRP(PS2_L_ALT, PS2_F5)},
	{GRP(PS2_EX_U_ARROW, 0), GRP(PS2_EX_D_ARROW, 0), PS2_2, PS2_5, PS2_8, PS2_0, PS2_BKSP},
	{GRP(PS2_EX_PG_DN, 0), GRP(PS2_EX_R_ARROW, 0), PS2_3, PS2_6, PS2_9, 0, PS2_TAB},
	{GRP(PS2_EX_SCRN, PS2_L_CTRL), GRP(PS2_L_ALT, PS2_F4), PS2_ESC, PS2_F5, PS2_F4, GRP(PS2_L_ALT, PS2_F1), PS2_ENTER}
};

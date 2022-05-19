/*****************************************
		NanoShell Operating System
		(C)2021-2022 iProgramInCpp

              Console module
******************************************/
#include <stdint.h>
#include <main.h>
#include <string.h>
#include <print.h>
#include <console.h>
#include <video.h>

#undef cli
#define cli __asm__("cli\n\t")

//note: important system calls are preceded by cli and succeeded by sti

uint16_t TextModeMakeChar(uint8_t fgbg, uint8_t chr) {
	uint8_t comb = fgbg;
	uint16_t comb2 = comb << 8 | chr;
	return comb2;
}
uint32_t g_vgaColorsToRGB[] = {
	0x00000000,
	0x000000AA,
	0x0000AA00,
	0x0000AAAA,
	0x00AA0000,
	0x00AA00AA,
	0x00AAAA00,
	0x00AAAAAA,
	0x00555555,
	0x005555FF,
	0x0055FF55,
	0x0055FFFF,
	0x00FF5555,
	0x00FF55FF,
	0x00FFFF55,
	0x00FFFFFF,
};
/*
uint32_t g_vgaColorsToRGB[] = {
	0x00000000,
	0x000000AA,
	0x0000AA00,
	0x0000AAAA,
	0x00AA0000,
	0x00AA00AA,
	0x00AAAA00,
	0x00AAAAAA,
	0x00555555,
	0x005555FF,
	0x0055FF55,
	0x0055FFFF,
	0x00FF5555,
	0x00FF55FF,
	0x00FFFF55,
	0x00FFFFFF,
};
*/
extern bool g_uses8by16Font;
Console g_debugConsole; // for LogMsg
Console g_debugSerialConsole; // for SLogMsg

Console* g_currentConsole = &g_debugConsole;

uint16_t* g_pBufferBase = (uint16_t*)(KERNEL_MEM_START + 0xB8000);
int g_textWidth = 80, g_textHeight = 25;

void SetConsole(Console* pConsole) {
	g_currentConsole = pConsole;
}
void ResetConsole() {
	g_currentConsole = &g_debugConsole;
}

// The mess that goes into generalizing the console implementation to support any console device ever invented. Ever.
#if 1
	
	// Function forward definitions
	#if 1
		void CoE9xClearScreen  (Console *this);
		void CoSerClearScreen  (Console *this);
		void CoVbeClearScreen  (Console *this);
		void CoVgaClearScreen  (Console *this);
		void CoWndClearScreen  (Console *this);
		void CoE9xInit         (Console *this);
		void CoSerInit         (Console *this);
		void CoVbeInit         (Console *this);
		void CoVgaInit         (Console *this);
		void CoWndInit         (Console *this);
		void CoE9xPlotChar     (Console *this, int,  int,  char);
		void CoSerPlotChar     (Console *this, int,  int,  char);
		void CoVbePlotChar     (Console *this, int,  int,  char);
		void CoVgaPlotChar     (Console *this, int,  int,  char);
		void CoWndPlotChar     (Console *this, int,  int,  char);
		bool CoE9xPrintCharInt (Console *this, char, char, bool);
		bool CoSerPrintCharInt (Console *this, char, char, bool);
		bool CoVisComPrnChrInt (Console *this, char, char, bool);//common for visual consoles
		void CoE9xRefreshChar  (Console *this, int,  int);
		void CoSerRefreshChar  (Console *this, int,  int);
		void CoVbeRefreshChar  (Console *this, int,  int);
		void CoVgaRefreshChar  (Console *this, int,  int);
		void CoWndRefreshChar  (Console *this, int,  int);
		void CoE9xUpdateCursor (Console *this);
		void CoSerUpdateCursor (Console *this);
		void CoVbeUpdateCursor (Console *this);
		void CoVgaUpdateCursor (Console *this);
		void CoWndUpdateCursor (Console *this);
		void CoE9xScrollUpByOne(Console *this);
		void CoSerScrollUpByOne(Console *this);
		void CoVbeScrollUpByOne(Console *this);
		void CoVgaScrollUpByOne(Console *this);
		void CoWndScrollUpByOne(Console *this);
	#endif
	
	// Type definitions
	#if 1
		typedef void (*tCoClearScreen)  (Console *this);
		typedef void (*tCoInit)         (Console *this);
		typedef void (*tCoPlotChar)     (Console *this, int,  int,  char);
		typedef bool (*tCoPrintCharInt) (Console *this, char, char, bool);
		typedef void (*tCoRefreshChar)  (Console *this, int,  int);
		typedef void (*tCoUpdateCursor) (Console *this);
		typedef void (*tCoScrollUpByOne)(Console *this);
	#endif
	
	// Array definitions
	#if 1
		static tCoClearScreen g_clear_screen[] = {
			NULL,
			CoVgaClearScreen,
			CoVbeClearScreen,
			CoSerClearScreen,
			CoE9xClearScreen,
			CoWndClearScreen,
		};
		static tCoInit g_init[] = {
			NULL,
			CoVgaInit,
			CoVbeInit,
			CoSerInit,
			CoE9xInit,
			CoWndInit,
		};
		static tCoPlotChar g_plot_char[] = {
			NULL,
			CoVgaPlotChar,
			CoVbePlotChar,
			CoSerPlotChar,
			CoE9xPlotChar,
			CoWndPlotChar,
		};
		static tCoPrintCharInt g_print_char_int[] = {
			NULL,
			CoVisComPrnChrInt,
			CoVisComPrnChrInt,
			CoSerPrintCharInt,
			CoE9xPrintCharInt,
			CoVisComPrnChrInt,
		};
		static tCoRefreshChar g_refresh_char[] = {
			NULL,
			CoVgaRefreshChar,
			CoVbeRefreshChar,
			CoSerRefreshChar,
			CoE9xRefreshChar,
			CoWndRefreshChar,
		};
		static tCoUpdateCursor g_update_cursor[] = {
			NULL,
			CoVgaUpdateCursor,
			CoVbeUpdateCursor,
			CoSerUpdateCursor,
			CoE9xUpdateCursor,
			CoWndUpdateCursor,
		};
		static tCoUpdateCursor g_scroll_up_by_one[] = {
			NULL,
			CoVgaScrollUpByOne,
			CoVbeScrollUpByOne,
			CoSerScrollUpByOne,
			CoE9xScrollUpByOne,
			CoWndScrollUpByOne,
		};
	#endif
	
	// The functions themselves.
	void CoClearScreen(Console *this)
	{
		g_clear_screen[this->type](this);
	}
	void CoInit(Console *this)
	{
		g_init[this->type](this);
	}
	void CoInitAsText(Console *this)
	{
		this->type = CONSOLE_TYPE_TEXT;
		CoInit(this);
	}
	void CoInitAsE9Hack(Console *this)
	{
		this->type = CONSOLE_TYPE_E9HACK;
		CoInit(this);
	}
	void CoInitAsGraphics(Console *this)
	{
		this->type = CONSOLE_TYPE_FRAMEBUFFER;
		CoInit(this);
	}
	void CoMoveCursor(Console* this)
	{
		g_update_cursor[this->type](this);
	}
	void CoPlotChar (Console *this, int x, int y, char c)
	{
		g_plot_char[this->type](this,x,y,c);
	}
	void CoRefreshChar (Console *this, int x, int y)
	{
		g_refresh_char[this->type](this,x,y);
	}
	void CoScrollUpByOne(Console *this)
	{
		g_scroll_up_by_one[this->type](this);
	}
	bool CoPrintCharInternal (Console* this, char c, char next, bool bDontUpdateCursor)
	{
		return g_print_char_int[this->type](this,c,next,bDontUpdateCursor);
	}
#endif
//returns a bool, if it's a true, we need to skip the next character.
bool CoVisComPrnChrInt (Console* this, char c, char next, bool bDontUpdateCursor)
{
	switch (c)
	{
		case '\x01':
			//allow foreground color switching.
			//To use this, just type `\x01\x0B`, for example, to switch to bright cyan
			//Typing \x00 will end the parsing, so you can use \x01\x10, or \x01\x30.
			
			if (!next) break;
			char color = next & 0xF;
			this->color = (this->color & 0xF0) | color;
			return true;
		case '\x02':
			//change X coordinate
			//To use this, just type `\x02\x0B`, for example, to move cursorX to 11
			
			if (!next) break;
			char xcoord = next;
			if (xcoord >= this->width)
				xcoord = this->width - 1;
			this->curX = xcoord;
			return true;
		case '\b':
			if (--this->curX < 0) {
				this->curX = this->width - 1;
				if (--this->curY < 0) this->curY = 0;
			}
			CoPlotChar(this, this->curX, this->curY, 0);
			break;
		case '\r': 
			this->curX = 0;
			break;
		case '\n': 
			this->curX = 0;
			this->curY++;
			while (this->curY >= this->height) {
				CoScrollUpByOne(this);
				this->curY--;
			}
			break;
		case '\t': 
			this->curX = (this->curX + 4) & ~3;
			if (this->curX >= this->width) {
				this->curX = 0;
				this->curY++;
			}
			while (this->curY >= this->height) {
				CoScrollUpByOne(this);
				this->curY--;
			}
			break;
		
		default: {
			CoPlotChar(this, this->curX, this->curY, c);
			// advance cursor
			if (++this->curX >= this->width) {
				this->curX = 0;
				this->curY++;
			}
			while (this->curY >= this->height) {
				CoScrollUpByOne(this);
				this->curY--;
			}
			break;
		}
	}
	if (!bDontUpdateCursor) CoMoveCursor(this);
	return false;
}

void CoPrintChar (Console* this, char c)
{
	CoPrintCharInternal(this, c, '\0', true);
}
void CoPrintString (Console* this, const char *c)
{
	if (this->type == CONSOLE_TYPE_NONE) return; // Not Initialized
	while (*c)
	{
		// if we need to advance 2 characters instead of 1:
		if (CoPrintCharInternal(this, *c, *(c + 1), false))
			c++;
		// advance the one we would've anyways
		c++;
	}
	CoMoveCursor(this);
}

// Input:
void CoAddToInputQueue (Console* this, char input)
{
	if (!input) return;
	
	this->m_inputBuffer[this->m_inputBufferEnd++] = input;
	while
	   (this->m_inputBufferEnd >= KB_BUF_SIZE)
		this->m_inputBufferEnd -= KB_BUF_SIZE;
}
bool CoAnythingOnInputQueue (Console* this)
{
	return this->m_inputBufferBeg != this->m_inputBufferEnd;
}
char CoReadFromInputQueue (Console* this)
{
	if (CoAnythingOnInputQueue(this))
	{
		char k = this->m_inputBuffer[this->m_inputBufferBeg++];
		while
		   (this->m_inputBufferBeg >= KB_BUF_SIZE)
			this->m_inputBufferBeg -= KB_BUF_SIZE;
		return k;
	}
	else return 0;
}

bool CoInputBufferEmpty()
{
	return !CoAnythingOnInputQueue (g_currentConsole);
}
extern void KeTaskDone();
char CoGetChar()
{
	while (!CoAnythingOnInputQueue (g_currentConsole))
		KeTaskDone();
	return CoReadFromInputQueue (g_currentConsole);
}
void CoGetString(char* buffer, int max_size)
{
	int index = 0, max_length = max_size - 1;
	//index represents where the next character we type would go
	while (index < max_length)
	{
		//! has to stall
		char k = CoGetChar();
		if (k == '\n')
		{
			//return:
			LogMsgNoCr("%c", k);
			buffer[index++] = 0;
			return;
		}
		else if (k == '\b')
		{
			if (index > 0)
			{
				LogMsgNoCr("%c", k);
				index--;
				buffer[index] = 0;
			}
		}
		else
		{
			buffer[index++] = k;
			LogMsgNoCr("%c", k);
		}
	}
	LogMsg("");
	buffer[index] = 0;
}


void CLogMsg (Console* pConsole, const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(pConsole, cr);
	
	va_end(list);
}
void CLogMsgNoCr (Console* pConsole, const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(pConsole, cr);
	
	va_end(list);
}
void LogMsg (const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(g_currentConsole, cr);
	
	va_end(list);
}
void LogMsgNoCr (const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(g_currentConsole, cr);
	
	va_end(list);
}
void SLogMsg (const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	
	sprintf (cr + strlen(cr), "\n");
	CoPrintString(&g_debugSerialConsole, cr);
	
	va_end(list);
}
void SLogMsgNoCr (const char* fmt, ...) {
	////allocate a buffer well sized
	char cr[8192];
	va_list list;
	va_start(list, fmt);
	vsprintf(cr, fmt, list);
	CoPrintString(&g_debugSerialConsole, cr);
	
	va_end(list);
}
const char* g_uppercaseHex = "0123456789ABCDEF";

void LogHexDumpData (void* pData, int size) {
	uint8_t* pDataAsNums = (uint8_t*)pData, *pDataAsText = (uint8_t*)pData;
	char c[7], d[4];
	c[5] = 0;   d[2] = ' ';
	c[6] = ' '; d[3] = 0;
	c[4] = ':';
	
	#define BYTES_PER_ROW 16
	for (int i = 0; i < size; i += BYTES_PER_ROW) {
		// print the offset
		c[0] = g_uppercaseHex[(i & 0xF000) >> 12];
		c[1] = g_uppercaseHex[(i & 0x0F00) >>  8];
		c[2] = g_uppercaseHex[(i & 0x00F0) >>  4];
		c[3] = g_uppercaseHex[(i & 0x000F) >>  0];
		LogMsgNoCr("%s", c);
		
		for (int j = 0; j < BYTES_PER_ROW; j++) {
			uint8_t p = *pDataAsNums++;
			d[0] = g_uppercaseHex[(p & 0xF0) >> 4];
			d[1] = g_uppercaseHex[(p & 0x0F) >> 0];
			LogMsgNoCr("%s", d);
		}
		LogMsgNoCr("   ");
		for (int j = 0; j < BYTES_PER_ROW; j++) {
			char c = *pDataAsText++;
			if (c < ' ') c = '.';
			LogMsgNoCr("%c",c);
		}
		LogMsg("");
	}
}

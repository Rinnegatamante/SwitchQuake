/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "errno.h"
#include <switch.h>
#include <stdio.h>
#include <sys/stat.h>

#define BIGSTACK_SIZE 20 * 1024 * 1024
byte sys_bigstack[BIGSTACK_SIZE];
int sys_bigstack_cursize;

// Mods support
/*int max_mod_idx = -1;
extern bool CheckForMod(char* dir);
extern void MOD_SelectModMenu(char *basedir);
extern char* modname;*/

extern int old_char;
extern int setup_cursor;
extern int lanConfig_cursor;
extern int isKeyboard;
extern uint64_t rumble_tick;
extern cvar_t res_val;
extern cvar_t psvita_touchmode;

uint64_t initialTime = 0;
int hostInitialized = 0;

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             Sys_FindHandle(void)
{
	int             i;

	for (i = 1; i<MAX_HANDLES; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error("out of handles");
	return -1;
}

void Log(const char *format, ...) {
#ifdef DEBUG
	__gnuc_va_list arg;
	va_start(arg, format);
	char msg[512];
	vsprintf(msg, format, arg);
	va_end(arg);
	sprintf(msg, "%s\n", msg);
	FILE* log = fopen("./log.txt", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
#endif
}

int Sys_FileLength(FILE *f)
{
	int             pos;
	int             end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead(char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = Sys_FindHandle();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return Sys_FileLength(f);
}

int Sys_FileOpenWrite(char *path)
{
	FILE    *f;
	int             i;

	i = Sys_FindHandle();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error("Error opening %s: %s", path, strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose(int handle)
{
	fclose(sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position)
{
	fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void *dest, int count)
{
	return fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, void *data, int count)
{
	return fwrite(data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime(char *path)
{
	FILE    *f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

void Sys_mkdir(char *path)
{
	mkdir(path, 0777);
}


void* Sys_Malloc(int size, char* purpose)
{
	void* m;

	m = malloc(size);
	if (m == 0)
	{
		Sys_Error("Sys_Malloc: %s - failed on %i bytes", purpose, size);
	};
	return m;
}
// <<< FIX

// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// New functions for big stack handling:
void Sys_BigStackRewind(void)
{
	sys_bigstack_cursize = 0;
}

void* Sys_BigStackAlloc(int size, char* purpose)
{
	void* p;

	p = 0;
	if (sys_bigstack_cursize + size < BIGSTACK_SIZE)
	{
		p = sys_bigstack + sys_bigstack_cursize;
		sys_bigstack_cursize = sys_bigstack_cursize + size;
	}
	else
	{
		Sys_Error("Sys_BigStackAlloc: %s - failed on %i bytes", purpose, size);
	};
	return p;
}

void Sys_BigStackFree(int size, char* purpose)
{
	if (sys_bigstack_cursize - size >= 0)
	{
		sys_bigstack_cursize = sys_bigstack_cursize - size;
	}
	else
	{
		Sys_Error("Sys_BigStackFree: %s - underflow on %i bytes", purpose, sys_bigstack_cursize - size);
	};
}
// <<< FIX


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length)
{
}

void Sys_Quit(void)
{
	Host_Shutdown();
}

void Sys_Error(char *error, ...)
{

	va_list         argptr;

	char buf[256];
	va_start(argptr, error);
	vsnprintf(buf, sizeof(buf), error, argptr);
	va_end(argptr);
	sprintf(buf, "%s\n", buf);
	FILE* f = fopen("./log.txt", "a+");
	fwrite(buf, 1, strlen(buf), f);
	fclose(f);
	Sys_Quit();
}

void Sys_Printf(char *fmt, ...)
{
#ifdef DEBUG
	if (hostInitialized)
		return;

	va_list argptr;
	char buf[256];
	va_start(argptr, fmt);
	vsnprintf(buf, sizeof(buf), fmt, argptr);
	va_end(argptr);
	Log(buf);
#endif

}

char *Sys_ConsoleInput(void)
{
	return NULL;
}

void Sys_Sleep(void)
{
}

double Sys_FloatTime(void)
{
	return svcGetSystemTick() * (1.0f / 19200000.0f);
}

void Sys_HighFPPrecision(void)
{
}

void Sys_LowFPPrecision(void)
{
}

/*
===============================================================================

KEYS & INPUTS

===============================================================================
*/
typedef struct
{
	uint64_t  button;
	int        key;
} nx_buttons;

#define MAX_NX_KEYS 12
nx_buttons KeyTable[MAX_NX_KEYS] =
{
	{ KEY_MINUS, K_SELECT},
	{ KEY_PLUS, K_START},

	{ KEY_DUP, K_UPARROW},
	{ KEY_DDOWN, K_DOWNARROW},
	{ KEY_DLEFT, K_LEFTARROW},
	{ KEY_DRIGHT, K_RIGHTARROW},
	{ KEY_L, K_LEFTTRIGGER},
	{ KEY_R, K_RIGHTTRIGGER},

	{ KEY_X, K_X},
	{ KEY_Y, K_Y},
	{ KEY_A, K_A},
	{ KEY_B, K_B}
};

void NX_KeyDown(int keys) {
	int i;
	for (i = 0; i < MAX_NX_KEYS; i++) {
		if (keys & KeyTable[i].button) {
				Key_Event(KeyTable[i].key, true);
		}
	}
}

void NX_KeyUp(int keys, int oldkeys) {
	int i;
	for (i = 0; i < MAX_NX_KEYS; i++) {
		if ((!(keys & KeyTable[i].button)) && (oldkeys & KeyTable[i].button)) {
			Key_Event(KeyTable[i].key, false);
		}
	}
}

void Sys_SendKeyEvents(void)
{
	hidScanInput();
	uint64_t kDown = hidKeysDown(CONTROLLER_P1_AUTO);
	uint64_t kUp = hidKeysUp(CONTROLLER_P1_AUTO);

	if (kDown)
		NX_KeyDown(kDown);
	if (kUp != kDown)
		NX_KeyUp(kDown, kUp);
}

//=============================================================================
char* mod_path = NULL;

void simulateKeyPress(char* text){
	
	//We first delete the current text
	int i;
	for (i=0;i<100;i++){
		Key_Event(K_BACKSPACE, true);
		Key_Event(K_BACKSPACE, false);
	}
	
	while (*text){
		Key_Event(*text, true);
		Key_Event(*text, false);
		text++;
	}
}

#define		MAXCMDLINE	256	// If changed, don't forget to change it in keys.c too!!
extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;

int main(int argc, char **argv)
{
	// Initializing stuffs
	gfxInitDefault();

	const float tickRate = 1.0f / 19200000.0f;
	static quakeparms_t    parms;

	parms.memsize = 16 * 1024 * 1024;
	parms.membase = malloc(parms.memsize);
	parms.basedir = ".";
	
	// Scanning main folder in search of mods
	/*int dd = sceIoDopen(parms.basedir);
	SceIoDirent entry;
	int res;
	while (sceIoDread(dd, &entry) > 0){
		if (SCE_S_ISDIR(entry.d_stat.st_mode)){
			if (CheckForMod(va("%s/%s", parms.basedir, entry.d_name)))
				max_mod_idx++;
		}
	}
	sceIoDclose(dd);
	
	// Do we have at least a mod running here?
	if (max_mod_idx > 0) 
		MOD_SelectModMenu(parms.basedir);
	
	// Mods support
	if (modname != NULL && strcmp(modname,"id1")) {
		int int_argc = 3;
		char* int_argv[3];
		int_argv[0] = int_argv[2] = "";

		// Special check for official missionpacks.
		if (!strcmp(modname, "hipnotic"))	int_argv[1] = "-hipnotic";
		else if (!strcmp(modname, "rogue")) int_argv[1] = "-rogue";
		else {
			int_argv[1] = "-game";
			int_argv[2] = modname;
		}
		COM_InitArgv(3, int_argv);
	}else */COM_InitArgv(argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	Host_Init(&parms);
	hostInitialized = 1;
	
	// Setting PSN Account if it's his first time
	/*if (!strcmp(cl_name.string, "player"))
	{
		char nickname[32];
		sceAppUtilSystemParamGetString(SCE_SYSTEM_PARAM_ID_USERNAME, nickname, SCE_SYSTEM_PARAM_USERNAME_MAXSIZE);

		static char cmd[256];
		sprintf(cmd, "_cl_name \"%s\"\n", nickname);
		Cbuf_AddText(cmd);
	}*/

	IN_ResetInputs();
	Cbuf_AddText("exec config.cfg\n");

	/*if ( sceKernelGetModelForCDialog() == PLATFORM_PSVITA) // Ch0wW: SOMEONE HEEEELP ME :c
	{
	Cvar_ForceSet("platform", "2");
	}*/

	// Just to be sure to use the correct resolution in config.cfg
	// VID_ChangeRes(res_val.value);

	uint64_t lastTick = svcGetSystemTick();

	while (1)
	{

		// OSK manage for Console / Input
		/*if (key_dest == key_console || m_state == m_lanconfig || m_state == m_setup)
		{
			if (old_char != 0) Key_Event(old_char, false);
			SceCtrlData tmp_pad, oldpad;
			sceCtrlPeekBufferPositive(0, &tmp_pad, 1);
			if (isKeyboard)
			{
				SceCommonDialogStatus status = sceImeDialogGetStatus();
				if (status == 2) {
					SceImeDialogResult result;
					memset(&result, 0, sizeof(SceImeDialogResult));
					sceImeDialogGetResult(&result);

					if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
					{
						if (key_dest == key_console)
						{
							utf2ascii(title_keyboard, input_text);
							Q_strcpy(key_lines[edit_line] + 1, title_keyboard);
							Key_SendText(key_lines[edit_line] + 1);
						}
						else {
							utf2ascii(title_keyboard, input_text);
							simulateKeyPress(title_keyboard);
						}
					}

					sceImeDialogTerm();
					isKeyboard = false;
				}
			}
			else {
				if ((tmp_pad.buttons & SCE_CTRL_SELECT) && (!(oldpad.buttons & SCE_CTRL_SELECT)))
				{
					if ((m_state == m_setup && (setup_cursor == 0 || setup_cursor == 1)) || (key_dest == key_console) || (m_state == m_lanconfig && (lanConfig_cursor == 0 || lanConfig_cursor == 2)))
					{
						memset(input_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1) << 1);
						memset(initial_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH) << 1);
						if (key_dest == key_console) {

							sprintf(title_keyboard, "Insert Quake command");
						}
						else if (m_state == m_setup) {
							(setup_cursor == 0) ? sprintf(title_keyboard, "Insert hostname") : sprintf(title_keyboard, "Insert player name");
						}else if (m_state == m_lanconfig){
							(lanConfig_cursor == 0) ? sprintf(title_keyboard, "Insert port number") : sprintf(title_keyboard, "Insert server address");
						}
						ascii2utf(title, title_keyboard);
						isKeyboard = true;
						SceImeDialogParam param;
						sceImeDialogParamInit(&param);
						param.supportedLanguages = 0x0001FFFF;
						param.languagesForced = SCE_TRUE;
						param.type = (m_state == m_lanconfig && lanConfig_cursor == 0) ? SCE_IME_TYPE_NUMBER : SCE_IME_TYPE_BASIC_LATIN;
						param.title = title;
						param.maxTextLength = (m_state == m_lanconfig && lanConfig_cursor == 0) ? 5 : SCE_IME_DIALOG_MAX_TEXT_LENGTH;
						if (key_dest == key_console)
						{
							Q_strcpy(initial_text, key_lines[edit_line] + 1);
							Q_strcpy(input_text, key_lines[edit_line] + 1);
						}
						param.initialText = initial_text;
						param.inputTextBuffer = input_text;
						sceImeDialogInit(&param);
					}
				}
			}
			oldpad = tmp_pad;
		}*/

		// Get current frame
		uint64_t tick = svcGetSystemTick();
		const unsigned int deltaTick = tick - lastTick;
		const float   deltaSecond = deltaTick * tickRate;

		// Show frame
		Host_Frame(deltaSecond);
		lastTick = tick;

	}

	// I'm sure those can be removed
	free(parms.membase);
	/*free(modname);
	if (mod_path != NULL) free(mod_path);*/
	//===============================

	gfxExit();
	return 0;
}

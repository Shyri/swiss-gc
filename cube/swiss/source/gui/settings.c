#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <gccore.h>
#include <ogc/exi.h>
#include <ogc/machine/processor.h>
#include "deviceHandler.h"
#include "FrameBufferMagic.h"
#include "IPLFontWrite.h"
#include "swiss.h"
#include "main.h"
#include "info.h"
#include "config.h"
#include "settings.h"
#include "exi.h"

static int page_x_ofs_key = 30;
static int page_x_ofs_val = 410;
static int page_y_line = 25;
static float label_size = 0.75f;
	
SwissSettings tempSettings;
char *uiVModeStr[] = {"Auto", "480i", "480p", "576i", "576p"};
char *gameVModeStr[] = {"No", "480i", "480sf", "240p", "960i", "480p", "1080i60", "540p60", "576i", "576sf", "288p", "1152i", "576p", "1080i50", "540p50"};
char *forceHScaleStr[] = {"Auto", "1:1", "11:10", "9:8", "640px", "704px", "720px"};
char *forceVFilterStr[] = {"Auto", "0", "1", "2"};
char *forceWidescreenStr[] = {"No", "3D", "2D+3D"};
char *forceEncodingStr[] = {"Auto", "ANSI", "SJIS", "No"};
char *invertCStickStr[] = {"No", "X", "Y", "X&Y"};
char *igrTypeStr[] = {"Disabled", "Reboot", "igr.dol"};
char *aveCompatStr[] = {"CMPV-DOL", "GCVideo", "AVE-RVL", "AVE N-DOL"};
char *fileBrowserStr[] = {"Standard", "Carousel"};
char *sramLang[] = {"English", "German", "French", "Spanish", "Italian", "Dutch"};

static char *tooltips_global[PAGE_GLOBAL_MAX+1] = {
	"System Sound:\n\nSets the default audio output type used by most games",
	"Screen Position:\n\nAdjusts the horizontal screen position in games.\nNote: This will take effect next boot.",
	"System Language:\n\nSystem language used in games, primarily multi-5 PAL games",
	"SD/IDE Speed:\n\nThe speed to try and use on the EXI bus for SD Card Adapters or IDE-EXI devices.\n32 MHz may not work on some SD cards.",
	 NULL,
	"In-Game Reset: (B + R + Z + DPad Down)\n\nReboot: Soft-Reset the GameCube\nigr.dol: Low mem (< 0x81300000) igr.dol at the root of SD Card",
	"Configuration Device:\n\nThe device that Swiss will use to load and save swiss.ini from.\nThis setting is stored in SRAM and will remain on reboot.",
	"AVE Compatibility:\n\nSets the compatibility mode for the used audio/video encoder.\n\nAVE N-DOL - Output PAL as NTSC 50\nCMPV-DOL - Enable 1080i & 540p\nGCVideo - Apply firmware workarounds for GCVideo (default)\nAVE-RVL - Support 960i & 1152i without WiiVideo",
	"File Browser Type:\n\nStandard - Displays files with minimal detail (default)\n\nCarousel - Suited towards Game/DOL only use, consider combining\nthis option with the File Management setting turned off\nand Hide Unknown File Types turned on for a better experience."
};

static char *tooltips_advanced[PAGE_ADVANCED_MAX+1] = {
	"Enable USB Gecko Debug via Slot B:\n\nIf a USB Gecko is present in slot B, debug output from\nSwiss & in game (if the game supported output over OSReport)\nwill be output. If nothing is reading the data out from the\ndevice it may cause Swiss/games to hang.",
	"Hide Unknown file types:\n\nDisabled - Show all files (default)\nEnabled - Swiss will hide unknown file types from being displayed\n\nKnown file types are:\n GameCube Executables (.dol)\n Disc backups (.iso/.gcm)\n MP3 Music (.mp3)\n WASP/WKF Flash files (.fzn)\n GameCube Memory Card Files (.gci)\n GameCube Executables with parameters appended (.dol+cli)",
	"Stop DVD Motor on startup\n\nDisabled - Leave it as-is (default)\nEnabled - Stop the DVD drive from spinning when Swiss starts\n\nThis option is mostly for users booting from game\nexploits where the disc will already be spinning.",
	"WiiRD debugging:\n\nDisabled - Boot as normal (default)\nEnabled - This will start a game with the WiiRD debugger enabled & paused\n\nThe WiiRD debugger takes up more memory and can cause issues.",
	"Savestate:\n\nEnable savestate.",
	"File Management:\n\nDisabled - Known files will load immediately instead (default)\nEnabled - A file management prompt will be displayed for all files",
	"Auto-load all cheats:\n\nIf enabled, and a cheats file for a particular game is found\ne.g. sd:/cheats/GPOP8D.txt (on a compatible device)\nthen all cheats in the file will be enabled",
	NULL,
	"Force DTV Status:\n\nDisabled - Use signal from the video interface (default)\nEnabled - Force on in case of hardware fault"
};

static char *tooltips_network[PAGE_NETWORK_MAX+1] = {
		"Init network at startup:\n\nDisabled - Do not initialise the BBA even if present (default)\nEnabled - If a BBA is present, it will be initialised at startup\n\nIf initialised, navigate to the IP in a web browser to backup various data"
};

static char *tooltips_game[PAGE_GAME_MAX+1] = {
	NULL,
	NULL,
	"Force Vertical Offset:\n\n+0 - Standard value\n-2 - GCVideo-DVI compatible (480i)\n-3 - GCVideo-DVI compatible (default)\n-4 - GCVideo-DVI compatible (240p)\n-12 - Datapath VisionRGB (480p)",
	NULL,
	NULL,
	NULL,
	NULL,
	"Force Text Encoding:\n\nNo - System native format\nAuto - Game native format (default)\nANSI - Force International format on a Japanese region game\nSJIS - Force Japanese format on an International region game\n\nThis effectively behaves the same as the USA/JPN region switch.",
	"Invert Camera Stick:\n\nNo - Leave C Stick as-is (default)\nX - Invert X-axis of the C Stick\nY - Invert Y-axis of the C Stick\nX&Y - Invert both axes of the C Stick"
};

syssram* sram;
syssramex* sramex;

// Number of settings (including Back, Next, Save, Exit buttons) per page
int settings_count_pp[5] = {PAGE_GLOBAL_MAX, PAGE_NETWORK_MAX, PAGE_ADVANCED_MAX, PAGE_GAME_DEFAULTS_MAX, PAGE_GAME_MAX};

void refreshSRAM(SwissSettings *settings) {
	sram = __SYS_LockSram();
	settings->sramHOffset = sram->display_offsetH;
	settings->sram60Hz = (sram->ntd >> 6) & 1;
	settings->sramLanguage = sram->lang;
	settings->sramProgressive = (sram->flags >> 7) & 1;
	settings->sramStereo = (sram->flags >> 2) & 1;
	__SYS_UnlockSram(0);
	sramex = __SYS_LockSramEx();
	settings->configDeviceId = sramex->__padding0;
	if(settings->configDeviceId > DEVICE_ID_MAX || (getDeviceByUniqueId(settings->configDeviceId)->features & (FEAT_WRITE|FEAT_BOOT_DEVICE)) != (FEAT_WRITE|FEAT_BOOT_DEVICE)) {
		settings->configDeviceId = DEVICE_ID_UNK;
	}
	__SYS_UnlockSramEx(0);
}

char* getConfigDeviceName(SwissSettings *settings) {
	DEVICEHANDLER_INTERFACE *configDevice = getDeviceByUniqueId(settings->configDeviceId);
	return configDevice != NULL ? (char*)(configDevice->deviceName) : "None";
}

char* get_tooltip(int page_num, int option) {
	char *textPtr = NULL;
	if(page_num == PAGE_GLOBAL) {
		textPtr = tooltips_global[option];
	}
	else if(page_num == PAGE_NETWORK) {
		textPtr = tooltips_network[option];
	}
	else if(page_num == PAGE_ADVANCED) {
		textPtr = tooltips_advanced[option];
	}
	else if(page_num == PAGE_GAME_DEFAULTS) {
		textPtr = tooltips_game[option];
	}
	else if(page_num == PAGE_GAME) {
		textPtr = tooltips_game[option];
	}
	return textPtr;
}

void add_tooltip_label(uiDrawObj_t* page, int page_num, int option) {
	if(get_tooltip(page_num, option)) {
		DrawAddChild(page, DrawFadingLabel(484, 54, "Press (Y) for help", 0.65f));
	}
}

void drawSettingEntryString(uiDrawObj_t* page, int *y, char *label, char *key, bool selected, bool enabled) {
	DrawAddChild(page, DrawStyledLabel(page_x_ofs_key, *y, label, label_size, false, enabled ? defaultColor:deSelectedColor));
	DrawAddChild(page, DrawStyledLabel(page_x_ofs_val, *y, key, label_size, false, enabled && selected ? defaultColor:deSelectedColor));
	*y += page_y_line; 
}

void drawSettingEntryBoolean(uiDrawObj_t* page, int *y, char *label, bool boolval, bool selected, bool enabled) {
	drawSettingEntryString(page, y, label, boolval ? "Enabled" : "Disabled", selected, enabled);
}

void drawSettingEntryNumeric(uiDrawObj_t* page, int *y, char *label, int num, bool selected, bool enabled) {
	sprintf(txtbuffer, "%i", num);
	drawSettingEntryString(page, y, label, txtbuffer, selected, enabled);
}

uiDrawObj_t* settings_draw_page(int page_num, int option, file_handle *file, ConfigEntry *gameConfig) {
	uiDrawObj_t* page = DrawEmptyBox(20,60, getVideoMode()->fbWidth-20, 460);
	char sramHOffsetStr[8];
	char forceVOffsetStr[8];
	
	// Save Settings to current device (**Shown on all tabs**)
	/** Global Settings (Page 1/) */
	// System Sound [Mono/Stereo]
	// Screen Position [+/-0]
	// System Language [English/German/French/Spanish/Italian/Dutch]
	// SD/IDE Speed [16/32 MHz]
	// Swiss Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), etc]
	// In-Game Reset [Yes/No]
	// Configuration Device [Writable device name]
	// AVE Compatibility
	// Filebrowser Type [Standard / Carousel]

	/** Advanced Settings (Page 2/) */
	// Enable USB Gecko Debug via Slot B [Yes/No]
	// Hide Unknown file types [Yes/No]	// TODO Implement
	// Stop DVD Motor on startup [Yes/No]
	// Enable WiiRD debugging in Games [Yes/No]
	// Enable File Management [Yes/No]
	// Auto-load all cheats [Yes/No]
	// Init network at startup [Yes/No]
	
	/** Current Game Settings - only if a valid GCM file is highlighted (Page 3/) */
	// Force Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), Auto, etc]
	// Force Horizontal Scale [Auto/1:1/11:10/9:8/704px/720px]
	// Force Vertical Offset [+/-0]
	// Force Vertical Filter [Auto/0/1/2]
	// Force Anisotropic Filter [Yes/No]
	// Force Widescreen [No/3D/2D+3D]
	// Force Text Encoding [Auto/ANSI/SJIS]
	// Disable Audio Streaming [Yes/No]

	bool isNavOption = false;
	// Add paging and save/cancel buttons
	if(page_num != PAGE_MIN) {
		isNavOption = option == settings_count_pp[page_num]-(page_num != PAGE_MAX ? 3:2);
		DrawAddChild(page, DrawSelectableButton(50, 390, -1, 420, "Back", isNavOption));
	}
	if(page_num != PAGE_MAX) {
		isNavOption = isNavOption || option == settings_count_pp[page_num]-2;
		DrawAddChild(page, DrawSelectableButton(510, 390, -1, 420, "Next", option == settings_count_pp[page_num]-2 ? B_SELECTED:B_NOSELECT));
	}
	DrawAddChild(page, DrawSelectableButton(120, 425, -1, 455, "Save & Exit", option == settings_count_pp[page_num]-1 ? B_SELECTED:B_NOSELECT));
	DrawAddChild(page, DrawSelectableButton(320, 425, -1, 455, "Discard & Exit", option ==  settings_count_pp[page_num] ? B_SELECTED:B_NOSELECT));
	isNavOption = isNavOption || (option >= settings_count_pp[page_num]-1);
	
	int page_y_ofs = 110;
	if(!isNavOption)
		DrawAddChild(page, DrawStyledLabel(20, page_y_ofs + (page_y_line * option) + (page_y_line / 4), "*", 0.35f, false, disabledColor));
	// Page specific buttons
	if(page_num == PAGE_GLOBAL) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Global Settings (1/5):"));
		drawSettingEntryString(page, &page_y_ofs, "System Sound:", swissSettings.sramStereo ? "Stereo":"Mono", option == SET_SYS_SOUND, true);
		sprintf(sramHOffsetStr, "%+hi", swissSettings.sramHOffset);
		drawSettingEntryString(page, &page_y_ofs, "Screen Position:", sramHOffsetStr, option == SET_SCREEN_POS, true);
		drawSettingEntryString(page, &page_y_ofs, "System Language:", swissSettings.sramLanguage > SRAM_LANG_MAX ? "Unknown" : sramLang[swissSettings.sramLanguage], option == SET_SYS_LANG, true);
		drawSettingEntryString(page, &page_y_ofs, "SD/IDE Speed:", swissSettings.exiSpeed ? "32 MHz":"16 MHz", option == SET_EXI_SPEED, true);	
		drawSettingEntryString(page, &page_y_ofs, "Swiss Video Mode:", uiVModeStr[swissSettings.uiVMode], option == SET_SWISS_VIDEOMODE, true);
		drawSettingEntryString(page, &page_y_ofs, "In-Game Reset:", igrTypeStr[swissSettings.igrType], option == SET_IGR, true);
		drawSettingEntryString(page, &page_y_ofs, "Configuration Device:", getConfigDeviceName(&swissSettings), option == SET_CONFIG_DEV, true);
		drawSettingEntryString(page, &page_y_ofs, "AVE Compatibility:", aveCompatStr[swissSettings.aveCompat], option == SET_AVE_COMPAT, true);
		drawSettingEntryString(page, &page_y_ofs, "File Browser Type:", fileBrowserStr[swissSettings.fileBrowserType], option == SET_FILEBROWSER_TYPE, true);
	}
	else if(page_num == PAGE_NETWORK) {
		bool netEnable = exi_bba_exists();
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Network Settings (2/5):"));	
		drawSettingEntryBoolean(page, &page_y_ofs, "Init network at startup:", swissSettings.initNetworkAtStart, option == SET_INIT_NET, netEnable);
		drawSettingEntryString(page, &page_y_ofs, "FTP Host IP:", swissSettings.ftpHostIp, option == SET_FTP_HOSTIP, netEnable);
		drawSettingEntryNumeric(page, &page_y_ofs, "FTP Port:", swissSettings.ftpPort, option == SET_FTP_PORT, netEnable);
		drawSettingEntryString(page, &page_y_ofs, "FTP Username:", swissSettings.ftpUserName, option == SET_FTP_USER, netEnable);
		drawSettingEntryString(page, &page_y_ofs, "FTP Password:", "*****", option == SET_FTP_PASS, netEnable);
		drawSettingEntryBoolean(page, &page_y_ofs, "FTP PASV Mode:", swissSettings.ftpUsePasv, option == SET_FTP_PASV, netEnable);
		drawSettingEntryString(page, &page_y_ofs, "FSP Host IP:", swissSettings.fspHostIp, option == SET_FSP_HOSTIP, netEnable);
		drawSettingEntryNumeric(page, &page_y_ofs, "FSP Port:", swissSettings.fspPort, option == SET_FSP_PORT, netEnable);
		drawSettingEntryString(page, &page_y_ofs, "FSP Password:", "*****", option == SET_FSP_PASS, netEnable);
	}
	else if(page_num == PAGE_ADVANCED) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Advanced Settings (3/5):"));
		drawSettingEntryBoolean(page, &page_y_ofs, "USB Gecko Debug via Slot B:", swissSettings.debugUSB, option == SET_ENABLE_USBGECKODBG, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Hide Unknown file types:", swissSettings.hideUnknownFileTypes, option == SET_HIDE_UNK, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Stop DVD Motor on startup:", swissSettings.stopMotor, option == SET_STOP_MOTOR, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "WiiRD debugging:", swissSettings.wiirdDebug, option == SET_WIIRDDBG, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Savestate:", swissSettings.enableSavestate, option == SET_SAVESTATE, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "File Management:", swissSettings.enableFileManagement, option == SET_FILE_MGMT, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Auto-load all cheats:", swissSettings.autoCheats, option == SET_ALL_CHEATS, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Disable Video Patches:", swissSettings.disableVideoPatches, option == SET_ENABLE_VIDPATCH, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Force DTV Status:", swissSettings.forceDTVStatus, option == SET_FORCE_DTVSTATUS, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Boot through IPL:", swissSettings.bs2Boot, option == SET_BS2BOOT, true);
	}
	else if(page_num == PAGE_GAME_DEFAULTS) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Default Game Settings (4/5):"));
		bool enableGameVideoPatches = !swissSettings.disableVideoPatches;
		drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[swissSettings.gameVMode], option == SET_DEFAULT_FORCE_VIDEOMODE, enableGameVideoPatches);
		drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[swissSettings.forceHScale], option == SET_DEFAULT_HORIZ_SCALE, enableGameVideoPatches);
		sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
		drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_DEFAULT_VERT_OFFSET, enableGameVideoPatches);
		drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[swissSettings.forceVFilter], option == SET_DEFAULT_VERT_FILTER, enableGameVideoPatches);
		drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", swissSettings.disableDithering, option == SET_DEFAULT_ALPHA_DITHER, enableGameVideoPatches);
		drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", swissSettings.forceAnisotropy, option == SET_DEFAULT_ANISO_FILTER, true);
		drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[swissSettings.forceWidescreen], option == SET_DEFAULT_WIDESCREEN, true);
		drawSettingEntryString(page, &page_y_ofs, "Force Text Encoding:", forceEncodingStr[swissSettings.forceEncoding], option == SET_DEFAULT_TEXT_ENCODING, true);
		drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[swissSettings.invertCStick], option == SET_DEFAULT_INVERT_CAMERA, true);
		drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Audio Streaming:", swissSettings.emulateAudioStreaming, option == SET_DEFAULT_AUDIO_STREAMING, true);
	}
	else if(page_num == PAGE_GAME) {
		DrawAddChild(page, DrawLabel(page_x_ofs_key, 65, "Current Game Settings (5/5):"));
		bool enableGamePatches = file != NULL && gameConfig != NULL;
		if(enableGamePatches) {
			bool enableGameVideoPatches = enableGamePatches && !swissSettings.disableVideoPatches;
			drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[gameConfig->gameVMode], option == SET_FORCE_VIDEOMODE, enableGameVideoPatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[gameConfig->forceHScale], option == SET_HORIZ_SCALE, enableGameVideoPatches);
			sprintf(forceVOffsetStr, "%+hi", gameConfig->forceVOffset);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_VERT_OFFSET, enableGameVideoPatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[gameConfig->forceVFilter], option == SET_VERT_FILTER, enableGameVideoPatches);
			drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", gameConfig->disableDithering, option == SET_ALPHA_DITHER, enableGameVideoPatches);
			drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", gameConfig->forceAnisotropy, option == SET_ANISO_FILTER, enableGamePatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[gameConfig->forceWidescreen], option == SET_WIDESCREEN, enableGamePatches);
			drawSettingEntryString(page, &page_y_ofs, "Force Text Encoding:", forceEncodingStr[gameConfig->forceEncoding], option == SET_TEXT_ENCODING, enableGamePatches);
			drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[gameConfig->invertCStick], option == SET_INVERT_CAMERA, enableGamePatches);
			drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Audio Streaming:", gameConfig->emulateAudioStreaming, option == SET_AUDIO_STREAMING, enableGamePatches);
		}
		else {
			// Just draw the defaults again
			drawSettingEntryString(page, &page_y_ofs, "Force Video Mode:", gameVModeStr[swissSettings.gameVMode], option == SET_DEFAULT_FORCE_VIDEOMODE, false);
			drawSettingEntryString(page, &page_y_ofs, "Force Horizontal Scale:", forceHScaleStr[swissSettings.forceHScale], option == SET_DEFAULT_HORIZ_SCALE, false);
			sprintf(forceVOffsetStr, "%+hi", swissSettings.forceVOffset);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Offset:", forceVOffsetStr, option == SET_DEFAULT_VERT_OFFSET, false);
			drawSettingEntryString(page, &page_y_ofs, "Force Vertical Filter:", forceVFilterStr[swissSettings.forceVFilter], option == SET_DEFAULT_VERT_FILTER, false);
			drawSettingEntryBoolean(page, &page_y_ofs, "Disable Alpha Dithering:", swissSettings.disableDithering, option == SET_DEFAULT_ALPHA_DITHER, false);
			drawSettingEntryBoolean(page, &page_y_ofs, "Force Anisotropic Filter:", swissSettings.forceAnisotropy, option == SET_DEFAULT_ANISO_FILTER, false);
			drawSettingEntryString(page, &page_y_ofs, "Force Widescreen:", forceWidescreenStr[swissSettings.forceWidescreen], option == SET_DEFAULT_WIDESCREEN, false);
			drawSettingEntryString(page, &page_y_ofs, "Force Text Encoding:", forceEncodingStr[swissSettings.forceEncoding], option == SET_DEFAULT_TEXT_ENCODING, false);
			drawSettingEntryString(page, &page_y_ofs, "Invert Camera Stick:", invertCStickStr[swissSettings.invertCStick], option == SET_DEFAULT_INVERT_CAMERA, false);
			drawSettingEntryBoolean(page, &page_y_ofs, "Emulate Audio Streaming:", swissSettings.emulateAudioStreaming, option == SET_DEFAULT_AUDIO_STREAMING, false);
		}
	}
	// If we have a tooltip for this page/option, add a fading label telling the user to press Y for help
	add_tooltip_label(page, page_num, option);
	
	DrawPublish(page);
	return page;
}

void settings_toggle(int page, int option, int direction, file_handle *file, ConfigEntry *gameConfig) {
	if(page == PAGE_GLOBAL) {
		switch(option) {
			case SET_SYS_SOUND:
				swissSettings.sramStereo ^= 1;
			break;
			case SET_SCREEN_POS:
				if(swissSettings.aveCompat == 1) {
					swissSettings.sramHOffset /= 2;
					swissSettings.sramHOffset += direction;
					swissSettings.sramHOffset *= 2;
				}
				else {
					swissSettings.sramHOffset += direction;
				}
			break;
			case SET_SYS_LANG:
				swissSettings.sramLanguage += direction;
				if(swissSettings.sramLanguage > SRAM_LANG_MAX)
					swissSettings.sramLanguage = 0;
				if(swissSettings.sramLanguage < 0)
					swissSettings.sramLanguage = SRAM_LANG_MAX;
			break;
			case SET_EXI_SPEED:
				swissSettings.exiSpeed ^= 1;
			break;
			case SET_SWISS_VIDEOMODE:
				swissSettings.uiVMode += direction;
				if(swissSettings.uiVMode > 4)
					swissSettings.uiVMode = 0;
				if(swissSettings.uiVMode < 0)
					swissSettings.uiVMode = 4;
			break;
			case SET_IGR:
				swissSettings.igrType += direction;
				if(swissSettings.igrType > 2)
					swissSettings.igrType = 0;
				if(swissSettings.igrType < 0)
					swissSettings.igrType = 2;
			break;
			case SET_CONFIG_DEV:
			{
				int curDevicePos = -1;
				
				// Set it to the first writable device available
				if(swissSettings.configDeviceId == DEVICE_ID_UNK) {
					for(int i = 0; i < MAX_DEVICES; i++) {
						if(allDevices[i] != NULL && (allDevices[i]->features & (FEAT_WRITE|FEAT_BOOT_DEVICE)) == (FEAT_WRITE|FEAT_BOOT_DEVICE)) {
							swissSettings.configDeviceId = allDevices[i]->deviceUniqueId;
							return;
						}
					}
				}
				
				// get position in allDevices for current save device
				for(int i = 0; i < MAX_DEVICES; i++) {
					if(allDevices[i] != NULL && allDevices[i]->deviceUniqueId == swissSettings.configDeviceId) {
						curDevicePos = i;
						break;
					}
				}

				if(curDevicePos >= 0) {
					if(direction > 0) {
						curDevicePos = allDevices[curDevicePos+1] == NULL ? 0 : curDevicePos+1;
					}
					else {
						curDevicePos = curDevicePos > 0 ? curDevicePos-1 : 0;
					}
					// Go to next writable device
					while((allDevices[curDevicePos] == NULL) || (allDevices[curDevicePos]->features & (FEAT_WRITE|FEAT_BOOT_DEVICE)) != (FEAT_WRITE|FEAT_BOOT_DEVICE)) {
						curDevicePos += direction;
						if((curDevicePos < 0) || (curDevicePos >= MAX_DEVICES)){
							curDevicePos = direction > 0 ? 0 : MAX_DEVICES-1;
						}
					}
					if(allDevices[curDevicePos] != NULL) {
						swissSettings.configDeviceId = allDevices[curDevicePos]->deviceUniqueId;
					}
				}
			}
			break;
			case SET_AVE_COMPAT:
				swissSettings.aveCompat += direction;
				if(swissSettings.aveCompat > 3)
					swissSettings.aveCompat = 0;
				if(swissSettings.aveCompat < 0)
					swissSettings.aveCompat = 3;
			break;
			case SET_FILEBROWSER_TYPE:
				swissSettings.fileBrowserType ^= 1;
			break;
		}	
	}
	else if(page == PAGE_NETWORK) {
		switch(option) {
			case SET_INIT_NET:
				swissSettings.initNetworkAtStart ^= 1;
			break;
			case SET_FTP_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "FTP Host IP", &swissSettings.ftpHostIp, sizeof(swissSettings.ftpHostIp));
			break;
			case SET_FTP_PORT:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "FTP Port", &swissSettings.ftpPort, 5);
			break;
			case SET_FTP_USER:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA, "FTP Username", &swissSettings.ftpUserName, sizeof(swissSettings.ftpUserName));
			break;
			case SET_FTP_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "FTP Password", &swissSettings.ftpPassword, sizeof(swissSettings.ftpPassword));
			break;
			case SET_FTP_PASV:
				swissSettings.ftpUsePasv ^= 1;
			break;
			case SET_FSP_HOSTIP:
				DrawGetTextEntry(ENTRYMODE_IP, "FSP Host IP", &swissSettings.fspHostIp, sizeof(swissSettings.fspHostIp));
			break;
			case SET_FSP_PORT:
				DrawGetTextEntry(ENTRYMODE_NUMERIC, "FSP Port", &swissSettings.fspPort, 5);
			break;
			case SET_FSP_PASS:
				DrawGetTextEntry(ENTRYMODE_NUMERIC|ENTRYMODE_ALPHA|ENTRYMODE_MASKED, "FSP Password", &swissSettings.fspPassword, sizeof(swissSettings.fspPassword));
			break;
		}
	}
	else if(page == PAGE_ADVANCED) {
		switch(option) {
			case SET_ENABLE_USBGECKODBG:
				swissSettings.debugUSB ^= 1;
			break;
			case SET_HIDE_UNK:
				swissSettings.hideUnknownFileTypes ^= 1;
			break;
			case SET_STOP_MOTOR:
				swissSettings.stopMotor ^= 1;
			break;
			case SET_WIIRDDBG:
				swissSettings.wiirdDebug ^=1;
			break;
			case SET_SAVESTATE:
				swissSettings.enableSavestate ^=1;
			break;
			case SET_FILE_MGMT:
				swissSettings.enableFileManagement ^=1;
			break;
			case SET_ALL_CHEATS:
				swissSettings.autoCheats ^=1;
			break;
			case SET_ENABLE_VIDPATCH:
				swissSettings.disableVideoPatches ^= 1;
			break;
			case SET_FORCE_DTVSTATUS:
				swissSettings.forceDTVStatus ^= 1;
			break;
			case SET_BS2BOOT:
				swissSettings.bs2Boot ^= 1;
			break;
		}
	}
	else if(page == PAGE_GAME_DEFAULTS) {
		switch(option) {
			case SET_DEFAULT_FORCE_VIDEOMODE:
				if(!swissSettings.disableVideoPatches) {
					swissSettings.gameVMode += direction;
					if(swissSettings.gameVMode > 14)
						swissSettings.gameVMode = 0;
					if(swissSettings.gameVMode < 0)
						swissSettings.gameVMode = 14;
					if(swissSettings.aveCompat) {
						while(swissSettings.gameVMode >= 6 && swissSettings.gameVMode <= 7)
							swissSettings.gameVMode += direction;
						while(swissSettings.gameVMode >= 13 && swissSettings.gameVMode <= 14)
							swissSettings.gameVMode += direction;
					}
					if(!swissSettings.forceDTVStatus && !VIDEO_HaveComponentCable()) {
						while(swissSettings.gameVMode >= 4 && swissSettings.gameVMode <= 7)
							swissSettings.gameVMode += direction;
						while(swissSettings.gameVMode >= 11 && swissSettings.gameVMode <= 14)
							swissSettings.gameVMode += direction;
					}
					if(swissSettings.gameVMode > 14)
						swissSettings.gameVMode = 0;
					if(swissSettings.gameVMode < 0)
						swissSettings.gameVMode = 14;
				}
			break;
			case SET_DEFAULT_HORIZ_SCALE:
				if(!swissSettings.disableVideoPatches) {
					swissSettings.forceHScale += direction;
					if(swissSettings.forceHScale > 6)
						swissSettings.forceHScale = 0;
					if(swissSettings.forceHScale < 0)
						swissSettings.forceHScale = 6;
				}
			break;
			case SET_DEFAULT_VERT_OFFSET:
				if(!swissSettings.disableVideoPatches)
					swissSettings.forceVOffset += direction;
			break;
			case SET_DEFAULT_VERT_FILTER:
				if(!swissSettings.disableVideoPatches) {
					swissSettings.forceVFilter += direction;
					if(swissSettings.forceVFilter > 3)
						swissSettings.forceVFilter = 0;
					if(swissSettings.forceVFilter < 0)
						swissSettings.forceVFilter = 3;
				}
			break;
			case SET_DEFAULT_ALPHA_DITHER:
				if(!swissSettings.disableVideoPatches)
					swissSettings.disableDithering ^= 1;
			break;
			case SET_DEFAULT_ANISO_FILTER:
				swissSettings.forceAnisotropy ^= 1;
			break;
			case SET_DEFAULT_WIDESCREEN:
				swissSettings.forceWidescreen += direction;
				if(swissSettings.forceWidescreen > 2)
					swissSettings.forceWidescreen = 0;
				if(swissSettings.forceWidescreen < 0)
					swissSettings.forceWidescreen = 2;
			break;
			case SET_DEFAULT_TEXT_ENCODING:
				swissSettings.forceEncoding += direction;
				if(swissSettings.forceEncoding > 3)
					swissSettings.forceEncoding = 0;
				if(swissSettings.forceEncoding < 0)
					swissSettings.forceEncoding = 3;
			break;
			case SET_DEFAULT_INVERT_CAMERA:
				swissSettings.invertCStick += direction;
				if(swissSettings.invertCStick > 3)
					swissSettings.invertCStick = 0;
				if(swissSettings.invertCStick < 0)
					swissSettings.invertCStick = 3;
			break;
			case SET_DEFAULT_AUDIO_STREAMING:
				swissSettings.emulateAudioStreaming ^= 1;
			break;
		}
	}
	else if(page == PAGE_GAME && file != NULL && gameConfig != NULL) {
		switch(option) {
			case SET_FORCE_VIDEOMODE:
				if(!swissSettings.disableVideoPatches) {
					gameConfig->gameVMode += direction;
					if(gameConfig->gameVMode > 14)
						gameConfig->gameVMode = 0;
					if(gameConfig->gameVMode < 0)
						gameConfig->gameVMode = 14;
					if(swissSettings.aveCompat) {
						while(gameConfig->gameVMode >= 6 && gameConfig->gameVMode <= 7)
							gameConfig->gameVMode += direction;
						while(gameConfig->gameVMode >= 13 && gameConfig->gameVMode <= 14)
							gameConfig->gameVMode += direction;
					}
					if(!swissSettings.forceDTVStatus && !VIDEO_HaveComponentCable()) {
						while(gameConfig->gameVMode >= 4 && gameConfig->gameVMode <= 7)
							gameConfig->gameVMode += direction;
						while(gameConfig->gameVMode >= 11 && gameConfig->gameVMode <= 14)
							gameConfig->gameVMode += direction;
					}
					if(gameConfig->gameVMode > 14)
						gameConfig->gameVMode = 0;
					if(gameConfig->gameVMode < 0)
						gameConfig->gameVMode = 14;
				}
			break;
			case SET_HORIZ_SCALE:
				if(!swissSettings.disableVideoPatches) {
					gameConfig->forceHScale += direction;
					if(gameConfig->forceHScale > 6)
						gameConfig->forceHScale = 0;
					if(gameConfig->forceHScale < 0)
						gameConfig->forceHScale = 6;
				}
			break;
			case SET_VERT_OFFSET:
				if(!swissSettings.disableVideoPatches)
					gameConfig->forceVOffset += direction;
			break;
			case SET_VERT_FILTER:
				if(!swissSettings.disableVideoPatches) {
					gameConfig->forceVFilter += direction;
					if(gameConfig->forceVFilter > 3)
						gameConfig->forceVFilter = 0;
					if(gameConfig->forceVFilter < 0)
						gameConfig->forceVFilter = 3;
				}
			break;
			case SET_ALPHA_DITHER:
				if(!swissSettings.disableVideoPatches)
					gameConfig->disableDithering ^= 1;
			break;
			case SET_ANISO_FILTER:
				gameConfig->forceAnisotropy ^= 1;
			break;
			case SET_WIDESCREEN:
				gameConfig->forceWidescreen += direction;
				if(gameConfig->forceWidescreen > 2)
					gameConfig->forceWidescreen = 0;
				if(gameConfig->forceWidescreen < 0)
					gameConfig->forceWidescreen = 2;
			break;
			case SET_TEXT_ENCODING:
				gameConfig->forceEncoding += direction;
				if(gameConfig->forceEncoding > 3)
					gameConfig->forceEncoding = 0;
				if(gameConfig->forceEncoding < 0)
					gameConfig->forceEncoding = 3;
			break;
			case SET_INVERT_CAMERA:
				gameConfig->invertCStick += direction;
				if(gameConfig->invertCStick > 3)
					gameConfig->invertCStick = 0;
				if(gameConfig->invertCStick < 0)
					gameConfig->invertCStick = 3;
			break;
			case SET_AUDIO_STREAMING:
				gameConfig->emulateAudioStreaming ^= 1;
			break;
		}
	}
}

int show_settings(file_handle *file, ConfigEntry *config) {
	int page = PAGE_GLOBAL, option = SET_SYS_SOUND;
	
	// Copy current settings to a temp copy in case the user cancels out
	memcpy((void*)&tempSettings,(void*)&swissSettings, sizeof(SwissSettings));
	
	// Setup the settings for the current game
	if(config != NULL) {
		page = PAGE_GAME;
	}
		
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A){ VIDEO_WaitVSync (); }
	while(1) {
		uiDrawObj_t* settingsPage = settings_draw_page(page, option, file, config);
		while (!((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_UP) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) 
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B)
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
			|| (PAD_ButtonsHeld(0) & PAD_BUTTON_Y)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
			|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L)))
			{ VIDEO_WaitVSync (); }
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_Y) {
			char *tooltip = get_tooltip(page, option);
			if(tooltip) {
				uiDrawObj_t* tooltipBox = DrawPublish(DrawTooltip(tooltip));
				while (PAD_ButtonsHeld(0) & PAD_BUTTON_Y){ VIDEO_WaitVSync (); }
				while (!((PAD_ButtonsHeld(0) & PAD_BUTTON_Y) || (PAD_ButtonsHeld(0) & PAD_BUTTON_B))){ VIDEO_WaitVSync (); }
				DrawDispose(tooltipBox);
			}
		}
		if(btns & PAD_BUTTON_RIGHT) {
			// If we're on a button (Back, Next, Save, Exit), allow left/right movement
			if((page == PAGE_MIN || page == PAGE_MAX) && (option >= settings_count_pp[page]-2) && option < settings_count_pp[page]) {
				option++;
			}
			else if((page != PAGE_MIN && page != PAGE_MAX) && (option >= settings_count_pp[page]-3) && option < settings_count_pp[page]) {
				option++;
			}
			else {
				settings_toggle(page, option, 1, file, config);
			}
		}
		if(btns & PAD_BUTTON_LEFT) {
			// If we're on a button (Back, Next, Save, Exit), allow left/right movement
			if((page == PAGE_MIN || page == PAGE_MAX) && (option > settings_count_pp[page]-2)) {
				option--;
			}
			else if((page != PAGE_MIN && page != PAGE_MAX) && (option > settings_count_pp[page]-3)) {
				option--;
			}
			else {
				settings_toggle(page, option, -1, file, config);
			}
		}
		if((btns & PAD_BUTTON_DOWN) && option < settings_count_pp[page])
			option++;
		if((btns & PAD_BUTTON_UP) && option > PAGE_MIN)
			option--;
		if((btns & PAD_TRIGGER_R) && page < PAGE_MAX) {
			page++; option = 0;
		}
		if((btns & PAD_TRIGGER_L) && page > PAGE_GLOBAL) {
			page--; option = 0;
		}
		if((btns & PAD_BUTTON_B))
			option = settings_count_pp[page];
		// Handle all options/buttons here
		if((btns & PAD_BUTTON_A)) {
			// Generic Save/Cancel/Back/Next button actions
			if(option == settings_count_pp[page]-1) {
				uiDrawObj_t *msgBox = DrawPublish(DrawProgressBar(true, 0, "Saving changes ..."));
				// Change Swiss video mode if it was modified.
				if(tempSettings.uiVMode != swissSettings.uiVMode) {
					GXRModeObj *newmode = getVideoModeFromSwissSetting(swissSettings.uiVMode);
					setVideoMode(newmode);
				}
				// Save settings to SRAM
				if(swissSettings.uiVMode > 0) {
					swissSettings.sram60Hz = (swissSettings.uiVMode >= 1) && (swissSettings.uiVMode <= 2);
					swissSettings.sramProgressive = (swissSettings.uiVMode == 2) || (swissSettings.uiVMode == 4);
				}
				if(swissSettings.aveCompat == 1) {
					swissSettings.sramHOffset &= ~1;
				}
				sram = __SYS_LockSram();
				sram->display_offsetH = swissSettings.sramHOffset;
				sram->ntd = swissSettings.sram60Hz ? (sram->ntd|0x40):(sram->ntd&~0x40);
				sram->lang = swissSettings.sramLanguage;
				sram->flags = swissSettings.sramProgressive ? (sram->flags|0x80):(sram->flags&~0x80);
				sram->flags = swissSettings.sramStereo ? (sram->flags|0x04):(sram->flags&~0x04);
				sram->flags = (swissSettings.sramVideo&0x03)|(sram->flags&~0x03);
				__SYS_UnlockSram(1);
				while(!__SYS_SyncSram());
				sramex = __SYS_LockSramEx();
				sramex->__padding0 = swissSettings.configDeviceId;
				__SYS_UnlockSramEx(1);
				while(!__SYS_SyncSram());
				// Update our .ini (in memory)
				if(config != NULL) {
					config_update(config);
					DrawDispose(msgBox);
				}
				// flush settings to .ini
				if(config_update_file()) {
					DrawDispose(msgBox);
					msgBox = DrawPublish(DrawMessageBox(D_INFO,"Config Saved Successfully!"));
					sleep(1);
					DrawDispose(msgBox);
				}
				else {
					DrawDispose(msgBox);
					msgBox = DrawPublish(DrawMessageBox(D_INFO,"Config Failed to Save!"));
					sleep(1);
					DrawDispose(msgBox);
				}
				DrawDispose(settingsPage);
				return 1;
			}
			if(option == settings_count_pp[page]) {
				// Exit without saving (revert)
				memcpy((void*)&swissSettings, (void*)&tempSettings, sizeof(SwissSettings));
				DrawDispose(settingsPage);
				return 0;
			}
			if((page != PAGE_MAX) && (option == settings_count_pp[page]-2)) {
				page++; option = 0;
			}
			if((page != PAGE_MIN) && (option == settings_count_pp[page]-(page != PAGE_MAX ? 3:2))) {
				page--; option = 0;
			}
			// These use text input, allow them to be accessed with the A button
			if(page == PAGE_NETWORK && ((option >= SET_FTP_HOSTIP && option <= SET_FTP_PASS) || (option >= SET_FSP_HOSTIP && option <= SET_FSP_PASS))) {
				settings_toggle(page, option, -1, file, config);
			}
		}
		while ((PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_UP) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_B) 
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
				|| (PAD_ButtonsHeld(0) & PAD_BUTTON_Y)
				|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_R)
				|| (PAD_ButtonsHeld(0) & PAD_TRIGGER_L))
			{ VIDEO_WaitVSync (); }
		DrawDispose(settingsPage);
	}
}

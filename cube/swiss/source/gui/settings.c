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
#include "ata.h"
#include "exi.h"
#include "bba.h"

SwissSettings tempSettings;
char *bootDevicesStr[] = {"SD Slot A", "SD Slot B", "IDE Slot A", "IDE Slot B"};
char *uiVModeStr[] = {"NTSC (480i)", "PAL (576i)", "NTSC (480p)"};

// Number of settings (including Back, Next, Save, Exit buttons) per page
int settings_count_pp[3] = {7, 10, 6};

void settings_draw_page(int page_num, int option) {
	doBackdrop();
	DrawEmptyBox(20,60, vmode->fbWidth-20, 460, COLOR_BLACK);
		
	// Save Settings to current device (**Shown on all tabs**)
	/** Global Settings (Page 1/) */
	// IPL/Game Language [English/German/French/Spanish/Italian/Dutch]
	// IPL/Game Audio [Mono/Stereo]
	// Default Device [SD A/SD B/IDE A/IDE B/Qoob/Ask]
	// SD/IDE Speed [16/32 MHz]
	// Swiss Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz)]
	
	/** Current Game Settings - only if a valid GCM file is highlighted (Page 2/) */
	// Force Video Mode [576i (PAL 50Hz), 480i (NTSC 60Hz), 480p (NTSC 60Hz), Auto]
	// Patch Type [Low / High Level]
	// If Low Level, Use Memory Location [Low/High]
	// If High Level, Disable Interrupts [Yes/No]
	// Mute Audio Streaming [Yes/No]
	// Try to mute audio stutter [Yes/No]
	
	/** Advanced Settings (Page 3/) */
	// Enable USB Gecko Debug via Slot B [Yes/No]
	// Force No DVD Drive Mode [Yes/No]
	// Hide Unknown file types [Yes/No]	// TO BE IMPLEMENTED

	if(!page_num) {
		WriteFont(30, 65, " Global Settings (1/3):");
		WriteFontStyled(30, 120, "IPL/Game Language:", 1.0f, false, defaultColor);
		DrawSelectableButton(380, 120, -1, 150, getSramLang(swissSettings.sramLanguage), option == 0 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 160, "IPL/Game Audio:", 1.0f, false, defaultColor);
		DrawSelectableButton(380, 160, -1, 190, swissSettings.sramStereo ? "Stereo":"Mono", option == 1 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 200, "Default Device:", 1.0f, false, defaultColor);
		DrawSelectableButton(380, 200, -1, 230, bootDevicesStr[swissSettings.defaultDevice], option == 2 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 240, "SD/IDE Speed:", 1.0f, false, defaultColor);
		DrawSelectableButton(380, 240, -1, 270, swissSettings.exiSpeed ? "32 MHz":"16 MHz", option == 3 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 280, "Swiss Video Mode:", 1.0f, false, defaultColor);
		DrawSelectableButton(380, 280, -1, 310, uiVModeStr[swissSettings.uiVMode], option == 4 ? B_SELECTED:B_NOSELECT,-1);
	}
	else if(page_num == 1) {
		WriteFont(30, 65, "Current Game Settings (2/3):");
		WriteFontStyled(30, 110, "Force Video Mode:", 1.0f, false, defaultColor);
		DrawSelectableButton(470, 110, -1, 140, "Auto", option == 0 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 150, "Patch Type:", 1.0f, false, defaultColor);
		DrawSelectableButton(470, 150, -1, 180, swissSettings.useHiLevelPatch ? "High":"Low", option == 1 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 190, "If Low Level, Memory Location:", 1.0f, false, defaultColor);
		DrawSelectableButton(470, 190, -1, 220, swissSettings.useHiMemArea ? "High":"Low", option == 2 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 230, "If High Level, Disable Interrupts:", 1.0f, false, defaultColor);
		DrawSelectableButton(470, 230, -1, 260, swissSettings.disableInterrupts ? "Yes":"No", option == 3 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 270, "Mute Audio Streaming:", 1.0f, false, defaultColor);
		DrawSelectableButton(470, 270, -1, 300, swissSettings.muteAudioStreaming ? "Yes":"No", option == 4 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 310, "Try to mute audio stutter:", 1.0f, false, defaultColor);
		DrawSelectableButton(470, 310, -1, 340, swissSettings.muteAudioStutter ? "Yes":"No", option == 5 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 350, "No Disc Mode:", 1.0f, false, defaultColor);
		DrawSelectableButton(470, 350, -1, 380, swissSettings.noDiscMode ? "Yes":"No", option == 6 ? B_SELECTED:B_NOSELECT,-1);
	}
	else if(page_num == 2) {
		WriteFont(30, 65, "Advanced Settings (3/3):");
		WriteFontStyled(30, 120, "Enable USB Gecko Debug via Slot B:", 1.0f, false, defaultColor);
		DrawSelectableButton(500, 120, -1, 150, swissSettings.debugUSB ? "Yes":"No", option == 0 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 160, "Force No DVD Drive Mode:", 1.0f, false, defaultColor);
		DrawSelectableButton(450, 160, -1, 190, swissSettings.hasDVDDrive ? "No":"Yes", option == 1 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 200, "Hide Unknown file types:", 1.0f, false, defaultColor);
		DrawSelectableButton(450, 200, -1, 230, swissSettings.hideUnknownFileTypes ? "Yes":"No", option == 2 ? B_SELECTED:B_NOSELECT,-1);
		WriteFontStyled(30, 240, "Stop DVD Motor on startup:", 1.0f, false, defaultColor);
		DrawSelectableButton(450, 240, -1, 270, swissSettings.stopMotor ? "Yes":"No", option == 3 ? B_SELECTED:B_NOSELECT,-1);
	}
	if(page_num != 0) {
		DrawSelectableButton(40, 390, -1, 420, "Back", 
		option == settings_count_pp[page_num]-(page_num != 2 ? 3:2) ? B_SELECTED:B_NOSELECT,-1);
	}
	if(page_num != 2) {
		DrawSelectableButton(510, 390, -1, 420, "Next", 
		option == settings_count_pp[page_num]-2 ? B_SELECTED:B_NOSELECT,-1);
	}
	DrawSelectableButton(100, 425, -1, 455, "Save & Exit", option == settings_count_pp[page_num]-1 ? B_SELECTED:B_NOSELECT,-1);
	DrawSelectableButton(320, 425, -1, 455, "Discard & Exit", option ==  settings_count_pp[page_num] ? B_SELECTED:B_NOSELECT,-1);
	DrawFrameFinish();
}

void settings_toggle(int page, int option, int direction) {
	if(page == 0) {
		switch(option) {
			case 0:
				swissSettings.sramLanguage += direction;
				if(swissSettings.sramLanguage > 5)
					swissSettings.sramLanguage = 0;
				if(swissSettings.sramLanguage < 0)
					swissSettings.sramLanguage = 5;
			break;
			case 1:
				swissSettings.sramStereo ^= 4;
			break;
			case 2:
				swissSettings.defaultDevice += direction;
				if(swissSettings.defaultDevice > 3)
					swissSettings.defaultDevice = 0;
				if(swissSettings.defaultDevice < 0)
					swissSettings.defaultDevice = 3;
			break;
			case 3:
				swissSettings.exiSpeed ^= 1;
			break;
			case 4:
				swissSettings.uiVMode += direction;
				if(swissSettings.uiVMode > 2)
					swissSettings.uiVMode = 0;
				if(swissSettings.uiVMode < 0)
					swissSettings.uiVMode = 2;
			break;
		}	
	}
	else if(page == 1) {
		switch(option) {
			case 0:
				
			break;
			case 1:
				swissSettings.useHiLevelPatch ^= 1;
			break;
			case 2:
				swissSettings.useHiMemArea ^= 1;
			break;
			case 3:
				swissSettings.disableInterrupts ^= 1;
			break;
			case 4:
				swissSettings.muteAudioStreaming ^= 1;
			break;
			case 5:
				swissSettings.muteAudioStutter ^= 1;
			break;
			case 6:
				swissSettings.noDiscMode ^= 1;
			break;
		}
	}
	else if(page == 2) {
		switch(option) {
			case 0:
				swissSettings.debugUSB ^= 1;
			break;
			case 1:
				swissSettings.hasDVDDrive ^= 1;
			break;
			case 2:
				swissSettings.hideUnknownFileTypes ^= 1;
			break;
			case 3:
				swissSettings.stopMotor ^= 1;
			break;
		}
	}
}

void show_settings() {
	// Refresh SRAM in case user changed it from IPL
	syssram* sram = __SYS_LockSram();
	swissSettings.sramStereo = sram->flags & 4;
	swissSettings.sramLanguage = sram->lang;
	__SYS_UnlockSram(0);
	
	// Copy current settings to a temp copy in case the user cancels out
	memcpy((void*)&tempSettings,(void*)&swissSettings, sizeof(SwissSettings));
	
	int page = 0, option = 0;
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A);
	while(1) {
		settings_draw_page(page, option);
		while (!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
			&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
			&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) 
			&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) 
			&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)
			&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A));
		u16 btns = PAD_ButtonsHeld(0);
		if(btns & PAD_BUTTON_RIGHT) {
			settings_toggle(page, option, 1);
		}
		if(btns & PAD_BUTTON_LEFT) {
			settings_toggle(page, option, -1);
		}
		if((btns & PAD_BUTTON_DOWN) && option < settings_count_pp[page])
			option++;
		if((btns & PAD_BUTTON_UP) && option > 0)
			option--;
		if((btns & PAD_BUTTON_B))
			option = settings_count_pp[page];
		// Handle all options/buttons here
		if((btns & PAD_BUTTON_A)) {
			// Generic Save/Cancel/Back/Next button actions
			if(option == settings_count_pp[page]-1) {
				// Save settings to SRAM
				/*sram = __SYS_LockSram();
				sram->flags = swissSettings.sramStereo ? (sram->flags|4):(sram->flags&~4);
				sram->lang = swissSettings.sramLanguage;
				__SYS_UnlockSram(1);
				while(!__SYS_SyncSram());*/
				// Save settings to current device
				if((curDevice != SD_CARD)&&((curDevice != IDEEXI))) {
					// If the device is Read-Only, warn/etc
					DrawFrameStart();
					DrawMessageBox(D_INFO,"Cannot save to read-only device!");
					DrawFrameFinish();
				}
				else {
					// Save to XML
					
				}
				DrawFrameStart();
				DrawMessageBox(D_INFO,"Changes successfully saved!");
				DrawFrameFinish();
				sleep(1);
				return;
			}
			if(option == settings_count_pp[page]) {
				// Exit without saving (revert)
				memcpy((void*)&swissSettings, (void*)&tempSettings, sizeof(SwissSettings));
				return;
			}
			if((page != 2) && (option == settings_count_pp[page]-2)) {
				page++; option = 0;
			}
			if((page != 0) && (option == settings_count_pp[page]-(page != 2 ? 3:2))) {
				page--; option = 0;
			}
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_UP) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_DOWN) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_B) 
				&& !(PAD_ButtonsHeld(0) & PAD_BUTTON_A)));
	}
	while (PAD_ButtonsHeld(0) & PAD_BUTTON_A);
}
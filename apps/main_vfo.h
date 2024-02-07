/* Copyright 2024 joaquim.org
 * https://github.com/joaquimorg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */


#include "apps.h"
#include "misc.h"
#include "settings.h"
#include "driver/bk4819.h"
#include "functions.h"
#include "radio.h"
#include "menu.h"
#include "app.h"

#include "gui/gui.h"

#ifdef ENABLE_UART
	#include "driver/uart.h"
#endif


uint8_t popupListSelected = 0;
uint8_t popupListSize = 0;

const int8_t dBmCorrTable[7] = {
			-15, // band 1
			-25, // band 2
			-20, // band 3
			-4, // band 4
			-7, // band 5
			-6, // band 6
			 -1  // band 7
		};

void MainVFO_showRSSI(void) {

    const uint8_t yPosVFO = 37;

    // 0x26 '&' RSSI Empty
    // 0x3F '?' RSSI Sep
    // 0x40 '@' RSSI Box

    UI_printf(&font_10, TEXT_ALIGN_LEFT, 2, 0, yPosVFO + 3, true, false, gEeprom.RX_VFO == 0 ? "SA" : "SB");
    UI_printf(&font_small, TEXT_ALIGN_LEFT, 18, 0, yPosVFO, true, false, "1?3?5?7?9?20?60?90");

    const int16_t s0_dBm   = -gEeprom.S0_LEVEL;                  // S0 .. base level
	const int16_t rssi_dBm =
		BK4819_GetRSSI_dBm()
/*#ifdef ENABLE_AM_FIX
		+ ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
#endif*/
		+ dBmCorrTable[gRxVfo->Band];

    int s0_9 = gEeprom.S0_LEVEL - gEeprom.S9_LEVEL;
	const uint8_t s_level = MIN(MAX((int32_t)(rssi_dBm - s0_dBm)*100 / (s0_9*100/9), 0), 9); // S0 - S9
	uint8_t overS9dBm = MIN(MAX(rssi_dBm + gEeprom.S9_LEVEL, 0), 99);
	uint8_t overS9Bars = MIN(overS9dBm/10, 9);

    uint8_t bar[19];
    memset(bar, '&', sizeof(bar));
    bar[18] = 0x00;

    memset(bar, '@', s_level + overS9Bars);

    if(overS9Bars != 0) {
        UI_printf(&font_small, TEXT_ALIGN_LEFT, 91, 0, yPosVFO + 4, true, false, "S9+%2d", overS9dBm);
    }

    UI_printf(&font_small, TEXT_ALIGN_LEFT, 18, 0, yPosVFO + 6, true, false, "%s", bar);

    /*int16_t rssi = BK4819_GetRSSI();
	uint8_t level;

	if (rssi >= settings_RSSI_CALIB[gRxVfo->Band][3]) {
		level = 6;
	} else if (rssi >= settings_RSSI_CALIB[gRxVfo->Band][2]) {
		level = 4;
	} else if (rssi >= settings_RSSI_CALIB[gRxVfo->Band][1]) {
		level = 2;
	} else if (rssi >= settings_RSSI_CALIB[gRxVfo->Band][0]) {
		level = 1;
	} else {
		level = 0;
	}

    UI_printf(&font_small, TEXT_ALIGN_LEFT, 100, 0, 51, true, false, "%u", level);*/

    //UI_printf(&font_small, TEXT_ALIGN_LEFT, 9, 0, yPosVFO + 6, true, false,    "@@@@@@@@@&&&&&&&&&");
    //@@@@@@@@@&&&&&&&&&
}

void MainVFO_showVFO(void) {

    char String[17] = { 0 };
    uint8_t vfoNumA;
    uint8_t vfoNumB;

    if(gEeprom.TX_VFO == 0) {
        vfoNumA = 0;
        vfoNumB = 1;
    } else {
        vfoNumA = 1;
        vfoNumB = 0;
    }

    const VFO_Info_t *vfoInfoA = &gEeprom.VfoInfo[vfoNumA];
    const VFO_Info_t *vfoInfoB = &gEeprom.VfoInfo[vfoNumB];

    const bool isChannelModeA = IS_MR_CHANNEL(gEeprom.ScreenChannel[vfoNumA]);
    const bool isChannelModeB = IS_MR_CHANNEL(gEeprom.ScreenChannel[vfoNumB]);

    uint32_t frequency;
    uint8_t  yPosVFO = 22;

    // VFO A
	if(FUNCTION_IsRx()) {
		UI_printf(&font_10, TEXT_ALIGN_LEFT, 3, 0, yPosVFO - 7, false, true, "%s %s", vfoNumA == 0 ? "A" : "B", gEeprom.RX_VFO == vfoNumA ? "$" : "");
	} else if (gCurrentFunction == FUNCTION_TRANSMIT){
		UI_printf(&font_10, TEXT_ALIGN_LEFT, 3, 0, yPosVFO - 7, false, true, "%s %s", vfoNumA == 0 ? "A" : "B", gEeprom.TX_VFO == vfoNumA ? "&" : "");
	} else {
    	UI_printf(&font_10, TEXT_ALIGN_LEFT, 3, 0, yPosVFO - 7, false, true, vfoNumA == 0 ? "A" : "B");
	}

    // Frequency A
    frequency = vfoInfoA->pRX->Frequency;
    if ( frequency > _1GHz_in_KHz ) {
        UI_printf(&font_n_20, TEXT_ALIGN_RIGHT, 20, 76, yPosVFO, true, false, "%1u.%3u.%03u", (frequency / 100000000), (frequency / 100000) % 1000, (frequency % 100000) / 100);
    } else {
        UI_printf(&font_n_20, TEXT_ALIGN_RIGHT, 20, 76, yPosVFO, true, false, "%3u.%03u", (frequency / 100000), (frequency % 100000) / 100);
    }
    UI_printf(&font_n_16,   TEXT_ALIGN_LEFT, 78, 90, yPosVFO - 2, true, false, "%02u", (frequency % 100));

    // Modulation A
    UI_printf(&font_small, TEXT_ALIGN_LEFT, 92, 0, yPosVFO - 9, false, true, gModulationStr[vfoInfoA->Modulation]);
    // OUTPUT_POWER
    UI_printf(&font_small, TEXT_ALIGN_LEFT, UI_nextX + 3, 0, yPosVFO - 9, false, true, gSubMenu_TXP[vfoInfoA->OUTPUT_POWER % 3]);
    // BANDWIDTH A
    UI_printf(&font_small, TEXT_ALIGN_LEFT, 92, 0, yPosVFO - 1, false, true, gSubMenu_W_N[vfoInfoA->CHANNEL_BANDWIDTH]);

    if ( vfoInfoA->Modulation == MODULATION_FM ) {
        // DCS/CT/DCR A
        const unsigned int code_type = vfoInfoA->pRX->CodeType;
        if ( code_type > 0 ) {
            const char *code_list[] = {"CT", "DCS", "DCR"};
            UI_printf(&font_small, TEXT_ALIGN_LEFT, UI_nextX + 3, 0, yPosVFO - 1, false, true, code_list[code_type - 1]);
        }
    }
    if (vfoInfoA->freq_config_RX.Frequency != vfoInfoA->freq_config_TX.Frequency) {
        // show the TX offset symbol
        if(vfoInfoA->TX_OFFSET_FREQUENCY_DIRECTION <=2 ) {
            const char dir_list[] = "\0+-";
            UI_printf(&font_10, TEXT_ALIGN_LEFT, 3, 0, yPosVFO, true, false, "%c", dir_list[vfoInfoA->TX_OFFSET_FREQUENCY_DIRECTION]);
        }
    }

    if ( isChannelModeA ) {
        // Channel Name A
        memcpy(String, vfoInfoA->Name, 16);
        if (String[0] == 0) {
            UI_printf(&font_10, TEXT_ALIGN_LEFT, 2, 0, yPosVFO + 7, true, false, "CH-%03u", gEeprom.ScreenChannel[vfoNumA] + 1);
        } else {
            UI_printf(&font_10, TEXT_ALIGN_LEFT, 2, 0, yPosVFO + 8, true, false, "M%03u", gEeprom.ScreenChannel[vfoNumA] + 1);
            UI_printf(&font_10, TEXT_ALIGN_RIGHT, UI_nextX + 1, 90, yPosVFO + 8, true, false, String);
        }
    } else {
        UI_printf(&font_10, TEXT_ALIGN_LEFT, 2, 0, yPosVFO + 7, true, false, "VFO");
    }


    yPosVFO = 61;
    // VFO B
	if(FUNCTION_IsRx()) {
		UI_printf(&font_10, TEXT_ALIGN_LEFT, 2, 0, yPosVFO - 8, true, false, "%s %s", vfoNumB == 1 ? "B" : "A", gEeprom.RX_VFO == vfoNumB ? "$" : "");
	} else {
    	UI_printf(&font_10, TEXT_ALIGN_LEFT, 2, 0, yPosVFO - 8, true, false, vfoNumB == 1 ? "B" : "A");
	}

    // Frequency B
    frequency = vfoInfoB->pRX->Frequency;
    if ( frequency > _1GHz_in_KHz ) {
        UI_printf(&font_n_16, TEXT_ALIGN_RIGHT, 20, 76, yPosVFO - 4, true, false, "%1u.%3u.%03u.%02u", (frequency / 100000000), (frequency / 100000) % 1000, (frequency % 100000) / 100, (frequency % 100));
    } else {
        UI_printf(&font_n_16, TEXT_ALIGN_RIGHT, 20, 76, yPosVFO - 4, true, false, "%3u.%03u.%02u", (frequency / 100000), (frequency % 100000) / 100, (frequency % 100));
    }
    // Modulation B
    UI_printf(&font_small, TEXT_ALIGN_LEFT, 78, 0, yPosVFO - 10, true, false, gModulationStr[vfoInfoB->Modulation]);
    // OUTPUT_POWER
    UI_printf(&font_small, TEXT_ALIGN_LEFT, 78, 0, yPosVFO - 4, true, false, gSubMenu_W_N[vfoInfoB->CHANNEL_BANDWIDTH]);

    if ( isChannelModeB ) {
        // Channel Name B
        memcpy(String, vfoInfoB->Name, 16);
        if (String[0] == 0) {
            UI_printf(&font_small, TEXT_ALIGN_RIGHT, 20, 76, yPosVFO + 2, true, false, "CH-%03u", gEeprom.ScreenChannel[vfoNumB] + 1);
        } else {
            UI_printf(&font_small, TEXT_ALIGN_LEFT, 2, 20, yPosVFO + 2, true, false, "M%03u", gEeprom.ScreenChannel[vfoNumB] + 1);
            UI_printf(&font_small, TEXT_ALIGN_RIGHT, 20, 76, yPosVFO + 2, true, false, String);
        }
    } else {
        UI_printf(&font_small, TEXT_ALIGN_LEFT, 2, 20, yPosVFO + 2, true, false, "VFO");
    }

    // extra info
    //UI_drawFastVLine(97, 46, 17, true);

}


void MainVFO_showInput() {
    const uint8_t popupW = 80;
	const uint8_t popupH = 30;

    uint8_t startX;
    uint8_t startY;
    GUI_showPopup(popupW, popupH, &startX, &startY);
    UI_printf(&font_small, TEXT_ALIGN_CENTER, startX, startX + popupW - 2, startY, true, false, "Input Memory");
    GUI_inputShow(startX, startX + popupW - 2, startY + 14);
}

void MainVFO_initFunction() {

}

void MainVFO_renderFunction() {

    UI_displayClear();
    MainVFO_showVFO();

    if ( FUNCTION_IsRx() ) {
        MainVFO_showRSSI();
    }

    if (GUI_inputGetSize() > 0) {
        MainVFO_showInput();
    }

}


void MainVFO_keyHandlerFunction(KEY_Code_t key, KEY_State_t state) {
    switch (key)
    {
        case KEY_1:
            if ( state == KEY_PRESSED_WITH_F || state == KEY_LONG_PRESSED ) {
            }
            break;
        case KEY_2:
            if ( state == KEY_PRESSED_WITH_F || state == KEY_LONG_PRESSED ) {
                main_push_message(RADIO_VFO_SWITCH);
            }
            break;
        case KEY_3:
            if ( state == KEY_PRESSED_WITH_F || state == KEY_LONG_PRESSED ) {
                main_push_message(RADIO_VFO_SWITCH_MODE);
            }
            break;
        case KEY_4:
            if ( state == KEY_PRESSED_WITH_F || state == KEY_LONG_PRESSED ) {
                popupListSelected = gTxVfo->CHANNEL_BANDWIDTH;
                application_showPopup(APP_POPUP_W_N, true);
            }
            break;
        case KEY_5:
            if ( state == KEY_PRESSED_WITH_F || state == KEY_LONG_PRESSED ) {
                popupListSelected = gTxVfo->Modulation;
                application_showPopup(APP_POPUP_AM, true);
            }
            break;
        case KEY_6:
            if ( state == KEY_PRESSED_WITH_F || state == KEY_LONG_PRESSED ) {
                popupListSelected = gTxVfo->OUTPUT_POWER;
                application_showPopup(APP_POPUP_TXP, true);
            }
            break;
        case KEY_STAR:
            if ( state == KEY_PRESSED ) {
            }
            break;

        case KEY_UP:
        case KEY_DOWN:
            if ( state == KEY_PRESSED || state == KEY_LONG_PRESSED_CONT) {
                main_push_message(key == KEY_UP ? RADIO_VFO_UP : RADIO_VFO_DOWN);
            } else if (state == KEY_RELEASED ) {
                // save if key released
                if (IS_FREQ_CHANNEL(gEeprom.ScreenChannel[gEeprom.TX_VFO])) {
                    main_push_message(RADIO_SAVE_CHANNEL);
                } else {
                    main_push_message(RADIO_SAVE_VFO);
                    main_push_message(RADIO_VFO_CONFIGURE_RELOAD);
                    main_push_message(RADIO_RECONFIGURE_VFO);
                }

            }
            break;

        case KEY_MENU:
            if ( state == KEY_PRESSED ) {
                if (GUI_inputGetSize() > 0) {
                    GUI_inputReset();
                } else {
                    load_application(APP_MENU);
                }
            }
            break;
        case KEY_EXIT:
            if ( state == KEY_PRESSED ) {
                if (GUI_inputGetSize() > 0) {
                    GUI_inputReset();
                }
            }
            break;

        default:
            break;
    }

    if ( state == KEY_RELEASED ) {
        switch (key)
        {
            case KEY_1:
            case KEY_2:
            case KEY_3:
            case KEY_4:
            case KEY_5:
            case KEY_6:
            case KEY_7:
            case KEY_8:
            case KEY_9:
            case KEY_0:
            case KEY_STAR:
            case KEY_F:
                GUI_inputAppendKey(key);
            break;

            default:
                break;
        }
    }
}

/* --------------------------------------------------------------------------- */
// Popups
/* --------------------------------------------------------------------------- */

void MainVFO_renderPopupFunction(APPS_Popup_t popup) {

    const uint8_t popupW = 70;
	const uint8_t popupH = 42;

    uint8_t startX;
    uint8_t startY;

    switch (popup)
    {
        case APP_POPUP_TXP:
            GUI_showPopup(popupW, popupH, &startX, &startY);
            UI_printf(&font_small, TEXT_ALIGN_CENTER, startX, startX + popupW - 2, startY, true, false, "VFO %s - TX POWER", gEeprom.TX_VFO == 0 ? "A" : "B");
            popupListSize = ARRAY_SIZE(gSubMenu_TXP) - 1;
            popupShowList(popupListSelected, popupListSize, startX, startY + 8, startX + popupW - 2, gSubMenu_TXP);
            break;
        case APP_POPUP_W_N:
            GUI_showPopup(popupW, popupH, &startX, &startY);
            UI_printf(&font_small, TEXT_ALIGN_CENTER, startX, startX + popupW - 2, startY, true, false, "VFO %s - BANDWIDTH", gEeprom.TX_VFO == 0 ? "A" : "B");
            popupListSize = ARRAY_SIZE(gSubMenu_W_N) - 1;
            popupShowList(popupListSelected, popupListSize, startX, startY + 8, startX + popupW - 2, gSubMenu_W_N);
            break;
        case APP_POPUP_AM:
            GUI_showPopup(popupW, popupH, &startX, &startY);
            UI_printf(&font_small, TEXT_ALIGN_CENTER, startX, startX + popupW - 2, startY, true, false, "VFO %s - MODULATION", gEeprom.TX_VFO == 0 ? "A" : "B");
            popupListSize = MODULATION_UKNOWN - 1;
            popupShowList(popupListSelected, popupListSize, startX, startY + 8, startX + popupW - 2, gModulationStr);
            break;
        default:
            break;
    }
}

void MainVFO_popupSave(APPS_Popup_t popup) {
    switch (popup)
    {
        case APP_POPUP_TXP:
            gTxVfo->OUTPUT_POWER = popupListSelected;
            //sys_push_message(RADIO_SAVE_CHANNEL);
            break;
        case APP_POPUP_W_N:
            gTxVfo->CHANNEL_BANDWIDTH = popupListSelected;
			//sys_push_message(RADIO_SAVE_CHANNEL);
            break;
        case APP_POPUP_AM:
            gTxVfo->Modulation = popupListSelected;
			//sys_push_message(RADIO_SAVE_CHANNEL);
            break;
        default:
            break;
    }
}

void MainVFO_keyHandlerPopupFunction(KEY_Code_t key, KEY_State_t state, APPS_Popup_t popup) {
    if ( state == KEY_PRESSED ) {
        switch (key)
        {
            case KEY_6:
            case KEY_5:
            case KEY_4:
            case KEY_DOWN:
                if ( popupListSelected < popupListSize ) {
                    popupListSelected++;
                } else {
                    popupListSelected = 0;
                }
                break;
            case KEY_UP:
                if ( popupListSelected > 0 ) {
                    popupListSelected--;
                } else {
                    popupListSelected = popupListSize;
                }
                break;
            case KEY_MENU:
                // save
                MainVFO_popupSave(popup);
                [[fallthrough]];
            case KEY_EXIT:
                application_closePopup();
                break;
            default:
                break;
        }
   }
}

/* --------------------------------------------------------------------------- */
// APP Main VFO
/* --------------------------------------------------------------------------- */

app_t APPMainVFO = {
    .showStatusLine = true,
    .init = MainVFO_initFunction,
    .render = MainVFO_renderFunction,
    .keyhandler = MainVFO_keyHandlerFunction,

    .renderPopup = MainVFO_renderPopupFunction,
    .keyhandlerPopup = MainVFO_keyHandlerPopupFunction,
};


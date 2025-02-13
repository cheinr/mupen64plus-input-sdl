/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-input-sdl - autoconfig.c                                  *
 *   Mupen64Plus homepage: https://mupen64plus.org/                        *
 *   Copyright (C) 2009-2013 Richard Goedeken                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef M64P_STATIC_PLUGINS
#define M64P_PLUGIN_PROTOTYPES 1
#define M64P_CORE_PROTOTYPES 1
#endif

#include "autoconfig.h"
#include "m64p_config.h"
#include "m64p_types.h"
#include "osal_preproc.h"
#include "plugin.h"

#if EMSCRIPTEN
extern int findAutoInputConfigName(void* gamepadNamePtr, void* responseBufferPointer, int maxCharacters);
#endif

/* local definitions */
#define INI_FILE_NAME "InputAutoCfg.ini"
typedef struct {
    m64p_handle pSrc;
    m64p_handle pDst;
} SCopySection;

/* local functions */
static char *StripSpace(char *pIn)
{
    char *pEnd = pIn + strlen(pIn) - 1;

    while (*pIn == ' ' || *pIn == '\t' || *pIn == '\r' || *pIn == '\n')
        pIn++;

    while (pIn <= pEnd && (*pEnd == ' ' || *pEnd == '\t' || *pEnd == '\r' || *pEnd == '\n'))
        *pEnd-- = 0;

    return pIn;
}

static void CopyParamCallback(void * context, const char *ParamName, m64p_type ParamType)
{
    SCopySection *pCpyContext = (SCopySection *) context;
    int paramInt;
    float paramFloat;
    char paramString[1024];

    // handle the parameter copy depending upon type
    switch (ParamType)
    {
        case M64TYPE_INT:
        case M64TYPE_BOOL:
            if (ConfigGetParameter(pCpyContext->pSrc, ParamName, ParamType, &paramInt, sizeof(int)) == M64ERR_SUCCESS)
                ConfigSetParameter(pCpyContext->pDst, ParamName, ParamType, &paramInt);
            break;
        case M64TYPE_FLOAT:
            if (ConfigGetParameter(pCpyContext->pSrc, ParamName, ParamType, &paramFloat, sizeof(float)) == M64ERR_SUCCESS)
                ConfigSetParameter(pCpyContext->pDst, ParamName, ParamType, &paramFloat);
            break;
        case M64TYPE_STRING:
            if (ConfigGetParameter(pCpyContext->pSrc, ParamName, ParamType, paramString, 1024) == M64ERR_SUCCESS)
                ConfigSetParameter(pCpyContext->pDst, ParamName, ParamType, paramString);
            break;
        default:
            // this should never happen
            DebugMessage(M64MSG_ERROR, "Unknown source parameter type %i in copy callback", (int) ParamType);
            return;
    }
}

/* global functions */
int auto_copy_inputconfig(const char *pccSourceSectionName, const char *pccDestSectionName, const char *sdlJoyName)
{
    SCopySection cpyContext;

    if (ConfigOpenSection(pccSourceSectionName, &cpyContext.pSrc) != M64ERR_SUCCESS)
    {
        DebugMessage(M64MSG_ERROR, "auto_copy_inputconfig: Couldn't open source config section '%s' for copying", pccSourceSectionName);
        return 0;
    }

    if (ConfigOpenSection(pccDestSectionName, &cpyContext.pDst) != M64ERR_SUCCESS)
    {
        DebugMessage(M64MSG_ERROR, "auto_copy_inputconfig: Couldn't open destination config section '%s' for copying", pccDestSectionName);
        return 0;
    }

    // set the 'name' parameter
    if (sdlJoyName != NULL)
    {
        if (ConfigSetParameter(cpyContext.pDst, "name", M64TYPE_STRING, sdlJoyName) != M64ERR_SUCCESS)
        {
            DebugMessage(M64MSG_ERROR, "auto_copy_inputconfig: Couldn't set 'name' parameter to '%s' in section '%s'", sdlJoyName, pccDestSectionName);
            return 0;
        }
    }

    // the copy gets done by the callback function
    if (ConfigListParameters(cpyContext.pSrc, (void *) &cpyContext, CopyParamCallback) != M64ERR_SUCCESS)
    {
        DebugMessage(M64MSG_ERROR, "auto_copy_inputconfig: parameter list copy failed");
        return 0;
    }

    return 1;
}

static int auto_compare_name(const char *joySDLName, char *line)
{
    char *wordPtr;
    int  joyFound = 1, joyFoundScore = 0;
    char Word[64];

    wordPtr = line;
    /* first, if there is a preceding system name in this .ini device name, and the system matches, then strip out */
#if defined(__unix__)
    if (strncmp(wordPtr, "Unix:", 5) == 0) {
        wordPtr = StripSpace(wordPtr + 5);
        joyFoundScore = 1;
    }
#endif
#if defined(__linux__)
    if (strncmp(wordPtr, "Linux:", 6) == 0) {
        wordPtr = StripSpace(wordPtr + 6);
        joyFoundScore = 1;
    }
#endif
#if defined(__APPLE__)
    if (strncmp(wordPtr, "OSX:", 4) == 0) {
        wordPtr = StripSpace(wordPtr + 4);
        joyFoundScore = 1;
    }
#endif
#if defined(WIN32)
    if (strncmp(wordPtr, "Win32:", 6) == 0) {
        wordPtr = StripSpace(wordPtr + 6);
        joyFoundScore = 1;
    }
#if SDL_VERSION_ATLEAST(2,0,0)
    else if (strncmp(wordPtr, "XInput:", 7) == 0) {
        wordPtr = StripSpace(wordPtr + 7);
        joyFoundScore = 2;
    }
#endif
#endif
    /* extra points if the section name is a perfect match */
    if (strcmp(wordPtr, joySDLName) == 0)
        joyFoundScore += 4;
    /* search in the .ini device name for all the words in the joystick name.  If any are missing, then this is not the right joystick model */
    while (wordPtr != NULL && strlen(wordPtr) > 0)
    {
        /* skip over any preceding spaces */
        while (*wordPtr == ' ')
            wordPtr++;
        if (*wordPtr == 0)
            break;
        /* search for the next space after the current word */
        char *nextSpace = strchr(wordPtr, ' ');
        if (nextSpace == NULL)
        {
            strncpy(Word, wordPtr, 63);
            Word[63] = 0;
            wordPtr = NULL;
        }
        else
        {
            int length = (int) (nextSpace - wordPtr);
            if (length > 63) length = 63;
            strncpy(Word, wordPtr, length);
            Word[length] = 0;
            wordPtr = nextSpace + 1;
        }
        if (strcasestr(joySDLName, Word) == NULL)
            joyFound = 0;
		else
			joyFoundScore += 4;
    }

    if (joyFound)
        return joyFoundScore;
    else
        return -1;
}

int auto_set_defaults(int iDeviceIdx, const char *joySDLName)
{
    FILE *pfIn;
    m64p_handle pConfig = NULL;
#if EMSCRIPTEN
    const char CfgFilePath[] = "/mupen64plus/data/InputAutoCfg.ini";
#else
    const char *CfgFilePath = ConfigGetSharedDataFilepath(INI_FILE_NAME);
#endif
    enum { E_NAME_SEARCH, E_NAME_FOUND, E_PARAM_READ } eParseState;
    char *pchIni, *pchNextLine, *pchCurLine;
    long iniLength;
    int ControllersFound = 0;
    int joyFoundScore = -1;

    /* if we couldn't get a name (no joystick plugged in to given port), then return with a failure */
    if (joySDLName == NULL)
        return 0;
    /* if we couldn't find the shared data file, dump an error and return */
    if (CfgFilePath == NULL || strlen(CfgFilePath) < 1)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't find config file '%s'", INI_FILE_NAME);
        return 0;
    }

#if EMSCRIPTEN

    // Ideally we get the config from our JS functions directly,
    // but that would require some additional parsing logic, so for now
    // we just force the parsing logic here to use the name that gets returned.
    char matchedConfigName[256];
    findAutoInputConfigName(joySDLName, matchedConfigName, 256);
#endif
    
    /* read the input auto-config .ini file */
    pfIn = fopen(CfgFilePath, "rb");
    if (pfIn == NULL)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't open config file '%s'", CfgFilePath);
        return 0;
    }
    fseek(pfIn, 0L, SEEK_END);
    iniLength = ftell(pfIn);
    fseek(pfIn, 0L, SEEK_SET);
    if (iniLength < 0) {
        DebugMessage(M64MSG_ERROR, "Couldn't get size of config file '%s'", CfgFilePath);
        fclose(pfIn);
        return 0;
    }

    pchIni = (char *) malloc(iniLength + 1);
    if (pchIni == NULL)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't allocate %li bytes for config file '%s'", iniLength, CfgFilePath);
        fclose(pfIn);
        return 0;
    }
    if (fread(pchIni, 1, iniLength, pfIn) != iniLength)
    {
        DebugMessage(M64MSG_ERROR, "File read failed for %li bytes of config file '%s'", iniLength, CfgFilePath);
        free(pchIni);
        fclose(pfIn);
        return 0;
    }
    fclose(pfIn);
    pchIni[iniLength] = 0;

    /* parse the INI file, line by line */
    DebugMessage(M64MSG_INFO, "Using auto-config file at: '%s'", CfgFilePath);
    pchNextLine = pchIni;
    eParseState = E_NAME_SEARCH;
    while (pchNextLine != NULL && *pchNextLine != 0)
    {
        char *pivot = NULL;
        int  joyFound = 0;
        /* set up character pointers */
        pchCurLine = pchNextLine;
        pchNextLine = strchr(pchNextLine, '\n');
        if (pchNextLine != NULL)
            *pchNextLine++ = 0;
        pchCurLine = StripSpace(pchCurLine);

        /* handle blank/comment lines */
        if (strlen(pchCurLine) < 1 || *pchCurLine == ';' || *pchCurLine == '#')
            continue;

        /* handle section (joystick name in ini file) */
        if (*pchCurLine == '[' && pchCurLine[strlen(pchCurLine)-1] == ']')
        {
            /* only switch to name search when some section body was identified since last header */
            if (eParseState == E_PARAM_READ)
                eParseState = E_NAME_SEARCH;

            /* we need to look through the device name word by word to see if it matches the joySDLName that we're looking for */ 
            pchCurLine[strlen(pchCurLine)-1] = 0;

#if EMSCRIPTEN
            joyFound = (strcmp(StripSpace(matchedConfigName), StripSpace(pchCurLine + 1)) == 0) ? 1 : 0;
#else
            joyFound = auto_compare_name(joySDLName, StripSpace(pchCurLine + 1));
#endif
            /* if we found the right joystick, then open up the core config section to store parameters and set the 'device' param */
            if (joyFound > joyFoundScore)
            {
                char SectionName[32];
                ControllersFound = 0;
                sprintf(SectionName, "AutoConfig%i", ControllersFound);
                if (ConfigOpenSection(SectionName, &pConfig) != M64ERR_SUCCESS)
                {
                    DebugMessage(M64MSG_ERROR, "auto_set_defaults(): Couldn't open config section '%s'", SectionName);
                    free(pchIni);
                    return 0;
                }
                eParseState = E_NAME_FOUND;
                ControllersFound++;
                ConfigSetParameter(pConfig, "device", M64TYPE_INT, &iDeviceIdx);
                joyFoundScore = joyFound;
            }
            continue;
        }

        /* handle parameters */
        pivot = strchr(pchCurLine, '=');
        if (pivot != NULL)
        {
            /* if we haven't found the correct section yet, just skip this */
            if (eParseState == E_NAME_SEARCH)
                continue;
            eParseState = E_PARAM_READ;
            /* otherwise, store this parameter in the current active joystick config */
            *pivot++ = 0;
            pchCurLine = StripSpace(pchCurLine);
            pivot = StripSpace(pivot);
            if (strcasecmp(pchCurLine, "device") == 0)
            {
                int iVal = atoi(pivot);
                ConfigSetParameter(pConfig, pchCurLine, M64TYPE_INT, &iVal);
            }
            else if (strcasecmp(pchCurLine, "plugged") == 0 || strcasecmp(pchCurLine, "mouse") == 0)
            {
                int bVal = (strcasecmp(pivot, "true") == 0);
                ConfigSetParameter(pConfig, pchCurLine, M64TYPE_BOOL, &bVal);
            }
            else
            {
                ConfigSetParameter(pConfig, pchCurLine, M64TYPE_STRING, pivot);
            }
            continue;
        }

        /* handle keywords */
        if (pchCurLine[strlen(pchCurLine)-1] == ':')
        {
            /* if we haven't found the correct section yet, just skip this */
            if (eParseState == E_NAME_SEARCH)
                continue;
            eParseState = E_PARAM_READ;
            /* otherwise parse the keyword */
            if (strcmp(pchCurLine, "__NextController:") == 0)
            {
                char SectionName[32];
                /* if there are no more N64 controller spaces left, then exit */
                if (ControllersFound == 4)
                {
                    free(pchIni);
                    return ControllersFound;
                }
                /* otherwise go to the next N64 controller */
                sprintf(SectionName, "AutoConfig%i", ControllersFound);
                if (ConfigOpenSection(SectionName, &pConfig) != M64ERR_SUCCESS)
                {
                    DebugMessage(M64MSG_ERROR, "auto_set_defaults(): Couldn't open config section '%s'", SectionName);
                    free(pchIni);
                    return ControllersFound;
                }
                ControllersFound++;
                ConfigSetParameter(pConfig, "device", M64TYPE_INT, &iDeviceIdx);
            }
            else
            {
                DebugMessage(M64MSG_ERROR, "Unknown keyword '%s' in %s", pchCurLine, INI_FILE_NAME);
            }
            continue;
        }

        /* unhandled line in .ini file */
        DebugMessage(M64MSG_ERROR, "Invalid line in %s: '%s'", INI_FILE_NAME, pchCurLine);
    }

    if (joyFoundScore != -1)
    {
        /* we've finished parsing all parameters for the discovered input device, which is the last in the .ini file */
        free(pchIni);
        return ControllersFound;
    }

    free(pchIni);
    return 0;
}



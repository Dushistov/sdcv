/*********************************************************************
 * 
 * This file part of WordPick - A Adobe Acrobat/Reader plugin for text
 * extraction by mouse click.
 * 2006 Dewolf Xue <deWolf_maTri_X@msn.com>
 * 2006 Hu Zheng <huzheng_001@163.com>
 *
 * This plugin is special for StarDict (http://stardict.sourceforge.net)
 *
 * Compile under Acrobat SDK 7 + M$ VS 2003.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
*********************************************************************/
#include "PIHeaders.h"
#include "PIRequir.h"
#include "wordPickUI.h"
//#include "DebugWindowHFT.h"
#include "ThTypes.h"

/*-------------------------------------------------------
	Constants/Declarations
-------------------------------------------------------*/
//ASAtom WordPick_K;

/*-------------------------------------------------------
	Core Handshake Callbacks
-------------------------------------------------------*/
ACCB1 ASBool ACCB2 PluginExportHFTs(void)
{
	return true;
}

ACCB1 ASBool ACCB2 PluginImportReplaceAndRegister(void)
{
	return true;
}

/* PluginInit
** ------------------------------------------------------
** */ 
/** 
 **	The main initialization routine.
 **	
 **	@return true to continue loading the plug-in
 **	@return false to cause plug-in loading to stop.
 **   */
ACCB1 ASBool ACCB2 PluginInit(void)
{
	//handButton = AVToolBarGetButtonByName (toolBar, ASAtomFromString("Hand"));
	//if (!handButton)
	//{
	//	AVAlertNote("Show \"Hand\" Button fist.");
	//	return false;
	//}
	ThTypes_Init();
	SetUpUI();
	return true;
}

/* PluginUnload
** ------------------------------------------------------
** */ 
/** 
 **	The unload routine.
 **	Called when your plug-in is being unloaded when the application quits.
 **	Use this routine to release any system resources you may have
 **	allocated.
 **
 **	Returning false will cause an alert to display that unloading failed.
 **	@return true to indicate the plug-in unloaded.
*/
ACCB1 ASBool ACCB2 PluginUnload(void)
{
	CleanUpUI();
	Thtypes_End();
	return true;
}

/* GetExtensionName
** ------------------------------------------------------
** */ 
/**
 **	Return the unique ASAtom associated with your plug-in.
 **	@return the plug-in 's name as an ASAtom.
 **
 **	@see ASAtomFromString
*/
ASAtom GetExtensionName()
{
	return ASAtomFromString("ADBE:WordPick");
}

/*
** PIHandshake
** */ 
/**
 ** Function that provides the initial interface between your plug-in and the application.
 ** This function provides the callback functions to the application that allow it to 
 ** register the plug-in with the application environment.
 ** 
 ** Required Plug-in handshaking routine: <b>Do not change its name!</b>
 ** 
 ** @param handshakeVersion the version this plug-in works with. There are two versions possible, the plug-in version 
 ** and the application version. The application calls the main entry point for this plug-in with its version.
 ** The main entry point will call this function with the version that is earliest. 
 ** @param handshakeData OUT the data structure used to provide the primary entry points for the plug-in. These
 ** entry points are used in registering the plug-in with the application and allowing the plug-in to register for 
 ** other plug-in services and offer its own.
 ** @return true to indicate success, false otherwise (the plug-in will not load).
 ** */
ACCB1 ASBool ACCB2 PIHandshake(Uns32 handshakeVersion, void *handshakeData)
{
	if (handshakeVersion == HANDSHAKE_V0200) {
		/* Cast handshakeData to the appropriate type */
		PIHandshakeData_V0200 *hsData = (PIHandshakeData_V0200 *)handshakeData;

		/* Set the name we want to go by */
		hsData->extensionName = GetExtensionName();

		/* If you export your own HFT, do so in here */
		hsData->exportHFTsCallback = ASCallbackCreateProto(PIExportHFTsProcType, &PluginExportHFTs);

		/*
		** If you import plug-in HFTs, replace functionality, and/or want to register for notifications before
		** the user has a chance to do anything, do so in here.
		*/
		hsData->importReplaceAndRegisterCallback = ASCallbackCreateProto(PIImportReplaceAndRegisterProcType,
																		 &PluginImportReplaceAndRegister);

		/* Perform your plug-in's initialization in here */
		hsData->initCallback = ASCallbackCreateProto(PIInitProcType, &PluginInit);

		/* Perform any memory freeing or state saving on "quit" in here */
		hsData->unloadCallback = ASCallbackCreateProto(PIUnloadProcType, &PluginUnload);

		/* All done */
		return true;

	} /* Each time the handshake version changes, add a new "else if" branch */

	/*
	** If we reach here, then we were passed a handshake version number we don't know about.
	** This shouldn't ever happen since our main() routine chose the version number.
	*/
	AVAlertNote("Error in hand shake");
	return false;
}


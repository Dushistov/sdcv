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

#ifndef _WordPickUI_H
#define _WordPickUI_H

/* SetUpUI
** Creates and registers  the AVTool, AVToolButton and AVMenuItem needed
*/
void SetUpUI(void);
void CleanUpUI(void);
void getCurWords(PVOID parm);
void resetWordPick(void);
ACCB1 void ACCB2 wordPickAVAppRegisterForPageViewIdleProc(void * clientData);
#endif /* !_WordPickUI_H */

/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */
#ifndef __PARTYFILTER_H__
#define __PARTYFILTER_H__ 1

bool DLL_IMPORT_EXPORT_ORKBASE PartyFilterActive(void);
bool DLL_IMPORT_EXPORT_ORKBASE PartyFilterMatches(CStdString& party);

#endif

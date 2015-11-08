/*
 Copyright (C) 2015 Felipe Izzo
 
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
 */


int CTRDrv_GetError(void);
const char *CTRDrv_ErrorString( int ErrorNumber );
int CTRDrv_PCM_Init(int *mixrate, int *numchannels, void * initdata);
void CTRDrv_PCM_Shutdown(void);
int CTRDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
              int NumDivisions, void ( *CallBackFunc )( void ) );
void CTRDrv_PCM_StopPlayback(void);
void CTRDrv_PCM_Lock(void);
void CTRDrv_PCM_Unlock(void);
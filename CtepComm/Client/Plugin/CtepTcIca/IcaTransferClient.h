#pragma once

extern int RegisterVirtualChannel(PVD pVd,char* pChannelName,USHORT* pusChannelNum, ULONG* pulChannelMask);
extern int RegisterWriteHook(PVD pVd,USHORT usChannelNum,VDWRITEHOOK* pRetVdwh);




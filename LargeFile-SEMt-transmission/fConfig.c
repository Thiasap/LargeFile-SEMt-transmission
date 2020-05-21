#pragma pack(1)
#include"fConfig.h"
void phex(uint8_t* str) {
	uint8_t len = 32;
	unsigned char i;
	for (i = 0; i < len; ++i)
		printf("%.2x", str[i]);
	printf("\n");
}
int getMem() {
	MEMORYSTATUSEX mstx;
	mstx.dwLength = sizeof(mstx);
	GlobalMemoryStatusEx(&mstx);
	int iAvailPhysMB = mstx.ullAvailPhys / 1024 / 1024;
}
DWORD GetProcessorCoreCount() {
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	DWORD returnLength = 0;
	DWORD processCoreCount = 0;
	while (1) {
		DWORD ro = GetLogicalProcessorInformation(buffer, &returnLength);
		if (ro == FALSE) {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				if (buffer)free(buffer);
				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
				if (buffer == NULL)return 0;
			}
			else { return 0; }
		}
		else { break; }
	}
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
	DWORD byteOffest = 0;
	while (byteOffest + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
		if (ptr->Relationship == RelationProcessorCore)
			++processCoreCount;
		byteOffest += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		++ptr;
	}
	free(buffer);
	return processCoreCount;
}
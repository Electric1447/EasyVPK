#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/sysmodule.h>
#include <psp2/promoterutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define HEAD_BIN PACKAGE_DIR "/sce_sys/package/head.bin"


typedef struct SfoHeader {
	uint32_t magic;
	uint32_t version;
	uint32_t keyofs;
	uint32_t valofs;
	uint32_t count;
} __attribute__((packed)) SfoHeader;
	
typedef struct SfoEntry {
	uint16_t nameofs;
	uint8_t	 alignment;
	uint8_t	 type;
	uint32_t valsize;
	uint32_t totalsize;
	uint32_t dataofs;
} __attribute__((packed)) SfoEntry;


int launchAppByUriExit(char *titleid) {
	char uri[32];
	sprintf(uri, "psgm:play?titleid=%s", titleid);

	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	sceKernelExitProcess(0);

	return 0;
}

static int loadScePaf() {
	static uint32_t argp[] = { 0x180000, -1, -1, 1, -1, -1 };

	int result = -1;

	uint32_t buf[4];
	buf[0] = sizeof(buf);
	buf[1] = (uint32_t)&result;
	buf[2] = -1;
	buf[3] = -1;
	
	SceSysmoduleOpt opt;
	opt.flags = sizeof(opt);
	opt.result = (int *)&result;
	opt.unused[0] = -1;
	opt.unused[1] = -1;

	return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, &opt);
}

static int unloadScePaf() {
  	SceSysmoduleOpt opt;
	opt.flags = 0;
	return sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &opt);
}

int promoteApp(const char *path) {
	int res;

	res = loadScePaf();
	if (res < 0)
		return res;

	res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
	if (res < 0)
		return res;

	res = scePromoterUtilityInit();
	if (res < 0)
		return res;

	res = scePromoterUtilityPromotePkgWithRif(path, 1);
	if (res < 0)
		return res;

	res = scePromoterUtilityExit();
	if (res < 0)
		return res;

	res = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
	if (res < 0)
		return res;

	res = unloadScePaf();
	if (res < 0)
		return res;

	return res;
}

char *get_title_id(const char *filename) {
	char *res = NULL;
	long size = 0;
	FILE *fin = NULL;
	char *buf = NULL;
	int i;

	SfoHeader *header;
	SfoEntry *entry;
	
	fin = fopen(filename, "rb");
	if (!fin)
		goto cleanup;
	if (fseek(fin, 0, SEEK_END) != 0)
		goto cleanup;
	if ((size = ftell(fin)) == -1)
		goto cleanup;
	if (fseek(fin, 0, SEEK_SET) != 0)
		goto cleanup;
	buf = calloc(1, size + 1);
	if (!buf)
		goto cleanup;
	if (fread(buf, size, 1, fin) != 1)
		goto cleanup;

	header = (SfoHeader*)buf;
	entry = (SfoEntry*)(buf + sizeof(SfoHeader));
	for (i = 0; i < header->count; ++i, ++entry) {
		const char *name = buf + header->keyofs + entry->nameofs;
		const char *value = buf + header->valofs + entry->dataofs;
		if (name >= buf + size || value >= buf + size)
			break;
		if (strcmp(name, "TITLE_ID") == 0)
			res = strdup(value);
	}

cleanup:
	if (buf)
		free(buf);
	if (fin)
		fclose(fin);

	return res;
}

int main(int argc, const char *argv[]) {
	// Destroy other apps
	sceAppMgrDestroyOtherApp();

	char *titleid = get_title_id(PACKAGE_DIR "/sce_sys/param.sfo");
	if (titleid && strcmp(titleid, UPDATE_TITLEID) == 0)
		promoteApp(PACKAGE_DIR);

	sceKernelDelayThread(250000);
	launchAppByUriExit(UPDATE_TITLEID);

	return 0;
}
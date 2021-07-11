/*
 * Copyright (C) 2015 Sergi Granell (xerpi)
 * Copyright (C) 2021 Itai Levin (Electric1447)
 */

#define _GNU_SOURCE char *strcasestr(const char *haystack, const char *needle);

#include "installer.hpp"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <vitasdk.h>
#include <curl/curl.h>

#include "../utils/minizip/unzip.h"
#include "../utils/filesystem.hpp"
#include "../utils/vitaPackage.h"


int state = IDLE, install_state;
bool isInstalling, launchUpdater = false;

string d_url = "", d_filename = "";
int dl_type;
string dl_dir;
bool dl_isEasyVPK = false;

const char *sizes[] = {
	"B",
	"KB",
	"MB",
	"GB"
};

uint64_t free_storage = 0;
uint64_t dummy;
uint64_t downloaded_bytes = 0;
uint64_t extracted_bytes = 0;
uint64_t total_bytes = 0xFFFFFFFF;
uint8_t abort_download = 0;

FILE *fh;

char *bytes_string;

static CURL *curl_handle = NULL;


static void removeIllegalCharactersFromFilename(string& str) {
	string illegalChars = "\\/:?\"<>|";

	for (char c: illegalChars)
		str.erase(remove(str.begin(), str.end(), c), str.end());
}

float format(float len) {
	while (len > 1024)
		len /= 1024.0f;
	
	return len;
}

uint8_t quota(uint64_t len) {
	uint8_t ret = 0;
	while (len > 1024) {
		ret++;
		len /= 1024;
	}
	return ret;
}

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
	if (abort_download)
		return -1;
	
	downloaded_bytes += size * nmemb;
	return fwrite(ptr, size, nmemb, fh);
}

static size_t header_cb(char *buffer, size_t size, size_t nitems, void *userdata) {
	
	if (total_bytes == 0xFFFFFFFF) {
		char *ptr = strcasestr(buffer, "Content-Length");
		if (ptr != NULL) { 
			sscanf(ptr, "Content-Length: %llu", &total_bytes);
			if (total_bytes * 2 > free_storage)
				abort_download = 1;
		}
	}
	
	return nitems * size;
}

static char asyncUrl[512];
static void resumeDownload() {
	
	curl_easy_reset(curl_handle);
	curl_easy_setopt(curl_handle, CURLOPT_URL, asyncUrl);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, bytes_string); // Dummy
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, bytes_string); // Dummy
	curl_easy_setopt(curl_handle, CURLOPT_RESUME_FROM, downloaded_bytes);
	curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, 524288);
	
	struct curl_slist *headerchunk = NULL;
	
	headerchunk = curl_slist_append(headerchunk, "Accept: */*");
	headerchunk = curl_slist_append(headerchunk, "Content-Type: application/json");
	headerchunk = curl_slist_append(headerchunk, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
	headerchunk = curl_slist_append(headerchunk, "Content-Length: 0");
	
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerchunk);
	curl_easy_perform(curl_handle);
}

static int downloadThread(unsigned int args, void* arg){
	curl_handle = curl_easy_init();
	fh = fopen((dl_dir + d_filename).c_str(), "wb");
	
	while (downloaded_bytes < total_bytes) {
		resumeDownload();
		if (abort_download)
			break;
	}
	
	fclose(fh);
	
	if (abort_download)
		state = ERROR;
	else
		state = DOWNLOADED;
	
	sceKernelExitDeleteThread(0);
	return 0;
}

void launchDownload(const char *url) {
	if (state != IDLE)
		return;
	
	sprintf(asyncUrl, "%s", url);
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP);
	int ret = sceNetShowNetstat();
	SceNetInitParam initparam;
	
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		void *net_memory = malloc(NET_INIT_SIZE);
		initparam.memory = net_memory;
		initparam.size = NET_INIT_SIZE;
		initparam.flags = 0;
		sceNetInit(&initparam);
	}
	
	sceNetCtlInit();
	sceHttpInit(NET_INIT_SIZE);
	SceUID thid = sceKernelCreateThread("Net Downloader Thread", &downloadThread, 0x10000100, 0x100000, 0, 0, NULL);
	sceKernelStartThread(thid, 0, NULL);
	state = DOWNLOADING;
}

static int installThread(unsigned int args, void* arg) {
	VitaPackage pkg = VitaPackage(std::string(dl_dir + d_filename));
	install_state = pkg.Install();
	isInstalling = false;
	
	sceKernelExitDeleteThread(0);
	return 0;
}

static int updaterThread(unsigned int args, void* arg) {
	UpdaterPackage().InstallUpdater();
	UpdatePackage pkg = UpdatePackage(std::string(dl_dir + d_filename));
	pkg.Extract();
	pkg.MakeHeadBin();
	launchUpdater = true;
	isInstalling = false;
	
	sceKernelExitDeleteThread(0);
	return 0;
}

void launchInstaller() {
	if (isInstalling)
		return;
	
	SceUID thid;
	if (!dl_isEasyVPK)
		thid = sceKernelCreateThread("Installer Thread", &installThread, 0x10000100, 0x100000, 0, 0, NULL);
	else
		thid = sceKernelCreateThread("Updater Thread", &updaterThread, 0x10000100, 0x100000, 0, 0, NULL);
	sceKernelStartThread(thid, 0, NULL);
	isInstalling = true;
}

static int downloader_main(unsigned int args, void* arg) {
	SceCtrlData pad;
	
	vita2d_init();
	vita2d_set_clear_color(BLACK);
	vita2d_pgf *pgf;
	pgf = vita2d_load_default_pgf();
	
	uLong zip_idx, zip_total;
	uint8_t extract_started = 0;
	uint64_t curr_extracted_bytes = 0;
	char read_buffer[8192], fname[512];
	unz_global_info global_info;
	unz_file_info file_info;
	unzFile *zipfile;
	FILE *f;
	
	char base_dir[256];
	sprintf(base_dir, dl_dir.c_str());
	
	// Getting free space on ux0
	SceIoDevInfo info;
	memset(&info, 0, sizeof(SceIoDevInfo));
	char *dev_name = (char*)malloc(12);
	sprintf(dev_name, "ux0:");
	int res = sceIoDevctl(dev_name, 0x3001, NULL, 0, &info, sizeof(SceIoDevInfo));
	
	if (res >= 0)
		free_storage = info.free_size;
	else
		sceAppMgrGetDevInfo("ux0:", &dummy, &free_storage);
	
	free(dev_name);
	
	install_state = 0;
	isInstalling = false;
	
	for (;;) {
		
		sceCtrlPeekBufferPositive(0, &pad, 1);

		if ((pad.buttons & SCE_CTRL_CROSS) && (state >= FINISHED) && (dl_type == VPK) && (!install_state))
			launchInstaller();
		
		if (((pad.buttons & SCE_CTRL_CIRCLE) && (state >= FINISHED)) || launchUpdater)
			break;

		vita2d_start_drawing();
		vita2d_clear_screen();
		
		if (isInstalling) vita2d_pgf_draw_text(pgf, 20, 380, RED, 1.0f, "Installing, Please wait...");

		vita2d_pgf_draw_text(pgf,  20,  30, YELLOW, 1.0f, "EasyVPK downloader");
		vita2d_pgf_draw_text(pgf,  20, 514, YELLOW, 1.0f, "based on vitaQuakeIII downloader by Rinnegatamante");
		vita2d_pgf_draw_textf(pgf, 20,  80, PURPLE, 1.0f, "Downloading %s", d_filename.c_str());
	
		// Starting the download
		launchDownload(d_url.c_str());
		
		if (state > DOWNLOADING) {
			if (state >= FINISHED)
				vita2d_pgf_draw_textf(pgf, 20, 400, WHITE, 1.0f, "%s\nPress O to exit.", (install_state || dl_type) ? "Finished!" : (dl_isEasyVPK ? "Press X to update." : "Press X to install. (May take several minutes)"));
			
			if (state < MISSING)
				vita2d_pgf_draw_textf(pgf, 20, 200, GREEN, 1.0f, "Files downloaded successfully! (%.2f %s)", format(downloaded_bytes), sizes[quota(downloaded_bytes)]);
			
			if (state < FINISHED) {
				vita2d_pgf_draw_text(pgf,  20, 220, WHITE, 1.0f, "Extracting files, please wait!");
				vita2d_pgf_draw_textf(pgf, 20, 300, WHITE, 1.0f, "File: %lu / %lu", zip_idx, zip_total);
				vita2d_pgf_draw_textf(pgf, 20, 320, WHITE, 1.0f, "Filename: %s", fname);
				vita2d_pgf_draw_textf(pgf, 20, 340, WHITE, 1.0f, "Filesize: (%.2f %s / %.2f %s)", format(curr_extracted_bytes), sizes[quota(curr_extracted_bytes)], format(file_info.uncompressed_size), sizes[quota(file_info.uncompressed_size)]);
				vita2d_pgf_draw_textf(pgf, 20, 360, WHITE, 1.0f, "Total Progress: (%.2f %s / %.2f %s)", format(extracted_bytes), sizes[quota(extracted_bytes)], format(total_bytes), sizes[quota(total_bytes)]);
				
				if (dl_type == DATA) {
					if (state < EXTRACTING) {
						extracted_bytes = 0;
						total_bytes = 0;
						zipfile = unzOpen(("ux0:/data/" + d_filename).c_str());
						unzGetGlobalInfo(zipfile, &global_info);
						zip_total = global_info.number_entry;
						unzGoToFirstFile(zipfile);
						
						for (zip_idx = 0; zip_idx < zip_total; ++zip_idx) {
							unzGetCurrentFileInfo(zipfile, &file_info, fname, 512, NULL, 0, NULL, 0);
							total_bytes += file_info.uncompressed_size;
							
							if ((zip_idx + 1) < zip_total)
								unzGoToNextFile(zipfile);
						}
						
						zip_idx = 0;
						unzGoToFirstFile(zipfile);
						state = EXTRACTING;
					} 
					
					if (state == EXTRACTING) {
						if (zip_idx < zip_total) {
							if (!extract_started) {
								char filename[512];
								unzGetCurrentFileInfo(zipfile, &file_info, fname, 512, NULL, 0, NULL, 0);
								sprintf(filename, "%s%s", base_dir, fname); 
								const size_t filename_length = strlen(filename);
								
								if (filename[filename_length - 1] == '/') {
									sceIoMkdir(filename, 0777);
								} else {
									unzOpenCurrentFile(zipfile);
									f = fopen(filename, "wb");
									extract_started = 1;
								}
							}
							
							if (extract_started) {
								int err = unzReadCurrentFile(zipfile, read_buffer, 8192);
								if (err > 0) {
									fwrite(read_buffer, err, 1, f);
									extracted_bytes += err;
									curr_extracted_bytes += err;
								} 
								if (curr_extracted_bytes == file_info.uncompressed_size) {
									fclose(f);
									unzCloseCurrentFile(zipfile);
									extract_started = 0;
									curr_extracted_bytes = 0;
								}
							}
							
							if (!extract_started) {
								if ((zip_idx + 1) < zip_total)
									unzGoToNextFile(zipfile);
								zip_idx++;
							}
							
						} else {
							unzClose(zipfile);
							sceIoRemove(("ux0:/data/" + d_filename).c_str());
							state = FINISHED;
						}
					}
					
				} else state = FINISHED;
				
			} else if (state == ERROR) {
				vita2d_pgf_draw_text(pgf,  20, 200,   RED, 1.0f, "ERROR: Not enough free space on ux0...");
				vita2d_pgf_draw_textf(pgf, 20, 220, WHITE, 1.0f, "Please free %.2f %s and restart this core!", format(total_bytes * 2 - free_storage), sizes[quota(total_bytes * 2 - free_storage)]);
			} else if (state == FINISHED && DATA) {
				vita2d_pgf_draw_text(pgf, 20, 220, GREEN, 1.0f, "Files extracted succesfully!");
			}
		} else {
			if (total_bytes == 0xFFFFFFFF) vita2d_pgf_draw_text(pgf, 20, 200, WHITE, 1.0f, "Downloading files, please wait.");
			else vita2d_pgf_draw_textf(pgf, 20, 200, WHITE, 1.0f, "Downloading files, please wait. (%.2f %s / %.2f %s)", format(downloaded_bytes), sizes[quota(downloaded_bytes)], format(total_bytes), sizes[quota(total_bytes)]);
		}

		vita2d_end_drawing();
		vita2d_swap_buffers();
	}
	
	return 0;
}

void Installer::draw(SharedData &sharedData) {
	reset();
	
	dl_type = sharedData.dl_type_sd;
	dl_isEasyVPK = sharedData.vpks[sharedData.cursorY]["titleid"].get<string>() == VITA_TITLEID;
	
	switch (dl_type) {
		case VPK:
			d_url = sharedData.vpks[sharedData.cursorY]["url"].get<string>().c_str();
			d_filename = (sharedData.vpks[sharedData.cursorY]["name"].get<string>() + ".vpk").c_str();
			removeIllegalCharactersFromFilename(d_filename);
			dl_dir = sharedData.vpkDownloadPath;
			break;
		case DATA:
			d_url = sharedData.vpks[sharedData.cursorY]["data"].get<string>().c_str();
			d_filename = getDataFileName(d_url);
			dl_dir = "ux0:/data/";
			break;
		default:
			sharedData.scene = 1;
			return;
	}
	
	if (d_url.empty()) {
		sharedData.scene = 1;
		return;
	}
	
    sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN); // Lock PS button
	
	SceUID main_thread = sceKernelCreateThread("main_downloader", downloader_main, 0x40, 0x1000000, 0, 0, NULL);
	if (main_thread >= 0){
		sceKernelStartThread(main_thread, 0, NULL);
		sceKernelWaitThreadEnd(main_thread, NULL, NULL);
	}
	
	sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN); // Unlock PS button
	
	if (launchUpdater)
		openApp(UPDATER_TITLEID);
	
	sharedData.scene = 1;
}

string Installer::getDataFileName(const string& s) {
	char sep = '/';

	size_t i = s.rfind(sep, s.length());
	if (i != string::npos)
		return (s.substr(i + 1, s.length() - i));

	return ("");
}

void Installer::reset() {
	state = IDLE;
	downloaded_bytes = 0;
	extracted_bytes = 0;
	total_bytes = 0xFFFFFFFF;
}

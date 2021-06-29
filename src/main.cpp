#include "main.hpp"

#include <psp2/kernel/processmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/apputil.h>
#include <psp2/display.h>
#include <psp2/ime_dialog.h>
#include <psp2/appmgr.h>
#include <psp2/promoterutil.h>

#include "net/download.hpp"
#include "utils/filesystem.hpp"
#include "utils/search.hpp"
#include "utils/vitaPackage.h"

#include "screens/list.hpp"
#include "screens/details.hpp"
#include "screens/installer.hpp"


SceCtrlData pad;


void initSceAppUtil() {

	// Init SceAppUtil
	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);

	// Set common dialog config
	SceCommonDialogConfigParam config;
	sceCommonDialogConfigParamInit(&config);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&config.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&config.enterButtonAssign);
	sceCommonDialogSetConfigParam(&config);
}

int main() {
	initSceAppUtil();
	
    // remove updater app if installed
    { // extra scope needed to cause VitaPackage cleanup procedure
        auto updaterPkg = InstalledVitaPackage(UPDATER_TITLEID);
        if (updaterPkg.IsInstalled())
            updaterPkg.Uninstall();
    }

	vita2d_init();
	
	SharedData sharedData;

	Filesystem::removePath(std::string(PACKAGE_TEMP_FOLDER));
	Filesystem::removePath("ux0:data/Easy_VPK");
	Filesystem::mkDir("ux0:data/Easy_VPK");
	Filesystem::mkDir(sharedData.vpkDownloadPath);

	vita2d_set_clear_color(WHITE);
	vita2d_texture *bgIMG = vita2d_load_PNG_file("ux0:app/" VITA_TITLEID "/resources/bg.png");

	httpInit();
	netInit();
	curlDownload(HOMEBREW_URL, "ux0:data/Easy_VPK/vpks.json");
	
	sharedData.vpks = json::parse(Filesystem::readFile("ux0:data/Easy_VPK/vpks.json"));
	sharedData.original = sharedData.vpks;

	List listView;
	Installer installerView;
	Details detailsView;

	while(1) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		vita2d_start_drawing();
		vita2d_clear_screen();

		vita2d_draw_texture(bgIMG, 0, 0);

		if (pad.buttons != SCE_CTRL_CROSS ) sharedData.blockCross  = false;
		if (pad.buttons != SCE_CTRL_SQUARE) sharedData.blockSquare = false;
		if (pad.buttons != SCE_CTRL_CIRCLE) sharedData.blockCircle = false;
		if (pad.buttons != SCE_CTRL_START ) sharedData.blockStart  = false;

		if (sharedData.scene == 0) listView.draw(sharedData, pad.buttons);
		if (sharedData.scene == 1) detailsView.draw(sharedData, pad.buttons);
		if (sharedData.scene == 2) installerView.draw(sharedData);

		vita2d_end_drawing();
		vita2d_common_dialog_update();
		vita2d_swap_buffers();
		sceDisplayWaitVblankStart();

		if (pad.buttons == SCE_CTRL_SELECT || sharedData.scene == -1)
			break;
	}

	httpTerm();
	netTerm();
	vita2d_free_font(sharedData.font);
	vita2d_free_texture(bgIMG);

	for (int i = 0; i < sharedData.screenshots.size(); i++)
		if (sharedData.screenshots[i] != NULL)
			vita2d_free_texture(sharedData.screenshots[i]);

	listView.free();
	detailsView.free();
	vita2d_fini();

	sceKernelExitProcess(0);
	return 0;
}

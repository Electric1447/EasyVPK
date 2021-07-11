#pragma once

#include <stdexcept>


#ifndef PACKAGE_TEMP_FOLDER
	#define PACKAGE_TEMP_FOLDER "ux0:/temp/pkg/"
#endif


using namespace std;


class VitaPackage{
public:
	explicit VitaPackage(string vpk);
	~VitaPackage();

	int Install();
	void Extract();
	int InstallExtracted();

private:
	string vpk_;
};

class UpdaterPackage : private VitaPackage {
public:
	UpdaterPackage() : VitaPackage("easyvpk_updater") {};

	int InstallUpdater();
};

class UpdatePackage : private VitaPackage {
public:
	explicit UpdatePackage(string vpk) : VitaPackage(vpk) {};

	void Extract() { VitaPackage::Extract(); }
	void MakeHeadBin();
};

class InstalledVitaPackage : private VitaPackage {
public:
	explicit InstalledVitaPackage(string title_id) : VitaPackage(""), title_id(move(title_id)) {}

	bool IsInstalled();

	int Uninstall();

private:
	string title_id;
};

bool isPackageInstalled(string titleid);
void openApp(string titleid);

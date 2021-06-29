#pragma once

#include <stdexcept>

#include "filesystem.hpp"

#ifndef PACKAGE_TEMP_FOLDER
	#define PACKAGE_TEMP_FOLDER "ux0:/temp/pkg/"
#endif


class VitaPackage{
public:
	explicit VitaPackage(std::string vpk);
	~VitaPackage();

	int Install();
	void Extract();
	int InstallExtracted();

private:
	std::string vpk_;
};

class UpdaterPackage : private VitaPackage {
public:
	UpdaterPackage() : VitaPackage("easyvpk_updater") {};

	int InstallUpdater();
};

class UpdatePackage : private VitaPackage {
public:
	explicit UpdatePackage(std::string vpk) : VitaPackage(vpk) {};

	void Extract() { VitaPackage::Extract(); }
	void MakeHeadBin();
};

class InstalledVitaPackage : private VitaPackage {
public:
	explicit InstalledVitaPackage(std::string title_id) : VitaPackage(""), title_id(std::move(title_id)) {}

	bool IsInstalled();

	int Uninstall();

private:
	std::string title_id;
};

bool isPackageInstalled(std::string titleid);
void openApp(std::string titleid);

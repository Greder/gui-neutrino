/*
	Based up Neutrino-GUI - Tuxbox-Project
	Copyright (C) 2001 by Steffen Hehn 'McClean'

	Implementation of component classes
	Copyright (C) 2013-2020, Thilo Graf 'dbt'

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if USE_STB_HAL
#include <version_hal.h>
#endif

#include "imageinfo.h"

#include <global.h>
#include <neutrino.h>

#include <driver/rcinput.h>
#include <driver/fontrenderer.h>

#include <sys/utsname.h>
#include <string>
#include <daemonc/remotecontrol.h>
#include <system/flashtool.h>
#include <system/helpers.h>
#ifdef VCS
#include "version.h"
#endif
#include "buildinfo.h"
#define LICENSEDIR DATADIR "/neutrino/license/"
#define POLICY_DIR DATADIR "/neutrino/policy/"
#ifdef ENABLE_LUA
#include <gui/lua/lua_api_version.h>
#endif
#include <nhttpd/yconfig.h>
#include <ctype.h>

#define OS_RELEASE_FILE "/usr/lib/os-release"
#define OE_IMAGE_VERSION_FILE "/etc/image-version"

using namespace std;

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

CImageInfo::CImageInfo(): config ('\t')
{
	Init();
}

//init all var members
void CImageInfo::Init(void)
{
	cc_win		= NULL;
	cc_info 	= NULL;
	cc_tv		= NULL;
	cc_lic		= NULL;
	cc_sub_caption	= NULL;
	btn_red		= NULL;
	btn_green	= NULL;
	item_offset	= OFFSET_INNER_MID;
	item_font 	= g_Font[SNeutrinoSettings::FONT_TYPE_WINDOW_GENERAL];
	item_height 	= 0;
	y_tmp 		= 0;
	license_txt	= "";
	policy_txt	= "";
	InitBuildInfos();
	v_info.clear();
}

CImageInfo::~CImageInfo()
{
	hide();
	//deallocate window object, deletes also added cc_items
	delete cc_win;
}

void CImageInfo::Clean()
{
	//deallocate of the window object causes also deallocate added items,
	if (cc_win){
		delete cc_win;
		//it's important to set here a null pointer
		cc_win 	= NULL;
		cc_info = NULL;
		cc_tv	= NULL;
		cc_lic	= NULL;
		cc_sub_caption = NULL;
		btn_red	= NULL;
		btn_green = NULL;
	}
}

int CImageInfo::exec(CMenuTarget* parent, const std::string &)
{
	int res = menu_return::RETURN_REPAINT;
	if (parent)
		parent->hide();

	//clean up before, because we could have a current instance with already initialized contents
	Clean();

	//fill manifest texts
	license_txt	= getLicenseText();
	policy_txt	= getPolicyText();

	//init window object, add cc-items and paint all
	ShowWindow();
	bool fadeout = false;
	neutrino_msg_t postmsg = 0;
	neutrino_msg_t msg;
	while (1)
	{
		neutrino_msg_data_t data;
		uint64_t timeoutEnd = CRCInput::calcTimeoutEnd_MS(100);
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ((msg == NeutrinoMessages::EVT_TIMER) && (data ==cc_win->GetFadeTimer())){
			if (cc_win->FadeDone())
				break;
			continue;
		}
		if (fadeout && msg == CRCInput::RC_timeout){
			if (cc_win->StartFadeOut()){
				msg = menu_return::RETURN_EXIT_ALL;
				continue;
			}
			else
				break;
		}
		if(msg == CRCInput::RC_setup) {
			res = menu_return::RETURN_EXIT_ALL;
			fadeout = true;
		}
		else if (msg == btn_red->getButtonDirectKey()/* CRCInput::RC_red*/){
			//toggle and assign caption and info contents
			if (btn_red->getCaptionLocale() == LOCALE_IMAGEINFO_LICENSE){
				cc_sub_caption->setText(LOCALE_IMAGEINFO_LICENSE, CTextBox::AUTO_WIDTH, item_font);
				InitInfoText(license_txt);
				btn_red->setCaption(LOCALE_BUILDINFO_MENU);
			}else{
				cc_sub_caption->setText(LOCALE_BUILDINFO_MENU, CTextBox::AUTO_WIDTH, item_font);
				InitInfoText(build_info_txt);
				btn_red->setCaption(LOCALE_IMAGEINFO_LICENSE);
			}
			//paint items
			cc_sub_caption->hide();
			cc_sub_caption->paint();
			cc_lic->paint(false);
			btn_red->kill();
			btn_red->paint(false);
		}
		else if (msg == btn_green->getButtonDirectKey()/* CRCInput::RC_green*/){
				cc_sub_caption->setText(LOCALE_IMAGEINFO_POLICY, CTextBox::AUTO_WIDTH, item_font);
				InitInfoText(policy_txt);
				cc_sub_caption->hide();
				cc_sub_caption->paint();
				cc_lic->paint(false);
		}
		else if (CNeutrinoApp::getInstance()->listModeKey(msg)) {
			postmsg = msg;
			res = menu_return::RETURN_EXIT_ALL;
			fadeout = true;
		}
		else if ((msg == CRCInput::RC_up) || (msg == CRCInput::RC_page_up)) {
			ScrollLic(false);
		}
		else if ((msg == CRCInput::RC_down) || (msg == CRCInput::RC_page_down)) {
			ScrollLic(true);
		}
		else if (msg <= CRCInput::RC_MaxRC){
			fadeout = true;
		}

		if ( msg >  CRCInput::RC_MaxRC && msg != CRCInput::RC_timeout){
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
	}

	if (postmsg)
		g_RCInput->postMsg(postmsg, 0);

	hide();
	return res;
}

//contains all actions to init and add the window object and items
void CImageInfo::ShowWindow()
{
	CComponentsFooter *footer = NULL;
	if (cc_win == NULL){
		cc_win = new CComponentsWindowMax(LOCALE_IMAGEINFO_HEAD, NEUTRINO_ICON_INFO, 0, CC_SHADOW_ON);
		cc_win->setWindowHeaderButtons(CComponentsHeader::CC_BTN_MENU | CComponentsHeader::CC_BTN_EXIT);
		footer = cc_win->getFooterObject();

		button_label_cc btn;
		vector<button_label_cc> v_buttons;

		btn.button = NEUTRINO_ICON_BUTTON_RED;
		btn.locale = LOCALE_BUILDINFO_MENU;
		btn.directKeys.push_back(CRCInput::RC_red);
		v_buttons.push_back(btn);

		btn.button = !policy_txt.empty() ? NEUTRINO_ICON_BUTTON_GREEN : NEUTRINO_ICON_BUTTON_DUMMY;
		btn.locale = LOCALE_IMAGEINFO_POLICY;
		btn.directKeys.clear();
		btn.directKeys.push_back(!policy_txt.empty() ? CRCInput::RC_green : RC_NOKEY);
		v_buttons.push_back(btn);

		footer->setButtonLabels(v_buttons, 0, footer->getWidth()/v_buttons.size());

		btn_red = footer->getButtonLabel(0);
		btn_red->setButtonTextColor(COL_MENUFOOT_TEXT);
		btn_red->setColBodyGradient(CC_COLGRAD_OFF);

		btn_green = footer->getButtonLabel(1);
// 		btn_green->allowPaint(!policy_txt.empty());
	}

	//prepare minitv
	InitMinitv();

	//prepare infos
	InitInfos();

	//prepare info text
	InitInfoText(license_txt);

	//paint window
	cc_win->StartFadeIn();
	cc_win->paint(CC_SAVE_SCREEN_NO);
}

//prepare minitv
void CImageInfo::InitMinitv()
{
	//init the minitv object
	if (cc_tv == NULL)
		cc_tv = new CComponentsPIP (0, item_offset);
	
#if 0 //static assign of dimensions are distorting ratio of mini tv picture
	//init width and height
	cc_tv->setWidth(cc_win->getWidth()/3);
	cc_tv->setHeight(cc_win->getHeight()/3);
#endif

	//init x pos and use as parameter for setXPos
	int cc_tv_x = (cc_win->getWidth() - cc_tv->getWidth()) - item_offset;
	cc_tv->setXPos(cc_tv_x);

	//add minitv to container
	if (!cc_tv->isAdded())
		cc_win->addWindowItem(cc_tv);
}

//prepare distribution infos
void CImageInfo::InitBuildInfos()
{
	CBuildInfo b_info;
	for (uint i=0; i<CBuildInfo::BI_TYPE_IDS; i++){
		build_info_txt += g_Locale->getText(b_info.getInfo(i).caption);
		build_info_txt += "\n";
		build_info_txt += b_info.getInfo(i).info_text  + "\n\n";
	}
}

//collect required data from environment
void CImageInfo::InitInfoData()
{
	v_info.clear();

	image_info_t pretty_name = {g_Locale->getText(LOCALE_IMAGEINFO_OS),""};
	if (file_exists(OS_RELEASE_FILE)){
		config.loadConfig(OS_RELEASE_FILE);
		string tmpstr = config.getString("PRETTY_NAME", "");
		pretty_name.info_text = str_replace("\"", "", tmpstr);
		config.clear();
	}

	string oe_image_version = "";
	if (file_exists(OE_IMAGE_VERSION_FILE)){
		config.loadConfig(OE_IMAGE_VERSION_FILE);
		oe_image_version = config.getString("imageversion", "");
		string desc = config.getString("imagedescription", "");
		if (!desc.empty())
			oe_image_version += " - " + desc;
		desc = config.getString("describe", "");
		if (!pretty_name.info_text.empty()) {
			if (!desc.empty())
				pretty_name.info_text += " - " + desc;
		}
		else
			pretty_name.info_text = "unknown";
		config.clear();
	}

	config.loadConfig(IMAGE_VERSION_FILE);

	string version_string = config.getString("version", "");

#ifdef IMAGE_VERSION
	version_string = IMAGE_VERSION;
#else
	bool is_version_code = true;
	for (size_t i=0; i<version_string.size(); i++){
		if (!isdigit(version_string[i])){
			is_version_code = false;
			break;
		}
	}
	if (is_version_code && version_string.size() == 16){
		static CFlashVersionInfo versionInfo(version_string.c_str());
		if (oe_image_version.empty()){
			version_string = versionInfo.getVersionString();
			version_string += " ";
			version_string += versionInfo.getType();
			version_string += " (";
			version_string += versionInfo.getDate();
			version_string += ")";
		}else
			version_string = oe_image_version;
	}else
		printf("[CImageInfo]\t[%s - %d], WARNING! %s contains possible wrong version format, content = [%s]\n", __func__, __LINE__, IMAGE_VERSION_FILE, version_string.c_str());
#endif

	image_info_t imagename 	= {g_Locale->getText(LOCALE_IMAGEINFO_IMAGE),	config.getString("imagename", PACKAGE_NAME)};
	if (!version_string.empty()){
		image_info_t version	= {g_Locale->getText(LOCALE_IMAGEINFO_VERSION),	version_string};
		imagename.info_text += " ";
		imagename.info_text += version_string;
		v_info.push_back(imagename);
	}else
		v_info.push_back(imagename);

	if (!pretty_name.info_text.empty())
		v_info.push_back(pretty_name);

	//kernel
	initKernelInfo();

	//build date
	initBuildDateInfo();

	//creator
	string creator = config.getString("creator", "");
	if (!creator.empty())
		v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_CREATOR), creator});

	//gui
	initGuiInfo();

	//stb info
	initHalInfo();

	//internal api versions
	initAPIVersions();

	//support infos
	initSupportInfo();
}

void CImageInfo::initBuildDateInfo()
{
	string builddate = "";
#ifdef BUILT_DATE
	builddate = BUILT_DATE;
#else
	builddate = config.getString("builddate", "").c_str();
#endif
	if (builddate.empty())
		builddate = PACKAGE_VERSION_DATE;
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_DATE),	builddate});
}

void CImageInfo::initGuiInfo()
{
	string gui_info = PACKAGE_NAME;
#if ENABLE_PKG_MANAGEMENT
	string pkg_name = "neutrino-mp";
	gui_info += " (";
	gui_info += pkg_name;
	gui_info += " ";
	gui_info += man.getPkgInfo(pkg_name, "Version",  true);
	gui_info += ")";
#endif
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_GUI), config.getString("gui", gui_info)});

#ifndef ENABLE_PKG_MANAGEMENT
# ifdef VCS
	//gui vcs
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_VCS),	VCS});
# else
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_VCS),	PACKAGE_VERSION_GIT});
# endif
#endif
}

void CImageInfo::initHalInfo()
{
#if USE_STB_HAL
	hal_libversion_t ver;
	hal_get_lib_version(&ver);
	//libstb-hal version
	v_info.push_back({"libstb-hal:", ver.vName});
	//libstb-hal git status
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_VCS), ver.vGitDescribe});
#endif
}

void CImageInfo::initKernelInfo()
{
	struct utsname uts_info;
	if (uname(&uts_info) == 0)
		v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_KERNEL),	uts_info.release});
}


void CImageInfo::initSupportInfo()
{
	//www
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_HOMEPAGE),	config.getString("homepage", PACKAGE_URL)});
	//doc
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_DOKUMENTATION), config.getString("docs", "http://wiki.tuxbox-neutrino.org")});
	//support
	v_info.push_back( {g_Locale->getText(LOCALE_IMAGEINFO_FORUM),	config.getString("forum", "http://forum.tuxbox.org")});
}

void CImageInfo::initAPIVersions()
{
	string s_api;
	#ifdef ENABLE_LUA
	s_api	+= "LUA " + to_string(LUA_API_VERSION_MAJOR) + "." + to_string(LUA_API_VERSION_MINOR);
	s_api	+= ", ";
	#endif
	s_api	+= "yWeb ";
	s_api	+= getYWebVersion();
	s_api	+= ", ";
	s_api	+= HTTPD_NAME;
	s_api	+= + " ";
	s_api	+= HTTPD_VERSION;
	s_api	+= + ", ";
	s_api	+= YHTTPD_NAME;
	s_api	+= + " ";
	s_api	+= YHTTPD_VERSION;
	v_info.push_back({g_Locale->getText(LOCALE_IMAGEINFO_API),	s_api});
}


//prepare distribution infos
void CImageInfo::InitInfos()
{
	InitInfoData();

	//initialize container for infos
	if (cc_info == NULL)
		cc_info = new CComponentsForm();
	if (!cc_info->isAdded())
		cc_win->addWindowItem(cc_info);
	
	cc_info->setPos(item_offset, item_offset);
	
	//set width, use size between left border and minitv
	cc_info->setWidth(cc_win->getWidth() - cc_tv->getWidth() - 2*item_offset);
	
	//create label and text items
	y_tmp = 0;
	for (size_t i=0; i<v_info.size(); i++) {
		CComponentsExtTextForm *item = new CComponentsExtTextForm(1, y_tmp, cc_info->getWidth(), 0, v_info[i].caption, v_info[i].info_text, item_font);
		item->setLabelWidthPercent(15);

		//calculate initial height for info form
		item_height = item_font->getHeight();

		item->setHeight(item_height);
		cc_info->setHeight(v_info.size()*item_height);

		//add ext-text object to window body
		if (!item->isAdded())
			cc_info->addCCItem(item);

		//add an offset before homepage and license and at the end
		if (v_info[i].caption == g_Locale->getText(LOCALE_IMAGEINFO_CREATOR) || v_info[i].caption == g_Locale->getText(LOCALE_IMAGEINFO_FORUM)){
			CComponentsShapeSquare *spacer = new CComponentsShapeSquare(1, y_tmp+=item_offset, 1, item_offset);
			//spacer ist not visible!
			spacer->allowPaint(false);
			cc_info->addCCItem(spacer);
			//increase height of cc_info object with offset
			int tmp_h = cc_info->getHeight();
			cc_info->setHeight(tmp_h + item_offset);
		}
		y_tmp += item->getHeight();
	}
}

string CImageInfo::getManifest(const std::string& directory, const std::string& language, const std::string& manifest_type)
{
	string file = directory;
	file += language;
	file += "." + manifest_type;

	string res = CComponentsText::getTextFromFile(file);

	//assign default text, if language file is not available
	if(res.empty()){
		file = directory;
		file += "english." + manifest_type;
		res = CComponentsText::getTextFromFile(file);
	}

	return res;
}

//get license
string CImageInfo::getLicenseText()
{
	return getManifest(LICENSEDIR, g_settings.language, "license");
}

//get policy
string CImageInfo::getPolicyText()
{
	return getManifest(POLICY_DIR, g_settings.language, "policy");
}

//prepare info text
void CImageInfo::InitInfoText(const std::string& text)
{
	//get window body object
	CComponentsForm *winbody = cc_win->getBodyObject();
	int h_body = winbody->getHeight();
	int w_body = winbody->getWidth();

	//add a caption for info contents
	Font * caption_font = g_Font[SNeutrinoSettings::FONT_TYPE_MENU];
	int caption_height = caption_font->getHeight();
	y_tmp = max(y_tmp, cc_tv->getYPos()+cc_tv->getHeight());
	if (cc_sub_caption == NULL)
		cc_sub_caption = new CComponentsLabel(cc_info->getXPos(), y_tmp, cc_info->getWidth(), caption_height,
						     g_Locale->getText(LOCALE_IMAGEINFO_LICENSE), CTextBox::AUTO_WIDTH, item_font);
	if (!cc_sub_caption->isAdded())
		cc_win->addWindowItem(cc_sub_caption);

	//add info text box
	int h_txt = h_body - item_offset - cc_info->getHeight() - cc_sub_caption->getHeight() - item_offset;

	if (cc_lic == NULL)
		cc_lic = new CComponentsInfoBox(CC_CENTERED, y_tmp+=cc_sub_caption->getHeight(), w_body-2*item_offset, h_txt);
	cc_lic->setSpaceOffset(1);
	cc_lic->setText(text, CTextBox::TOP | CTextBox::AUTO_WIDTH | CTextBox::SCROLL, item_font);
	cc_lic->doPaintTextBoxBg(true);

	//add text to container
	if (!cc_lic->isAdded())
		cc_win->addWindowItem(cc_lic);
}

//scroll licens text
void CImageInfo::ScrollLic(bool scrollDown)
{
	if (cc_lic && (cc_lic->cctext)) {
		//get the textbox instance from infobox object and use CTexBbox scroll methods
		CTextBox* ctb = cc_lic->cctext->getCTextBoxObject();
		if (ctb) {
			//ctb->enableBackgroundPaint(true);
			if (scrollDown)
				ctb->scrollPageDown(1);
			else
				ctb->scrollPageUp(1);
			//ctb->enableBackgroundPaint(false);
		}
	}
}

void CImageInfo::hide()
{
	printf("[CImageInfo]   [%s - %d] hide...\n", __FUNCTION__, __LINE__);
	if (cc_win){
		cc_win->kill();
		cc_win->StopFade();
		Clean();
	}
}

string CImageInfo::getYWebVersion()
{
	CConfigFile yConf('=', false);
	yConf.loadConfig(CONFIGDIR "/nhttpd.conf");
	std::string ver = yConf.getString("WebsiteMain.directory", "n/a") + "/Y_Version.txt";

	CConfigFile yV('=', false);
	if (file_exists(ver.c_str()))
	{
		yV.loadConfig(ver);
		return yV.getString("version", "n/a");
	}
	return "n/a";
}

AM_CPPFLAGS = -fno-rtti -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
BUILT_SOURCES = version.h
version.h:
	@rm -f $@; \
	echo '#define BUILT_DATE "'`date`'"' > $@
	@if test -d $(top_srcdir)/.git ; then \
		pushd $(top_srcdir) ; \
		GITBRANCH=$$(git rev-parse --abbrev-ref HEAD) ; \
		GITDESCRIBE=$$(git describe --always --tags --dirty || echo 'VCS failed') ; \
		popd ; \
		echo '#define VCS "'$${GITDESCRIBE} [$${GITBRANCH}]'"' >> $@ ; \
	fi
noinst_HEADERS = version.h
.PHONY: version.h

SUBDIRS = bedit components moviebrowser widget

if ENABLE_LUA
SUBDIRS += lua
endif

AM_CPPFLAGS += \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/xmltree \
	-I$(top_srcdir)/lib/libupnpclient \
	-I$(top_srcdir)/lib/jsoncpp \
	@SIGC_CFLAGS@ \
	@CURL_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@AVFORMAT_CFLAGS@ \
	@LUA_CFLAGS@ \
	@HWLIB_CFLAGS@

noinst_LIBRARIES = libtimerlist.a libneutrino_gui.a libneutrino_gui2.a

libneutrino_gui_a_SOURCES = \
	adzap.cpp \
	audio_select.cpp \
	audio_setup.cpp \
	audiomute.cpp \
	audioplayer.cpp \
	audioplayer_setup.cpp\
	bookmarkmanager.cpp \
	bouquetlist.cpp \
	buildinfo.cpp \
	channellist.cpp \
	cec_setup.cpp \
	dboxinfo.cpp \
	epgplus.cpp \
	epgview.cpp \
	eventlist.cpp \
	favorites.cpp \
	filebrowser.cpp \
	followscreenings.cpp \
	imageinfo.cpp \
	info_menue.cpp \
	infoviewer.cpp \
	infoviewer_bb.cpp \
	keybind_setup.cpp \
	mdb-imdb.cpp \
	mdb-tmdb.cpp \
	mediaplayer.cpp \
	mediaplayer_setup.cpp \
	miscsettings_menu.cpp \
	movieplayer.cpp \
	network_service.cpp \
	network_setup.cpp \
	nfs.cpp \
	osd_helpers.cpp \
	osd_progressbar_setup.cpp \
	osd_setup.cpp \
	osdlang_setup.cpp \
	parentallock_setup.cpp \
	personalize.cpp \
	pictureviewer.cpp \
	pictureviewer_setup.cpp \
	pluginlist.cpp \
	plugins.cpp \
	proxyserver_setup.cpp \
	radiotext_window.cpp \
	rate_banner.cpp \
	rc_lock.cpp \
	record_setup.cpp \
	scan.cpp \
	scan_setup.cpp \
	screensaver.cpp \
	screensetup.cpp \
	settings_manager.cpp \
	sleeptimer.cpp \
	start_wizard.cpp \
	subchannel_select.cpp \
	test_menu.cpp \
	themes.cpp \
	timeosd.cpp \
	update.cpp \
	update_check.cpp \
	update_menue.cpp \
	update_settings.cpp \
	user_menue.cpp \
	user_menue_setup.cpp \
	vfd_setup.cpp \
	videosettings.cpp \
	volumebar.cpp \
	weather.cpp \
	weather_setup.cpp \
	webchannels_setup.cpp \
	xmltv_setup.cpp \
	zapit_setup.cpp

if ENABLE_PIP
libneutrino_gui_a_SOURCES += \
	pipsetup.cpp
if ENABLE_QUADPIP
libneutrino_gui_a_SOURCES += \
	quadpip_setup.cpp
endif
endif

if ENABLE_PKG_MANAGEMENT
libneutrino_gui_a_SOURCES += \
	opkg_manager.cpp \
	update_check_packages.cpp
endif

if ENABLE_LCD4LINUX
libneutrino_gui_a_SOURCES += \
	lcd4l_setup.cpp
endif

if ENABLE_GRAPHLCD
libneutrino_gui_a_SOURCES += \
	glcdsetup.cpp \
	glcdthemes.cpp
endif

if BOXTYPE_CST
libneutrino_gui_a_SOURCES += \
	streaminfo1.cpp
else
libneutrino_gui_a_SOURCES += \
	streaminfo2.cpp
endif

if ENABLE_EXTUPDATE
libneutrino_gui_a_SOURCES += \
	update_ext.cpp
endif

if BOXTYPE_ARMBOX
libneutrino_gui_a_SOURCES += \
	psisetup.cpp
endif

if BOXTYPE_MIPSBOX
libneutrino_gui_a_SOURCES += \
	psisetup.cpp
endif

libneutrino_gui2_a_SOURCES = \
	cam_menu.cpp \
	color.cpp \
	hdd_menu.cpp \
	infoclock.cpp \
	motorcontrol.cpp

if ENABLE_UPNP
libneutrino_gui2_a_SOURCES += \
	upnpbrowser.cpp
endif

libtimerlist_a_SOURCES = \
	timerlist.cpp

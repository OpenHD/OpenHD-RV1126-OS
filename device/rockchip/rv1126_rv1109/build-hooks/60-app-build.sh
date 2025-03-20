#!/bin/bash -e

app_clean()
{
	make -f buildroot/Makefile \
		dbserver-dirclean \
		common_algorithm-dirclean \
		ipcweb-backend-dirclean \
		libgdbus-dirclean \
		libIPCProtocol-dirclean \
		librkdb-dirclean \
		mediaserver-dirclean \
		camera_engine_rkaiq-dirclean \
		netserver-dirclean \
		storage_manager-dirclean \
		rkmedia-dirclean \
		rk_oem-dirclean \
		mpp-dirclean \
		ipc-daemon-dirclean \
		rockface-dirclean \
		CallFunIpc-dirclean \
		isp2-ipc-dirclean
}

app_sync()
{
	.repo/repo/repo sync -c --no-tags \
		app/dbserver \
		app/ipcweb-backend \
		app/libgdbus \
		app/libIPCProtocol \
		app/librkdb \
		app/mediaserver \
		app/netserver \
		app/ipc-daemon \
		app/storage_manager \
		external/camera_engine_rkaiq \
		external/rkmedia \
		external/common_algorithm \
		external/rockface \
		external/mpp \
		external/CallFunIpc \
		external/isp2-ipc
}

app_rebuild()
{
	make -f buildroot/Makefile \
		dbserver-rebuild \
		common_algorithm-rebuild \
		libgdbus-rebuild \
		libIPCProtocol-rebuild \
		librkdb-rebuild \
		CallFunIpc-rebuild \
		camera_engine_rkaiq-rebuild \
		isp2-ipc-rebuild \
		ipcweb-backend-rebuild \
		netserver-rebuild \
		storage_manager-rebuild \
		rk_oem-rebuild \
		mpp-rebuild \
		ipc-daemon-rebuild \
		rockface-rebuild \
		rkmedia-rebuild \
		mediaserver-rebuild
}

# Hooks

usage_hook()
{
	echo "app-clean          - clean buildroot app"
	echo "app-rebuild        - rebuild buildroot app"
	echo "app-sync           - sync buildroot app"
}

BUILD_CMDS="app-clean app-rebuild app-sync"
build_hook()
{
	source buildroot/build/envsetup.sh $RK_BUILDROOT_CFG

	case "$1" in
		app-clean) app_clean ;;
		app-rebuild) app_rebuild ;;
		app-sync) app_sync ;;
	esac

	finish_build $1
}

source "${BUILD_HELPER:-$(dirname "$(realpath "$0")")/../build-hooks/build-helper}"

build_hook $@

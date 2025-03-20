#!/bin/bash -e

MAKE="make ${DEBUG:+V=1} -C $(realpath --relative-to="$SDK_DIR" "$COMMON_DIR")"

switch_defconfig()
{
	CONFIG="$1"

	[ -f "$CONFIG" ] || CONFIG="$CHIP_DIR/$CONFIG"

	if [ ! -f "$CONFIG" ]; then
		echo "No such defconfig: $1"
		exit 1
	fi

	echo "Switching to defconfig: $CONFIG"
	rm -f "$RK_DEFCONFIG"
	ln -rsf "$CONFIG" "$RK_DEFCONFIG"

	CONFIG="$(realpath "$CONFIG")"
	rm -rf "$CHIP_DIR"
	ln -rsf "$(dirname "$CONFIG")" "$CHIP_DIR"

	$MAKE $(basename "$CONFIG")
	exit 0
}

rockchip_defconfigs()
{
	cd "$CHIP_DIR"
	ls rockchip_defconfig 2>/dev/null || true
	ls *_defconfig | grep -v rockchip_defconfig || true
}

lunch()
{
	CONFIG_ARRAY=( $(rockchip_defconfigs) )

	CONFIG_ARRAY_LEN=${#CONFIG_ARRAY[@]}
	if [ $CONFIG_ARRAY_LEN -eq 0 ]; then
		echo "No available defconfig"
		return 1
	fi

	if [ $CONFIG_ARRAY_LEN -eq 1 ]; then
		switch_defconfig ${CONFIG_ARRAY[0]}
		return 0
	fi

	echo
	echo "You're building on Linux"
	echo "Lunch menu...pick a combo:"
	echo ""

	echo ${CONFIG_ARRAY[@]} | xargs -n 1 | sed "=" | sed "N;s/\n/. /"

	local INDEX
	read -p "Which would you like? [1]: " INDEX
	INDEX=$((${INDEX:-1} - 1))
	CONFIG="${CONFIG_ARRAY[$INDEX]}"

	switch_defconfig $CONFIG
}

prepare_config()
{
	if [ ! -r "$RK_DEFCONFIG" ]; then
		echo "WARN: $RK_DEFCONFIG not exists"
		lunch
		return 0
	fi

	DEFCONFIG=$(basename "$(realpath "$RK_DEFCONFIG")")
	if [ ! "$RK_DEFCONFIG" -ef "$CHIP_DIR/$DEFCONFIG" ]; then
		echo "WARN: $RK_DEFCONFIG is invalid"
		lunch
		return 0
	fi

	if [ "$RK_CONFIG" -ot "$RK_DEFCONFIG" ]; then
		echo "WARN: $RK_CONFIG is out-dated"
		$MAKE $DEFCONFIG
		return 0
	fi

	CONFIG_DIR="$(dirname "$RK_CONFIG_IN")"
	if find "$CONFIG_DIR" -cnewer "$RK_CONFIG" | grep -q ""; then
		echo "WARN: $CONFIG_DIR is updated"
		$MAKE $DEFCONFIG
		return 0
	fi

	CFG="RK_DEFCONFIG=\"$(realpath "$RK_DEFCONFIG")\""
	if ! grep -wq "$CFG" "$RK_CONFIG"; then
		echo "WARN: $RK_CONFIG is invalid"
		$MAKE $DEFCONFIG
		return 0
	fi

	$MAKE olddefconfig
}

# Hooks

usage_hook()
{
	echo "lunch              - choose defconfig"
	echo "*_defconfig        - switch to specified defconfig"
	echo "    Available defconfigs:"
	echo "$(ls "$CHIP_DIR/" | grep "defconfig$" | sed "s/^/\t/")"
	echo "olddefconfig       - resolve any unresolved symbols in .config"
	echo "savedefconfig      - save current config to defconfig"
	echo "menuconfig         - interactive curses-based configurator"
}

clean_hook()
{
	RK_BUILDING=1 $MAKE distclean
}

INIT_CMDS="lunch .*_defconfig olddefconfig savedefconfig menuconfig default"
init_hook()
{
	case "${1:-default}" in
		lunch) lunch ;;
		*_defconfig) switch_defconfig "$1" ;;
		olddefconfig | savedefconfig | menuconfig) $MAKE $1 ;;
		default) prepare_config ;; # End of init
		*) usage ;;
	esac
}

source "${BUILD_HELPER:-$(dirname "$(realpath "$0")")/../build-hooks/build-helper}"

init_hook $@

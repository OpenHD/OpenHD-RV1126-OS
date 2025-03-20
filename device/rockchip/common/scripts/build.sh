#!/bin/bash

if [ -z "$BASH_SOURCE" ]; then
	echo "Not in bash, switching to it..."
	case "${@:-shell}" in
		shell) ./build.sh shell ;;
		*)
			./build.sh $@
			bash
			;;
	esac
fi

usage()
{
	echo "Usage: $(basename $BASH_SOURCE) [OPTIONS]"
	echo "Available options:"

	run_build_hooks usage

	# Global options
	echo "cleanall           - cleanup"
	echo "post-rootfs        - trigger post-rootfs hook scripts"
	echo "shell              - setup a shell for developing"
	echo "help               - usage"
	echo ""
	echo "Default option is 'allsave'."
	exit 0
}

err_handler()
{
	ret=${1:-$?}
	[ "$ret" -eq 0 ] && return

	echo "ERROR: Running $BASH_SOURCE - ${2:-${FUNCNAME[1]}} failed!"
	echo "ERROR: exit code $ret from line ${BASH_LINENO[0]}:"
	echo "    ${3:-$BASH_COMMAND}"
	echo "ERROR: call stack:"
	for i in $(seq 1 $((${#FUNCNAME[@]} - 1))); do
		SOURCE="${BASH_SOURCE[$i]}"
		LINE=${BASH_LINENO[$(( $i - 1 ))]}
		echo "    $(basename "$SOURCE"): ${FUNCNAME[$i]}($LINE)"
	done
	exit $ret
}

# Export global functions
set -a

finish_build()
{
	echo -e "\e[35mRunning ${@:-${FUNCNAME[1]}} succeeded.\e[0m"
	cd "$SDK_DIR"
}

check_config()
{
	unset missing
	for var in $@; do
		eval [ \$$var ] && continue

		missing="$missing $var"
	done

	[ -z "$missing" ] && return 0

	echo "Skipping ${FUNCNAME[1]} for missing configs: $missing."
	return 1
}

kernel_version()
{
	[ -d kernel ] || return 0

	VERSION_KEYS="VERSION PATCHLEVEL"
	VERSION=""

	for k in $VERSION_KEYS; do
		v=$(grep "^$k = " kernel/Makefile | cut -d' ' -f3)
		VERSION=${VERSION:+${VERSION}.}$v
	done
	echo $VERSION
}

start_log()
{
	LOG_FILE="$RK_LOG_DIR/${2:-$1_$(date +%F_%H-%M-%S)}.log"
	ln -rsf "$LOG_FILE" "$RK_LOG_DIR/$1.log"
	echo "# $(date +"%F %T")" >> "$LOG_FILE"
	echo "$LOG_FILE"
}

set +a
# End of global functions

run_hooks()
{
	DIR="$1"
	shift

	unset HOOK_HANDLED
	for dir in "$CHIP_DIR/$(basename "$DIR")/" "$DIR"; do
		[ -d "$dir" ] || continue

		for hook in $(find "$dir" -maxdepth 1 -name "*.sh" | sort); do
			"$hook" $@ && continue
			HOOK_RET=$?

			if [ $HOOK_RET -eq $HOOK_RET_HANDLED ]; then
				HOOK_HANDLED=1
				continue
			fi

			err_handler $HOOK_RET "${FUNCNAME[0]} $*" "$hook $*"
			exit $HOOK_RET
		done
	done

	[ -z "$HOOK_HANDLED" ] || return $HOOK_RET_HANDLED
}

run_build_hooks()
{
	# Don't log these hooks
	case "$1" in
		init | usage | option-check)
			run_hooks "$RK_BUILD_HOOK_DIR" $@ || true
			return 0
			;;
	esac

	LOG_FILE="$(start_log "$1")"

	echo -e "# run hook: $@\n" >> "$LOG_FILE"
	run_hooks "$RK_BUILD_HOOK_DIR" $@ 2>&1 | tee -a "$LOG_FILE"
	case "${PIPESTATUS[0]}" in
		0) ;;
		$HOOK_RET_HANDLED) HOOK_HANDLED=1 ;;
		*) exit 1 ;;
	esac
}

run_post_hooks()
{
	LOG_FILE="$(start_log post-rootfs)"

	echo -e "# run hook: $@\n" >> "$LOG_FILE"
	run_hooks "$RK_POST_HOOK_DIR" $@ 2>&1 | tee -a "$LOG_FILE"
	case "${PIPESTATUS[0]}" in
		0) ;;
		$HOOK_RET_HANDLED) HOOK_HANDLED=1 ;;
		*) exit 1 ;;
	esac
}

main()
{
	[ -z "$DEBUG" ] || set -x

	trap 'err_handler' ERR
	set -eE

	# Save intial envionments
	INITIAL_ENV=$(mktemp -u)
	if [ -z "$RK_BUILDING" ]; then
		env > "$INITIAL_ENV"
	fi

	export LC_ALL=C

	export SCRIPTS_DIR="$(dirname "$(realpath "$BASH_SOURCE")")"
	export COMMON_DIR="$(realpath "$SCRIPTS_DIR/..")"
	export SDK_DIR="$(realpath "$COMMON_DIR/../../..")"
	export CHIP_DIR="$SDK_DIR/device/rockchip/.chip"

	export RK_IMAGE_DIR="$COMMON_DIR/images"
	export RK_CONFIG_IN="$COMMON_DIR/configs/Config.in"

	export RK_BUILD_HOOK_DIR="$COMMON_DIR/build-hooks"
	export BUILD_HELPER="$RK_BUILD_HOOK_DIR/build-helper"
	export RK_POST_HOOK_DIR="$COMMON_DIR/post-hooks"
	export POST_HELPER="$RK_POST_HOOK_DIR/post-helper"
	export HOOK_RET_HANDLED=254

	export PARTITION_HELPER="$SCRIPTS_DIR/partition-helper"

	export RK_OUTDIR="$SDK_DIR/output"
	export RK_LOG_BASE_DIR="$RK_OUTDIR/log"
	export RK_LOG_SESSION="${RK_LOG_SESSION:-$(date +%F_%H-%M-%S)}"
	export RK_LOG_DIR="$RK_LOG_BASE_DIR/$RK_LOG_SESSION"
	export RK_FIRMWARE_DIR="$RK_OUTDIR/firmware"
	export RK_INITIAL_ENV="$RK_OUTDIR/initial.env"
	export RK_CUSTOM_ENV="$RK_OUTDIR/custom.env"
	export RK_FINAL_ENV="$RK_OUTDIR/final.env"
	export RK_CONFIG="$RK_OUTDIR/.config"
	export RK_DEFCONFIG="$RK_OUTDIR/defconfig"

	export RK_BUILDING=1

	mkdir -p "$RK_LOG_DIR"
	rm -rf "$RK_LOG_BASE_DIR/latest"
	ln -rsf "$RK_LOG_DIR" "$RK_LOG_BASE_DIR/latest"

	# Drop old logs
	cd "$RK_LOG_BASE_DIR"
	rm -rf $(ls -t | sed '1,6d')

	mkdir -p "$RK_FIRMWARE_DIR"
	rm -rf "$SDK_DIR/rockdev"
	ln -rsf "$RK_FIRMWARE_DIR" "$SDK_DIR/rockdev"

	cd "$SDK_DIR"
	[ -f README.md ] || ln -rsf "$COMMON_DIR/README.md" .

	OPTIONS="${@:-allsave}"

	if [ "$OPTIONS" = targets ]; then
		echo $(run_build_hooks usage | grep -oE "^[^ \*]*") cleanall
		exit 0
	fi

	for opt in $OPTIONS; do
		case "$opt" in
			help | h | -h | --help | usage | \?) usage ;;
			shell | cleanall)
				# Check single options
				if [ "$opt" = "$OPTIONS" ]; then
					break
				fi

				echo "ERROR: $opt cannot combine with other options!"
				;;
			post-rootfs)
				if [ "$opt" = "$1" -a -d "$2" ]; then
					# Hide other args from build stages
					OPTIONS=$opt
					break
				fi

				echo "ERROR: $opt should be the first option followed by rootfs dir!"
				;;
			*)
				# Make sure that all options are handled
				run_build_hooks option-check $opt
				[ -z "$HOOK_HANDLED" ] || continue

				echo "ERROR: Unhandled option: $opt"
				;;
		esac

		usage
	done

	# Init stage (preparing SDK configs, etc.)
	run_build_hooks init $OPTIONS

	# Force exporting config environments
	set -a

	# Load config environments
	source "$RK_CONFIG"

	# Save initial environment
	if [ -e "$INITIAL_ENV" ]; then
		cat "$INITIAL_ENV" > "$RK_INITIAL_ENV"
		rm -f "$RK_CUSTOM_ENV"

		# Find custom environments
		for cfg in $(grep "^RK_" "$RK_INITIAL_ENV" || true); do
			env | grep -q "^$cfg$" || \
				echo "$cfg" >> "$RK_CUSTOM_ENV"
		done

		# Allow custom environments overriding
		if [ -e "$RK_CUSTOM_ENV" ]; then
			echo -e "\e[31mWARN: Found custom environments: \e[0m"
			cat "$RK_CUSTOM_ENV"

			read -t 10 -p "Press enter to continue."
			source "$RK_CUSTOM_ENV"
		fi
	fi

	set +a

	# RV1126 uses custom toolchain
	if [ "$RK_CHIP_FAMILY" = "rv1126_rv1109" ]; then
		TOOLCHAIN_OS=rockchip
	else
		TOOLCHAIN_OS=none
	fi

	TOOLCHAIN_ARCH=${RK_KERNEL_ARCH/arm64/aarch64}
	TOOLCHAIN_DIR="$(realpath prebuilts/gcc/*/$TOOLCHAIN_ARCH)"
	GCC="$(find "$TOOLCHAIN_DIR" -name "*$TOOLCHAIN_OS*-gcc" | tail -n 1)"
	if [ ! -x "$GCC" ]; then
		echo "No prebuilt GCC toolchain!"
		exit 1
	fi

	export CROSS_COMPILE="${GCC%gcc}"
	echo "Using prebuilt GCC toolchain: $CROSS_COMPILE"

	CPUS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
	export KMAKE="make -C kernel/ ARCH=$RK_KERNEL_ARCH -j$(( $CPUS + 1 ))"

	export PYTHON3=/usr/bin/python3

	# Handle special commands
	case "$OPTIONS" in
		shell)
			echo -e "\e[35mDoing this is dangerous and for developing only.\e[0m"
			/bin/bash --init-file "$PARTITION_HELPER"
			echo -e "\e[35mExit from $BASH_SOURCE shell.\e[0m"
			exit 0 ;;
		cleanall)
			run_build_hooks clean
			rm -rf "$RK_OUTDIR"
			finish_build cleanall
			exit 0 ;;
		post-rootfs)
			shift
			run_post_hooks $@
			finish_build post-rootfs
			exit 0 ;;
	esac

	# Pre-build stage (configs checking and applying, etc.)
	run_build_hooks pre-build $OPTIONS

	# Allow changing kernel version with kernel-* options
	export RK_KERNEL_VERSION=$(kernel_version)

	# Save final environments
	export > "$RK_FINAL_ENV"

	# Build stage (building, etc.)
	run_build_hooks build $OPTIONS

	# Post-build stage (firmware packing, etc.)
	run_build_hooks post-build $OPTIONS
}

if [ "$0" != "$BASH_SOURCE" ]; then
	# Sourced, executing it directly
	"$BASH_SOURCE" ${@:-shell}
elif [ "$0" == "$BASH_SOURCE" ]; then
	# Executed directly
	main $@
fi

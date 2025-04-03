cmd_arch/arm/dts/rk3399-evb.dtb := mkdir -p arch/arm/dts/ ; (cat arch/arm/dts/rk3399-evb.dts; ) > arch/arm/dts/.rk3399-evb.dtb.pre.tmp; /github/OpenHD-RV1126-OS/prebuilts/gcc/linux-x86/arm/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf/bin/arm-rockchip631-linux-gnueabihf-gcc -E -Wp,-MD,arch/arm/dts/.rk3399-evb.dtb.d.pre.tmp -nostdinc -I./arch/arm/dts -I./arch/arm/dts/include -Iinclude -I./include -I./arch/arm/include -include ./include/linux/kconfig.h -D__ASSEMBLY__ -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/dts/.rk3399-evb.dtb.dts.tmp arch/arm/dts/.rk3399-evb.dtb.pre.tmp ; ./scripts/dtc/dtc -O dtb -o arch/arm/dts/rk3399-evb.dtb -b 0 -i arch/arm/dts/  -Wno-unit_address_vs_reg -Wno-simple_bus_reg -Wno-unit_address_format -Wno-pci_bridge -Wno-pci_device_bus_num -Wno-pci_device_reg  -d arch/arm/dts/.rk3399-evb.dtb.d.dtc.tmp arch/arm/dts/.rk3399-evb.dtb.dts.tmp ; cat arch/arm/dts/.rk3399-evb.dtb.d.pre.tmp arch/arm/dts/.rk3399-evb.dtb.d.dtc.tmp > arch/arm/dts/.rk3399-evb.dtb.d

source_arch/arm/dts/rk3399-evb.dtb := arch/arm/dts/.rk3399-evb.dtb.pre.tmp

deps_arch/arm/dts/rk3399-evb.dtb := \
  arch/arm/dts/include/dt-bindings/pwm/pwm.h \
  arch/arm/dts/include/dt-bindings/pinctrl/rockchip.h \
  arch/arm/dts/rk3399.dtsi \
  arch/arm/dts/include/dt-bindings/clock/rk3399-cru.h \
  arch/arm/dts/include/dt-bindings/gpio/gpio.h \
  arch/arm/dts/include/dt-bindings/interrupt-controller/arm-gic.h \
  arch/arm/dts/include/dt-bindings/interrupt-controller/irq.h \
  arch/arm/dts/include/dt-bindings/power/rk3399-power.h \
  arch/arm/dts/include/dt-bindings/thermal/thermal.h \
  arch/arm/dts/rk3399-sdram-lpddr3-4GB-1600.dtsi \
  arch/arm/dts/rk3399-u-boot.dtsi \
  include/linux/media-bus-format.h \
  arch/arm/dts/include/dt-bindings/input/input.h \
  arch/arm/dts/include/dt-bindings/input/linux-event-codes.h \

arch/arm/dts/rk3399-evb.dtb: $(deps_arch/arm/dts/rk3399-evb.dtb)

$(deps_arch/arm/dts/rk3399-evb.dtb):

# Target platforms for u-boot-concept packages
# debian/rules includes this Makefile snippet.

# u-boot-concept-qemu (Architecture: all)

  u-boot-concept-qemu_platforms += qemu-ppce500
  qemu-ppce500_CROSS_COMPILE := powerpc-linux-gnu-
  qemu-ppce500_targets := u-boot.bin uboot.elf

  u-boot-concept-qemu_platforms += qemu-riscv64
  qemu-riscv64_CROSS_COMPILE := riscv64-linux-gnu-
  qemu-riscv64_targets := u-boot.bin uboot.elf

  u-boot-concept-qemu_platforms += qemu-riscv64_smode
  qemu-riscv64_smode_CROSS_COMPILE := riscv64-linux-gnu-
  qemu-riscv64_smode_targets := u-boot.bin uboot.elf

  u-boot-concept-qemu_platforms += qemu-x86
  qemu-x86_CROSS_COMPILE := i686-linux-gnu-
  qemu-x86_targets := u-boot.bin u-boot.rom uboot.elf

  u-boot-concept-qemu_platforms += qemu-x86_64
  qemu-x86_64_CROSS_COMPILE := x86_64-linux-gnu-
  qemu-x86_64_targets := u-boot.bin u-boot.rom uboot.elf

  u-boot-concept-qemu_platforms += qemu-x86_64_nospl
  qemu-x86_64_nospl_CROSS_COMPILE := x86_64-linux-gnu-
  qemu-x86_64_nospl_targets := u-boot.bin u-boot.rom uboot.elf

  u-boot-concept-qemu_platforms += qemu_arm
  qemu_arm_CROSS_COMPILE := arm-linux-gnueabihf-
  qemu_arm_targets := u-boot.bin uboot.elf

  u-boot-concept-qemu_platforms += qemu_arm64
  qemu_arm64_CROSS_COMPILE := aarch64-linux-gnu-
  qemu_arm64_targets := u-boot.bin uboot.elf

# u-boot-concept-efi (Architecture: all)

  u-boot-concept-efi_platforms += efi-x86_app32
  efi-x86_app32_CROSS_COMPILE := i686-linux-gnu-
  efi-x86_app32_targets := u-boot-app.efi

  u-boot-concept-efi_platforms += efi-x86_app64
  efi-x86_app64_CROSS_COMPILE := x86_64-linux-gnu-
  efi-x86_app64_targets := u-boot-app.efi

  u-boot-concept-efi_platforms += efi-arm_app64
  efi-arm_app64_CROSS_COMPILE := aarch64-linux-gnu-
  efi-arm_app64_targets := u-boot-app.efi

  u-boot-concept-efi_platforms += efi-riscv_app64
  efi-riscv_app64_CROSS_COMPILE := riscv64-linux-gnu-
  efi-riscv_app64_targets := u-boot-app.efi

  u-boot-concept-efi_platforms += efi-x86_payload32
  efi-x86_payload32_CROSS_COMPILE := i686-linux-gnu-
  efi-x86_payload32_targets := u-boot-payload.efi

  u-boot-concept-efi_platforms += efi-x86_payload64
  efi-x86_payload64_CROSS_COMPILE := x86_64-linux-gnu-
  efi-x86_payload64_targets := u-boot-payload.efi

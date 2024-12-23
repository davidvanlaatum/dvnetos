if (NOT TEST_MODE)
  find_program(SGDISK_PATH sgdisk)
  if (NOT SGDISK_PATH)
    message(FATAL_ERROR "sgdisk not found, please install gdisk https://sourceforge.net/projects/gptfdisk/")
  endif ()
  find_program(MFORMAT_PATH mformat)
  find_program(MMD_PATH mmd)
  find_program(MCOPY_PATH mcopy)
  if (NOT MFORMAT_PATH OR NOT MMD_PATH OR NOT MCOPY_PATH)
    message(FATAL_ERROR "mtools not found, please install mtools https://www.gnu.org/software/mtools/")
  endif ()
  find_program(QEMU_SYSTEM_PATH qemu-system-${ARCH})
  find_program(QEMU_IMG_PATH qemu-img)

  if (ARCH STREQUAL "aarch64")
    set(RPI_VERSION 4 CACHE STRING "Raspberry Pi version")
    if (RPI_VERSION STREQUAL "4")
      set(RPI_UEFI_URL "https://github.com/pftf/RPi4/releases/download/v1.38/RPi4_UEFI_Firmware_v1.38.zip")
    elseif (RPI_VERSION STREQUAL "3")
      set(RPI_UEFI_URL "https://github.com/pftf/RPi3/releases/download/v1.39/RPi3_UEFI_Firmware_v1.39.zip")
    else ()
      message(FATAL_ERROR "Unsupported Raspberry Pi version: ${RPI_VERSION}")
    endif ()
    ExternalProject_Add(
        rpi-uefi
        URL ${RPI_UEFI_URL}
        BUILD_COMMAND ${CMAKE_COMMAND} -E rm Readme.md
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        USES_TERMINAL_DOWNLOAD TRUE
        EXCLUDE_FROM_ALL TRUE
    )

    ExternalProject_Get_Property(rpi-uefi SOURCE_DIR)
    set(FIRMWARE ${SOURCE_DIR})
  endif ()

  add_custom_command(
      OUTPUT image.hdd
      COMMAND rm -f image.hdd
      COMMAND dd if=/dev/zero of=image.hdd bs=1M count=0 seek=64
      COMMAND ${SGDISK_PATH} image.hdd -n 1:2048 -t 1:ef00
      DEPENDS kernel limine
      COMMENT "Creating image.hdd"
  )

  if (ARCH STREQUAL "x86_64")
    add_custom_command(
        OUTPUT image.hdd
        COMMAND ${LIMINE_PATH}/bin/limine bios-install image.hdd
        APPEND
    )
  endif ()

  add_custom_command(
      OUTPUT image.hdd
      COMMAND ${MFORMAT_PATH} -i image.hdd@@1M
      COMMAND ${MMD_PATH} -i image.hdd@@1M ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
      COMMAND ${MCOPY_PATH} -i image.hdd@@1M $<TARGET_FILE:kernel> ::/boot
      COMMAND ${MCOPY_PATH} -i image.hdd@@1M ${CMAKE_CURRENT_SOURCE_DIR}/limine.conf ::/boot/limine
      APPEND
  )

  if (ARCH STREQUAL "x86_64")
    add_custom_command(
        OUTPUT image.hdd
        COMMAND ${MCOPY_PATH} -i image.hdd@@1M ${LIMINE_PATH}/share/limine/limine-bios.sys ::/boot/limine
        COMMAND ${MCOPY_PATH} -i image.hdd@@1M ${LIMINE_PATH}/share/limine/BOOTX64.EFI ::/EFI/BOOT
        COMMAND ${MCOPY_PATH} -i image.hdd@@1M ${LIMINE_PATH}/share/limine/BOOTIA32.EFI ::/EFI/BOOT
        APPEND
    )
  elseif (ARCH STREQUAL "aarch64")
    add_custom_command(
        OUTPUT image.hdd
        COMMAND ${MCOPY_PATH} -i image.hdd@@1M ${LIMINE_PATH}/share/limine/BOOTAA64.EFI ::/EFI/BOOT
        DEPENDS rpi-uefi
        APPEND
    )
    add_custom_command(
        OUTPUT image.hdd
        COMMAND ${MMD_PATH} -i image.hdd@@1M ::/firmware ::/firmware/brcm ::/overlays
        COMMAND ${MCOPY_PATH} -i image.hdd@@1M -sv ${FIRMWARE}/* ::
        APPEND
    )
  endif ()
  add_custom_target(image DEPENDS image.hdd)

  if (QEMU_IMG_PATH)
    add_custom_command(
        OUTPUT image.vmdk
        COMMAND ${QEMU_IMG_PATH} convert -f raw -O vmdk image.hdd image.vmdk
        DEPENDS image.hdd
        COMMENT "Creating image.vmdk"
    )
    add_custom_target(vmdk DEPENDS image.vmdk)
  endif ()

  if (QEMU_SYSTEM_PATH)
    ExternalProject_Add(
        ovmf
        URL https://github.com/osdev0/edk2-ovmf-nightly/releases/latest/download/ovmf-code-${ARCH}.fd
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        EXCLUDE_FROM_ALL TRUE
        USES_TERMINAL_DOWNLOAD TRUE
        DOWNLOAD_NO_EXTRACT TRUE
        DOWNLOAD_NAME ovmf-code-${ARCH}.fd
        STEP_TARGETS download
    )
    ExternalProject_Get_Property(ovmf DOWNLOAD_DIR)
    ExternalProject_Get_Property(ovmf DOWNLOAD_NAME)
    set(OVMF_DOWNLOAD_FILE "${DOWNLOAD_DIR}/${DOWNLOAD_NAME}")

    set(QEMU_ARGS)# -d int,mmu,cpu_reset,unimp,guest_errors,page)
    set(QEMU_ARGS ${QEMU_ARGS} -drive file=image.hdd,format=raw)
    if (ARCH STREQUAL "x86_64")
      set(QEMU_ARGS ${QEMU_ARGS} -smp 2)
    elseif (ARCH STREQUAL "aarch64")
      set(QEMU_ARGS ${QEMU_ARGS} -machine virt -cpu cortex-a72 -device ramfb -device qemu-xhci -device usb-kbd -device usb-mouse)
      ExternalProject_Add_Step(ovmf fix-size COMMAND dd if=/dev/zero of=${OVMF_DOWNLOAD_FILE} bs=1 count=0 seek=67108864)
    endif ()

    set(QEMU_OVMF_ARGS -drive if=pflash,unit=0,format=raw,file=${OVMF_DOWNLOAD_FILE},readonly=on)

    add_custom_target(run-qemu
        COMMAND ${QEMU_SYSTEM_PATH} ${QEMU_OVMF_ARGS} ${QEMU_ARGS} $$QEMU_EXTRA_ARGS
        USES_TERMINAL
        DEPENDS image.hdd ovmf
    )
    add_test(NAME kernel-boots COMMAND ${PYTHON_VENV} ${CMAKE_CURRENT_SOURCE_DIR}/kernel-boots.py ${QEMU_SYSTEM_PATH} ${QEMU_OVMF_ARGS} ${QEMU_ARGS})
    set_tests_properties(kernel-boots PROPERTIES TIMEOUT 120 ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR};ARTIFACTS_DIR=${CMAKE_BINARY_DIR}/Testing/Temporary")

    string(REPLACE ";" " " QEMU_OVMF_ARGS "${QEMU_OVMF_ARGS}")
    string(REPLACE ";" " " QEMU_ARGS "${QEMU_ARGS}")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/qemu.sh "#!/bin/sh\nset -xe\ncd ${CMAKE_CURRENT_BINARY_DIR}\nexec ${QEMU_SYSTEM_PATH} ${QEMU_OVMF_ARGS} -serial stdio ${QEMU_ARGS} \$QEMU_EXTRA_ARGS \"$@\"\n")
    file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/qemu.sh PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  endif ()
endif ()

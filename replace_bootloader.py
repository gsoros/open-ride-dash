import os
import sys
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

# Define the relative path to your custom rollback bootloader
CUSTOM_BOOTLOADER_PATH = os.path.join("bootloader", "bootloader_c3_rollback_5.5.3.bin")

if not os.path.isfile(CUSTOM_BOOTLOADER_PATH):
    sys.stderr.write(
        f"\n[ERROR] Custom bootloader not found at: {CUSTOM_BOOTLOADER_PATH}\n\n"
    )
    env.Exit(1)

abs_path = os.path.abspath(CUSTOM_BOOTLOADER_PATH)


def patch_bootloader_images(target, source, env):
    """
    Callback function that catches FLASH_EXTRA_IMAGES after PlatformIO
    populates it, then swaps the default bootloader path for our custom one.
    """
    new_extra_images = []
    found_bootloader = False

    # Safely extract images array
    extra_images = env.get("FLASH_EXTRA_IMAGES", [])

    for address, image_path in extra_images:
        # ESP32-C3 bootloader mapping address is typically 0x0000 or 0x0
        if "bootloader" in image_path or address in ["0x0000", "0x00", 0x0, 0]:
            new_extra_images.append((address, abs_path))
            found_bootloader = True
        else:
            new_extra_images.append((address, image_path))

    if found_bootloader:
        env.Replace(FLASH_EXTRA_IMAGES=new_extra_images)
        print(
            f"\n[SUCCESS] Hook active: Swapped PlatformIO bootloader with: {abs_path}\n"
        )
    else:
        sys.stderr.write(
            "\n[WARNING] Could not locate bootloader inside FLASH_EXTRA_IMAGES loop.\n\n"
        )


# Attach the callback hook to the firmware image target generation sequence
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", patch_bootloader_images)

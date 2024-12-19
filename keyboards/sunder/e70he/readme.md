# Sunder E70 HE

![Sunder E70 HE](https://i.imgur.com/5FkyZsX.jpeg)

A hall effect split keyboard that is compatible with Gateron KS-20T (Magnetic Jade) hall effect switch.

* Keyboard Maintainer: [squishyliquid](https://github.com/squishyliquid)
* Hardware Supported: Sunder E70 HE
* Hardware Availability: [Sunder](https://sunderkeyboards.com/)

Make example for this keyboard (after setting up your build environment):

    make sunder/e70he:vial

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Hold down `ESC`+`DEL` and plug in left half to reset. Hold down `Backspace`+`Backslash` and plug in right half to reset.
* **Physical reset button**: Hold down the button on the back of the PCB and plug in
* **Keycode in layout**: Press the key mapped to `QK_BOOT` if available



# winmgm

`winmgm` is a window management tool inspired by `wmctrl` and tiling window managers. Unlike tiling window managers, `winmgm` allows you to manage application windows without relying on a full tiling manager. It's designed to provide flexibility for arranging and managing windows in a non-intrusive way, without imposing a tiling window management style.

This project is mainly targeted for users who prefer to manage windows in a more manual, customizable fashion while still benefiting from automation in tasks such as window resizing, arrangement, and workspace management.


### Note:  
Currently, `winmgm` **only supports Linux with X11**. It won’t work on Wayland or other windowing systems, and it’s not available on other operating systems (like macOS or Windows) at this time.

---


## Installation

### Dependencies

Make sure you have the following installed:
- X11 development libraries (`libx11-dev`)
- X11 utilities like `xprop`
- Other standard libraries for C development (e.g., `gcc`, `cmake`)

### Clone the Repository

```bash
git clone https://github.com/zyxidra/winmgm.git
cd winmgm
```

### Build the Project

If you're using `make`, run the following commands to build `winmgm`:

```bash
make
```

Alternatively, if you're using `CMake`, create a build directory and run the following commands:

```bash
mkdir build
cd build
cmake ..
make
```

---

## Usage

Once compiled, you can run the `winmgm` executable to manage your windows. For example:

```bash
./winmgm
```

## Development

This project is open-source, and contributions are welcome. If you'd like to contribute, please fork the repository, create a branch, and submit a pull request with your changes.

For further development, you may also want to modify the configuration settings based on your preferred window manager (e.g., X11, Wayland).

---

## License

`winmgm` is licensed under the [MIT License](LICENSE).

---

## Acknowledgements

- Inspired by `wmctrl` and tiling window managers.
- Thanks to the X11 community for providing useful tools like `xprop` and `xwininfo`.

---

# DSi Mini Browser — build-ready source

This ZIP is the **source project**, not a precompiled `.nds`.

## What it does

- Nintendo DS/DSi homebrew application
- Uses devkitPro `libnds`
- Uses devkitPro `dswifi`
- Connects using the DSi/DS Wi-Fi firmware settings
- Performs lightweight HTTP GET requests
- Converts basic HTML into readable text
- Uses the touchscreen keyboard for URL entry
- A = load/reload
- B = return to URL entry
- START = quit

## Important limitation

This is deliberately a small first browser. It supports **HTTP**, not HTTPS, and does not implement JavaScript, modern CSS, images, cookies, TLS, or complex HTML.

That is important on a DSi: a modern web browser engine is far larger than a small homebrew application.

## Building

The project follows the current devkitPro DS build structure and expects:

- devkitARM
- libnds
- dswifi

The normal build command is:

    make

The output should be:

    dsi-browser.nds

## Online compiler

If you are using an online compiler, upload this ZIP as a devkitPro Nintendo DS project.

The compiler must provide the `devkitARM`, `libnds`, and `dswifi` packages. A generic ZIP-to-NDS converter will not work because `.nds` is a compiled Nintendo DS ROM container.

## Source references

Project structure follows the current devkitPro DS examples and `ds_rules`.
dswifi provides the BSD-style networking stack used here.

- https://github.com/devkitPro/nds-examples
- https://github.com/devkitPro/dswifi
- https://github.com/devkitPro/libnds

## License

The original application code in this project is provided for personal/homebrew use. The project links against devkitPro libraries; those libraries retain their own licenses.


## Build online with GitHub Actions

This repository includes `.github/workflows/build.yml`.

After uploading the project to GitHub:

1. Open the repository.
2. Open **Actions**.
3. Select **Build DSi Browser**.
4. Click **Run workflow**.
5. Wait for the green check mark.
6. Open the completed workflow run.
7. Under **Artifacts**, download `dsi-browser-nds`.
8. Inside the downloaded artifact is `dsi-browser.nds`.

The workflow uses the official devkitPro `devkitarm` container and installs the Nintendo DS development metapackage before running `make`.

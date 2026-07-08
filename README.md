# Knuckle

Desktop app wrapper around [CEF](https://bitbucket.org/chromiumembedded/cef/). Runs any URL in a kiosk-like window with script injection, proxy support, and persistent profiles.

## Usage

```
Knuckle --url="https://example.com"
```

## Flags

| Flag | Description |
|------|-------------|
| `--url=<url>` | Navigate to a URL |
| `--proxy=<proxy>` | Set proxy server (`HTTP`/`SOCKS4`/`SOCKS5`) |
| `--profile=<name>` | Enable persistent cache at `profiles/<name>/` |
| `--script=<files>` | Inject JS files on every page load (comma-separated) |
| `--script-dir=<dir>` | Load all `.js` files from a directory as scripts |

## Keys

| Key | Action |
|-----|--------|
| F5  | Reload the page |
| F9  | Toggle script panel (dock right) |

The script panel shows all `--script` and `--script-dir` files. Uncheck a script to disable it on subsequent page loads.

## Build

Built automatically via GitHub Actions for Windows, Linux, and macOS. See [releases](https://github.com/DevilTekGame/Knuckle/releases) for prebuilt archives.

To build manually:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DUSE_SANDBOX=OFF
cmake --build build --config Release
```

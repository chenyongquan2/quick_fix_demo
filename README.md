# quick_fix_demo

Echo FIX 4.4 server (acceptor) and client (initiator) using QuickFIX C++, built with xmake and dependencies managed via Conan.

## Prerequisites

- Git Bash or PowerShell on Windows 10+
- Python 3.8+
- Conan 2.x (`pip install conan`)
- xmake (`choco install xmake` or see `https://xmake.io`)

Initialize Conan client (first time only):

```bash
conan profile detect --force
```

## Build

```bash
# from repo root
xmake f -c -y
xmake -y
```

This builds two binaries:
- `echo_acceptor`
- `echo_initiator`

## Run

Open two terminals.

Terminal A (server):

```bash
./build/windows/x64/release/echo_acceptor.exe config/acceptor.cfg
```

Terminal B (client):

```bash
./build/windows/x64/release/echo_initiator.exe config/initiator.cfg
```

By default, the server listens on port 5001. The client connects to `127.0.0.1:5001`. Logs and stores are under `log/` and `store/`.

## FIX 4.4 dictionary and Nelogica notes

For strict validation compatible with FIX 4.4 (20030618 errata), set in `config/*.cfg`:

```ini
UseDataDictionary=Y
DataDictionary=spec/FIX44.xml
```

Provide a suitable `spec/FIX44.xml` (QuickFIX ships a default). For Nelogica gateway specifics (subset, custom tags), extend the dictionary or disable strict checks during early development.

## Project structure

```
config/            # session configs for acceptor & initiator
src/               # C++ sources
log/, store/       # runtime files
spec/              # place FIX44.xml here if using dictionary
xmake.lua          # build file
```

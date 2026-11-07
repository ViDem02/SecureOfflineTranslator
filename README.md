# Secure Offline Translator

Small local-only prototype for technical translation. It has no cloud SDKs and
no pip dependencies: the application uses only Python's standard library.

The application does not implement a translation model itself. It sends
requests to an already installed local Ollama instance running the approved
model. Ollama must therefore be installed and prepared separately.

## Option 1: run directly with Python

Install and load the model during the preparation phase:

```sh
ollama pull translategemma:12b
ollama serve
```

Start the application:

```sh
python3 app.py
```

Open <http://127.0.0.1:8080>.

The app supports Italian, English, and French, manual text, TXT/Markdown
imports, local export, a session glossary, and explicit session clearing.
Inputs are size-limited, technical tokens are protected, and request logs do
not include content. Translation responses are streamed from the local model,
so the UI displays received characters while the translation is in progress.
`Shift+Enter` starts a translation.

Before every translation, the server pings `8.8.8.8`. If it responds, the
request is rejected with `EXTERNAL CONNECTIONS ALLOWED` and no model request
is made.

## Option 2: run with Docker

The Docker setup exposes the browser interface only on the host loopback:

```text
host 127.0.0.1:1010 -> container 8080 -> Ollama on the host
```

Open <http://127.0.0.1:1010>.

### Preparation phase (Internet allowed)

Run these commands before moving the system to an offline environment:

```sh
/Applications/Ollama.app/Contents/MacOS/Ollama
docker pull python:3.12-slim
docker compose build
docker save offline-translator:latest > offline-translator.tar
```

The image build installs `iputils-ping` during this preparation phase. It is
stored inside the image and is not downloaded when the container starts.

Transfer `offline-translator.tar` and the Ollama model using the organization's
approved, integrity-checked procedure.

### Offline execution

```sh
docker load < offline-translator.tar
ollama serve
docker compose up --no-build
```

`docker compose up --no-build` is intentional: it prevents Docker from trying
to build or download anything at runtime.

At every container start, `start.sh` pings `8.8.8.8` before starting Python. If
the address responds, startup stops and prints:

```text
EXTERNAL CONNECTIONS ALLOWED
```

With the `internal: true` Compose network, the expected result is that the
ping fails and the application starts. This check is a useful fail-closed
signal, but it is not a complete proof of network isolation: it checks one
destination and ICMP may be filtered independently of other traffic.

## Network isolation

The application refuses arbitrary model endpoints. It allows only loopback
addresses or Docker Desktop's explicit `host.docker.internal` host gateway,
which is required to reach Ollama on the Mac. The container uses:

- read-only filesystem;
- dropped Linux capabilities;
- `no-new-privileges`;
- no runtime package installation.

The startup script tests `8.8.8.8` and terminates if it is reachable. Because
Docker Desktop cannot reliably publish a host port from an `internal: true`
network on every macOS configuration, the effective outbound block must be
enforced by LuLu/macOS firewall. Block Docker Desktop's backend and Ollama
from external destinations, while allowing the local Ollama API. Validate the
effective network behavior with an independent packet monitor before
processing confidential data. Confirm that Ollama itself has no cloud,
telemetry, automatic model download, or remote integration enabled.

## Development checks

```sh
python3 -m unittest -v
```

This prototype is not a security certification and must not be used with
confidential data until the requirements in `doc_requisiti.md` have been
validated by the responsible organization.

## Verify Ollama isolation

Perform these checks after blocking Ollama in LuLu.

First verify that the local API still works:

```sh
ollama list
curl -fsS http://127.0.0.1:11434/api/tags
```

These commands must succeed. They use the local Ollama API and do not test
Internet access.

Then test an uncached model name:

```sh
ollama run translategemma-isolation-test:does-not-exist
```

The command must fail without downloading anything. LuLu should show and block
the attempted external connection. Do not approve the request. Stop it with
`Ctrl-C` if it remains pending.

For independent observation, monitor Ollama's sockets while repeating the
test:

```sh
sudo lsof -nP -a -c Ollama -i
```

Only the local listener/API connection should remain. To inspect packets
directly, use LuLu's network events or a packet capture approved by your
organization. A failed `ping` alone is not sufficient evidence because ICMP
can be blocked while HTTP/HTTPS remains available.

# Secure Translator

Native macOS application for offline technical translation.

## Current stack

- C++20
- Qt 6 Widgets
- CMake + Ninja
- `llama.cpp` vendored under `third_party/llama.cpp`
- Apple Silicon / Metal target
- no HTTP server, Docker, Ollama, telemetry, or runtime downloads

## Recreating the repository

The repository intentionally does not contain the large GGUF model. The
directory structure is preserved by `model/.gitkeep`; copy exactly one local
`.gguf` file there before configuring CMake:

```sh
cp /path/to/translategemma-4b-it.Q4_K_M.gguf model/
```

`llama.cpp` is pinned to commit
`9e99f65173e88dcdaab24011ab820c131ec4abfe`. If `third_party/llama.cpp` is
not present after cloning the repository, recreate it with:

```sh
sh scripts/fetch-llama.sh
```

The script downloads only the pinned build dependency into
`third_party/llama.cpp`; the application itself performs no downloads or
network access. If the vendored directory is already present, do not run the
script again: it refuses to overwrite existing files.

Install the build tools and Qt locally, then configure and build:

```sh
brew install cmake ninja qtbase
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
open build/SecureTranslator.app
```

## VS Code development

Install the recommended extensions:

- CMake Tools
- C/C++ (Microsoft)

The project is configured for Homebrew Qt under `/opt/homebrew`. Configure and
build from the repository root:

```sh
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
```

The application bundle is:

```text
build/SecureTranslator.app
```

The build compiles vendored `llama.cpp` statically with Metal enabled. It does
not use the Homebrew `libllama` at runtime.

The macOS build also runs `macdeployqt`, copying the required Qt frameworks and
platform plugins (including `cocoa`) into the application bundle. The resulting
`.app` can therefore be launched without setting `QT_PLUGIN_PATH` or relying on
the system Qt installation.

On macOS development builds are ad-hoc signed with
`entitlements/SecureTranslator.entitlements`. Verify the result with:

```sh
codesign --verify --deep --strict build/SecureTranslator.app
codesign -d --entitlements :- build/SecureTranslator.app
```

Ad-hoc signing is not notarization. A release build requires an organization
Developer ID certificate and a separate notarization procedure.

For a signed release, pass the organization certificate identity:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DSECURE_TRANSLATOR_SIGNING_IDENTITY="Developer ID Application: Organization"
```

In VS Code, use `Cmd+Shift+B` to run the default build task or select
**Debug SecureTranslator** from the Run and Debug panel.

## Model integration status

The application skeleton, Qt GUI, file access, SHA-256 verifier, and worker
thread boundary are present. The local `llama.cpp` model uses full Metal
offload (`n_gpu_layers = -1`) for Apple Silicon. The context is configured up
to 8192 tokens, subject to the model's training limit. Prompt and
generated-token batches use explicit positions, sequence IDs, and logits.

Place exactly one GGUF model in the project `model/` folder:

```sh
cp /path/to/translategemma-4b-it.Q4_K_M.gguf model/
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
```

The build requires exactly one `.gguf` file in `model/`. That file is copied
into `SecureTranslator.app/Contents/Resources` and loaded from inside the
signed application bundle. To use another model, replace the file in `model/`
and rebuild the app. The application does not accept a second model path at
runtime.

The interface also provides a `max end_of_turn` setting from 1 to 8. The
decoder stops after reaching that many end-of-turn markers; the default is 1.

Enable **Explain** to ask Gemma for a separate explanation. The explanation is
displayed in its own panel and is not included in the saved translation.

Long input is translated incrementally. The worker splits it into bounded
chunks, preferring paragraph, line, sentence, and word boundaries, then
translates and recombines the chunks in order. This avoids exhausting the
model context while supporting arbitrarily large finite text inputs.

At startup the application verifies the bundled model hash and loads the GGUF
once on the worker thread. Translations reuse that loaded model; verification
and loading are not repeated for each translation.

An optional glossary can be placed in `glossary/`. The folder may contain at
most one `.yaml` file, which is loaded once at startup and bundled with the
application:

```yaml
terms:
  - source: "torque"
    target: "coppia"
  - source: "firmware"
    target: "firmware"
```

Replace the glossary file and rebuild to change it. The glossary is supplied
to the local model as translation instructions and is not sent anywhere.
No model is downloaded automatically, and the source GGUF is not modified.

The approved manifest is also embedded in the application bundle. If
`SECURE_TRANSLATOR_MANIFEST` is not set, the application uses that embedded
copy, which works correctly under App Sandbox.

The manifest must contain the exact file name and a SHA-256 obtained from an
independent, trusted source:

```json
{
  "models": [
    {
      "file": "translategemma-4b-it.Q4_K_M.gguf",
      "sha256": "trusted-lowercase-sha256"
    }
  ]
}
```

`models/approved-models.example.json` is only a schema example. Its placeholder
hash is intentionally rejected. The application rejects missing files,
unknown file names, and every hash that is not present in the manifest.
Changing from the 4B model to the 12B model requires only the configured path
and a separately approved manifest entry.

The current debug manifest authorizes the configured TranslateGemma 4B file
with SHA-256
`81200d03e843d2ec1ece6eeafe7d13cb6e5211e1fcd336ade55790b683a08330`.
The hash was verified locally before loading. The model file itself is not
stored in this repository.

The interface displays the host's currently available memory and refreshes it
every two seconds. This is a local diagnostic only: CPU and GPU saturation are
not yet reported, and no resource data leaves the application.

Text files are read and written as UTF-8. Prompt and generated text are
decoded cumulatively so multibyte characters, accented characters, CJK text,
and emoji are not split between model tokens. The model's own tokenizer still
determines translation quality for each language.

This project is not a security certification. The signed App Sandbox bundle,
entitlements, model provenance, and negative security tests must be validated
before processing confidential data.

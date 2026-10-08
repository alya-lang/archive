# archive

[![CI](https://github.com/alya-lang/archive/actions/workflows/ci.yml/badge.svg)](https://github.com/alya-lang/archive/actions/workflows/ci.yml)
[![License](https://img.shields.io/github/license/alya-lang/archive?color=blue&label=License)](LICENSE)
[![Alya](https://img.shields.io/badge/dynamic/toml?url=https%3A%2F%2Fraw.githubusercontent.com%2Falya-lang%2Farchive%2Fmain%2Falya.toml&query=%24.package.alya-version&label=Alya&color=orange&prefix=%3E%3D)](https://github.com/alya-lang/alya)
[![Package Version](https://img.shields.io/badge/dynamic/toml?url=https%3A%2F%2Fraw.githubusercontent.com%2Falya-lang%2Farchive%2Fmain%2Falya.toml&query=%24.package.version&label=Version&color=brightgreen)](alya.toml)

Multi-format archive toolkit for Alya: TAR/USTAR, ZIP stored/deflate and TAR.GZ packing with CRC integrity, powered by compress codecs

---

## 🌟 Features

- 📦 **Three Container Formats**: POSIX ustar `tar`, ZIP with central directory plus EOCD, and gzip-compressed `tar.gz`/`tgz` tarballs through one unified facade
- 🧱 **Pure-Alya Engines**: TAR and stored-ZIP builders/parsers written in zero-dependency Alya on top of shared little-endian and octal codecs
- 🗜️ **Compress-Powered Codecs**: ZIP deflate plus tarball gzip reuse the battle-tested `compress` package (pinned dependency); decompression targets exact declared sizes (gzip ISIZE, ZIP central directory), so payloads of any size unpack without output-growth limits
- 🛡️ **Binary-Safe By Design**: Payloads are byte arrays end to end; a tiny bundled C bridge (`c/archive.c`) persists images without NUL truncation, and every ZIP payload is CRC-32 verified on read
- 🔍 **Magic-Based Detection**: `detect_format` recognizes ZIP, gzip, 7z, RAR, and ustar signatures, so `unpack` auto-dispatches with `-1`
- 🚫 **Honest Limits**: 7z/RAR report descriptive unsupported-format errors instead of silent corruption; v0.1.0 limits (100-byte tar names, no ZIP64, no encryption) are documented, not hidden
- 🧪 **Enterprise Test & Benchmark Suite**: Round-trip coverage for text plus NUL-bearing binary payloads, file persistence, and micro-benchmarks (`std/test`)

---

## 📁 Project Architecture

```
archive/
├── .alyalint               # Linter configuration (rules, exclusions, severity overrides)
├── .editorconfig           # Uniform formatting rules across IDEs and editors
├── .gitignore              # Ecosystem standard ignore filters
├── .vscode/                # VS Code workspace settings, DAP launch configurations & tasks
├── alya.toml               # Package manifest with dependencies, [features] and [build]
├── c/                      # Bundled native engine: binary file bridge + CRC-32
│   ├── archive.c           # fread/fwrite with explicit lengths, IEEE CRC-32
│   └── archive.h           # Native function declarations
├── src/
│   ├── lib.alya            # Public API facade (detection, pack/unpack dispatch, file helpers)
│   ├── types.alya          # ArchiveFormat/ZipMethod enums, ArchiveEntry model, constructors
│   ├── ffi.alya            # Native extern "C" declarations
│   └── core/               # Subdirectory module hierarchy
│       ├── bytes.alya      # LE codecs, octal fields, byte/string conversion, buffers
│       ├── file.alya       # Binary-safe save/load bridge plus archive_crc32
│       ├── tar.alya        # USTAR builder and parser
│       ├── zip.alya        # ZIP builder and parser (stored always, deflate gated)
│       └── codecs.alya     # TAR.GZ packing (gzip via compress, exact-size unpack)
├── examples/
│   └── demo.alya           # Runnable walkthrough of all package capabilities
├── tests/
│   └── test_basic.alya     # Automated test suite with round-trip coverage
└── benches/
    └── bench_basic.alya    # Micro-benchmarks measuring pack/parse throughput
```

> [!NOTE]
> **Visibility & Modularity:** Symbols annotated with `pub` (`pub function`, `pub struct`, `pub enum`) are exported to external consumers and re-exporting modules. Symbols without `pub` remain strictly internal to their declaring module, preventing symbol collisions and implementation leakage.

---

## 📦 Installation

Add `archive` to the `[dependencies]` section in your `alya.toml`:

```toml
[dependencies]
archive = { git = "https://github.com/alya-lang/archive", branch = "main" }
```

Or install it directly using the Alya package CLI:

```bash
alya add archive --git https://github.com/alya-lang/archive --branch main
alya install
```

### Dependencies

`archive` builds on the [`compress`](https://github.com/alya-lang/compress) package (pinned `v0.1.0`) for DEFLATE/gzip codecs, plus a small bundled C engine (`c/archive.c`) for binary-safe file persistence, CRC-32, and exact-size decompression.

---

## 🚀 Quick Start

### 1. TAR Round-Trip

```alya
import "archive" as pkg

function main()
    let entries = [
        pkg::make_text_entry("hello.txt", "Hello, Alya!"),
        pkg::make_dir_entry("assets")
    ]

    let image = pkg::pack(entries, pkg::ArchiveFormat.Tar)
    say "Detected: " + pkg::format_name(pkg::detect_format(image))

    let back = pkg::unpack(image)
    for e in back
        say "Entry: " + e.name + " (" + str(pkg::entry_size(e)) + " bytes)"
    end
end

main()
```

### 2. ZIP With File Persistence

```alya
import "archive" as pkg

function main()
    let entries = [
        pkg::make_text_entry("hello.txt", "Hello, Alya!"),
        pkg::make_text_entry("config/app.json", "{\"status\": \"ok\"}")
    ]

    pkg::pack_file("./demo.zip", entries, pkg::ArchiveFormat.Zip)

    let names = pkg::list_names(pkg::load_archive("./demo.zip"))
    for n in names
        say "Archived: " + n
    end

    let first = pkg::find_entry(pkg::unpack_file("./demo.zip"), "hello.txt")
    say "Extracted: " + pkg::entry_text(first)
    remove_file("./demo.zip")
end

main()
```

### 3. TAR.GZ Tarball

```alya
import "archive" as pkg

function main()
    let entries = [pkg::make_text_entry("hello.txt", "Hello, Alya!")]

    let tgz = pkg::targz_pack(entries)
    say "Detected: " + pkg::format_name(pkg::detect_format(tgz))

    let back = pkg::targz_unpack(tgz)
    say "Entries: " + str(len(back))
end

main()
```

---

## 📖 API Reference

### Facade (`archive`)

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `detect_format(data)` | `data: list<int>` | `int` | Detects the container format from magic bytes (`ArchiveFormat`). |
| `pack(entries, format)` | `entries: list<ArchiveEntry>, format: int` | `list<int>` | Packs entries into the requested container (default `Zip`). |
| `unpack(data, format)` | `data: list<int>, format: int = -1` | `list<ArchiveEntry>` | Unpacks an image, auto-detecting with `-1`. |
| `list_names(data, format)` | `data: list<int>, format: int = -1` | `list<string>` | Lists entry names in archive order. |
| `make_text_entry(name, text, mode, mtime)` | `name: string, text: string, mode: int, mtime: int` | `ArchiveEntry` | Builds a file entry from UTF-8 text. |
| `entry_text(entry)` | `entry: ArchiveEntry` | `string` | Decodes a NUL-free payload back to text. |
| `find_entry(entries, name)` | `entries: list<ArchiveEntry>, name: string` | `ArchiveEntry | null` | Finds the first entry with the given name. |
| `save_archive(path, data)` | `path: string, data: list<int>` | `int` | Saves an image without text mangling. |
| `load_archive(path)` | `path: string` | `list<int>` | Loads an image preserving every byte. |
| `pack_file(path, entries, format)` | `path: string, entries, format: int` | `int` | Packs entries directly to disk. |
| `unpack_file(path, format)` | `path: string, format: int = -1` | `list<ArchiveEntry>` | Loads and unpacks an archive file. |

### Engines

| Function | Arguments | Returns | Description |
|---|---|---|---|
| `tar_build(entries)` | `entries: list<ArchiveEntry>` | `list<int>` | Builds a ustar image with two-block trailer. |
| `tar_parse(data)` | `data: list<int>` | `list<ArchiveEntry>` | Parses a ustar image with checksum verification. |
| `tar_names(data)` | `data: list<int>` | `list<string>` | Lists ustar entry names. |
| `zip_build(entries, method)` | `entries, method: int = 0` | `list<int>` | Builds a ZIP image (0 stored, 8 deflated). |
| `zip_parse(data)` | `data: list<int>` | `list<ArchiveEntry>` | Parses a ZIP image with CRC-32 verification. |
| `zip_names(data)` | `data: list<int>` | `list<string>` | Lists ZIP entry names via central directory. |
| `targz_pack(entries)` | `entries: list<ArchiveEntry>` | `list<int>` | Packs a gzip tarball. |
| `targz_unpack(data)` | `data: list<int>` | `list<ArchiveEntry>` | Unpacks a gzip tarball (exact-size, single-member). |
| `archive_crc32(data)` | `data: list<int>` | `int` | IEEE CRC-32 checksum of a byte array. |
| `inflate_exact(payload, expected_len)` | `payload: list<int>, expected_len: int` | `list<int>` | Inflates a raw DEFLATE payload into its exact size. |
| `gunzip_exact(data)` | `data: list<int>` | `list<int>` | Decompresses single-member gzip into its ISIZE size. |

### Models

- **`ArchiveFormat`**: `Unknown (0)`, `Tar (1)`, `Zip (2)`, `TarGz (3)`, `SevenZ (4)`, `Rar (5)`
- **`ZipMethod`**: `Stored (0)`, `Deflated (8)`
- **`ArchiveEntry`**: `{ name: string, data: array, mode: int, mtime: int, is_dir: int }` plus `make_entry`, `make_dir_entry`, `entry_size`, `entry_is_dir`, `format_name`

---

## 🧪 Running Tests & Benchmarks

Run the automated test suite using `alya test`:

```bash
alya test
```

Generate static API documentation:

```bash
alya doc . -o docs --markdown
```

Run the benchmark suite:

```bash
alya run benches/bench_basic.alya
```

Run the demonstration:

```bash
alya run examples/demo.alya
```

Check code formatting:

```bash
alya fmt . --check
```

Run static code linter:

```bash
alya lint . --check
```

---

## 🤝 Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository and clone it locally
2. Install dependencies:
   ```bash
   alya install
   ```
3. Create your feature branch (`git checkout -b feature/my-feature`)
4. Verify tests and formatting before opening a PR:
   ```bash
   alya test
   ```
5. Commit your changes (`git commit -m "feat: add feature"`) and open a Pull Request

---

## 📄 License

MIT License. See [LICENSE](LICENSE) for details. ZIP and TAR are open container formats; RAR is proprietary and intentionally unsupported.

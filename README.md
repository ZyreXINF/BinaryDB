# BinaryDB

### A lightweight binary database engine written in C

BinaryDB is a small, file-based database engine built from scratch in **C23**.

It is designed around a simple idea: keep persistent data storage compact, understandable, and under your control — without relying on an external database server or heavyweight dependencies.

BinaryDB currently provides an interactive CLI for creating and managing databases, inserting and modifying records, querying by ID, soft-deleting records, listing databases, automatic storage optimization, and filesystem-based logging.

> **Status:** Early development / experimental
> **Version:** `1.0`

---

## ✨ Features

* 🗃️ **File-based databases** — each database is stored as a `.khdb` file
* 💻 **Interactive CLI** — manage everything directly from the terminal
* ➕ **Insert records** with names and 8-bit binary masks
* 🔎 **Retrieve records by ID**
* ✏️ **Update existing records**
* 🗑️ **Soft deletion** — records are marked deleted instead of immediately being removed
* 📋 **List records** in the currently opened database
* 🗂️ **List all databases** available to BinaryDB
* ❌ **Delete entire databases** with `dropdb`
* 🧹 **Automatic optimization** — deleted records are compacted automatically when fragmentation exceeds the configured threshold
* 🔐 **Database integrity validation** when opening a database
* 📝 **Persistent logging** for database events and errors
* 🪟 **Windows support** alongside POSIX systems
* 🧩 **No external database server required**

---

## 🧠 How BinaryDB Works

BinaryDB stores databases directly on disk.

Each database consists of:

```text
DataBase header
├── metadata
└── BinaryObject records
    ├── record #1
    ├── record #2
    ├── record #3
    └── ...
```

The database header keeps track of:

* database mark value
* last assigned record ID
* number of active records
* number of deleted records

Records are stored as binary `BinaryObject` structures.

Every record has:

* an ID
* a name
* an 8-bit binary mask
* a deletion flag

This keeps the storage format intentionally simple and makes BinaryDB a useful project for experimenting with low-level database concepts.

---

## 📁 Storage Layout

BinaryDB stores its runtime data relative to the executable:

```text
BinaryDB/
├── BinaryDB.exe
├── db/
│   ├── users.khdb
│   ├── test.khdb
│   └── example.khdb
└── logs/
    ├── database.log
    ├── error.log
    └── misc.log
```

On Linux/macOS-style systems the executable is resolved through `/proc/self/exe`; on Windows BinaryDB uses the Windows executable path APIs.

Database files use the `.khdb` extension. Database names may contain letters, numbers, `_`, and `-`, with a maximum length of 50 characters. Paths, dots, and slashes are rejected.

---

# 🚀 Getting Started

## Requirements

You need:

* A C23-compatible compiler
* CMake **4.2 or newer**
* Git

BinaryDB's current CMake configuration declares:

```cmake
cmake_minimum_required(VERSION 4.2)
project(BinaryDB C)
set(CMAKE_C_STANDARD 23)
```

and builds the `BinaryDB` executable.

---

## 🔨 Build

Clone the repository:

```bash
git clone https://github.com/ZyreXINF/BinaryDB.git
cd BinaryDB
```

Create a build directory:

```bash
cmake -S . -B build
```

Build the project:

```bash
cmake --build build
```

The resulting executable will be generated inside the CMake build directory.

### Release build

For generators supporting CMake build configurations:

```bash
cmake --build build --config Release
```

---

# 💻 Using BinaryDB

Run the executable:

```bash
./BinaryDB
```

On Windows:

```powershell
.\BinaryDB.exe
```

When BinaryDB starts, you'll see:

```text
BinaryDB v1.0
Type 'help' for available commands.

(no db)>
```

The prompt changes to the currently opened database:

```text
mydb>
```

---

# 📚 CLI Reference

BinaryDB currently exposes the following commands.

| Command                     | Description                    |
| --------------------------- | ------------------------------ |
| `create <name>`             | Create and open a new database |
| `open <name>`               | Open an existing database      |
| `databases`                 | List all available databases   |
| `dropdb <name>`             | Permanently delete a database  |
| `add <name> <mask>`         | Add a new record               |
| `get <id>`                  | Retrieve a record by ID        |
| `update <id> <name> <mask>` | Update a record                |
| `delete <id>`               | Soft-delete a record           |
| `list`                      | List all active records        |
| `help`                      | Display available commands     |
| `quit`                      | Exit BinaryDB                  |

---

# 🧹 Automatic Optimization

Soft deletion creates unused space inside the database file.

BinaryDB handles this automatically.

The engine tracks:

```text
active records
+
deleted records
```

When deleted records represent more than **40%** of the total stored records, BinaryDB automatically compacts the database.

For example:

```text
Before:

[ACTIVE][DELETED][ACTIVE][DELETED][DELETED][ACTIVE]
                         ↓
                   fragmentation
```

After optimization:

```text
[ACTIVE][ACTIVE][ACTIVE]
```

The optimizer:

1. Reads the existing records
2. Skips deleted records
3. Moves live records toward the beginning of the file
4. Rewrites the database header
5. Resets the deleted-record counter
6. Truncates the unused tail of the file

BinaryDB also records how many deleted records were purged and how many bytes were reclaimed.

Optimization can occur automatically when a database is opened and immediately after a deletion causes the fragmentation threshold to be exceeded.

---

# 🛡️ Database Validation

BinaryDB performs integrity checks when opening a database.

The validation process checks things including:

* database mark
* minimum file size
* expected file size
* record/deletion counts
* valid deletion flags
* consistency of record IDs

If validation fails, BinaryDB refuses to open the database and writes an error to the logging system.

This helps prevent malformed or obviously corrupted files from being treated as valid databases.

---

# 📝 Logging

BinaryDB includes a lightweight filesystem logger.

Logs are stored next to the executable:

```text
logs/
├── database.log
├── error.log
└── misc.log
```

Database events are written to:

```text
logs/database.log
```

Errors are written to:

```text
logs/error.log
```

Other categories fall back to:

```text
logs/misc.log
```

Each log entry receives a timestamp.

Example:

```text
[2026-08-29 23:42:17] Database 'users' created
```

Logging is intentionally non-critical: if a log file cannot be opened, the logger simply returns rather than crashing the database process.

---

# 🏗️ Project Structure

The current repository is intentionally small:

```text
BinaryDB/
├── CMakeLists.txt
├── LICENSE
├── .gitignore
└── src/
    ├── main.c
    ├── cli.c
    ├── cli.h
    ├── database.c
    ├── database.h
    ├── logger.c
    ├── logger.h
    ├── paths.c
    └── paths.h
```

### `main.c`

The application entry point. It starts the interactive CLI.

### `cli.c / cli.h`

Handles:

* command parsing
* interactive prompts
* database commands
* record commands
* user-facing output

The CLI currently supports database creation/opening/deletion, record CRUD operations, listing, help, and quitting.

### `database.c / database.h`

Contains the database engine itself:

* database creation
* opening/closing
* validation
* record insertion
* lookup
* updates
* deletion
* listing
* database enumeration
* automatic optimization

### `logger.c / logger.h`

Provides the filesystem logging system and log categories.

### `paths.c / paths.h`

Provides platform-specific executable-path resolution so BinaryDB can keep its runtime data relative to the executable. Windows and POSIX implementations are provided.

---

# ⚙️ Current Storage Characteristics

Database files use the custom:

```text
.khdb
```

extension.

The database header contains a marker which is used to recognize BinaryDB database files.

Records receive integer IDs automatically.

Deleted records remain physically present until automatic optimization compacts the database.

This means:

```text
delete
```

and:

```text
physical removal
```

are intentionally separate operations.

---

# 🖥️ Platform Support

BinaryDB currently contains explicit platform handling for:

* **Windows**
* **POSIX/Linux-style environments**

The database layer uses Windows APIs when compiled under `_WIN32`, while POSIX builds use standard directory and filesystem APIs.

Windows-specific handling also exists for database-file truncation during optimization.

---

# ⚠️ Limitations

BinaryDB is an experimental project and should **not** currently be treated as a replacement for a production database system.

Some important limitations of the current design:

* Single-process / simple file access model
* Interactive CLI is the primary interface
* No SQL/query language
* No indexes
* No transactions
* No concurrency control
* No authentication
* No encryption
* No replication
* No network/server mode
* Fixed-size binary record structures
* Binary file format is currently implementation-specific
* No migration/versioning system for future format changes

If you need battle-tested durability, concurrency, SQL, or crash-recovery guarantees, use a mature database engine instead.

BinaryDB is primarily a **small database-engine project and learning platform**.

---

# 🛠️ Development

The project currently uses a compact CMake setup:

```text
CMake
   ↓
src/main.c
   ↓
CLI
   ↓
Database engine
   ├── paths
   ├── logger
   └── .khdb storage
```

The current build target is:

```text
BinaryDB
```

and the project is configured for:

```text
C23
```

with no third-party database dependency.

---

# 🤝 Contributing

Contributions, bug reports, experiments, and ideas are welcome.

If you find a bug:

1. Reproduce it if possible.
2. Check whether it already exists as an issue.
3. Open an issue with:

   * operating system
   * compiler
   * CMake version
   * BinaryDB version/commit
   * reproduction steps
   * relevant logs

For code contributions, please keep changes focused and consistent with the project's lightweight design.

---

# 📜 License

BinaryDB is released under the **MIT License**.

See [`LICENSE`](LICENSE) for the full license text.

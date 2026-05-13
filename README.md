\# CursedCrypt



> Co-op FPS + Tower Defense + Horror | Unreal Engine 5.6



A multiplayer cooperative game where players defend their position against waves of cursed enemies, building barricades and managing resources in a dark dungeon setting.



\## Tech Stack



\- \*\*Engine:\*\* Unreal Engine 5.6

\- \*\*Languages:\*\* C++ (\~30%), Blueprint (\~80%)

\- \*\*Networking:\*\* Replicated co-op with dedicated server architecture

\- \*\*Source Control:\*\* Git + Git LFS (for binary assets)



\## Project Structure

CursedCrypt/
├── Config/           # Project settings (UE config files)
├── Content/          # Game assets (Blueprints, UI, levels)
│   ├── CursedCrypt/  # Custom gameplay content
│   ├── Characters/   # Mannequin animations + custom characters
│   └── ...
├── Source/           # C++ source code
│   └── CursedCrypt/  # Game module
├── .clang-tidy       # Static code analysis configuration
├── .gitattributes    # Git LFS rules for binary files
└── .gitignore        # UE-specific ignore patterns
## Setup

### Requirements
- Unreal Engine 5.6
- Visual Studio 2022 with C++ workload + Unreal Engine integration
- Git LFS

### Installation
1. Clone this repository:
```bash
   git clone https://github.com/Andacs/CursedCrypt.git
```
2. **Marketplace assets are not included in the repo** (size limit). Download from:
[Drive link will be added before final submission]
3. Extract Marketplace assets into `Content/` folder. The following folders must be present:
   - `Content/ParagonGreystone/`
   - `Content/ParagonMinions/`
   - `Content/MedievalDungeon/`
   - `Content/AncientTreasures/`
4. Right-click `CursedCrypt.uproject` → **Generate Visual Studio project files**
5. Open `CursedCrypt.sln`, build the solution
6. Run from Visual Studio or open `.uproject` directly

## Development Workflow

### Branching Strategy (GitFlow)

| Branch | Purpose | Merges Into |
|--------|---------|-------------|
| `main` | Stable release | — |
| `develop` | Active development | `main` (release) |
| `feature/*` | New features | `develop` |
| `hotfix/*` | Critical fixes | `main` + `develop` |

All changes are introduced via Pull Requests. Direct pushes to `main` and `develop` are restricted via branch protection rulesets.

### Static Code Analysis

This project uses **Clang-Tidy** for static analysis. Configuration: `.clang-tidy` at the project root.

Active checks include:
- `bugprone-*` for bug-prone patterns
- `performance-*` for performance issues
- `modernize-use-nullptr`, `use-override`, `default-member-init`
- `readability-*` for code clarity
- `misc-unused-*` for unused parameters/imports

UE engine headers are excluded via `HeaderFilterRegex` to avoid third-party noise.

#### Issues Found and Resolved

Static analysis surfaced the following real issues that were fixed:
- **Missing `AActor` include** in `AttributeComponent.cpp` (undefined type errors)
- **IWYU violation** in include ordering (UE requires own header first)
- **UTF-8 encoding issues** in headers with non-ASCII comments (C4828)
- **Missing includes** for `USkeletalMeshComponent`, `UAnimInstance`, `ULocalPlayer` across multiple files
- **Wrong `WorldContextObject` type** in `SphereTraceMulti` call

All fixes are tracked via individual Pull Requests in the commit history.

## Features Implemented

- ✅ Character system with health/stamina
- ✅ Replicated melee combat with hit confirmation
- ✅ Co-op networking foundation
- ✅ AI enemies with Behavior Trees
- ✅ Dynamic barricade system with NavMesh integration
- ✅ Inventory system
- ✅ Save/Load system (async, JSON-based, interface-driven)
- ✅ Internationalization (Turkish/English language switch in Settings)

## Author

Andac — Solo developer

## License

Proprietary — Academic project submission


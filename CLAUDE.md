# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CursedCrypt is an Unreal Engine 5.6 co-op FPS + tower defense + horror game (solo academic project). Gameplay is a mix of C++ (~30%) and Blueprint (~80%): C++ provides core gameplay classes, components, subsystems, and networking/RPC logic; Blueprints (in `Content/`) handle visual scripting, UI, and content wiring on top of the C++ base classes. Networking is replicated co-op with a dedicated-server architecture (see RPC patterns in `CCPlayerCharacter`).

## Build & Development Commands

This is a standard UE5 C++ project — there is no CMake/npm/cargo build pipeline.

- **Generate project files:** right-click `CursedCrypt.uproject` → "Generate Visual Studio project files" (or run `UnrealBuildTool.exe -projectfiles -project="CursedCrypt.uproject" -game -engine`).
- **Build:** open `CursedCrypt.sln` in Visual Studio 2022 (with the C++ and Unreal Engine workloads) and build, or run `Build.bat CursedCryptEditor Win64 Development -Project="<path>\CursedCrypt.uproject"` from the engine's `Engine/Build/BatchFiles` directory.
- **Run:** open `CursedCrypt.uproject` directly (launches the editor), or run from Visual Studio with the `CursedCryptEditor` target for debugging.
- **Targets:** `CursedCryptTarget` (Game) and `CursedCryptEditorTarget` (Editor), both defined in `Source/*.Target.cs`, both build the single `CursedCrypt` module.
- **Static analysis:** Clang-Tidy is configured via `.clang-tidy` at the repo root (bugprone-*, performance-*, modernize-use-nullptr/override, readability-*, misc-unused-*; engine headers excluded via `HeaderFilterRegex`). Run it through your IDE's clang-tidy integration or `clang-tidy` directly against files under `Source/`.
- **No automated test suite exists** in this repo (no UE Automation Spec/Test files). Validate gameplay changes by running in the editor (PIE) and, for networked features, using multiplayer PIE (2+ clients) or a standalone dedicated server.

### Marketplace content dependency

`Content/ParagonGreystone/`, `Content/ParagonMinions/`, `Content/MedievalDungeon/`, and `Content/AncientTreasures/` are third-party Marketplace assets not stored in the repo. If they're missing, the project will fail to open fully — see README setup instructions for how they're obtained.

## Architecture

### Module layout

Everything lives in the single `CursedCrypt` module (`Source/CursedCrypt/`). Within it there are two layers:

1. **Custom gameplay code** — directly under `Source/CursedCrypt/` (e.g. `CCPlayerCharacter`, `CCEnemyCharacter`, `CCEnemyAIController`, `AttributeComponent`, save/load system, `CursedCryptGameInstance`, `CursedCryptGameMode`, `CursedCryptPlayerController`). This is where actual game features are implemented; the `CC*` prefix marks project-specific classes.
2. **Variant_* template samples** — `Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` are leftover Epic sample content from the UE5 Third Person template (its own AI, characters, game modes, UI). They are largely untouched reference/scaffolding rather than shipping features — don't assume code in these folders reflects current design intent unless a task specifically targets it. Each variant subfolder is added to `PublicIncludePaths` in `CursedCrypt.Build.cs`, so headers across variants are includable without relative paths.

### Core gameplay systems

- **`AttributeComponent`** — replicated actor component providing health/stamina for both players and enemies (`OnRep_Health`/`OnRep_Stamina`, `ApplyDamage`/`ApplyHeal`/`ConsumeStamina`/`RestoreStamina`). Also tracks a per-attack `HitActorsDuringAttack` list to prevent double-hits from a single melee swing (reset via `ResetMeleeHitList`). `SetHealth` is the server-authoritative path used by the save/load system to restore state without going through damage/heal semantics.
- **Combat** — `CCPlayerCharacter` drives attacks through a `Server_Attack` RPC (validates + consumes stamina server-side) and a `Multicast_PlayAttackAnim` RPC (plays the animation on all clients); `AnimNotifyState_MeleeTrace` performs the actual hit detection during the montage.
- **Enemies** — `CCEnemyCharacter` + `CCEnemyAIController` drive a Behavior Tree (`BehaviorTreeAsset`, assigned in the editor, run via `RunEnemyBT`). Do not confuse these with the `Variant_Combat/AI/CombatEnemy*` template AI, which is separate sample code.
- **Save/Load system** (`CursedCryptSaveable.h/.cpp`, `CursedCryptSaveSubsystem.h/.cpp`, `CursedCryptSaveGame.h/.cpp`):
  - `ICursedCryptSaveable` is a Blueprint-native interface (`GatherSaveData` / `ApplySaveData` / `GetSaveableId`) that any actor implements to opt into persistence.
  - `UCursedCryptSaveSubsystem` (a `GameInstanceSubsystem`) holds a weak-referenced registry of saveables and drives the pipeline: on save, gathers each saveable's JSON and writes via `AsyncSaveGameToSlot`; on load, reads via `AsyncLoadGameFromSlot` and dispatches JSON back out. Completion is signaled via `OnSaveCompleted`/`OnLoadCompleted` dynamic multicast delegates — never poll for save/load completion.
  - `UCursedCryptSaveGame` is the on-disk container: a `TMap<FString, FString>` of per-saveable JSON payloads plus global state (language, timestamp, `SaveVersion` for schema migrations).
  - `UCursedCryptSaveableLibrary` exposes JSON parse/format helpers to Blueprint (UE's JSON API is C++-only). `FormatPlayerSaveData` deliberately uses `FString::Printf` rather than locale-aware formatting because Turkish Windows uses `,` as the decimal separator, which would corrupt JSON on TR locale machines.
  - Save/load is currently triggered via test keybinds wired up in `CursedCryptPlayerController` (`HandleSaveKey`/`HandleLoadKey`).
- **Localization** — `CursedCryptGameInstance` implements a lightweight custom i18n system: a `TMap<FString, TPair<FText, FText>>` translation table (TR/EN) populated in `InitializeTranslationTable`, looked up via `GetLocalizedText(Key)`, with `SetLanguage` broadcasting `OnLanguageChanged` so bound widgets refresh. This is separate from Unreal's built-in localization dashboard/gather-and-compile pipeline under `Config/Localization/`.

### Networking conventions

Player-affecting actions follow a server-RPC-then-multicast pattern: validate and mutate authoritative state in a `Server_*` RPC, then fan out cosmetic effects via a `Multicast_*` RPC (see `CCPlayerCharacter::Server_Attack`/`Multicast_PlayAttackAnim`). Replicated state uses `ReplicatedUsing=OnRep_*` callbacks rather than direct polling.

### Coding conventions

- Comments and identifiers should be in English (a past cleanup pass — see `fe204b5`/`0e5bafd` history — removed Turkish comments and non-ASCII characters that caused MSVC C4828 UTF-8 encoding warnings).
- Include UE headers in IWYU order: the class's own header first, then engine headers — missing includes for engine types (`AActor`, `USkeletalMeshComponent`, `UAnimInstance`, `ULocalPlayer`, etc.) have been a recurring clang-tidy finding.
- Branching follows GitFlow: `feature/*` branches off `develop`, PR back into `develop`; `hotfix/*` branches off `main`. Direct pushes to `main`/`develop` are blocked by branch protection — always go through a PR.

## Working Agreement (Andaç)

- Communicate in Turkish for explanations; keep UE terminology, code, and comments in English.
- Default mode is STABLE MODE: one feature at a time, no scope creep, no mid-task plan switching (don't offer Option A/B/C mid-execution — commit to one path).
- Never stop mid-task without finishing or flagging clearly with "DİKKAT". "dur" (stop) mid-task is not acceptable behavior to default to.
- Before any structural change (multi-file edits, C++ class hierarchy changes, replication/networking changes, Behavior Tree/Blackboard structural changes, changes to GameMode/GameInstance/PlayerController), explicitly ask for a backup/commit checkpoint first.
- Prefer full-file delivery over partial diffs when writing C++, unless the user asks for a diff.
- Ask clarifying questions only when truly ambiguous — don't over-ask.
- Git workflow: feature branches off develop, PR back into develop; no direct push to main/develop.

## Code Migration Policy

- Yeni yazılacak her şey → C++ default. UI wiring, basit event binding gibi zorunlu Blueprint katmanları hariç (Widget binding'ler, montage/anim event'ler, level'a aktör yerleştirme gibi UE'nin doğası gereği BP gerektiren yerler).
- Çalışan mevcut sistemler (karakter, envanter, co-op sync, AI temel davranış) → dokunma. Bunlar zaten test edilmiş, çalışıyor. "Daha hızlı olsun" diye şimdi ellemek risk/fayda açısından mantıksız — MVP bitene kadar post-launch listesine yazıyorum.
- İstisna: Bir BP sisteminde gerçek, ölçülmüş bir performans sorunu varsa (profiler'da görülen tick maliyeti, çok sayıda actor'da yavaşlama gibi) o zaman spesifik olarak o sistemi C++'a taşırız — genel "daha iyi olur" hissiyle değil, kanıtla.
- AI-Barricade sistemi zaten bu kuralın ilk uygulama alanı olacak — BTS_CheckPathToTarget/BTT_AttackBarricade gibi henüz stabilize olmamış parçaları C++'a çekmek mantıklı, çünkü zaten üzerinde çalışıyoruz ve tick'te path query yapıyor (performans-kritik).

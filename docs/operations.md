# Operations Guide

## Localization & Unicode

NotepadLite supports runtime language selection for English (default) and Bangla (বাংলা).

* Set `NOTEPADLITE_LANG=bn` (or `bn-BD`, `bangla`) to enable Bangla strings.
* English is the fallback when the variable is unset or has another value.
* When `NOTEPADLITE_LANG` is unset, the app detects the OS locale and auto-enables Bangla for `bn` locales.
* Custom translations can be provided via `NOTEPADLITE_LOCALE_FILE`, a UTF-8 `key=value` file (see `src/localization.c` for key names).
* The Windows UI exposes a View → Language toggle for English/Bangla runtime switching.
* Windows UI uses Unicode `wchar_t` APIs.

## Update & Upload Scheduling

The 5-hour update workspace cadence is decommissioned in favor of a 20-minute incremental upload cycle.

Operational guidelines:

* **Cadence**: trigger incremental uploads every 20 minutes.
* **Lightweight payloads**: upload only the delta since the last successful checkpoint.
* **Resumable uploads**: store checkpoints locally (timestamp, byte offset, hash) and resume from the last known good state.
* **Fault tolerance**: retry with exponential backoff; never block the UI thread.
* **Backwards compatibility**: retain the ability to read prior checkpoint metadata to ensure clean upgrades.

## Platform Deployment Targets

One standalone artifact is required:

* Windows 64-bit executable

Build commands are listed in the repository README along with QA expectations.

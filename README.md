# NotepadLite

NotepadLite is a streamlined, high-efficiency alternative to the native Windows Notepad. Built in C for maximum throughput, the solution minimizes overhead while delivering a crisp, frictionless editing experience tailored for both power users and mainstream workloads.

## Key Value Drivers

* **Lightweight Footprint** – C-optimized code path ensures rapid launch, minimal memory utilization, and strong runtime stability.
* **Enhanced Productivity Layer** – A dedicated **Analysis** module sits on the top bar, providing accelerated insights and workflow intelligence.
* **Drop-In Compatibility** – Operates as a direct substitute for the Windows Notepad experience with zero behavioral disruption.

## Feature Highlights

* Minimal, clean, zero-latency UI
* High-speed file load/save performance
* Full Unicode readiness
* Extended **Analysis** panel for deep text operations
* Windows-targeted build pipeline using MinGW

## Latest Build

Leverage the most recent release package for streamlined deployment:
👉 **Download Latest Build:**
[https://github.com/sabbir28/NotepadLite/releases/latest](https://github.com/sabbir28/NotepadLite/releases/latest)

## Build Workflow (WSL + Makefile)

Execute a seamless cross-platform build pipeline through WSL:

```bash
# Clone the repository
git clone https://github.com/sabbir28/NotepadLite.git
cd NotepadLite

# Build using the provided Makefile (MinGW cross-compiler required)
make

# Output binary will appear in:
./build/bin/notepad.exe
```

Environment prerequisites:

* WSL (Ubuntu recommended)
* `mingw-w64` toolchain
* `make` build utilities

## Installation

1. Download the latest build or compile via WSL using the Makefile.
2. Place the executable in your preferred working path or integrate it system-wide.
3. Launch and begin executing your editing workflows immediately.

## Contact & Engagement

* **Facebook:** [https://fb.com/sabbir28.github.io](https://fb.com/sabbir28.github.io)
* **Email:** [sabbirb28@gmail.com](mailto:sabbirb28@gmail.com)

## License

Distributed under standard open-source licensing terms as defined in the repository.

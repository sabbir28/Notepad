# Contributing to NotepadLite

Thank you for considering contributing to **NotepadLite**. We welcome improvements, bug fixes, and feature suggestions from developers of all skill levels. To ensure the software remains performant, reliable, and lightweight, please follow these guidelines.

---

## Code of Conduct

* Always respect other contributors.
* Maintain professionalism in all discussions, issues, and pull requests.
* Avoid introducing changes that could negatively impact the performance or stability of the software.

---

## Contribution Guidelines

### 1. Performance First

NotepadLite is a **high-performance text editor**. Any new feature or change must **not hamper runtime speed, memory usage, or stability**.

* If a feature could impact performance, consider making it **optional**, configurable, or behind a compile-time flag.
* Example in C:

```c
#ifdef ENABLE_ANALYSIS_PANEL
    init_analysis_panel();
#endif
```

### 2. Code Style

* Use **C99/C11** standards.
* Follow **consistent indentation** (4 spaces).
* Avoid unnecessary global variables.
* Comment critical logic clearly.

### 3. Branching

* Fork the repository and create a feature branch:

```bash
git checkout -b feature/my-new-feature
```

* Keep commits focused and descriptive.
* Submit pull requests against the `main` branch.

### 4. Bug Reports

When opening a bug report:

* Clearly describe the issue and steps to reproduce.
* Include your environment details (Windows version, WSL if used, compiler, etc.).
* Example template:

```markdown
### Bug Description
...

### Steps to Reproduce
1. ...
2. ...

### Environment
- Windows 10
- MinGW-w64
```

### 5. Feature Suggestions

* Ensure features **enhance usability without degrading performance**.
* Provide an optional implementation if the feature is heavy or resource-intensive.

---

## How to Test Your Changes

* Build using the Makefile (`make`) under WSL or native Windows MinGW environment.
* Run functional tests to verify no regressions occur.
* Ensure memory footprint and runtime speed remain acceptable.

---

## Submitting a Pull Request

1. Fork the repository.
2. Create a feature branch.
3. Make your changes following the above guidelines.
4. Submit a pull request with a detailed description of your changes.

---

> **Reminder:** Features that degrade user experience or performance **may not be merged**. Always prioritize lightweight, efficient, and optional enhancements.


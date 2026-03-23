# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| main branch | Yes |

## Reporting a Vulnerability

If you discover a security vulnerability in amiport, please report it responsibly.

**Do NOT open a public GitHub issue for security vulnerabilities.**

Instead, please email: **duncan@platesteel.net**

Include:
- Description of the vulnerability
- Steps to reproduce
- Affected component (website, shim library, build pipeline, etc.)
- Potential impact

## What to Expect

- Acknowledgement within 48 hours
- Assessment and plan within 1 week
- Fix deployed as soon as practical, with credit to the reporter (unless you prefer anonymity)

## Scope

This policy covers:
- The amiport website (amiport.platesteel.net)
- The POSIX shim and emulation libraries (`lib/posix-shim/`, `lib/posix-emu/`)
- Build pipeline scripts and toolchain configuration
- Published Aminet packages

The ported programs themselves inherit upstream security properties. If you find a vulnerability in a ported program that also exists upstream, please report it to the upstream project.

## Security Considerations

amiport targets AmigaOS 3.x, which has no memory protection, no privilege separation, and no network security model. The shim libraries prioritize correctness and compatibility over hardening. Programs ported with amiport should not be used in security-sensitive contexts on modern systems.

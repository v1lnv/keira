# Security Policy

## Supported Versions

We only support security updates for the active branches (`develop` and `main`). Please ensure you are running the latest compiled version of Keira Kernel before reporting an issue.

| Version | Supported |
| ------- | --------- |
| 0.7.x   | Yes       |
| < 0.7.0 | No        |

## Reporting a Vulnerability

Because Keira Kernel is a freestanding hobby and educational operating system running in Ring 0 (with user-space programs in Ring 3), security vulnerabilities (such as buffer overflows, privilege escalation bypasses, or scheduler memory leaks) are highly valued as educational learning opportunities.

To report a vulnerability:
1. Please do **not** open a public issue. Instead, report the vulnerability by sending an email to [v1lnv@proton.me](mailto:v1lnv@proton.me).
2. We will investigate the issue and coordinate a fix.
3. Once a fix is merged, a security advisory will be published to credit you for finding the bug.

Thank you for helping keep Keira Kernel secure!

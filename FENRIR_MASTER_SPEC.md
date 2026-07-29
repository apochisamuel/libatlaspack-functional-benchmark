# Fenrir Master Spec Mirror

The controlling requirements are maintained in the supplied Fenrir Control
Pack. This repository mirrors its public mechanical layout and records evidence
conservatively. No check is marked verified unless the command was executed for
the exact revision represented by the archive.

This safe hardened baseline covers the functional repository, build, test, harness,
source-size, artifact-separation, and fail-closed control surfaces. It does not
contain private task candidates, task inputs, task patches, or task evidence.
Consequently, successful repository preflight does not imply complete benchmark
release readiness; `scripts/release_gate.sh` must continue to fail until its
independent evidence-count requirement is satisfied.

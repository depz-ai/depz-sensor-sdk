# Contributing

Thanks for your interest in the DEPZ Sensor SDK!

## Issues

Bug reports and feature requests are welcome —
[open an issue](https://github.com/depz-ai/depz-sensor-sdk/issues). When
reporting a protocol/decoding problem, please include the sensor model,
firmware version (`depz-sensor list` prints it) and, if possible, a byte
capture.

## Code changes

The SDK is contract-first: the seven language implementations are kept in
byte-for-byte lockstep by shared golden test vectors and a cross-language
parity gate, and releases land here as integrated, tested snapshots. Because
of that, pull requests are not merged directly — the maintainers take patches
through the integration pipeline (all seven SDKs + vector regeneration +
hardware QA) and ship them in the next release, with credit.

The practical route: open an issue describing the change first. For small
fixes a patch attached to the issue (or a PR used as a reference) is perfect;
for protocol-level changes an issue is the place to start the discussion.

## License

By submitting a patch you agree to license it under the repository's MIT
license.

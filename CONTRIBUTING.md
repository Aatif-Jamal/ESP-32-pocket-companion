# Contributing

Small improvements are welcome: an expressive reaction, a clearer build photo, a reproducible fix, or a correction to the wiring guide.

## Keep it approachable

- Keep GPIO assignments at the top of each sketch and update BUILD.md if they change.
- Use the existing non-blocking frame and button handling for animations. Avoid long delays that freeze the character.
- Keep the idle face and pet/feed interactions usable without Wi-Fi or an account.
- Add dependencies only when they materially improve the small-device experience.
- Describe hardware you actually tested. Distinguish a successful compile from a working physical build.

## Build locally

Install Arduino CLI 1.5.1, ESP32 core 3.3.12, and U8g2 2.36.19, or use Arduino IDE with those board/library versions. From the repository root:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc RoboPetEyes
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc examples/DisplayCheck
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc examples/ButtonCheck
```

The GitHub Actions workflow runs these three compile checks. On hardware, verify the display, button press/release, both reactions, and return to idle. Holding the button should not repeatedly start a reaction.

## Report a problem

Open an issue with the board model, Arduino core and U8g2 versions, pin mapping, exact error or serial output, and a clear wiring photo if relevant. Remove Wi-Fi passwords, tokens, and personal details from logs and photos.

## Send a change

Open a focused pull request describing what changes, why, and how you checked it. Add a short clip for animation changes when possible. Contributions are provided under this repository's MIT license; retain notices required by any third-party material.

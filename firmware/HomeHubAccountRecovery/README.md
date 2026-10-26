# Home Hub local account recovery

Service-only sketch for a user-authorized account reset. It removes only
`username`, `salt`, and `passhash` in the `homehub` NVS namespace, then
sets `account` to false. It does not read credentials or change the sensor
registry, router credentials, or other namespaces.

Compile with the declared XIAO ESP32-C3 profile. Upload only after explicit
authorization for the named device/port; uploading resets the board. Allow
setup to run, then restore the previously verified normal HomeHub firmware.
Do not leave this image installed: it runs account recovery at every boot
and does not provide the website or sensor reception.

After restoration, check `/api/auth/state` for `account: false` and
`storage_ready: true`. The owner should immediately create a new local
account on the Home Hub page. Do not ask them to share their new password.

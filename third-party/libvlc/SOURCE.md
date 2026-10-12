# libVLC in auxed

- **Version:** libVLC for Android `3.7.7` (Maven Central: `org.videolan.android:libvlc-all:3.7.7`), used unmodified.
- **Licence:** GNU LGPL 2.1 or later.
- **Source:** the VLC for Android project, https://code.videolan.org/videolan/vlc-android (libvlc 3.x branch), which builds libVLC from VLC 3.0 (https://code.videolan.org/videolan/vlc/-/tree/3.0.x) and its contribs. VideoLAN's source archives: https://download.videolan.org/pub/videolan/vlc-android/
- **Modifications:** none. auxed calls the public libVLC Java API.

The native libraries ship as separate files in the app (`lib/arm64-v8a/`, `lib/armeabi-v7a/`) and can be replaced with your own build. If any link above stops working, ask for a copy in this repository's Issues (see [SOURCE-OFFER.md](../../SOURCE-OFFER.md)).
